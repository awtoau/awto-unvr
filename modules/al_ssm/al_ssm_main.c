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
 *             ->  this driver: enqueue, submit to HW, return -EINPROGRESS
 *             ->  completion workqueue polls UDMA, calls skcipher_request_complete()
 *             ->  AL HAL SSM crypto  ->  UDMA DMA  ->  HW crypto engine
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
	kfree(container_of(ref, struct al_ssm_dev, ref));
}

static void al_ssm_dev_put(struct al_ssm_dev *dev)
{
	kref_put(&dev->ref, al_ssm_dev_release);
}

static void al_ssm_poll_completions(struct work_struct *work);
static void al_ssm_complete_one(struct al_ssm_reqctx *rctx, int err);
static void al_ssm_schedule_poll(struct al_ssm_dev *dev, unsigned long delay);

/* ── DMA resource management ──────────────────────────────────────────── */

static int al_ssm_alloc_resources(struct al_ssm_reqctx *rctx,
				  struct al_ssm_ctx *ctx,
				  struct scatterlist *src,
				  unsigned int nbytes, u8 *iv,
				  enum al_crypto_dir dir)
{
	struct al_ssm_dev *dev = ctx->dev;
	struct al_crypto_sa sa;

	rctx->dev = dev;
	rctx->dir = dir;
	rctx->enc_type = ctx->enc_type;
	rctx->nbytes = nbytes;

	if (nbytes == 0 || nbytes > AL_SSM_MAX_DATA_SIZE)
		return -EINVAL;

	rctx->src_virt = dma_alloc_coherent(dev->dev, nbytes, &rctx->src_dma, GFP_KERNEL);
	if (!rctx->src_virt) {
		al_ssm_fault(dev, AL_SSM_FAULT_DMA_ALLOC);
		return -ENOMEM;
	}
	rctx->dst_virt = dma_alloc_coherent(dev->dev, nbytes, &rctx->dst_dma, GFP_KERNEL);
	if (!rctx->dst_virt)
		goto err_free_src;
	rctx->iv_buf = dma_alloc_coherent(dev->dev, AES_BLOCK_SIZE, &rctx->iv_dma, GFP_KERNEL);
	if (!rctx->iv_buf)
		goto err_free_dst;
	rctx->hw_sa = dma_alloc_coherent(dev->dev, sizeof(*rctx->hw_sa), &rctx->sa_dma, GFP_KERNEL);
	if (!rctx->hw_sa)
		goto err_free_iv;

	scatterwalk_map_and_copy(rctx->src_virt, src, 0, nbytes, 0);
	memcpy(rctx->iv_buf, iv, AES_BLOCK_SIZE);

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
	memcpy(sa.enc_iv, rctx->iv_buf, AES_BLOCK_SIZE);

	memset(rctx->hw_sa, 0, sizeof(*rctx->hw_sa));
	al_crypto_hw_sa_init(&sa, rctx->hw_sa);
	return 0;

err_free_iv:
	dma_free_coherent(dev->dev, AES_BLOCK_SIZE, rctx->iv_buf, rctx->iv_dma);
err_free_dst:
	dma_free_coherent(dev->dev, nbytes, rctx->dst_virt, rctx->dst_dma);
err_free_src:
	dma_free_coherent(dev->dev, nbytes, rctx->src_virt, rctx->src_dma);
	al_ssm_fault(dev, AL_SSM_FAULT_DMA_ALLOC);
	return -ENOMEM;
}

static void al_ssm_free_resources(struct al_ssm_reqctx *rctx)
{
	struct al_ssm_dev *dev = rctx->dev;

	if (rctx->hw_sa)
		dma_free_coherent(dev->dev, sizeof(*rctx->hw_sa), rctx->hw_sa, rctx->sa_dma);
	if (rctx->iv_buf)
		dma_free_coherent(dev->dev, AES_BLOCK_SIZE, rctx->iv_buf, rctx->iv_dma);
	if (rctx->dst_virt)
		dma_free_coherent(dev->dev, rctx->nbytes, rctx->dst_virt, rctx->dst_dma);
	if (rctx->src_virt)
		dma_free_coherent(dev->dev, rctx->nbytes, rctx->src_virt, rctx->src_dma);
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

/*
 * Split one DMA range into descriptor-sized buffers (#332). The HAL writes
 * every descriptor length as `len & 0xffff` and never checks it, so a larger
 * buffer is silently truncated. AL_SSM_MAX_DESC_LEN stays below 64 KiB whatever
 * the 64K-as-zero encoding does.
 */
static unsigned int al_ssm_split_buf(struct al_buf *bufs, dma_addr_t addr,
				     unsigned int len)
{
	unsigned int n = 0;

	while (len) {
		unsigned int chunk = min_t(unsigned int, len, AL_SSM_MAX_DESC_LEN);

		bufs[n].addr = addr;
		bufs[n].len = chunk;
		addr += chunk;
		len -= chunk;
		n++;
	}
	return n;
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
	struct al_buf src_bufs[AL_SSM_MAX_DESC_BUFS], dst_bufs[AL_SSM_MAX_DESC_BUFS];
	int rc;

	if (chan->pending_count >= AL_SSM_MAX_IN_FLIGHT)
		return -ENOSPC;

	memset(&xaction, 0, sizeof(xaction));
	xaction.dir = rctx->dir;
	xaction.flags = AL_SSM_INTERRUPT;
	xaction.src.bufs = src_bufs;
	xaction.src.num = al_ssm_split_buf(src_bufs, rctx->src_dma, rctx->nbytes);
	xaction.src_size = rctx->nbytes;
	xaction.dst.bufs = dst_bufs;
	xaction.dst.num = al_ssm_split_buf(dst_bufs, rctx->dst_dma, rctx->nbytes);
	xaction.sa_indx = 0;
	xaction.sa_in.addr = rctx->sa_dma;
	xaction.sa_in.len = sizeof(*rctx->hw_sa);
	xaction.enc_iv_in.addr = rctx->iv_dma;
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
	return skcipher_request_ctx(skcipher_request_cast(areq));
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
 * Copy engine output back to req->dst and, for CBC, update req->iv to the last
 * ciphertext block (7.1 skcipher IV-chaining contract). XTS/others: no update.
 * Reads the IV from the private bounce buffers (src_virt/dst_virt), so in-place
 * (req->src == req->dst) is correct even though req->src is overwritten here.
 */
static void al_ssm_copy_result(struct al_ssm_reqctx *rctx)
{
	struct skcipher_request *req = rctx->req;

	scatterwalk_map_and_copy(rctx->dst_virt, req->dst, 0, rctx->nbytes, 1);

	if (rctx->enc_type == AL_CRYPT_AES_CBC && rctx->nbytes >= AES_BLOCK_SIZE) {
		/* enc: last block of ciphertext (dst); dec: last block of input (src) */
		const u8 *last = (rctx->dir == AL_CRYPT_ENCRYPT ?
				  (const u8 *)rctx->dst_virt :
				  (const u8 *)rctx->src_virt) +
				 rctx->nbytes - AES_BLOCK_SIZE;
		memcpy(req->iv, last, AES_BLOCK_SIZE);
	}
}

static void al_ssm_complete_one(struct al_ssm_reqctx *rctx, int err)
{
	struct skcipher_request *req = rctx->req;

	if (!err)
		al_ssm_copy_result(rctx);
	al_ssm_free_resources(rctx);
	if (rctx->notify_owed)	/* promoted out of the backlog: say so first */
		crypto_request_complete(&req->base, -EINPROGRESS);
	skcipher_request_complete(req, err);
}

/* Callbacks for what a service pass collected. No lock held. */
static void al_ssm_run_done(struct list_head *done)
{
	struct al_ssm_reqctx *rctx, *tmp;

	list_for_each_entry_safe(rctx, tmp, done, list) {
		list_del(&rctx->list);
		al_ssm_complete_one(rctx, rctx->status);
	}
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
 *   0            - completed synchronously, result already in dst_virt
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

static int al_ssm_enqueue(struct al_ssm_ctx *ctx, struct skcipher_request *req,
			   enum al_crypto_dir dir)
{
	struct al_ssm_dev *dev = ctx->dev;
	struct al_ssm_reqctx *rctx = skcipher_request_ctx(req);
	u8 iv[AES_BLOCK_SIZE];
	int rc;

	if (!dev)
		return -ENODEV;

	memcpy(iv, req->iv, AES_BLOCK_SIZE);
	INIT_LIST_HEAD(&rctx->list);
	rctx->req = req;
	rctx->inline_wait = false;
	rctx->finished = false;
	rctx->notify_owed = false;

	rc = al_ssm_alloc_resources(rctx, ctx, req->src, req->cryptlen, iv, dir);
	if (rc)
		return rc;

	rc = al_ssm_submit_and_poll(rctx);

	if (rc == 0) {
		/* Completed synchronously — copy result + output IV, return success */
		al_ssm_copy_result(rctx);
		al_ssm_free_resources(rctx);
		return 0;
	}

	if (rc == -EINPROGRESS || rc == -EBUSY)
		return rc;	/* the callback frees rctx */

	/* Not accepted, or failed synchronously; removed dev is -ENODEV */
	al_ssm_free_resources(rctx);
	return rc == -ESHUTDOWN ? -ENODEV : rc;
}

/* ── Crypto algorithm callbacks ───────────────────────────────────────── */

static int al_ssm_init_tfm(struct crypto_skcipher *tfm)
{
	struct al_ssm_ctx *ctx = crypto_skcipher_ctx(tfm);

	ctx->dev = al_ssm_dev_get();
	if (!ctx->dev)
		return -ENODEV;
	atomic_inc(&g_ssm_tfms);
	crypto_skcipher_set_reqsize(tfm, sizeof(struct al_ssm_reqctx));
	return 0;
}

static void al_ssm_exit_tfm(struct crypto_skcipher *tfm)
{
	struct al_ssm_ctx *ctx = crypto_skcipher_ctx(tfm);
	memzero_explicit(ctx->key, sizeof(ctx->key));
	memzero_explicit(ctx->xts_key, sizeof(ctx->xts_key));
	al_ssm_dev_put(ctx->dev);
	atomic_dec(&g_ssm_tfms);
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
	ctx->enc_type = AL_CRYPT_AES_CBC;
	switch (keylen) {
	case AES_KEYSIZE_128: ctx->aes_ksize = AL_CRYPT_AES_128; break;
	case AES_KEYSIZE_192: ctx->aes_ksize = AL_CRYPT_AES_192; break;
	case AES_KEYSIZE_256: ctx->aes_ksize = AL_CRYPT_AES_256; break;
	default: return -EINVAL;
	}
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
		.base.cra_flags		= CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY,
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
		.base.cra_flags		= CRYPTO_ALG_ASYNC | CRYPTO_ALG_KERN_DRIVER_ONLY,
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
	dev->pdev = pdev;
	dev->dev = &pdev->dev;
	atomic_set(&dev->total_pending, 0);
	dev->last_completion = jiffies;

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
