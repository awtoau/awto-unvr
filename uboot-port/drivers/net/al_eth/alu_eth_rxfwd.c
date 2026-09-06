// SPDX-License-Identifier: GPL-2.0-or-later
/* alu_eth - EC RX forwarding, shared by the 1G and 10G ports.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * Without this the EC drops every received frame before it reaches the S2M
 * ring: the RFW control table's reset default routes nothing to a UDMA queue
 * and the MAC table has no matching entry. TX is unaffected, so the symptom is
 * "packets go out, replies never arrive" (#234).
 *
 * Linux's al_eth_config_rx_fwd() adds pbits/priority/hash tables for its
 * multi-queue RSS. U-Boot has ONE queue, so the ctrl-table default takes a
 * constant queue instead of a hash lookup, and one promiscuous MAC entry
 * replaces the unicast/broadcast/hash set.
 */

#include <errno.h>
#include <linux/string.h>

#include <al_hal_eth.h>

#include "alu_eth_core.h"

/* Entry 0 of the MAC table, the same region Linux fills from
 * AL_ETH_MAC_TABLE_UNICAST_IDX_BASE. Zero mask matches every DA, so one entry
 * covers unicast, broadcast and multicast - which is what a bootloader needs
 * (ARP replies unicast, ARP requests and DHCP broadcast). */
#define ALU_ETH_RXFWD_MAC_IDX	0
#define ALU_ETH_RXFWD_UDMA	0
#define ALU_ETH_RXFWD_QID	0

int alu_eth_rxfwd_config(struct al_hal_eth_adapter *adapter)
{
	struct al_eth_fwd_ctrl_table_entry ctrl;
	struct al_eth_fwd_mac_table_entry mac;
	int rc;

	/* Fixed UDMA/queue, no priority or hash lookup, filtering off. VAL_0 /
	 * NO_PRIO make the selectors constants rather than table reads we would
	 * otherwise have to populate. */
	memset(&ctrl, 0, sizeof(ctrl));
	ctrl.prio_sel = AL_ETH_CTRL_TABLE_PRIO_SEL_VAL_0;
	ctrl.queue_sel_1 = AL_ETH_CTRL_TABLE_QUEUE_SEL_1_VAL_0;
	ctrl.queue_sel_2 = AL_ETH_CTRL_TABLE_QUEUE_SEL_2_NO_PRIO;
	ctrl.udma_sel = AL_ETH_CTRL_TABLE_UDMA_SEL_MAC_TABLE;
	ctrl.hdr_split_len_sel = AL_ETH_CTRL_TABLE_HDR_SPLIT_LEN_SEL_0;
	ctrl.filter = AL_FALSE;

	rc = al_eth_ctrl_table_def_set(adapter, AL_FALSE, &ctrl);
	if (rc)
		return rc;

	/* Promiscuous catch-all: addr 0 / mask 0 matches every DA. */
	memset(&mac, 0, sizeof(mac));
	mac.rx_valid = AL_TRUE;
	mac.tx_valid = AL_FALSE;
	mac.udma_mask = 1 << ALU_ETH_RXFWD_UDMA;
	mac.qid = ALU_ETH_RXFWD_QID;
	mac.filter = AL_FALSE;

	return al_eth_fwd_mac_table_set(adapter, ALU_ETH_RXFWD_MAC_IDX, &mac);
}
