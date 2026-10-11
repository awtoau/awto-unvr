/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * al_ssm_drv.h - shared types for the Alpine V2 SSM crypto driver
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 */

#ifndef __AL_SSM_DRV_H__
#define __AL_SSM_DRV_H__

#include <linux/atomic.h>
#include <linux/kref.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#include <crypto/aes.h>
#include <crypto/algapi.h>
#include <crypto/skcipher.h>

#include "al_hal_ssm.h"
#include "al_hal_ssm_crypto.h"
#include "al_hal_unit_adapter.h"

#define DRV_NAME		"al_ssm"
#define DRV_VERSION		"2.0.4"

#define AL_SSM_RING_SIZE	256
#define AL_SSM_MAX_BACKLOG	4096
#define AL_SSM_POLL_INTERVAL_US	100
#define AL_SSM_MAX_DATA_SIZE	(256 * 1024)
/* One UDMA descriptor length is 16 bits; see al_ssm_add_range() (#332). */
#define AL_SSM_MAX_DESC_LEN	(32 * 1024)
/*
 * Data buffers per direction in one transaction. HW takes 31 descriptors:
 * tx = 31 - metadata - SA - IV - sign, rx = 31 - sa/iv/next-iv/sign outputs
 * (vendor al_crypto.h:59-66). We use SA + IV only; 27 is the vendor's bound.
 */
#define AL_SSM_MAX_DATA_BUFS	27
#define RING_BYTES		(AL_SSM_RING_SIZE * sizeof(union al_udma_desc))

/*
 * Each crypto operation uses 3 TX descriptors and 1 RX descriptor.
 * Ring size is 256 entries. With HAL reserving AL_CRYPT_DESC_RES (16)
 * descriptors for padding, max concurrent ops = (256 - 16) / 3 = 80.
 * Use 64 for safety margin.
 */
#define AL_SSM_MAX_IN_FLIGHT	64

/*
 * Stall bound for the devlink health reporter: in-flight work with no
 * completion for this long. Matches al_dma's own 50ms bound; the largest
 * op we accept (256KB) is orders below it. Diagnostic only - nothing is
 * cancelled, the fault is reported once per stall episode.
 */
#define AL_SSM_STALL_MS		50

struct al_ssm_dev;
struct devlink;
struct devlink_health_reporter;

/* Fault classes reported through devlink health. Names/order kept in step
 * with al_dma's enum so one userspace tool reads both. */
enum al_ssm_fault {
	AL_SSM_FAULT_SUBMIT,		/* hardware refused a prepared transaction */
	AL_SSM_FAULT_COMPLETION,	/* completion descriptor carried an error */
	AL_SSM_FAULT_STALL,		/* in-flight work never completed (#182) */
	AL_SSM_FAULT_DMA_ALLOC,		/* request could not be DMA-mapped; served in software */
	AL_SSM_FAULT_COUNT
};

/* Read by the engine (DMA_TO_DEVICE); one mapping per request. */
struct al_ssm_hwbuf {
	struct al_crypto_hw_sa sa;
	u8 iv[AES_BLOCK_SIZE];
};

struct al_ssm_reqctx {
	struct list_head list;		/* chan->pending, then a service pass's done list */
	bool notify_owed;		/* chan->lock: MAY_BACKLOG promotion, owes -EINPROGRESS */
	int status;			/* result, valid once reaped */
	bool inline_wait;		/* chan->lock: submitter is spinning on this request */
	bool finished;			/* chan->lock: reaped while inline_wait; submitter completes it */
	struct skcipher_request *req;
	struct al_ssm_dev *dev;
	enum al_crypto_dir dir;
	enum al_crypto_sa_enc_type enc_type;
	unsigned int nbytes;

	/* req->src / req->dst mapped in place; no bounce, no allocation */
	int src_nents, dst_nents;	/* as passed to dma_map_sg(), for the unmap */
	bool in_place, src_mapped, dst_mapped, hw_mapped;
	struct al_buf src_bufs[AL_SSM_MAX_DATA_BUFS];
	struct al_buf dst_bufs[AL_SSM_MAX_DATA_BUFS];
	unsigned int src_num, dst_num;
	u8 next_iv[AES_BLOCK_SIZE];	/* CBC decrypt: output IV saved from the input */

	dma_addr_t hw_dma;
	struct al_ssm_hwbuf hw __aligned(CRYPTO_DMA_ALIGN);

	struct skcipher_request fallback_req;	/* must be last */
};

struct al_ssm_chan {
	spinlock_t lock;
	void *tx_ring;
	dma_addr_t tx_ring_dma;
	void *tx_cdesc;
	dma_addr_t tx_cdesc_dma;
	void *rx_ring;
	dma_addr_t rx_ring_dma;
	void *rx_cdesc;
	dma_addr_t rx_cdesc_dma;
	struct list_head pending;	/* on the ring, FIFO == completion order */
	int pending_count;
	struct crypto_queue backlog;	/* waiting for ring space; MAY_BACKLOG contract */
	bool stopping;		/* chan->lock: remove started, refuse new work (#301) */
};

struct al_ssm_ctx {
	struct al_ssm_dev *dev;
	struct crypto_skcipher *fallback;	/* sync software cipher, same name */
	enum al_crypto_sa_enc_type enc_type;
	enum al_crypto_sa_aes_ksize aes_ksize;
	u8 key[AES_MAX_KEY_SIZE];
	u8 xts_key[AES_MAX_KEY_SIZE];
	unsigned int keylen;
};

struct al_ssm_dev {
	struct kref ref;	/* probe + one per live tfm; last put frees (#301) */
	struct pci_dev *pdev;
	struct device *dev;
	void __iomem *bar0;
	void __iomem *bar4;
	void *bars[6];
	struct al_ssm_dma ssm_dma;
	struct al_unit_adapter unit_adapter;
	struct al_ssm_unit_regs_info unit_info;
	struct al_ssm_chan channel;
	bool crypto_registered;
	struct workqueue_struct *wq;
	struct delayed_work poll_work;
	atomic_t total_pending;

	/* devlink diagnostics (#212) + fault reporting (#182) */
	struct devlink *devlink;
	struct devlink_health_reporter *hr;
	bool devlink_live;
	struct work_struct fault_work;
	unsigned long fault_pending;	/* bitmask of enum al_ssm_fault */
	atomic64_t stat_submitted;
	atomic64_t stat_completed;
	atomic64_t stat_errors;
	atomic64_t stat_poll_cycles;
	atomic64_t stat_fallback;	/* requests served by software */
	unsigned long last_completion;	/* jiffies, 0 = none since probe */
	unsigned long stall_clock;	/* jiffies: last completion, or work became outstanding */
	bool stall_reported;
};

/* al_ssm_devlink.c */
int al_ssm_devlink_init(struct al_ssm_dev *dev);
void al_ssm_devlink_fini(struct al_ssm_dev *dev);
void al_ssm_fault(struct al_ssm_dev *dev, enum al_ssm_fault fault);

#endif /* __AL_SSM_DRV_H__ */
