// SPDX-License-Identifier: GPL-2.0
/*
 * Annapurna Labs Alpine V2 SSM (Security Services Module) Crypto Driver
 *
 * Asynchronous PCI driver for the Alpine V2 SoC hardware crypto engine.
 * Registers AES-XTS and AES-CBC with the Linux crypto framework
 * so dm-crypt/LUKS can use hardware-accelerated encryption transparently.
 *
 * PCI device: vendor 0x1c36, device 0x0022, class 0x100000
 * BAR 0: UDMA registers (128KB)
 * BAR 4: Application/crypto registers (64KB)
 *
 * Architecture:
 *   dm-crypt  ->  Linux crypto API (async skcipher)
 *             ->  this driver: map req->src/dst, submit to HW, spin briefly
 *                 for the result, else return -EINPROGRESS
 *             ->  completion workqueue polls UDMA, calls skcipher_request_complete()
 *             ->  AL HAL SSM crypto  ->  UDMA DMA  ->  HW crypto engine
 *   Lengths or shapes the engine cannot do go to a software fallback tfm.
 *
 * IMPORTANT: The UDMA requires SEPARATE DMA buffers for submission
 * descriptor rings and completion descriptor rings. Sharing the same
 * buffer causes corruption when multiple operations are in-flight
 * because the hardware overwrites submission descriptors with completion data.
 *
 * Copyright (C) 2015 Annapurna Labs Ltd. (HAL code)
 * Copyright (C) 2024-2026 SecFirstNAS contributors (driver, async rewrite)
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <crypto/internal/skcipher.h>
#include <crypto/aes.h>
#include <crypto/xts.h>
#include <crypto/scatterwalk.h>
#include <crypto/algapi.h>

#include "al_ssm_drv.h"

#define AL_SSM_VENDOR_ID	0x1c36
#define AL_SSM_DEVICE_ID	0x0022
#define AL_SSM_CLASS_CRYPTO	0x100000

/*
 * g_ssm_dev is the only way a new tfm finds the device; g_ssm_lock makes
 * "look up + take a reference" atomic against remove (#301). Every live tfm
 * holds a reference, so the memory outlives a PCI unbind.
 */
static struct al_ssm_dev *g_ssm_dev;
static DEFINE_SPINLOCK(g_ssm_lock);
/* Live tfms: the static al_ssm_algs[] cannot be re-registered under them. */
static atomic_t g_ssm_tfms = ATOMIC_INIT(0);

static struct al_ssm_dev *al_ssm_dev_get(void)
{
	struct al_ssm_dev *dev;

	spin_lock(&g_ssm_lock);
	dev = g_ssm_dev;
	if (dev)
		kref_get(&dev->ref);
	spin_unlock(&g_ssm_lock);
	return dev;
}

static void al_ssm_dev_release(struct kref *ref)
{
	struct al_ssm_dev *dev = container_of(ref, struct al_ssm_dev, ref);

	pci_dev_put(dev->pdev);	/* dev->dev: requests unmap through it until the last put */
	kfree(dev);
}

static void al_ssm_dev_put(struct al_ssm_dev *dev)
{
	kref_put(&dev->ref, al_ssm_dev_release);
}

static void al_ssm_poll_completions(struct work_struct *work);
static void al_ssm_schedule_poll(struct al_ssm_dev *dev, unsigned long delay);

/* ── DMA mapping (no allocation in the request path) ──────────────────── */

/*
 * Append [addr, addr+len) to @bufs as descriptor-sized buffers (#332): the HAL
 * writes every descriptor length as `len & 0xffff` and checks nothing, so no
 * buffer may reach 64 KiB. Physically adjacent ranges are merged. Returns
 * -E2BIG when the request needs more descriptors than one transaction holds.
 */
static int al_ssm_add_range(struct al_buf *bufs, unsigned int *n,
			    dma_addr_t addr, unsigned int len)
{
	while (len) {
		struct al_buf *last = *n ? &bufs[*n - 1] : NULL;
		unsigned int chunk;

		if (last && last->addr + last->len == addr &&
		    last->len < AL_SSM_MAX_DESC_LEN) {
			chunk = min_t(unsigned int, len, AL_SSM_MAX_DESC_LEN - last->len);
			last->len += chunk;
		} else {
			if (*n == AL_SSM_MAX_DATA_BUFS)
				return -E2BIG;
			chunk = min_t(unsigned int, len, AL_SSM_MAX_DESC_LEN);
			bufs[*n].addr = addr;
			bufs[*n].len = chunk;
			(*n)++;
		}
		addr += chunk;
		len -= chunk;
	}
	return 0;
}

/* The first @nbytes of a mapped scatterlist as descriptor buffers. */
static int al_ssm_sg_to_bufs(struct al_buf *bufs, unsigned int *n,
			     struct scatterlist *sgl, int mapped,
			     unsigned int nbytes)
{
	struct scatterlist *sg;
	int i, rc;

	*n = 0;
	for_each_sg(sgl, sg, mapped, i) {
		unsigned int len = min_t(unsigned int, sg_dma_len(sg), nbytes);

		rc = al_ssm_add_range(bufs, n, sg_dma_address(sg), len);
		if (rc)
			return rc;
		nbytes -= len;
		if (!nbytes)
			break;
	}
	return nbytes ? -EINVAL : 0;
}

static void al_ssm_unmap(struct al_ssm_reqctx *rctx)
{
	struct device *d = rctx->dev->dev;
	struct skcipher_request *req = rctx->req;

	if (rctx->hw_mapped)
		dma_unmap_single(d, rctx->hw_dma, sizeof(rctx->hw), DMA_TO_DEVICE);
	if (rctx->dst_mapped)
		dma_unmap_sg(d, req->dst, rctx->dst_nents, DMA_FROM_DEVICE);
	if (rctx->src_mapped)
		dma_unmap_sg(d, req->src, rctx->src_nents,
			     rctx->in_place ? DMA_BIDIRECTIONAL : DMA_TO_DEVICE);
	rctx->hw_mapped = rctx->dst_mapped = rctx->src_mapped = false;
}

/*
 * Map the request's own scatterlists and build the per-request SA + IV.
 * No allocation: everything lives in the request context. Any failure leaves
 * nothing mapped, and the caller serves the request from the fallback.
 */
static int al_ssm_prepare(struct al_ssm_reqctx *rctx, struct al_ssm_ctx *ctx,
			  struct skcipher_request *req, enum al_crypto_dir dir)
{
	struct al_ssm_dev *dev = ctx->dev;
	struct al_crypto_sa sa;
	unsigned int nbytes = req->cryptlen;
	int rc, mapped;

	rctx->dev = dev;
	rctx->req = req;
	rctx->dir = dir;
	rctx->enc_type = ctx->enc_type;
	rctx->nbytes = nbytes;
	rctx->in_place = req->src == req->dst;
	rctx->src_mapped = rctx->dst_mapped = rctx->hw_mapped = false;

	/* CBC output IV: the last ciphertext block. A decrypt must take it from
	 * the input before the engine runs - in place it is overwritten. */
	if (rctx->enc_type == AL_CRYPT_AES_CBC && dir == AL_CRYPT_DECRYPT)
		scatterwalk_map_and_copy(rctx->next_iv, req->src,
					 nbytes - AES_BLOCK_SIZE, AES_BLOCK_SIZE, 0);

	rctx->src_nents = sg_nents_for_len(req->src, nbytes);
	rctx->dst_nents = rctx->in_place ? rctx->src_nents :
			  sg_nents_for_len(req->dst, nbytes);
	if (rctx->src_nents < 0 || rctx->dst_nents < 0)
		return -EINVAL;

	mapped = dma_map_sg(dev->dev, req->src, rctx->src_nents,
			    rctx->in_place ? DMA_BIDIRECTIONAL : DMA_TO_DEVICE);
	if (!mapped) {
		rc = -ENOMEM;
		goto err;
	}
	rctx->src_mapped = true;
	rc = al_ssm_sg_to_bufs(rctx->src_bufs, &rctx->src_num, req->src, mapped, nbytes);
	if (rc)
		goto err;

	if (rctx->in_place) {
		memcpy(rctx->dst_bufs, rctx->src_bufs, sizeof(rctx->dst_bufs));
		rctx->dst_num = rctx->src_num;
	} else {
		mapped = dma_map_sg(dev->dev, req->dst, rctx->dst_nents, DMA_FROM_DEVICE);
		if (!mapped) {
			rc = -ENOMEM;
			goto err;
		}
		rctx->dst_mapped = true;
		rc = al_ssm_sg_to_bufs(rctx->dst_bufs, &rctx->dst_num, req->dst, mapped, nbytes);
		if (rc)
			goto err;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sa_op = AL_CRYPT_ENC;
	sa.enc_type = ctx->enc_type;
	sa.aes_ksize = ctx->aes_ksize;
	if (ctx->enc_type == AL_CRYPT_AES_XTS || ctx->enc_type == AL_CRYPT_AES_CTR)
		sa.cntr_size = AL_CRYPT_CNTR_128_BIT;
	memcpy(sa.enc_key, ctx->key, min_t(unsigned int, ctx->keylen, sizeof(sa.enc_key)));
	if (ctx->enc_type == AL_CRYPT_AES_XTS)
		memcpy(sa.enc_xts_tweak_key, ctx->xts_key,
		       min_t(unsigned int, ctx->keylen, sizeof(sa.enc_xts_tweak_key)));
	memcpy(sa.enc_iv, req->iv, AES_BLOCK_SIZE);

	memset(&rctx->hw, 0, sizeof(rctx->hw));
	al_crypto_hw_sa_init(&sa, &rctx->hw.sa);
	memcpy(rctx->hw.iv, req->iv, AES_BLOCK_SIZE);

	rctx->hw_dma = dma_map_single(dev->dev, &rctx->hw, sizeof(rctx->hw),
				      DMA_TO_DEVICE);
	if (dma_mapping_error(dev->dev, rctx->hw_dma)) {
		rc = -ENOMEM;
		goto err;
	}
	rctx->hw_mapped = true;
	return 0;

err:
	al_ssm_unmap(rctx);
	return rc;
}

/* ── Submit, completion, backlog ───────────────────────────────────────
 *
 * One lock (chan->lock) covers the ring, the pending list and the backlog
 * queue. Every completion is consumed by al_ssm_service_locked(), which
 * also refills the ring from the backlog, so a request is never queued
 * behind a ring nobody will drain (#300). Callbacks run after the unlock.
 *
 * The SSM engine completes most 4KB operations in < 10us, so a submitter
 * spins briefly for its own request and returns 0 (synchronous) when it
 * finishes; otherwise the poll worker completes it asynchronously.
 * ──────────────────────────────────────────────────────────────────── */

#define AL_SSM_INLINE_POLL_US	500	/* Max spin-poll time per request */

/* One hardware completion consumed: counters + stall clock. Caller holds
 * chan->lock. */
static void al_ssm_account_completion(struct al_ssm_dev *dev, uint32_t comp_status)
{
	atomic64_inc(&dev->stat_completed);
	dev->last_completion = jiffies;
	dev->stall_clock = jiffies;
	dev->stall_reported = false;
	if (comp_status) {
		atomic64_inc(&dev->stat_errors);
		al_ssm_fault(dev, AL_SSM_FAULT_COMPLETION);
	}
}

/* Hand a finished request to whoever completes it. An inline submitter still
 * spinning on its own request owns the result (chan->lock); everyone else
 * goes on @done and is completed after the unlock. */
static void al_ssm_finish_locked(struct al_ssm_reqctx *rctx, int status,
				 struct list_head *done)
{
	rctx->status = status;
	if (rctx->inline_wait)
		rctx->finished = true;
	else
		list_add_tail(&rctx->list, done);
}

/**
 * al_ssm_hw_submit_locked - put one prepared request on the hardware ring
 *
 * Returns 0 (now on chan->pending), -ENOSPC (ring or in-flight cap full),
 * or a hard error. Caller holds chan->lock.
 */
static int al_ssm_hw_submit_locked(struct al_ssm_dev *dev,
				   struct al_ssm_reqctx *rctx)
{
	struct al_ssm_chan *chan = &dev->channel;
	struct al_crypto_transaction xaction;
	int rc;

	if (chan->pending_count >= AL_SSM_MAX_IN_FLIGHT)
		return -ENOSPC;

	memset(&xaction, 0, sizeof(xaction));
	xaction.dir = rctx->dir;
	xaction.flags = AL_SSM_INTERRUPT;
	xaction.src.bufs = rctx->src_bufs;
	xaction.src.num = rctx->src_num;
	xaction.src_size = rctx->nbytes;
	xaction.dst.bufs = rctx->dst_bufs;
	xaction.dst.num = rctx->dst_num;
	xaction.sa_indx = 0;
	xaction.sa_in.addr = rctx->hw_dma + offsetof(struct al_ssm_hwbuf, sa);
	xaction.sa_in.len = sizeof(rctx->hw.sa);
	xaction.enc_iv_in.addr = rctx->hw_dma + offsetof(struct al_ssm_hwbuf, iv);
	xaction.enc_iv_in.len = AES_BLOCK_SIZE;

	rc = al_crypto_dma_prepare(&dev->ssm_dma, 0, &xaction);
	if (rc)
		return -ENOSPC;

	rc = al_crypto_dma_action(&dev->ssm_dma, 0, xaction.tx_descs_count);
	if (rc) {
		atomic64_inc(&dev->stat_errors);
		al_ssm_fault(dev, AL_SSM_FAULT_SUBMIT);
		return rc;
	}
	atomic64_inc(&dev->stat_submitted);

	/* Completions are FIFO: pending order is ring order. */
	if (!chan->pending_count)
		dev->stall_clock = jiffies;	/* idle -> busy starts the stall clock */
	list_add_tail(&rctx->list, &chan->pending);
	chan->pending_count++;
	atomic_inc(&dev->total_pending);
	return 0;
}

/* Reap every finished completion off the ring. Caller holds chan->lock. */
static int al_ssm_reap_locked(struct al_ssm_dev *dev, struct list_head *done)
{
	struct al_ssm_chan *chan = &dev->channel;
	struct al_ssm_reqctx *rctx;
	uint32_t comp_status;
	int reaped = 0;

	while (!list_empty(&chan->pending)) {
		if (al_crypto_dma_completion(&dev->ssm_dma, 0, &comp_status) <= 0)
			break;
		/* FIFO: the first pending entry is the one that completed */
		rctx = list_first_entry(&chan->pending, struct al_ssm_reqctx, list);
		list_del(&rctx->list);
		chan->pending_count--;
		atomic_dec(&dev->total_pending);
		al_ssm_account_completion(dev, comp_status);
		al_ssm_finish_locked(rctx, comp_status ? -EIO : 0, done);
		reaped++;
	}
	return reaped;
}

static struct al_ssm_reqctx *al_ssm_areq_ctx(struct crypto_async_request *areq)
{
	return skcipher_request_ctx_dma(skcipher_request_cast(areq));
}

/*
 * Take the next backlogged request off the queue. The MAY_BACKLOG submitter
 * this pop moves out of the backlog zone is owed an -EINPROGRESS
 * (crypto_enqueue_request() contract). It is flagged, not called back here:
 * the flag is honoured by al_ssm_complete_one(), the only place the request is
 * known to still exist. Calling back after the unlock could race another
 * thread completing (and the caller freeing) a request this pass just put on
 * the ring.
 */
static struct al_ssm_reqctx *al_ssm_backlog_pop_locked(struct al_ssm_chan *chan)
{
	struct crypto_async_request *backlog = crypto_get_backlog(&chan->backlog);
	struct crypto_async_request *areq = crypto_dequeue_request(&chan->backlog);

	if (backlog)
		al_ssm_areq_ctx(backlog)->notify_owed = true;
	return areq ? al_ssm_areq_ctx(areq) : NULL;
}

/* Move backlogged requests onto the ring while it has room. */
static void al_ssm_fill_locked(struct al_ssm_dev *dev, struct list_head *done)
{
	struct al_ssm_chan *chan = &dev->channel;
	struct al_ssm_reqctx *rctx;
	int rc;

	while (!chan->stopping && crypto_queue_len(&chan->backlog) &&
	       chan->pending_count < AL_SSM_MAX_IN_FLIGHT) {
		rctx = al_ssm_backlog_pop_locked(chan);
		rc = al_ssm_hw_submit_locked(dev, rctx);
		if (rc == -ENOSPC && chan->pending_count) {
			/* Out of descriptors, not slots: back to the head, wait
			 * for the next completion. */
			crypto_enqueue_request_head(&chan->backlog, &rctx->req->base);
			break;
		}
		if (rc)	/* hard error, or nothing in flight to wait for */
			al_ssm_finish_locked(rctx, rc == -ENOSPC ? -EIO : rc, done);
	}
}

/* The one place completions are consumed and the backlog is drained. */
static int al_ssm_service_locked(struct al_ssm_dev *dev, struct list_head *done)
{
	int reaped = al_ssm_reap_locked(dev, done);

	al_ssm_fill_locked(dev, done);
	return reaped;
}

/*
 * CBC output IV (7.1 skcipher IV-chaining contract): the last ciphertext
 * block - read from req->dst after an encrypt, saved from the input before a
 * decrypt (al_ssm_prepare). XTS and the rest: no update. Called after the
 * unmap, once the CPU may read the output.
 */
static void al_ssm_update_iv(struct al_ssm_reqctx *rctx)
{
	struct skcipher_request *req = rctx->req;

	if (rctx->enc_type != AL_CRYPT_AES_CBC)
		return;
	if (rctx->dir == AL_CRYPT_ENCRYPT)
		scatterwalk_map_and_copy(req->iv, req->dst,
					 rctx->nbytes - AES_BLOCK_SIZE, AES_BLOCK_SIZE, 0);
	else
		memcpy(req->iv, rctx->next_iv, AES_BLOCK_SIZE);
}

/* The engine is done with the request: unmap, then publish the IV. */
static void al_ssm_finish_request(struct al_ssm_reqctx *rctx, int err)
{
	al_ssm_unmap(rctx);
	if (!err)
		al_ssm_update_iv(rctx);
}

static void al_ssm_complete_one(struct al_ssm_reqctx *rctx, int err)
{
	struct skcipher_request *req = rctx->req;

	al_ssm_finish_request(rctx, err);
	if (rctx->notify_owed)	/* promoted out of the backlog: say so first */
		crypto_request_complete(&req->base, -EINPROGRESS);
	skcipher_request_complete(req, err);
}

/*
 * Callbacks for what a service pass collected. No lock held. Completion
 * callbacks assume softirq context (crypto_engine asserts it), but this runs
 * from the poll worker and from submitters: disable BH around them.
 */
static void al_ssm_run_done(struct list_head *done)
{
	struct al_ssm_reqctx *rctx, *tmp;

	if (list_empty(done))
		return;
	local_bh_disable();
	list_for_each_entry_safe(rctx, tmp, done, list) {
		list_del(&rctx->list);
		al_ssm_complete_one(rctx, rctx->status);
	}
	local_bh_enable();
}

/* Reap + refill + complete. Returns the number of completions reaped. */
static int al_ssm_service(struct al_ssm_dev *dev)
{
	struct al_ssm_chan *chan = &dev->channel;
	LIST_HEAD(done);
	unsigned long flags;
	int reaped;

	spin_lock_irqsave(&chan->lock, flags);
	reaped = al_ssm_service_locked(dev, &done);
	spin_unlock_irqrestore(&chan->lock, flags);

	al_ssm_run_done(&done);
	return reaped;
}

/**
 * al_ssm_submit_and_poll - submit one request, try to finish it inline
 *
 * Returns:
 *   0            - completed synchronously, output already in req->dst
 *  -EIO          - hardware completion error (synchronous)
 *  -EINPROGRESS  - accepted; the worker completes it through the callback
 *  -EBUSY        - queued behind a full ring, MAY_BACKLOG request
 *  -ENOSPC       - queue full, request not backlog-capable; not accepted
 *  -ESHUTDOWN    - removed; not accepted
 */
static int al_ssm_submit_and_poll(struct al_ssm_reqctx *rctx)
{
	struct al_ssm_dev *dev = rctx->dev;
	struct al_ssm_chan *chan = &dev->channel;
	LIST_HEAD(done);
	unsigned long flags;
	ktime_t deadline;
	int rc;

	spin_lock_irqsave(&chan->lock, flags);

	/* Checked under the lock remove takes to set it: no HW access after. */
	if (chan->stopping) {
		spin_unlock_irqrestore(&chan->lock, flags);
		return -ESHUTDOWN;
	}

	/*
	 * The ring-full decision and the backlog insert are ONE critical
	 * section (#300). Older queued requests keep their place: only an
	 * empty backlog may submit directly.
	 */
	rc = crypto_queue_len(&chan->backlog) ? -ENOSPC :
	     al_ssm_hw_submit_locked(dev, rctx);
	if (rc == -ENOSPC) {
		rc = crypto_enqueue_request(&chan->backlog, &rctx->req->base);
		/* Older requests may be waiting while the ring has room again. */
		al_ssm_fill_locked(dev, &done);
		spin_unlock_irqrestore(&chan->lock, flags);
		al_ssm_run_done(&done);
		/* -EINPROGRESS / -EBUSY: queued, worker drains it. -ENOSPC: full. */
		if (rc != -ENOSPC)
			al_ssm_schedule_poll(dev, 0);
		return rc;
	}
	if (rc) {
		spin_unlock_irqrestore(&chan->lock, flags);
		return rc;
	}

	rctx->inline_wait = true;
	rctx->finished = false;
	spin_unlock_irqrestore(&chan->lock, flags);

	deadline = ktime_add_us(ktime_get(), AL_SSM_INLINE_POLL_US);
	for (;;) {
		spin_lock_irqsave(&chan->lock, flags);
		al_ssm_service_locked(dev, &done);
		if (rctx->finished || chan->stopping ||
		    !ktime_before(ktime_get(), deadline)) {
			/* Hand over: from here a reaper completes it by callback. */
			rctx->inline_wait = false;
			rc = rctx->finished ? rctx->status : -EINPROGRESS;
			spin_unlock_irqrestore(&chan->lock, flags);
			al_ssm_run_done(&done);
			return rc;
		}
		spin_unlock_irqrestore(&chan->lock, flags);
		al_ssm_run_done(&done);
		cpu_relax();
	}
}

/* ── Completion polling ───────────────────────────────────────────────── */

static void al_ssm_poll_completions(struct work_struct *work)
{
	struct delayed_work *dw = to_delayed_work(work);
	struct al_ssm_dev *dev = container_of(dw, struct al_ssm_dev, poll_work);
	struct al_ssm_chan *chan = &dev->channel;
	unsigned long flags;
	int completed;
	int pending, backlog;

	atomic64_inc(&dev->stat_poll_cycles);

	completed = al_ssm_service(dev);

	spin_lock_irqsave(&chan->lock, flags);
	pending = chan->pending_count;
	backlog = crypto_queue_len(&chan->backlog);
	spin_unlock_irqrestore(&chan->lock, flags);

	/*
	 * #182 signature: work outstanding (on the ring, or queued) with no
	 * completion since it became outstanding. Reported once per stall
	 * episode; cleared by the next real completion.
	 */
	if (pending + backlog > 0 && !dev->stall_reported &&
	    time_after(jiffies,
		       dev->stall_clock + msecs_to_jiffies(AL_SSM_STALL_MS))) {
		dev->stall_reported = true;
		dev_warn(dev->dev,
			 "engine stall: %d in flight, %d queued, no completion for %dms\n",
			 pending, backlog, AL_SSM_STALL_MS);
		al_ssm_fault(dev, AL_SSM_FAULT_STALL);
	}

	dev_dbg(dev->dev, "poll_work: completed=%d pending=%d backlog=%d\n",
		completed, pending, backlog);

	/* Idle: stop polling; the next submit re-arms it. */
	if (pending + backlog > 0)
		al_ssm_schedule_poll(dev, completed > 0 ? 0 :
				     usecs_to_jiffies(AL_SSM_POLL_INTERVAL_US));
}

/* dev->wq is destroyed by remove after it sets chan->stopping under
 * chan->lock; arming the work under that lock never touches a dead wq. */
static void al_ssm_schedule_poll(struct al_ssm_dev *dev, unsigned long delay)
{
	struct al_ssm_chan *chan = &dev->channel;
	unsigned long flags;

	spin_lock_irqsave(&chan->lock, flags);
	if (!chan->stopping)
		mod_delayed_work(dev->wq, &dev->poll_work, delay);
	spin_unlock_irqrestore(&chan->lock, flags);
}

/* ── Async crypto entry ───────────────────────────────────────────────── */

static struct al_ssm_reqctx *al_ssm_req_rctx(struct skcipher_request *req)
{
	return skcipher_request_ctx_dma(req);
}

/* Serve the request from the software cipher (sync; completes inline). */
static int al_ssm_do_fallback(struct al_ssm_ctx *ctx, struct skcipher_request *req,
			      enum al_crypto_dir dir)
{
	struct skcipher_request *subreq = &al_ssm_req_rctx(req)->fallback_req;

	atomic64_inc(&ctx->dev->stat_fallback);
	skcipher_request_set_tfm(subreq, ctx->fallback);
	skcipher_request_set_callback(subreq, req->base.flags,
				      req->base.complete, req->base.data);
	skcipher_request_set_crypt(subreq, req->src, req->dst, req->cryptlen, req->iv);
	return dir == AL_CRYPT_ENCRYPT ? crypto_skcipher_encrypt(subreq) :
					 crypto_skcipher_decrypt(subreq);
}

/*
 * Length rules per mode (#291). Returns 0 to use the engine, a negative errno
 * to fail the request, or 1 to use the software fallback.
 *  - CBC needs whole blocks: -EINVAL like qce (skcipher.c:232-234).
 *  - XTS needs one block at least: -EINVAL like crypto/xts.c:248.
 *  - XTS with a partial last block is ciphertext stealing; the engine's CTS
 *    is not proven, so software does it.
 *  - Above what the engine was sized for: software.
 */
static int al_ssm_check_len(struct al_ssm_ctx *ctx, unsigned int len)
{
	if (ctx->enc_type == AL_CRYPT_AES_CBC && !IS_ALIGNED(len, AES_BLOCK_SIZE))
		return -EINVAL;
	if (ctx->enc_type == AL_CRYPT_AES_XTS) {
		if (len < AES_BLOCK_SIZE)
			return -EINVAL;
		if (!IS_ALIGNED(len, AES_BLOCK_SIZE))
			return 1;
	}
	return len > AL_SSM_MAX_DATA_SIZE;
}

static int al_ssm_enqueue(struct al_ssm_ctx *ctx, struct skcipher_request *req,
			   enum al_crypto_dir dir)
{
	struct al_ssm_reqctx *rctx = al_ssm_req_rctx(req);
	int rc;

	if (!req->cryptlen)	/* nothing to do; a valid request */
		return 0;

	rc = al_ssm_check_len(ctx, req->cryptlen);
	if (rc < 0)
		return rc;
	if (rc)
		return al_ssm_do_fallback(ctx, req, dir);

	INIT_LIST_HEAD(&rctx->list);
	rctx->inline_wait = false;
	rctx->finished = false;
	rctx->notify_owed = false;

	/* Too many segments, or the mapping failed: software, not an error. */
	rc = al_ssm_prepare(rctx, ctx, req, dir);
	if (rc) {
		if (rc == -ENOMEM)
			al_ssm_fault(ctx->dev, AL_SSM_FAULT_DMA_ALLOC);
		return al_ssm_do_fallback(ctx, req, dir);
	}

	rc = al_ssm_submit_and_poll(rctx);

	if (rc == 0) {
		/* Completed synchronously — output IV published, return success */
		al_ssm_finish_request(rctx, 0);
		return 0;
	}

	if (rc == -EINPROGRESS || rc == -EBUSY)
		return rc;	/* the callback unmaps rctx */

	/* Not accepted, or failed synchronously; removed dev is -ENODEV */
	al_ssm_finish_request(rctx, rc);
	return rc == -ESHUTDOWN ? -ENODEV : rc;
}

/* ── Crypto algorithm callbacks ───────────────────────────────────────── */

static int al_ssm_init_tfm(struct crypto_skcipher *tfm)
{
	struct al_ssm_ctx *ctx = crypto_skcipher_ctx(tfm);
	const char *name = crypto_tfm_alg_name(crypto_skcipher_tfm(tfm));

	ctx->dev = al_ssm_dev_get();
	if (!ctx->dev)
		return -ENODEV;

	/* A synchronous, non-fallback implementation of the same name. */
	ctx->fallback = crypto_alloc_skcipher(name, 0,
					      CRYPTO_ALG_NEED_FALLBACK | CRYPTO_ALG_ASYNC);
	if (IS_ERR(ctx->fallback)) {
		int rc = PTR_ERR(ctx->fallback);

		dev_err(ctx->dev->dev, "no software fallback for %s: %d\n", name, rc);
		al_ssm_dev_put(ctx->dev);
		return rc;
	}

	atomic_inc(&g_ssm_tfms);
	crypto_skcipher_set_reqsize_dma(tfm, sizeof(struct al_ssm_reqctx) +
					crypto_skcipher_reqsize(ctx->fallback));
	return 0;
}

static void al_ssm_exit_tfm(struct crypto_skcipher *tfm)
{
	struct al_ssm_ctx *ctx = crypto_skcipher_ctx(tfm);

	memzero_explicit(ctx->key, sizeof(ctx->key));
	memzero_explicit(ctx->xts_key, sizeof(ctx->xts_key));
	crypto_free_skcipher(ctx->fallback);
	al_ssm_dev_put(ctx->dev);
	atomic_dec(&g_ssm_tfms);
}

static int al_ssm_setkey_fallback(struct crypto_skcipher *tfm, const u8 *key,
				  unsigned int keylen)
{
	struct al_ssm_ctx *ctx = crypto_skcipher_ctx(tfm);

	crypto_skcipher_clear_flags(ctx->fallback, CRYPTO_TFM_REQ_MASK);
	crypto_skcipher_set_flags(ctx->fallback,
				  crypto_skcipher_get_flags(tfm) & CRYPTO_TFM_REQ_MASK);
	return crypto_skcipher_setkey(ctx->fallback, key, keylen);
}

static int al_ssm_xts_setkey(struct crypto_skcipher *tfm, const u8 *key, unsigned int keylen)
{
	struct al_ssm_ctx *ctx = crypto_skcipher_ctx(tfm);
	unsigned int half = keylen / 2;
	int ret = xts_verify_key(tfm, key, keylen);
	if (ret)
		return ret;
	ctx->enc_type = AL_CRYPT_AES_XTS;
	switch (half) {
	case AES_KEYSIZE_128: ctx->aes_ksize = AL_CRYPT_AES_128; break;
	case AES_KEYSIZE_256: ctx->aes_ksize = AL_CRYPT_AES_256; break;
	default: return -EINVAL;
	}
	ret = al_ssm_setkey_fallback(tfm, key, keylen);
	if (ret)
		return ret;
	ctx->keylen = half;
	memcpy(ctx->key, key, half);
	memcpy(ctx->xts_key, key + half, half);
	return 0;
}

static int al_ssm_xts_encrypt(struct skcipher_request *req)
{
	return al_ssm_enqueue(crypto_skcipher_ctx(crypto_skcipher_reqtfm(req)), req, AL_CRYPT_ENCRYPT);
}

static int al_ssm_xts_decrypt(struct skcipher_request *req)
{
	return al_ssm_enqueue(crypto_skcipher_ctx(crypto_skcipher_reqtfm(req)), req, AL_CRYPT_DECRYPT);
}

static int al_ssm_cbc_setkey(struct crypto_skcipher *tfm, const u8 *key, unsigned int keylen)
{
	struct al_ssm_ctx *ctx = crypto_skcipher_ctx(tfm);
	int ret;

	ctx->enc_type = AL_CRYPT_AES_CBC;
	switch (keylen) {
	case AES_KEYSIZE_128: ctx->aes_ksize = AL_CRYPT_AES_128; break;
	case AES_KEYSIZE_192: ctx->aes_ksize = AL_CRYPT_AES_192; break;
	case AES_KEYSIZE_256: ctx->aes_ksize = AL_CRYPT_AES_256; break;
	default: return -EINVAL;
	}
	ret = al_ssm_setkey_fallback(tfm, key, keylen);
	if (ret)
		return ret;
	ctx->keylen = keylen;
	memcpy(ctx->key, key, keylen);
	return 0;
}

static int al_ssm_cbc_encrypt(struct skcipher_request *req)
{
	return al_ssm_enqueue(crypto_skcipher_ctx(crypto_skcipher_reqtfm(req)), req, AL_CRYPT_ENCRYPT);
}

static int al_ssm_cbc_decrypt(struct skcipher_request *req)
{
	return al_ssm_enqueue(crypto_skcipher_ctx(crypto_skcipher_reqtfm(req)), req, AL_CRYPT_DECRYPT);
}

/* ── Algorithm registration ───────────────────────────────────────────── */

static struct skcipher_alg al_ssm_algs[] = {
	{
		.base.cra_name		= "xts(aes)",
		.base.cra_driver_name	= "xts-aes-al-ssm",
		.base.cra_priority	= 400,
		.base.cra_flags		= CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY |
					  CRYPTO_ALG_NEED_FALLBACK,
		.base.cra_blocksize	= AES_BLOCK_SIZE,
		.base.cra_ctxsize	= sizeof(struct al_ssm_ctx),
		.base.cra_module	= THIS_MODULE,
		.min_keysize		= 2 * AES_MIN_KEY_SIZE,
		.max_keysize		= 2 * AES_MAX_KEY_SIZE,
		.ivsize			= AES_BLOCK_SIZE,
		.setkey			= al_ssm_xts_setkey,
		.encrypt		= al_ssm_xts_encrypt,
		.decrypt		= al_ssm_xts_decrypt,
		.init			= al_ssm_init_tfm,
		.exit			= al_ssm_exit_tfm,
	},
	{
		.base.cra_name		= "cbc(aes)",
		.base.cra_driver_name	= "cbc-aes-al-ssm",
		.base.cra_priority	= 400,
		.base.cra_flags		= CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY |
					  CRYPTO_ALG_NEED_FALLBACK,
		.base.cra_blocksize	= AES_BLOCK_SIZE,
		.base.cra_ctxsize	= sizeof(struct al_ssm_ctx),
		.base.cra_module	= THIS_MODULE,
		.min_keysize		= AES_MIN_KEY_SIZE,
		.max_keysize		= AES_MAX_KEY_SIZE,
		.ivsize			= AES_BLOCK_SIZE,
		.setkey			= al_ssm_cbc_setkey,
		.encrypt		= al_ssm_cbc_encrypt,
		.decrypt		= al_ssm_cbc_decrypt,
		.init			= al_ssm_init_tfm,
		.exit			= al_ssm_exit_tfm,
	},
};

/* ── Channel init with SEPARATE completion rings ──────────────────────── */

static int al_ssm_init_channel(struct al_ssm_dev *dev)
{
	struct al_ssm_chan *chan = &dev->channel;
	struct al_udma_q_params tx_params, rx_params;
	int rc;

	spin_lock_init(&chan->lock);
	INIT_LIST_HEAD(&chan->pending);
	chan->pending_count = 0;
	crypto_init_queue(&chan->backlog, AL_SSM_MAX_BACKLOG);

	chan->tx_ring = dma_alloc_coherent(dev->dev, RING_BYTES, &chan->tx_ring_dma, GFP_KERNEL);
	if (!chan->tx_ring)
		return -ENOMEM;
	chan->tx_cdesc = dma_alloc_coherent(dev->dev, RING_BYTES, &chan->tx_cdesc_dma, GFP_KERNEL);
	if (!chan->tx_cdesc)
		goto err_free_tx;
	chan->rx_ring = dma_alloc_coherent(dev->dev, RING_BYTES, &chan->rx_ring_dma, GFP_KERNEL);
	if (!chan->rx_ring)
		goto err_free_tx_cdesc;
	chan->rx_cdesc = dma_alloc_coherent(dev->dev, RING_BYTES, &chan->rx_cdesc_dma, GFP_KERNEL);
	if (!chan->rx_cdesc)
		goto err_free_rx;

	memset(&tx_params, 0, sizeof(tx_params));
	tx_params.size = AL_SSM_RING_SIZE;
	tx_params.desc_base = chan->tx_ring;
	tx_params.desc_phy_base = chan->tx_ring_dma;
	tx_params.cdesc_base = chan->tx_cdesc;
	tx_params.cdesc_phy_base = chan->tx_cdesc_dma;
	tx_params.cdesc_size = sizeof(union al_udma_desc);

	memset(&rx_params, 0, sizeof(rx_params));
	rx_params.size = AL_SSM_RING_SIZE;
	rx_params.desc_base = chan->rx_ring;
	rx_params.desc_phy_base = chan->rx_ring_dma;
	rx_params.cdesc_base = chan->rx_cdesc;
	rx_params.cdesc_phy_base = chan->rx_cdesc_dma;
	rx_params.cdesc_size = sizeof(union al_udma_desc);

	rc = al_ssm_dma_q_init(&dev->ssm_dma, 0, &tx_params, &rx_params, AL_CRYPT_AUTH_Q);
	if (rc) {
		dev_err(dev->dev, "failed to init queue 0: %d\n", rc);
		goto err_free_rx_cdesc;
	}
	return 0;

err_free_rx_cdesc:
	dma_free_coherent(dev->dev, RING_BYTES, chan->rx_cdesc, chan->rx_cdesc_dma);
err_free_rx:
	dma_free_coherent(dev->dev, RING_BYTES, chan->rx_ring, chan->rx_ring_dma);
err_free_tx_cdesc:
	dma_free_coherent(dev->dev, RING_BYTES, chan->tx_cdesc, chan->tx_cdesc_dma);
err_free_tx:
	dma_free_coherent(dev->dev, RING_BYTES, chan->tx_ring, chan->tx_ring_dma);
	return rc ? rc : -ENOMEM;
}

static void al_ssm_free_channel(struct al_ssm_dev *dev)
{
	struct al_ssm_chan *chan = &dev->channel;
	if (chan->rx_cdesc)
		dma_free_coherent(dev->dev, RING_BYTES, chan->rx_cdesc, chan->rx_cdesc_dma);
	if (chan->rx_ring)
		dma_free_coherent(dev->dev, RING_BYTES, chan->rx_ring, chan->rx_ring_dma);
	if (chan->tx_cdesc)
		dma_free_coherent(dev->dev, RING_BYTES, chan->tx_cdesc, chan->tx_cdesc_dma);
	if (chan->tx_ring)
		dma_free_coherent(dev->dev, RING_BYTES, chan->tx_ring, chan->tx_ring_dma);
}

/* ── PCI driver ───────────────────────────────────────────────────────── */

static const struct pci_device_id al_ssm_pci_tbl[] = {
	{ PCI_VDEVICE(AMAZON_ANNAPURNA_LABS, AL_SSM_DEVICE_ID),
	  .class = AL_SSM_CLASS_CRYPTO, .class_mask = 0xffffff },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, al_ssm_pci_tbl);

static int al_ssm_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct al_ssm_dev *dev;
	struct al_ssm_dma_params dma_params;
	int rc;
	u8 rev_id;

	pci_read_config_byte(pdev, PCI_REVISION_ID, &rev_id);
	dev_info(&pdev->dev, "Alpine V2 SSM crypto engine found (vendor=%04x dev=%04x class=%06x rev=%d)\n",
		 pdev->vendor, pdev->device, pdev->class, rev_id);

	/* Tfms from a previous bind still point at the static al_ssm_algs[]. */
	if (atomic_read(&g_ssm_tfms)) {
		dev_err(&pdev->dev, "%d tfm(s) from a previous bind still live\n",
			atomic_read(&g_ssm_tfms));
		return -EBUSY;
	}

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	kref_init(&dev->ref);
	dev->pdev = pci_dev_get(pdev);
	dev->dev = &pdev->dev;
	atomic_set(&dev->total_pending, 0);
	dev->last_completion = jiffies;
	dev->stall_clock = jiffies;

	rc = pci_enable_device(pdev);
	if (rc)
		goto err_free;
	rc = pci_request_regions(pdev, DRV_NAME);
	if (rc)
		goto err_disable;
	pci_set_master(pdev);

	rc = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(40));
	if (rc)
		rc = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (rc)
		goto err_regions;

	dev->bar0 = pci_iomap(pdev, 0, 0);
	if (!dev->bar0) { rc = -ENOMEM; goto err_regions; }
	dev->bar4 = pci_iomap(pdev, 4, 0);
	if (!dev->bar4) { rc = -ENOMEM; goto err_unmap_bar0; }

	dev_info(&pdev->dev, "BAR0=%pR mapped at %p, BAR4=%pR mapped at %p\n",
		 &pdev->resource[0], dev->bar0, &pdev->resource[4], dev->bar4);

	memset(dev->bars, 0, sizeof(dev->bars));
	dev->bars[0] = dev->bar0;
	dev->bars[4] = dev->bar4;

	al_ssm_unit_regs_info_get(dev->bars, AL_CRYPTO_ALPINE_V2_DEV_ID,
				  AL_SSM_REV_ID_REV2, &dev->unit_info);

	memset(&dma_params, 0, sizeof(dma_params));
	dma_params.rev_id = AL_SSM_REV_ID_REV2;
	dma_params.udma_regs_base = dev->bar0;
	dma_params.name = DRV_NAME;
	dma_params.num_of_queues = 1;

	rc = al_ssm_dma_init(&dev->ssm_dma, &dma_params);
	if (rc)
		goto err_unmap_bar4;
	rc = al_ssm_dma_state_set(&dev->ssm_dma, UDMA_NORMAL);
	if (rc)
		goto err_unmap_bar4;
	rc = al_ssm_init_channel(dev);
	if (rc)
		goto err_dma_disable;

	/*
	 * Configure the XTS tweak-calculation swap register. Passing NULL here
	 * falls through to the HAL's own internal default
	 * (cfg_default = { .swap_pre_inc_bytes = AL_TRUE }, al_hal_ssm_crypto.c)
	 * - that default fails self-test on vector 1 (#70). An explicit
	 * all-zero cfg (every swap disabled, tweak_calc_swap register left at
	 * 0) is the one config out of all 6 individual swap bits + the HAL
	 * default that passes cleanly - confirmed on hardware across all 7
	 * candidates, all-zero being the only pass.
	 */
	{
		static const struct al_crypto_xts_cfg xts_cfg_no_swap = { };
		al_crypto_xts_cfg_set(&dev->ssm_dma, dev->unit_info.crypto_regs_base, &xts_cfg_no_swap);
	}

	dev->wq = alloc_workqueue("al_ssm_wq", WQ_HIGHPRI | WQ_UNBOUND, 0);
	if (!dev->wq) { rc = -ENOMEM; goto err_free_channel; }
	INIT_DELAYED_WORK(&dev->poll_work, al_ssm_poll_completions);

	rc = al_ssm_devlink_init(dev);
	if (rc)
		goto err_destroy_wq;

	pci_set_drvdata(pdev, dev);
	spin_lock(&g_ssm_lock);
	g_ssm_dev = dev;
	spin_unlock(&g_ssm_lock);

	rc = crypto_register_skciphers(al_ssm_algs, ARRAY_SIZE(al_ssm_algs));
	if (rc)
		goto err_devlink;
	dev->crypto_registered = true;

	dev_info(&pdev->dev,
		 "Alpine V2 SSM crypto engine initialized: AES-XTS, AES-CBC "
		 "(async, ring %d, max %d in-flight, separate completion rings)\n",
		 AL_SSM_RING_SIZE, AL_SSM_MAX_IN_FLIGHT);
	return 0;

err_devlink:
	spin_lock(&g_ssm_lock);
	g_ssm_dev = NULL;
	spin_unlock(&g_ssm_lock);
	al_ssm_devlink_fini(dev);
err_destroy_wq:
	destroy_workqueue(dev->wq);
err_free_channel:
	al_ssm_free_channel(dev);
err_dma_disable:
	if (al_ssm_dma_state_set(&dev->ssm_dma, UDMA_DISABLE))
		dev_err(&pdev->dev, "failed to disable DMA during probe unwind\n");
err_unmap_bar4:
	pci_iounmap(pdev, dev->bar4);
err_unmap_bar0:
	pci_iounmap(pdev, dev->bar0);
err_regions:
	pci_release_regions(pdev);
err_disable:
	pci_disable_device(pdev);
err_free:
	pci_dev_put(pdev);
	kfree(dev);
	return rc;
}

/*
 * Stop the engine before anything it may still write to is reclaimed (#301).
 * q_pause stops fetch and waits for completions to catch up with prefetch
 * (al_udma_q_pause: dcp == crhp). Bus-mastering off afterwards is the hard
 * stop: it holds even when the pause timed out on a stuck engine.
 */
static void al_ssm_quiesce(struct al_ssm_dev *dev)
{
	struct al_udma_q *tx_q = al_ssm_dma_tx_queue_handle_get(&dev->ssm_dma, 0);
	struct al_udma_q *rx_q = al_ssm_dma_rx_queue_handle_get(&dev->ssm_dma, 0);

	if (tx_q && al_udma_q_pause(tx_q))
		dev_err(dev->dev, "tx queue did not pause on remove\n");
	if (rx_q && al_udma_q_pause(rx_q))
		dev_err(dev->dev, "rx queue did not pause on remove\n");
	pci_clear_master(dev->pdev);
}

/* Engine is stopped: what it finished is reaped normally, the rest and the
 * whole backlog fail with -ESHUTDOWN. Callbacks run with no lock held. */
static void al_ssm_fail_outstanding(struct al_ssm_dev *dev)
{
	struct al_ssm_chan *chan = &dev->channel;
	struct al_ssm_reqctx *rctx, *tmp;
	LIST_HEAD(done);
	unsigned long flags;

	spin_lock_irqsave(&chan->lock, flags);
	al_ssm_reap_locked(dev, &done);

	list_for_each_entry_safe(rctx, tmp, &chan->pending, list) {
		list_del(&rctx->list);
		al_ssm_finish_locked(rctx, -ESHUTDOWN, &done);
	}
	chan->pending_count = 0;
	atomic_set(&dev->total_pending, 0);

	while (crypto_queue_len(&chan->backlog)) {
		rctx = al_ssm_backlog_pop_locked(chan);
		al_ssm_finish_locked(rctx, -ESHUTDOWN, &done);
	}
	spin_unlock_irqrestore(&chan->lock, flags);

	al_ssm_run_done(&done);
}

static void al_ssm_pci_remove(struct pci_dev *pdev)
{
	struct al_ssm_dev *dev = pci_get_drvdata(pdev);
	struct al_ssm_chan *chan;
	struct al_udma_q *tx_q, *rx_q;
	unsigned long flags;

	if (!dev)
		return;
	chan = &dev->channel;

	/* No new tfm can find the device. Live tfms keep it alive by refcount. */
	spin_lock(&g_ssm_lock);
	g_ssm_dev = NULL;
	spin_unlock(&g_ssm_lock);

	if (dev->crypto_registered) {
		crypto_unregister_skciphers(al_ssm_algs, ARRAY_SIZE(al_ssm_algs));
		dev->crypto_registered = false;
	}

	/* From here every submit path returns -ESHUTDOWN/-ENODEV under chan->lock. */
	spin_lock_irqsave(&chan->lock, flags);
	chan->stopping = true;
	spin_unlock_irqrestore(&chan->lock, flags);

	/* disable, not cancel: the worker re-arms itself. */
	disable_delayed_work_sync(&dev->poll_work);
	destroy_workqueue(dev->wq);
	/* After this no fault can still be in flight to a live reporter. */
	al_ssm_devlink_fini(dev);

	al_ssm_quiesce(dev);
	al_ssm_fail_outstanding(dev);

	/* UDMA_DISABLE below only pauses new-descriptor fetch, it doesn't
	 * drain/reset the queue - without this a live reload hung forever on
	 * stale hardware state (#167). Matches vendor al_eth's own
	 * al_eth_hw_stop(). See docs/kernel-boot-fixes.md Bug 6. */
	tx_q = al_ssm_dma_tx_queue_handle_get(&dev->ssm_dma, 0);
	rx_q = al_ssm_dma_rx_queue_handle_get(&dev->ssm_dma, 0);
	if (tx_q && al_udma_q_reset(tx_q))
		dev_err(&pdev->dev, "failed to reset tx queue on remove\n");
	if (rx_q && al_udma_q_reset(rx_q))
		dev_err(&pdev->dev, "failed to reset rx queue on remove\n");

	al_ssm_free_channel(dev);
	if (al_ssm_dma_state_set(&dev->ssm_dma, UDMA_DISABLE))
		dev_err(&pdev->dev, "failed to disable DMA on remove\n");
	if (dev->bar4) pci_iounmap(pdev, dev->bar4);
	if (dev->bar0) pci_iounmap(pdev, dev->bar0);
	pci_release_regions(pdev);
	pci_disable_device(pdev);
	dev_info(&pdev->dev, "Alpine V2 SSM crypto engine removed\n");
	al_ssm_dev_put(dev);
}

static struct pci_driver al_ssm_pci_driver = {
	.name		= DRV_NAME,
	.id_table	= al_ssm_pci_tbl,
	.probe		= al_ssm_pci_probe,
	.remove		= al_ssm_pci_remove,
};

module_pci_driver(al_ssm_pci_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Annapurna Labs (HAL), SecFirstNAS contributors (driver)");
MODULE_DESCRIPTION("Annapurna Labs Alpine V2 SSM hardware crypto engine driver");
MODULE_VERSION(DRV_VERSION);
