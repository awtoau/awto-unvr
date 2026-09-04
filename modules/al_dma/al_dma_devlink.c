// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * al_dma_devlink.c - devlink instance + health reporter for al_dma
 *
 * Counter names match al_ssm's reporter so one userspace tool reads both
 * (#212). Stall signature to look for: in_flight stuck non-zero while
 * poll_cycles keeps climbing.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 */

#include <linux/bitops.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <net/devlink.h>

#include "al_dma_drv.h"
#include "al_hal_udma.h"
#include "al_hal_udma_iofic.h"
#include "al_hal_iofic.h"

static const char * const al_dma_fault_msg[AL_DMA_FAULT_COUNT] = {
	[AL_DMA_FAULT_SUBMIT]		= "submit failed: hardware refused a prepared transaction",
	[AL_DMA_FAULT_COMPLETION]	= "completion error: descriptor returned a non-zero status",
	[AL_DMA_FAULT_STALL]		= "engine stall: work in flight never completed",
	[AL_DMA_FAULT_DMA_ALLOC]	= "dma allocation failed: coherent buffer unavailable",
	[AL_DMA_FAULT_SPURIOUS]		= "spurious completion: no submitted descriptor for it",
	[AL_DMA_FAULT_QUEUE_INIT]	= "queue init failed: HAL refused the ring setup",
};

static const char *al_dma_udma_state_str(enum al_udma_state state)
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

/* Shadow indices for one queue. For RX also read the hardware completion
 * head register directly - a comp head ahead of next_cdesc_idx is hardware
 * having produced completions the driver never consumed. Read-only: the
 * driver's own shadow is deliberately not touched here. */
static void al_dma_fmsg_queue(struct devlink_fmsg *fmsg, const char *prefix,
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
}

/* UDMA engine counters - al_udma_stats_get() was an empty stub until #235. */
static void al_dma_fmsg_hw_stats(struct devlink_fmsg *fmsg, const char *prefix,
				 struct al_udma *udma, bool is_rx)
{
	struct al_udma_stats st;
	char name[48];

	if (al_udma_stats_get(udma, &st))
		return;

	snprintf(name, sizeof(name), "%s_hw_pkts", prefix);
	devlink_fmsg_u32_pair_put(fmsg, name, st.pkts);
	snprintf(name, sizeof(name), "%s_hw_bytes", prefix);
	devlink_fmsg_u64_pair_put(fmsg, name, st.bytes);
	snprintf(name, sizeof(name), "%s_hw_prefed_desc", prefix);
	devlink_fmsg_u32_pair_put(fmsg, name, st.prefed_desc);
	snprintf(name, sizeof(name), "%s_hw_comp_pkt", prefix);
	devlink_fmsg_u32_pair_put(fmsg, name, st.comp_pkt);
	snprintf(name, sizeof(name), "%s_hw_comp_desc", prefix);
	devlink_fmsg_u32_pair_put(fmsg, name, st.comp_desc);
	snprintf(name, sizeof(name), "%s_hw_ack_pkts", prefix);
	devlink_fmsg_u32_pair_put(fmsg, name, st.ack_pkts);
	if (is_rx) {
		snprintf(name, sizeof(name), "%s_hw_drop_pkt", prefix);
		devlink_fmsg_u32_pair_put(fmsg, name, st.drop_pkt);
	}
}

static void al_dma_fmsg_state(struct al_dma_device *dev, struct devlink_fmsg *fmsg)
{
	struct al_udma *udma;
	u64 worst_idle_ms = 0;
	u32 in_flight = 0;
	int i;

	for (i = 0; i < dev->num_channels; i++) {
		struct al_dma_chan *ch = &dev->channels[i];
		unsigned long flags;
		int outstanding;

		spin_lock_irqsave(&ch->lock, flags);
		outstanding = (ch->tail - ch->completed + ch->sw_ring_count) %
			      (ch->sw_ring_count ? ch->sw_ring_count : 1);
		spin_unlock_irqrestore(&ch->lock, flags);

		in_flight += outstanding;
		if (outstanding && ch->last_completion) {
			u64 idle = jiffies_to_msecs(jiffies - ch->last_completion);

			worst_idle_ms = max(worst_idle_ms, idle);
		}
	}

	devlink_fmsg_string_pair_put(fmsg, "driver", DRV_NAME);
	devlink_fmsg_string_pair_put(fmsg, "version", DRV_VERSION);

	/* Counter names below are kept identical in al_ssm. */
	devlink_fmsg_u32_pair_put(fmsg, "in_flight", in_flight);
	devlink_fmsg_u64_pair_put(fmsg, "submitted", atomic64_read(&dev->stat_submitted));
	devlink_fmsg_u64_pair_put(fmsg, "completed", atomic64_read(&dev->stat_completed));
	devlink_fmsg_u64_pair_put(fmsg, "errors", atomic64_read(&dev->stat_errors));
	devlink_fmsg_u64_pair_put(fmsg, "poll_cycles", atomic64_read(&dev->stat_poll_cycles));
	devlink_fmsg_u64_pair_put(fmsg, "since_last_completion_ms", worst_idle_ms);

	devlink_fmsg_u32_pair_put(fmsg, "channels", dev->num_channels);
	devlink_fmsg_u32_pair_put(fmsg, "pci_revision", dev->rev_id);
	devlink_fmsg_u32_pair_put(fmsg, "stall_threshold_ms", AL_DMA_STALL_MS);

	devlink_fmsg_arr_pair_nest_start(fmsg, "channel");
	for (i = 0; i < dev->num_channels; i++) {
		struct al_dma_chan *ch = &dev->channels[i];
		unsigned long flags;
		int head, tail, completed;
		bool stalled;

		spin_lock_irqsave(&ch->lock, flags);
		head = ch->head;
		tail = ch->tail;
		completed = ch->completed;
		stalled = ch->stall_reported;
		spin_unlock_irqrestore(&ch->lock, flags);

		devlink_fmsg_obj_nest_start(fmsg);
		devlink_fmsg_u32_pair_put(fmsg, "idx", ch->idx);
		devlink_fmsg_u32_pair_put(fmsg, "ring_size", ch->sw_ring_count);
		devlink_fmsg_u32_pair_put(fmsg, "head", head);
		devlink_fmsg_u32_pair_put(fmsg, "tail", tail);
		devlink_fmsg_u32_pair_put(fmsg, "completed", completed);
		devlink_fmsg_bool_pair_put(fmsg, "stalled", stalled);
		al_dma_fmsg_queue(fmsg, "tx",
				  al_ssm_dma_tx_queue_handle_get(&dev->hal_dma, i), false);
		al_dma_fmsg_queue(fmsg, "rx",
				  al_ssm_dma_rx_queue_handle_get(&dev->hal_dma, i), true);
		devlink_fmsg_obj_nest_end(fmsg);
	}
	devlink_fmsg_arr_pair_nest_end(fmsg);

	if (al_m2m_udma_handle_get(&dev->hal_dma.m2m_udma, UDMA_TX, &udma) == 0) {
		devlink_fmsg_string_pair_put(fmsg, "tx_udma_state",
			al_dma_udma_state_str(al_udma_state_get(udma)));
		al_dma_fmsg_hw_stats(fmsg, "tx", udma, false);
	}
	if (al_m2m_udma_handle_get(&dev->hal_dma.m2m_udma, UDMA_RX, &udma) == 0) {
		devlink_fmsg_string_pair_put(fmsg, "rx_udma_state",
			al_dma_udma_state_str(al_udma_state_get(udma)));
		al_dma_fmsg_hw_stats(fmsg, "rx", udma, true);
	}

	/* Secondary-level IOFIC is where UDMA error causes land. This driver
	 * services no interrupt, so nothing else is reading (and potentially
	 * clearing) these. */
	if (dev->udma_regs) {
		static const char * const grp[] = {
			"iofic_sec_cause_a", "iofic_sec_cause_b",
			"iofic_sec_cause_c", "iofic_sec_cause_d",
		};

		for (i = 0; i < AL_IOFIC_MAX_GROUPS; i++)
			devlink_fmsg_u32_pair_put(fmsg, grp[i],
				al_udma_iofic_read_cause(
					(struct unit_regs __iomem *)dev->udma_regs,
					AL_UDMA_IOFIC_LEVEL_SECONDARY, i));
	}
}

static int al_dma_devlink_diagnose(struct devlink_health_reporter *reporter,
				   struct devlink_fmsg *fmsg,
				   struct netlink_ext_ack *extack)
{
	al_dma_fmsg_state(devlink_health_reporter_priv(reporter), fmsg);
	return 0;
}

static int al_dma_devlink_dump(struct devlink_health_reporter *reporter,
			       struct devlink_fmsg *fmsg, void *priv_ctx,
			       struct netlink_ext_ack *extack)
{
	al_dma_fmsg_state(devlink_health_reporter_priv(reporter), fmsg);
	return 0;
}

static const struct devlink_health_reporter_ops al_dma_reporter_ops = {
	.name		= "dma",
	.diagnose	= al_dma_devlink_diagnose,
	.dump		= al_dma_devlink_dump,
};

static int al_dma_devlink_info_get(struct devlink *devlink,
				   struct devlink_info_req *req,
				   struct netlink_ext_ack *extack)
{
	return devlink_info_version_running_put(req, "driver", DRV_VERSION);
}

static const struct devlink_ops al_dma_devlink_ops = {
	.info_get = al_dma_devlink_info_get,
};

/* devlink_health_report() sleeps (devl_lock, GFP_KERNEL netlink alloc) and
 * WARNs on an unregistered instance. Fault sites are spinlock/tasklet
 * context, so they only set a bit here and let this work item report. */
static void al_dma_fault_work_fn(struct work_struct *work)
{
	struct al_dma_device *dev = container_of(work, struct al_dma_device, fault_work);
	unsigned long pending;
	int bit;

	if (!READ_ONCE(dev->devlink_live))
		return;

	pending = xchg(&dev->fault_pending, 0);
	for_each_set_bit(bit, &pending, AL_DMA_FAULT_COUNT)
		devlink_health_report(dev->hr, al_dma_fault_msg[bit], NULL);
}

void al_dma_fault(struct al_dma_device *dev, enum al_dma_fault fault)
{
	if (WARN_ON_ONCE(fault >= AL_DMA_FAULT_COUNT))
		return;
	set_bit(fault, &dev->fault_pending);
	if (READ_ONCE(dev->devlink_live))
		schedule_work(&dev->fault_work);
}

int al_dma_devlink_init(struct al_dma_device *dev)
{
	struct devlink *dl;
	int rc;

	INIT_WORK(&dev->fault_work, al_dma_fault_work_fn);

	dl = devlink_alloc(&al_dma_devlink_ops, 0, &dev->pdev->dev);
	if (!dl)
		return -ENOMEM;

	dev->hr = devlink_health_reporter_create(dl, &al_dma_reporter_ops, dev);
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

void al_dma_devlink_fini(struct al_dma_device *dev)
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
