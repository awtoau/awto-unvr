// SPDX-License-Identifier: GPL-2.0
/*
 * al_dma_drv.c - Annapurna Labs DMA Engine Driver for Alpine V2 SoC
 *
 * Provides hardware-accelerated XOR/PQ parity calculations for MD RAID5/6
 * via the Linux DMA engine (dmaengine) framework.
 *
 * The Alpine V2 SSM (Security/Storage/Memory) PCI device (vendor 0x1c36,
 * device 0x0022) contains RAID acceleration hardware that uses the same
 * UDMA (Unified DMA) infrastructure as the Ethernet adapter, but configured
 * for M2M (memory-to-memory) operations.
 *
 * Hardware capabilities:
 *   - XOR parity calculation (RAID5) - up to 31 sources, 16KB blocks
 *   - PQ parity calculation (RAID6) - up to 31 sources, 8KB blocks
 *   - DMA memcpy - hardware-accelerated memory copy
 *   - DMA memset - hardware-accelerated memory set
 *   - XOR validate / PQ validate
 *
 * BAR layout:
 *   BAR 0: UDMA registers (128KB)
 *   BAR 4: Application (RAID engine) registers (64KB)
 *
 * Copyright (C) 2015 Annapurna Labs Ltd.
 * Copyright (C) 2024 - Ported to kernel 6.12
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>

/*
 * Cookie helper functions — moved from include/linux/dmaengine.h to
 * drivers/dma/dmaengine.h (private) in kernel 6.x. Inlined here for
 * out-of-tree module compatibility.
 */
static inline dma_cookie_t dma_cookie_assign(struct dma_async_tx_descriptor *tx)
{
	struct dma_chan *chan = tx->chan;
	dma_cookie_t cookie;

	cookie = chan->cookie + 1;
	if (cookie < DMA_MIN_COOKIE)
		cookie = DMA_MIN_COOKIE;
	tx->cookie = cookie;
	chan->cookie = cookie;
	return cookie;
}

static inline void dma_cookie_complete(struct dma_async_tx_descriptor *tx)
{
	tx->chan->completed_cookie = tx->cookie;
	tx->cookie = 0;
}

static inline enum dma_status dma_cookie_status(struct dma_chan *chan,
	dma_cookie_t cookie, struct dma_tx_state *state)
{
	dma_cookie_t used, complete;

	used = chan->cookie;
	complete = chan->completed_cookie;
	barrier();
	if (state) {
		state->last = complete;
		state->used = used;
		state->residue = 0;
		state->in_flight_bytes = 0;
	}
	if (complete <= used) {
		if (cookie <= complete || cookie > used)
			return DMA_COMPLETE;
	} else {
		if (cookie <= complete && cookie > used)
			return DMA_COMPLETE;
	}
	return DMA_IN_PROGRESS;
}
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/async_tx.h>

#include "al_dma_drv.h"
#include "al_hal_udma_iofic.h"

/* Real completion descriptor stride on real silicon (vendor al_dma.h:
 * AL_DMA_RAID_RX_CDESC_SIZE, 16 only under the emulator-only
 * CONFIG_ALPINE_VP_WA) - NOT sizeof(union al_udma_cdesc)=4, which only
 * models the descriptor's first word. See docs/kernel-boot-fixes.md Bug 4
 * (#23) for the root-cause writeup. */
#define AL_DMA_RAID_RX_CDESC_SIZE	8

/* PCI IDs */
#ifndef PCI_VENDOR_ID_AMAZON_ANNAPURNA_LABS
#define PCI_VENDOR_ID_AMAZON_ANNAPURNA_LABS	0x1c36
#endif
#define AL_DMA_PCI_DEVICE_ID	0x0022  /* Alpine V2 SSM (combined crypto+RAID) */

/* Module parameters */
static int max_channels = 4;
module_param(max_channels, int, 0444);
MODULE_PARM_DESC(max_channels, "Maximum number of DMA channels (default: 4)");

static int ring_alloc_order = 10;  /* 1024 descriptors */
module_param(ring_alloc_order, int, 0444);
MODULE_PARM_DESC(ring_alloc_order, "Descriptor ring size as power of 2 (default: 10 = 1024)");

static inline struct al_dma_chan *to_al_dma_chan(struct dma_chan *c)
{
	return container_of(c, struct al_dma_chan, chan);
}

static inline struct al_dma_device *to_al_dma_device(struct dma_device *d)
{
	return container_of(d, struct al_dma_device, dma_dev);
}

static inline struct al_dma_sw_desc *al_dma_get_sw_desc(struct al_dma_chan *ch)
{
	struct al_dma_sw_desc *desc;

	/* #23: must check against `completed`, not `tail` - tail catches up
	 * to head on every issue_pending() regardless of hardware progress,
	 * so it never actually protected an in-flight descriptor from reuse. */
	if (((ch->head + 1) % ch->sw_ring_count) == ch->completed)
		return NULL;  /* ring full */

	desc = &ch->sw_ring[ch->head];
	ch->head = (ch->head + 1) % ch->sw_ring_count;
	return desc;
}

/* ======== DMA Engine Callbacks ======== */

static enum dma_status al_dma_tx_status(struct dma_chan *c,
					dma_cookie_t cookie,
					struct dma_tx_state *txstate)
{
	return dma_cookie_status(c, cookie, txstate);
}

static dma_cookie_t al_dma_tx_submit(struct dma_async_tx_descriptor *txd)
{
	struct al_dma_chan *ch = to_al_dma_chan(txd->chan);
	dma_cookie_t cookie;
	unsigned long flags;

	spin_lock_irqsave(&ch->lock, flags);
	cookie = dma_cookie_assign(txd);
	spin_unlock_irqrestore(&ch->lock, flags);

	return cookie;
}

static void al_dma_issue_pending(struct dma_chan *c)
{
	struct al_dma_chan *ch = to_al_dma_chan(c);
	struct al_dma_device *dev = ch->device;
	struct al_dma_sw_desc *desc;
	unsigned long flags;
	int submitted = 0;
	int start;

	spin_lock_irqsave(&ch->lock, flags);

	start = ch->tail;
	while (ch->tail != ch->head) {
		desc = &ch->sw_ring[ch->tail];
		if (desc->tx_descs_count > 0) {
			int rc = al_raid_dma_action(&dev->hal_dma, ch->idx,
						     desc->tx_descs_count);
			if (rc) {
				pr_err("al_dma: failed to trigger dma action on ch %d (rc %d)\n",
				       ch->idx, rc);
				atomic64_inc(&dev->stat_errors);
				al_dma_fault(dev, AL_DMA_FAULT_SUBMIT);
			} else {
				atomic64_inc(&dev->stat_submitted);
			}
			submitted++;
		}
		ch->tail = (ch->tail + 1) % ch->sw_ring_count;
	}

	spin_unlock_irqrestore(&ch->lock, flags);

	if (submitted)
		tasklet_schedule(&ch->cleanup_task);
}

static void al_dma_cleanup_tasklet(struct tasklet_struct *t)
{
	struct al_dma_chan *ch = from_tasklet(ch, t, cleanup_task);
	struct al_dma_device *dev = ch->device;
	uint32_t comp_status;
	int ret;
	unsigned long flags;

	atomic64_inc(&dev->stat_poll_cycles);

	/* Each al_raid_dma_completion() call pops one hardware completion,
	 * mapping 1:1 to the oldest still-submitted descriptor (ring order
	 * guarantees FIFO completion). */
	do {
		ret = al_raid_dma_completion(&dev->hal_dma, ch->idx,
					     &comp_status);
		if (ret > 0) {
			struct al_dma_sw_desc *desc;

			/* Matches vendor al_dma_cleanup_fn's barrier between
			 * seeing a completion and reading the descriptor it
			 * names; smp_read_barrier_depends() no longer exists
			 * in this kernel, smp_rmb() is the modern equivalent. */
			smp_rmb();

			atomic64_inc(&dev->stat_completed);
			if (comp_status) {
				dev_warn(&dev->pdev->dev,
					 "DMA chan %d completion error: 0x%x\n",
					 ch->idx, comp_status);
				atomic64_inc(&dev->stat_errors);
				al_dma_fault(dev, AL_DMA_FAULT_COMPLETION);
			}

			spin_lock_irqsave(&ch->lock, flags);
			if (ch->completed == ch->tail) {
				/* Shouldn't happen (hardware reported a
				 * completion we have no submitted descriptor
				 * for) - warn and stop rather than run past
				 * the ring into unsubmitted slots. */
				spin_unlock_irqrestore(&ch->lock, flags);
				dev_warn(&dev->pdev->dev,
					 "DMA chan %d spurious completion\n",
					 ch->idx);
				atomic64_inc(&dev->stat_errors);
				al_dma_fault(dev, AL_DMA_FAULT_SPURIOUS);
				break;
			}
			desc = &ch->sw_ring[ch->completed];
			ch->completed = (ch->completed + 1) % ch->sw_ring_count;
			ch->stall_start = 0;
			ch->stall_reported = false;
			ch->last_completion = jiffies;
			spin_unlock_irqrestore(&ch->lock, flags);

			dma_cookie_complete(&desc->txd);
			if (desc->txd.callback)
				desc->txd.callback(desc->txd.callback_param);
			dma_run_dependencies(&desc->txd);
		}
	} while (ret > 0);

	/* #23: the real vendor driver also has a full smp_mb() here, after
	 * the completion-drain loop and before the tail-equivalent state
	 * update below becomes visible to other CPUs - also dropped in our
	 * port. */
	smp_mb();

	/* No hardware interrupt (no request_irq anywhere in this driver) -
	 * this tasklet only ever runs from issue_pending() or its own
	 * reschedule below, so it must re-arm itself while work remains
	 * outstanding or nothing will ever poll again. Bounded at 50ms
	 * (~50x a real completion's ~1ms) so a genuinely dead descriptor
	 * logs and gives up instead of pegging a core forever. */
	spin_lock_irqsave(&ch->lock, flags);
	if (ch->completed != ch->tail) {
		if (ch->stall_start == 0)
			ch->stall_start = jiffies;
		if (time_after(jiffies, ch->stall_start + msecs_to_jiffies(AL_DMA_STALL_MS))) {
			if (!ch->stall_reported) {
				dev_warn(&dev->pdev->dev,
					 "DMA chan %d: descriptor(s) never completed "
					 "after %dms (completed=%d tail=%d) - giving "
					 "up polling, not spinning forever\n",
					 ch->idx, AL_DMA_STALL_MS,
					 ch->completed, ch->tail);
				ch->stall_reported = true;
				al_dma_fault(dev, AL_DMA_FAULT_STALL);
			}
		} else {
			tasklet_schedule(&ch->cleanup_task);
		}
	} else {
		ch->stall_start = 0;
		ch->stall_reported = false;
	}
	spin_unlock_irqrestore(&ch->lock, flags);
}

static int al_dma_alloc_chan_resources(struct dma_chan *c)
{
	struct al_dma_chan *ch = to_al_dma_chan(c);
	struct al_dma_device *dev = ch->device;
	struct al_udma_q_params tx_params, rx_params;
	int ring_size;
	int rc = 0;	/* stays 0 on the alloc-failure paths below */

	ring_size = 1 << ring_alloc_order;
	ch->sw_ring_count = ring_size;
	ch->sw_ring = kcalloc(ring_size, sizeof(*ch->sw_ring), GFP_KERNEL);
	if (!ch->sw_ring) {
		al_dma_fault(dev, AL_DMA_FAULT_DMA_ALLOC);
		return -ENOMEM;
	}

	/* Allocate TX submission ring */
	ch->tx_ring = dma_alloc_coherent(&dev->pdev->dev,
		ring_size * sizeof(union al_udma_desc),
		&ch->tx_ring_dma, GFP_KERNEL);
	if (!ch->tx_ring)
		goto err_tx_ring;

	/* Allocate RX submission ring */
	ch->rx_ring = dma_alloc_coherent(&dev->pdev->dev,
		ring_size * sizeof(union al_udma_desc),
		&ch->rx_ring_dma, GFP_KERNEL);
	if (!ch->rx_ring)
		goto err_rx_ring;

	/* Allocate RX completion ring */
	ch->rx_cring = dma_alloc_coherent(&dev->pdev->dev,
		ring_size * AL_DMA_RAID_RX_CDESC_SIZE,
		&ch->rx_cring_dma, GFP_KERNEL);
	if (!ch->rx_cring)
		goto err_rx_cring;

	/* Initialize HAL queue */
	memset(&tx_params, 0, sizeof(tx_params));
	tx_params.size = ring_size;
	tx_params.desc_base = ch->tx_ring;
	tx_params.desc_phy_base = ch->tx_ring_dma;
	/* #23: no TX completion ring - vendor's al_dma_core.c never allocates
	 * chan->tx_dma_cdesc_virt either (confirmed by reading it directly),
	 * matching that here. */
	tx_params.cdesc_base = NULL;
	tx_params.cdesc_phy_base = 0;

	memset(&rx_params, 0, sizeof(rx_params));
	rx_params.size = ring_size;
	rx_params.desc_base = ch->rx_ring;
	rx_params.desc_phy_base = ch->rx_ring_dma;
	rx_params.cdesc_base = ch->rx_cring;
	rx_params.cdesc_phy_base = ch->rx_cring_dma;
	rx_params.cdesc_size = AL_DMA_RAID_RX_CDESC_SIZE;

	rc = al_ssm_dma_q_init(&dev->hal_dma, ch->idx,
				&tx_params, &rx_params, AL_RAID_Q);
	if (rc) {
		dev_err(&dev->pdev->dev,
			"Failed to init DMA queue %d: %d\n", ch->idx, rc);
		al_dma_fault(dev, AL_DMA_FAULT_QUEUE_INIT);
		goto err_q_init;
	}

	/* This driver has no interrupt, only polling - a nonzero coalescing
	 * timeout/threshold could buffer a completion past when polling
	 * checks for it. Disable outright rather than rely on hardware's
	 * power-on default. */
	{
		struct al_udma *rx_udma_for_coal;
		struct al_udma_q *rx_q_for_coal;

		if (al_m2m_udma_handle_get(&dev->hal_dma.m2m_udma, UDMA_RX, &rx_udma_for_coal) == 0 &&
		    al_udma_q_handle_get(rx_udma_for_coal, ch->idx, &rx_q_for_coal) == 0) {
			int coal_rc = al_udma_s2m_q_compl_coal_config(rx_q_for_coal, AL_FALSE, 0);

			if (coal_rc)
				dev_warn(&dev->pdev->dev,
					 "chan %d: failed to disable S2M completion coalescing, rc=%d\n",
					 ch->idx, coal_rc);
		}
	}

	ch->head = 0;
	ch->tail = 0;
	ch->completed = 0;
	ch->stall_start = 0;
	ch->stall_reported = false;
	ch->last_completion = jiffies;

	/* dma_cookie_init removed in 6.12 — handled automatically */

	dev_dbg(&dev->pdev->dev, "DMA chan %d: allocated %d descriptors\n",
		ch->idx, ring_size);

	return ring_size;

err_q_init:
	dma_free_coherent(&dev->pdev->dev,
		ring_size * AL_DMA_RAID_RX_CDESC_SIZE,
		ch->rx_cring, ch->rx_cring_dma);
err_rx_cring:
	dma_free_coherent(&dev->pdev->dev,
		ring_size * sizeof(union al_udma_desc),
		ch->rx_ring, ch->rx_ring_dma);
err_rx_ring:
	dma_free_coherent(&dev->pdev->dev,
		ring_size * sizeof(union al_udma_desc),
		ch->tx_ring, ch->tx_ring_dma);
err_tx_ring:
	kfree(ch->sw_ring);
	ch->sw_ring = NULL;
	if (rc == 0)
		al_dma_fault(dev, AL_DMA_FAULT_DMA_ALLOC);
	return -ENOMEM;
}

static void al_dma_free_chan_resources(struct dma_chan *c)
{
	struct al_dma_chan *ch = to_al_dma_chan(c);
	struct al_dma_device *dev = ch->device;
	int ring_size = ch->sw_ring_count;

	if (!ch->sw_ring)
		return;

	tasklet_kill(&ch->cleanup_task);

	if (ch->rx_cring)
		dma_free_coherent(&dev->pdev->dev,
			ring_size * AL_DMA_RAID_RX_CDESC_SIZE,
			ch->rx_cring, ch->rx_cring_dma);
	if (ch->rx_ring)
		dma_free_coherent(&dev->pdev->dev,
			ring_size * sizeof(union al_udma_desc),
			ch->rx_ring, ch->rx_ring_dma);
	if (ch->tx_ring)
		dma_free_coherent(&dev->pdev->dev,
			ring_size * sizeof(union al_udma_desc),
			ch->tx_ring, ch->tx_ring_dma);

	kfree(ch->sw_ring);
	ch->sw_ring = NULL;

	dev_dbg(&dev->pdev->dev, "DMA chan %d: freed resources\n", ch->idx);
}

/* ======== DMA Operation Preparation ======== */

static struct dma_async_tx_descriptor *
al_dma_prep_dma_memcpy(struct dma_chan *c, dma_addr_t dest,
		       dma_addr_t src, size_t len, unsigned long flags)
{
	struct al_dma_chan *ch = to_al_dma_chan(c);
	struct al_dma_device *dev = ch->device;
	struct al_dma_sw_desc *sw_desc;
	struct al_raid_transaction xaction;
	struct al_block src_block, dst_block;
	struct al_buf src_buf, dst_buf;
	unsigned long irqflags;
	int rc;

	spin_lock_irqsave(&ch->lock, irqflags);
	sw_desc = al_dma_get_sw_desc(ch);
	if (!sw_desc) {
		spin_unlock_irqrestore(&ch->lock, irqflags);
		return NULL;
	}

	src_buf.addr = src;
	src_buf.len = len;
	src_block.bufs = &src_buf;
	src_block.num = 1;
	src_block.tgtid = 0;

	dst_buf.addr = dest;
	dst_buf.len = len;
	dst_block.bufs = &dst_buf;
	dst_block.num = 1;
	dst_block.tgtid = 0;

	memset(&xaction, 0, sizeof(xaction));
	xaction.op = AL_RAID_OP_MEM_CPY;
	/* No NO_SNOOP (matches al_ssm's known-working driver) - no-snoop
	 * completions never invalidate the CPU's cached copy of that ring
	 * slot, reading as a frozen/stuck completion forever. See
	 * docs/kernel-boot-fixes.md Bug 4 (#23). */
	xaction.flags = 0;
	if (flags & DMA_PREP_INTERRUPT)
		xaction.flags |= AL_SSM_INTERRUPT;
	xaction.srcs_blocks = &src_block;
	xaction.num_of_srcs = 1;
	xaction.total_src_bufs = 1;
	xaction.dsts_blocks = &dst_block;
	xaction.num_of_dsts = 1;
	xaction.total_dst_bufs = 1;

	rc = al_raid_dma_prepare(&dev->hal_dma, ch->idx, &xaction);
	if (rc) {
		/* Undo sw_desc allocation */
		ch->head = (ch->head - 1 + ch->sw_ring_count) % ch->sw_ring_count;
		spin_unlock_irqrestore(&ch->lock, irqflags);
		atomic64_inc(&dev->stat_errors);
		al_dma_fault(dev, AL_DMA_FAULT_SUBMIT);
		return NULL;
	}

	sw_desc->tx_descs_count = xaction.tx_descs_count;
	sw_desc->is_memset = false;

	dma_async_tx_descriptor_init(&sw_desc->txd, c);
	sw_desc->txd.tx_submit = al_dma_tx_submit;
	sw_desc->txd.flags = flags;

	spin_unlock_irqrestore(&ch->lock, irqflags);

	return &sw_desc->txd;
}

static struct dma_async_tx_descriptor *
al_dma_prep_dma_xor(struct dma_chan *c, dma_addr_t dest,
		    dma_addr_t *src, unsigned int src_cnt,
		    size_t len, unsigned long flags)
{
	struct al_dma_chan *ch = to_al_dma_chan(c);
	struct al_dma_device *dev = ch->device;
	struct al_dma_sw_desc *sw_desc;
	struct al_raid_transaction xaction;
	struct al_block *src_blocks;
	struct al_buf *src_bufs;
	struct al_block dst_block;
	struct al_buf dst_buf;
	unsigned long irqflags;
	unsigned int i;
	int rc;

	if (src_cnt > AL_DMA_MAX_XOR)
		return NULL;

	src_blocks = kcalloc(src_cnt, sizeof(*src_blocks), GFP_ATOMIC);
	if (!src_blocks)
		return NULL;
	src_bufs = kcalloc(src_cnt, sizeof(*src_bufs), GFP_ATOMIC);
	if (!src_bufs) {
		kfree(src_blocks);
		return NULL;
	}

	for (i = 0; i < src_cnt; i++) {
		src_bufs[i].addr = src[i];
		src_bufs[i].len = len;
		src_blocks[i].bufs = &src_bufs[i];
		src_blocks[i].num = 1;
		src_blocks[i].tgtid = 0;
	}

	dst_buf.addr = dest;
	dst_buf.len = len;
	dst_block.bufs = &dst_buf;
	dst_block.num = 1;
	dst_block.tgtid = 0;

	spin_lock_irqsave(&ch->lock, irqflags);
	sw_desc = al_dma_get_sw_desc(ch);
	if (!sw_desc) {
		spin_unlock_irqrestore(&ch->lock, irqflags);
		kfree(src_bufs);
		kfree(src_blocks);
		return NULL;
	}

	memset(&xaction, 0, sizeof(xaction));
	xaction.op = AL_RAID_OP_P_CALC;
	/* No NO_SNOOP (matches al_ssm's known-working driver) - no-snoop
	 * completions never invalidate the CPU's cached copy of that ring
	 * slot, reading as a frozen/stuck completion forever. See
	 * docs/kernel-boot-fixes.md Bug 4 (#23). */
	xaction.flags = 0;
	if (flags & DMA_PREP_INTERRUPT)
		xaction.flags |= AL_SSM_INTERRUPT;
	xaction.srcs_blocks = src_blocks;
	xaction.num_of_srcs = src_cnt;
	xaction.total_src_bufs = src_cnt;
	xaction.dsts_blocks = &dst_block;
	xaction.num_of_dsts = 1;
	xaction.total_dst_bufs = 1;

	rc = al_raid_dma_prepare(&dev->hal_dma, ch->idx, &xaction);
	if (rc) {
		ch->head = (ch->head - 1 + ch->sw_ring_count) % ch->sw_ring_count;
		spin_unlock_irqrestore(&ch->lock, irqflags);
		atomic64_inc(&dev->stat_errors);
		al_dma_fault(dev, AL_DMA_FAULT_SUBMIT);
		kfree(src_bufs);
		kfree(src_blocks);
		return NULL;
	}

	sw_desc->tx_descs_count = xaction.tx_descs_count;
	sw_desc->is_memset = false;

	dma_async_tx_descriptor_init(&sw_desc->txd, c);
	sw_desc->txd.tx_submit = al_dma_tx_submit;
	sw_desc->txd.flags = flags;

	spin_unlock_irqrestore(&ch->lock, irqflags);

	kfree(src_bufs);
	kfree(src_blocks);

	return &sw_desc->txd;
}

static struct dma_async_tx_descriptor *
al_dma_prep_dma_pq(struct dma_chan *c, dma_addr_t *dst,
		   dma_addr_t *src, unsigned int src_cnt,
		   const unsigned char *scf, size_t len,
		   unsigned long flags)
{
	struct al_dma_chan *ch = to_al_dma_chan(c);
	struct al_dma_device *dev = ch->device;
	struct al_dma_sw_desc *sw_desc;
	struct al_raid_transaction xaction;
	struct al_block *src_blocks;
	struct al_buf *src_bufs;
	struct al_block dst_blocks[2];
	struct al_buf dst_bufs[2];
	uint8_t *coefs;
	unsigned long irqflags;
	unsigned int i, num_dsts;
	int rc;

	if (src_cnt > AL_DMA_MAX_XOR)
		return NULL;

	src_blocks = kcalloc(src_cnt, sizeof(*src_blocks), GFP_ATOMIC);
	if (!src_blocks)
		return NULL;
	src_bufs = kcalloc(src_cnt, sizeof(*src_bufs), GFP_ATOMIC);
	if (!src_bufs) {
		kfree(src_blocks);
		return NULL;
	}
	coefs = kcalloc(src_cnt, sizeof(*coefs), GFP_ATOMIC);
	if (!coefs) {
		kfree(src_bufs);
		kfree(src_blocks);
		return NULL;
	}

	for (i = 0; i < src_cnt; i++) {
		src_bufs[i].addr = src[i];
		src_bufs[i].len = len;
		src_blocks[i].bufs = &src_bufs[i];
		src_blocks[i].num = 1;
		src_blocks[i].tgtid = 0;
		coefs[i] = scf[i];
	}

	num_dsts = 0;

	/* P destination (XOR parity) */
	if (!(flags & DMA_PREP_PQ_DISABLE_P)) {
		dst_bufs[num_dsts].addr = dst[0];
		dst_bufs[num_dsts].len = len;
		dst_blocks[num_dsts].bufs = &dst_bufs[num_dsts];
		dst_blocks[num_dsts].num = 1;
		dst_blocks[num_dsts].tgtid = 0;
		num_dsts++;
	}

	/* Q destination (Reed-Solomon) */
	if (!(flags & DMA_PREP_PQ_DISABLE_Q)) {
		dst_bufs[num_dsts].addr = dst[1];
		dst_bufs[num_dsts].len = len;
		dst_blocks[num_dsts].bufs = &dst_bufs[num_dsts];
		dst_blocks[num_dsts].num = 1;
		dst_blocks[num_dsts].tgtid = 0;
		num_dsts++;
	}

	spin_lock_irqsave(&ch->lock, irqflags);
	sw_desc = al_dma_get_sw_desc(ch);
	if (!sw_desc) {
		spin_unlock_irqrestore(&ch->lock, irqflags);
		goto err_free;
	}

	memset(&xaction, 0, sizeof(xaction));

	/* Choose operation based on which parity is enabled */
	if ((flags & DMA_PREP_PQ_DISABLE_P) && !(flags & DMA_PREP_PQ_DISABLE_Q))
		xaction.op = AL_RAID_OP_Q_CALC;
	else if (!(flags & DMA_PREP_PQ_DISABLE_P) && (flags & DMA_PREP_PQ_DISABLE_Q))
		xaction.op = AL_RAID_OP_P_CALC;
	else
		xaction.op = AL_RAID_OP_PQ_CALC;

	/* No NO_SNOOP (matches al_ssm's known-working driver) - no-snoop
	 * completions never invalidate the CPU's cached copy of that ring
	 * slot, reading as a frozen/stuck completion forever. See
	 * docs/kernel-boot-fixes.md Bug 4 (#23). */
	xaction.flags = 0;
	if (flags & DMA_PREP_INTERRUPT)
		xaction.flags |= AL_SSM_INTERRUPT;
	xaction.srcs_blocks = src_blocks;
	xaction.num_of_srcs = src_cnt;
	xaction.total_src_bufs = src_cnt;
	xaction.dsts_blocks = dst_blocks;
	xaction.num_of_dsts = num_dsts;
	xaction.total_dst_bufs = num_dsts;
	xaction.coefs = coefs;

	rc = al_raid_dma_prepare(&dev->hal_dma, ch->idx, &xaction);
	if (rc) {
		ch->head = (ch->head - 1 + ch->sw_ring_count) % ch->sw_ring_count;
		spin_unlock_irqrestore(&ch->lock, irqflags);
		atomic64_inc(&dev->stat_errors);
		al_dma_fault(dev, AL_DMA_FAULT_SUBMIT);
		goto err_free;
	}

	sw_desc->tx_descs_count = xaction.tx_descs_count;
	sw_desc->is_memset = false;

	dma_async_tx_descriptor_init(&sw_desc->txd, c);
	sw_desc->txd.tx_submit = al_dma_tx_submit;
	sw_desc->txd.flags = flags;

	spin_unlock_irqrestore(&ch->lock, irqflags);

	kfree(coefs);
	kfree(src_bufs);
	kfree(src_blocks);

	return &sw_desc->txd;

err_free:
	kfree(coefs);
	kfree(src_bufs);
	kfree(src_blocks);
	return NULL;
}

/* ======== PCI Probe / Remove ======== */

static const struct pci_device_id al_dma_pci_ids[] = {
	{ PCI_DEVICE_CLASS(0x010400, 0xffffff) }, /* RAID controller class only */
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, al_dma_pci_ids);

static int al_dma_pci_probe(struct pci_dev *pdev,
			    const struct pci_device_id *id)
{
	struct al_dma_device *aldev;
	struct al_ssm_dma_params hal_params;
	struct dma_device *dma;
	int rc, i, num_channels;

	dev_info(&pdev->dev, "Annapurna Labs DMA engine v%s\n", DRV_VERSION);

	aldev = kzalloc(sizeof(*aldev), GFP_KERNEL);
	if (!aldev)
		return -ENOMEM;

	aldev->pdev = pdev;
	pci_set_drvdata(pdev, aldev);

	rc = pci_enable_device(pdev);
	if (rc) {
		dev_err(&pdev->dev, "Failed to enable PCI device\n");
		goto err_free;
	}

	rc = pci_request_regions(pdev, DRV_NAME);
	if (rc) {
		dev_err(&pdev->dev, "Failed to request PCI regions\n");
		goto err_disable;
	}

	pci_set_master(pdev);

	rc = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(40));
	if (rc) {
		dev_err(&pdev->dev, "Failed to set DMA mask\n");
		goto err_regions;
	}

	/* Map BARs */
	aldev->udma_regs = pci_iomap(pdev, 0, 0);
	if (!aldev->udma_regs) {
		dev_err(&pdev->dev, "Failed to map BAR 0 (UDMA regs)\n");
		rc = -ENOMEM;
		goto err_regions;
	}

	aldev->app_regs = pci_iomap(pdev, 4, 0);
	if (!aldev->app_regs) {
		dev_err(&pdev->dev, "Failed to map BAR 4 (App regs)\n");
		rc = -ENOMEM;
		goto err_unmap_udma;
	}

	/* Read PCI revision to determine Alpine version */
	pci_read_config_byte(pdev, PCI_REVISION_ID, &aldev->rev_id);
	dev_info(&pdev->dev, "PCI revision ID: %d\n", aldev->rev_id);

	/* Initialize HAL */
	num_channels = min(max_channels, DMA_MAX_Q);
	aldev->num_channels = num_channels;

	memset(&hal_params, 0, sizeof(hal_params));
	hal_params.rev_id = aldev->rev_id;
	hal_params.udma_regs_base = aldev->udma_regs;
	hal_params.name = DRV_NAME;
	hal_params.num_of_queues = num_channels;

	rc = al_ssm_dma_init(&aldev->hal_dma, &hal_params);
	if (rc) {
		dev_err(&pdev->dev, "Failed to initialize SSM DMA HAL: %d\n", rc);
		goto err_unmap_app;
	}

	/* Set DMA to normal state */
	rc = al_ssm_dma_state_set(&aldev->hal_dma, UDMA_NORMAL);
	if (rc) {
		dev_err(&pdev->dev, "Failed to set DMA state: %d\n", rc);
		goto err_unmap_app;
	}

	/* #23: vendor's al_dma_core.c calls this unconditionally in probe
	 * (al_dma_setup_interrupts()) before any channel does DMA work; our
	 * port never called it at all. LEGACY mode since we poll for
	 * completion rather than wiring up MSI-X - untested whether the
	 * M2S/S2M state machine depends on this being configured regardless
	 * of whether Linux ever services a real interrupt from it. */
	rc = al_udma_iofic_config((struct unit_regs *)aldev->udma_regs,
				   AL_IOFIC_MODE_LEGACY, 0x480, 0x480, 0x1E0, 0x1E0);
	if (rc) {
		dev_err(&pdev->dev, "al_udma_iofic_config failed: %d\n", rc);
		goto err_unmap_app;
	}

	/* s2m_aborts_disable=0x1E0 above masks S2M_NO_DESC_TO, so a starved
	 * RX completion ring would hang silently instead of aborting.
	 * al_ssm's HAL re-enables it post-init since RAID (unlike eth) has
	 * no MAC to back-pressure instead - matches here, untested standalone. */
	al_iofic_abort_mask_clear(
		al_udma_iofic_reg_base_get((struct unit_regs *)aldev->udma_regs,
					    AL_UDMA_IOFIC_LEVEL_SECONDARY),
		AL_INT_GROUP_B,
		AL_INT_2ND_GROUP_B_S2M_NO_DESC_TO);

	/* Initialize RAID GF tables */
	al_raid_init(&aldev->hal_dma, aldev->app_regs);

	/* Register with dmaengine framework */
	dma = &aldev->dma_dev;
	dma->dev = &pdev->dev;

	dma_cap_set(DMA_MEMCPY, dma->cap_mask);
	dma_cap_set(DMA_XOR, dma->cap_mask);
	dma_cap_set(DMA_PQ, dma->cap_mask);

	dma->device_alloc_chan_resources = al_dma_alloc_chan_resources;
	dma->device_free_chan_resources = al_dma_free_chan_resources;
	dma->device_prep_dma_memcpy = al_dma_prep_dma_memcpy;
	dma->device_prep_dma_xor = al_dma_prep_dma_xor;
	dma->device_prep_dma_pq = al_dma_prep_dma_pq;
	dma->device_issue_pending = al_dma_issue_pending;
	dma->device_tx_status = al_dma_tx_status;

	dma->max_xor = AL_DMA_MAX_XOR;
	dma->max_pq = AL_DMA_MAX_XOR;
	dma->pq_align = 1;
	dma->xor_align = 1;
	dma->copy_align = 0;

	INIT_LIST_HEAD(&dma->channels);

	for (i = 0; i < num_channels; i++) {
		struct al_dma_chan *ch = &aldev->channels[i];

		ch->device = aldev;
		ch->idx = i;
		spin_lock_init(&ch->lock);
		tasklet_setup(&ch->cleanup_task, al_dma_cleanup_tasklet);

		ch->chan.device = dma;
		list_add_tail(&ch->chan.device_node, &dma->channels);
	}

	rc = al_dma_devlink_init(aldev);
	if (rc) {
		dev_err(&pdev->dev, "Failed to init devlink: %d\n", rc);
		goto err_unmap_app;
	}

	rc = dma_async_device_register(dma);
	if (rc) {
		dev_err(&pdev->dev, "Failed to register DMA device: %d\n", rc);
		goto err_devlink;
	}

	rc = al_dma_sysfs_init(aldev);
	if (rc)
		dev_err(&pdev->dev, "Failed to init sysfs: %d\n", rc);

	dev_info(&pdev->dev,
		 "Registered %d DMA channels (XOR max %d sources, PQ max %d sources)\n",
		 num_channels, AL_DMA_MAX_XOR, AL_DMA_MAX_XOR);

	return 0;

err_devlink:
	al_dma_devlink_fini(aldev);
err_unmap_app:
	pci_iounmap(pdev, aldev->app_regs);
err_unmap_udma:
	pci_iounmap(pdev, aldev->udma_regs);
err_regions:
	pci_release_regions(pdev);
err_disable:
	pci_disable_device(pdev);
err_free:
	kfree(aldev);
	return rc;
}

static void al_dma_pci_remove(struct pci_dev *pdev)
{
	struct al_dma_device *aldev = pci_get_drvdata(pdev);
	int i;

	al_dma_sysfs_remove(aldev);

	dma_async_device_unregister(&aldev->dma_dev);

	for (i = 0; i < aldev->num_channels; i++)
		tasklet_kill(&aldev->channels[i].cleanup_task);

	/* After this no fault can still be in flight to a live reporter. */
	al_dma_devlink_fini(aldev);

	/* Set DMA to disable state */
	{
		int rc = al_ssm_dma_state_set(&aldev->hal_dma, UDMA_DISABLE);

		if (rc)
			dev_err(&pdev->dev, "al_dma: failed to disable dma state (rc %d)\n", rc);
	}

	pci_iounmap(pdev, aldev->app_regs);
	pci_iounmap(pdev, aldev->udma_regs);
	pci_release_regions(pdev);
	pci_disable_device(pdev);

	kfree(aldev);

	dev_info(&pdev->dev, "Annapurna Labs DMA engine removed\n");
}

static struct pci_driver al_dma_pci_driver = {
	.name		= DRV_NAME,
	.id_table	= al_dma_pci_ids,
	.probe		= al_dma_pci_probe,
	.remove		= al_dma_pci_remove,
};

module_pci_driver(al_dma_pci_driver);

MODULE_AUTHOR("Annapurna Labs");
MODULE_DESCRIPTION("Annapurna Labs Alpine V2 DMA Engine (RAID5/6 acceleration)");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
