/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * al_ssm_drv.h - shared types for the Alpine V2 SSM crypto driver
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 */

#ifndef __AL_SSM_DRV_H__
#define __AL_SSM_DRV_H__

#include <linux/atomic.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#include <crypto/aes.h>
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
	AL_SSM_FAULT_DMA_ALLOC,		/* coherent DMA buffer allocation failed */
	AL_SSM_FAULT_COUNT
};

struct al_ssm_reqctx {
	struct list_head list;
	struct skcipher_request *req;
	struct al_ssm_dev *dev;
	enum al_crypto_dir dir;
	enum al_crypto_sa_enc_type enc_type;
	void *src_virt;
	void *dst_virt;
	u8 *iv_buf;
	struct al_crypto_hw_sa *hw_sa;
	dma_addr_t src_dma;
	dma_addr_t dst_dma;
	dma_addr_t sa_dma;
	dma_addr_t iv_dma;
	unsigned int nbytes;
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
	struct list_head pending;
	int pending_count;
};

struct al_ssm_ctx {
	struct al_ssm_dev *dev;
	enum al_crypto_sa_enc_type enc_type;
	enum al_crypto_sa_aes_ksize aes_ksize;
	u8 key[AES_MAX_KEY_SIZE];
	u8 xts_key[AES_MAX_KEY_SIZE];
	unsigned int keylen;
};

struct al_ssm_dev {
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
	spinlock_t backlog_lock;
	struct list_head backlog;
	atomic_t backlog_count;
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
	unsigned long last_completion;	/* jiffies, 0 = none since probe */
	bool stall_reported;
};

/* al_ssm_devlink.c */
int al_ssm_devlink_init(struct al_ssm_dev *dev);
void al_ssm_devlink_fini(struct al_ssm_dev *dev);
void al_ssm_fault(struct al_ssm_dev *dev, enum al_ssm_fault fault);

#endif /* __AL_SSM_DRV_H__ */
