// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * al_ssm_devlink.c - devlink instance + health reporter for al_ssm
 *
 * al_ssm had no userspace surface at all (#212), which is why #182 (an
 * AF_ALG op wedging a process in D-state) was undiagnosable. The #182
 * signature to look for in `devlink health diagnose`: in_flight stuck
 * non-zero while poll_cycles keeps climbing.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 */

#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <net/devlink.h>

#include "al_ssm_drv.h"
#include "al_hal_udma.h"
#include "al_hal_udma_iofic.h"
#include "al_hal_iofic.h"

static const char * const al_ssm_fault_msg[AL_SSM_FAULT_COUNT] = {
	[AL_SSM_FAULT_SUBMIT]		= "submit failed: hardware refused a prepared transaction",
	[AL_SSM_FAULT_COMPLETION]	= "completion error: descriptor returned a non-zero status",
	[AL_SSM_FAULT_STALL]		= "engine stall: work in flight never completed",
	[AL_SSM_FAULT_DMA_ALLOC]	= "dma allocation failed: coherent buffer unavailable",
};

static const char *al_ssm_udma_state_str(enum al_udma_state state)
{
	switch (state) {
	case UDMA_DISABLE:	return "disable";
	case UDMA_IDLE:		return "idle";
	case UDMA_NORMAL:	return "normal";
	case UDMA_ABORT:	return "abort";
	case UDMA_RESET:	return "reset";
	default:		return "unknown";
	}
}

/* Emit one queue's shadow indices. For RX also read the hardware completion
 * head register directly - a comp_head ahead of next_cdesc_idx is hardware
 * having produced completions the driver never consumed (#182). Read-only:
 * the driver's own shadow is deliberately not touched here. */
static void al_ssm_fmsg_queue(struct devlink_fmsg *fmsg, const char *prefix,
			      struct al_udma_q *q, bool is_rx)
{
	char name[48];

	if (!q)
		return;

	snprintf(name, sizeof(name), "%s_next_desc_idx", prefix);
	devlink_fmsg_u32_pair_put(fmsg, name, q->next_desc_idx);
	snprintf(name, sizeof(name), "%s_next_cdesc_idx", prefix);
	devlink_fmsg_u32_pair_put(fmsg, name, q->next_cdesc_idx);
	snprintf(name, sizeof(name), "%s_desc_ring_id", prefix);
	devlink_fmsg_u32_pair_put(fmsg, name, q->desc_ring_id);
	snprintf(name, sizeof(name), "%s_comp_ring_id", prefix);
	devlink_fmsg_u32_pair_put(fmsg, name, q->comp_ring_id);

	if (is_rx && q->q_regs) {
		snprintf(name, sizeof(name), "%s_hw_comp_head", prefix);
		devlink_fmsg_u32_pair_put(fmsg, name,
			al_reg_read32(&q->q_regs->rings.crhp) & 0xFFFF);
	}

	if (q->udma) {
		snprintf(name, sizeof(name), "%s_udma_state", prefix);
		devlink_fmsg_string_pair_put(fmsg, name,
			al_ssm_udma_state_str(al_udma_state_get(q->udma)));
	}
}

static void al_ssm_fmsg_state(struct al_ssm_dev *dev, struct devlink_fmsg *fmsg)
{
	struct al_udma_q *tx_q, *rx_q;
	unsigned long flags, last;
	int hw_pending;
	u64 idle_ms;

	spin_lock_irqsave(&dev->channel.lock, flags);
	hw_pending = dev->channel.pending_count;
	last = dev->last_completion;
	spin_unlock_irqrestore(&dev->channel.lock, flags);

	idle_ms = last ? jiffies_to_msecs(jiffies - last) : 0;

	devlink_fmsg_string_pair_put(fmsg, "driver", DRV_NAME);
	devlink_fmsg_string_pair_put(fmsg, "version", DRV_VERSION);
	devlink_fmsg_bool_pair_put(fmsg, "crypto_registered", dev->crypto_registered);

	/* Counter names below are kept identical in al_dma. */
	devlink_fmsg_u32_pair_put(fmsg, "in_flight", atomic_read(&dev->total_pending));
	devlink_fmsg_u64_pair_put(fmsg, "submitted", atomic64_read(&dev->stat_submitted));
	devlink_fmsg_u64_pair_put(fmsg, "completed", atomic64_read(&dev->stat_completed));
	devlink_fmsg_u64_pair_put(fmsg, "errors", atomic64_read(&dev->stat_errors));
	devlink_fmsg_u64_pair_put(fmsg, "poll_cycles", atomic64_read(&dev->stat_poll_cycles));
	devlink_fmsg_u64_pair_put(fmsg, "since_last_completion_ms", idle_ms);

	devlink_fmsg_u32_pair_put(fmsg, "hw_pending", hw_pending);
	devlink_fmsg_u32_pair_put(fmsg, "backlog", atomic_read(&dev->backlog_count));
	devlink_fmsg_u32_pair_put(fmsg, "max_in_flight", AL_SSM_MAX_IN_FLIGHT);
	devlink_fmsg_u32_pair_put(fmsg, "ring_size", AL_SSM_RING_SIZE);
	devlink_fmsg_u32_pair_put(fmsg, "stall_threshold_ms", AL_SSM_STALL_MS);

	tx_q = al_ssm_dma_tx_queue_handle_get(&dev->ssm_dma, 0);
	rx_q = al_ssm_dma_rx_queue_handle_get(&dev->ssm_dma, 0);
	al_ssm_fmsg_queue(fmsg, "tx", tx_q, false);
	al_ssm_fmsg_queue(fmsg, "rx", rx_q, true);

	/* Secondary-level IOFIC is where UDMA error causes land. Neither this
	 * driver nor al_dma services an interrupt, so nothing else is reading
	 * (and potentially clearing) these. */
	if (rx_q && rx_q->udma) {
		void __iomem *sec = al_udma_iofic_reg_base_get_adv(rx_q->udma,
					AL_UDMA_IOFIC_LEVEL_SECONDARY);
		static const char * const grp[] = {
			"iofic_sec_cause_a", "iofic_sec_cause_b",
			"iofic_sec_cause_c", "iofic_sec_cause_d",
		};
		int i;

		for (i = 0; i < AL_IOFIC_MAX_GROUPS; i++)
			devlink_fmsg_u32_pair_put(fmsg, grp[i],
						  al_iofic_read_cause(sec, i));
	}
}

static int al_ssm_devlink_diagnose(struct devlink_health_reporter *reporter,
				   struct devlink_fmsg *fmsg,
				   struct netlink_ext_ack *extack)
{
	al_ssm_fmsg_state(devlink_health_reporter_priv(reporter), fmsg);
	return 0;
}

static int al_ssm_devlink_dump(struct devlink_health_reporter *reporter,
			       struct devlink_fmsg *fmsg, void *priv_ctx,
			       struct netlink_ext_ack *extack)
{
	al_ssm_fmsg_state(devlink_health_reporter_priv(reporter), fmsg);
	return 0;
}

static const struct devlink_health_reporter_ops al_ssm_reporter_ops = {
	.name		= "ssm",
	.diagnose	= al_ssm_devlink_diagnose,
	.dump		= al_ssm_devlink_dump,
};

static int al_ssm_devlink_info_get(struct devlink *devlink,
				   struct devlink_info_req *req,
				   struct netlink_ext_ack *extack)
{
	return devlink_info_version_running_put(req, "driver", DRV_VERSION);
}

static const struct devlink_ops al_ssm_devlink_ops = {
	.info_get = al_ssm_devlink_info_get,
};

/* devlink_health_report() sleeps (devl_lock, GFP_KERNEL netlink alloc) and
 * WARNs on an unregistered instance. Fault sites are spinlock/tasklet
 * context, so they only set a bit here and let this work item report. */
static void al_ssm_fault_work_fn(struct work_struct *work)
{
	struct al_ssm_dev *dev = container_of(work, struct al_ssm_dev, fault_work);
	unsigned long pending;
	int bit;

	if (!READ_ONCE(dev->devlink_live))
		return;

	pending = xchg(&dev->fault_pending, 0);
	for_each_set_bit(bit, &pending, AL_SSM_FAULT_COUNT)
		devlink_health_report(dev->hr, al_ssm_fault_msg[bit], NULL);
}

void al_ssm_fault(struct al_ssm_dev *dev, enum al_ssm_fault fault)
{
	if (WARN_ON_ONCE(fault >= AL_SSM_FAULT_COUNT))
		return;
	set_bit(fault, &dev->fault_pending);
	if (READ_ONCE(dev->devlink_live))
		schedule_work(&dev->fault_work);
}

int al_ssm_devlink_init(struct al_ssm_dev *dev)
{
	struct devlink *dl;
	int rc;

	INIT_WORK(&dev->fault_work, al_ssm_fault_work_fn);

	dl = devlink_alloc(&al_ssm_devlink_ops, 0, dev->dev);
	if (!dl)
		return -ENOMEM;

	dev->hr = devlink_health_reporter_create(dl, &al_ssm_reporter_ops, dev);
	if (IS_ERR(dev->hr)) {
		rc = PTR_ERR(dev->hr);
		dev->hr = NULL;
		devlink_free(dl);
		return rc;
	}

	dev->devlink = dl;
	devlink_register(dl);
	WRITE_ONCE(dev->devlink_live, true);
	return 0;
}

void al_ssm_devlink_fini(struct al_ssm_dev *dev)
{
	if (!dev->devlink)
		return;

	WRITE_ONCE(dev->devlink_live, false);
	cancel_work_sync(&dev->fault_work);
	devlink_unregister(dev->devlink);
	devlink_health_reporter_destroy(dev->hr);
	devlink_free(dev->devlink);
	dev->hr = NULL;
	dev->devlink = NULL;
}
