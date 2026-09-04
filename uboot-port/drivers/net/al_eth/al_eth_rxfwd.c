// SPDX-License-Identifier: GPL-2.0-or-later
/* Annapurna Labs al_eth - EC RX-forwarding config shared by eth1 and eth2.
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * Without this the EC drops every received frame before it reaches the S2M
 * ring: the RFW control table's reset default routes nothing to a UDMA queue
 * and the MAC table has no matching entry. TX is unaffected, so the symptom is
 * "packets go out, replies never arrive" (#234).
 *
 * Linux does the equivalent in al_eth_config_rx_fwd() with a full RSS/hash
 * setup. U-Boot has ONE queue, so this steers everything to UDMA 0 / queue 0
 * via the ctrl-table default and takes promiscuous (mask 0) rather than
 * replicating the unicast/broadcast/hash tables.
 */

#include <linux/string.h>

#include <al_hal_eth.h>

#include "al_eth_rxfwd.h"

/* Entry 0 of the MAC table, matching Linux's AL_ETH_MAC_TABLE_UNICAST_IDX_BASE
 * region. A zero mask matches every DA, so one entry covers unicast, broadcast
 * and multicast - which is what a bootloader needs (ARP replies are unicast,
 * ARP requests and DHCP are broadcast). */
#define AL_ETH_RXFWD_MAC_IDX	0
#define AL_ETH_RXFWD_UDMA	0
#define AL_ETH_RXFWD_QID	0

void al_eth_rxfwd_config(struct al_hal_eth_adapter *adapter)
{
	struct al_eth_fwd_ctrl_table_entry ctrl;
	struct al_eth_fwd_mac_table_entry mac;

	/* Default control-table entry: fixed UDMA/queue, no priority or hash
	 * lookup, filtering off. VAL_0/NO_PRIO make the selectors constants
	 * instead of table reads we would otherwise have to populate. */
	memset(&ctrl, 0, sizeof(ctrl));
	ctrl.prio_sel = AL_ETH_CTRL_TABLE_PRIO_SEL_VAL_0;
	ctrl.queue_sel_1 = AL_ETH_CTRL_TABLE_QUEUE_SEL_1_VAL_0;
	ctrl.queue_sel_2 = AL_ETH_CTRL_TABLE_QUEUE_SEL_2_NO_PRIO;
	ctrl.udma_sel = AL_ETH_CTRL_TABLE_UDMA_SEL_MAC_TABLE;
	ctrl.filter = AL_FALSE;
	al_eth_ctrl_table_def_set(adapter, AL_FALSE, &ctrl);

	/* Promiscuous catch-all: addr 0 / mask 0 matches every DA. */
	memset(&mac, 0, sizeof(mac));
	mac.rx_valid = AL_TRUE;
	mac.tx_valid = AL_FALSE;
	mac.udma_mask = 1 << AL_ETH_RXFWD_UDMA;
	mac.qid = AL_ETH_RXFWD_QID;
	mac.filter = AL_FALSE;
	al_eth_fwd_mac_table_set(adapter, AL_ETH_RXFWD_MAC_IDX, &mac);
}
