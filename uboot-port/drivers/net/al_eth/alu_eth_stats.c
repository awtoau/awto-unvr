// SPDX-License-Identifier: GPL-2.0-or-later
/* `eth stats` - every alu_eth hardware counter as plain text.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * U-Boot's answer to Linux's `ethtool -S`: MAC + EC + per-UDMA counters, plus
 * the Clause-49 PCS errored-block/BER counters on a 10G port.
 *
 * The EC counters are CLEAR-ON-READ (5be0d9b), so each read is a delta since
 * the last. They are accumulated per port here, exactly as the Linux driver
 * does - without it a second `eth stats` reports near-zero and the numbers
 * appear to go backwards.
 *
 * Read-only otherwise: the adapter handle is filled in here rather than by
 * al_eth_adapter_init(), so the command never disturbs a port already up.
 */

#include <command.h>
#include <errno.h>
#include <stdio.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <vsprintf.h>
#include <asm/io.h>

#include <al_hal_eth.h>
#include <al_hal_eth_mac_regs.h>

#include "alu_eth.h"

#define ALU_ETH_STATS_DEFAULT_PORT	ALU_ETH_PORT_10G

/* eth2 is the "advanced" function with 4 UDMAs; eth1 has 1. */
#define ALU_ETH_UDMAS_1G		1
#define ALU_ETH_UDMAS_10G		4

struct alu_ctr {
	const char	*name;
	u16		off;
	u8		width;		/* 4 or 8 bytes */
};

#define C32(s, f)	{ #f, offsetof(struct s, f), 4 }
#define C64(s, f)	{ #f, offsetof(struct s, f), 8 }

/* IEEE 802.3 names, as the shared HAL's struct al_eth_mac_stats spells them.
 * The names ARE the ethtool -S names Linux reports, so a counter can be
 * compared between the two stages without a translation table. */
static const struct alu_ctr mac_ctrs[] = {
	C64(al_eth_mac_stats, aOctetsReceivedOK),
	C64(al_eth_mac_stats, aOctetsTransmittedOK),
	C32(al_eth_mac_stats, etherStatsPkts),
	C32(al_eth_mac_stats, etherStatsOctets),
	C32(al_eth_mac_stats, ifInUcastPkts),
	C32(al_eth_mac_stats, ifInMulticastPkts),
	C32(al_eth_mac_stats, ifInBroadcastPkts),
	C32(al_eth_mac_stats, ifInErrors),
	C32(al_eth_mac_stats, ifOutUcastPkts),
	C32(al_eth_mac_stats, ifOutMulticastPkts),
	C32(al_eth_mac_stats, ifOutBroadcastPkts),
	C32(al_eth_mac_stats, ifOutErrors),
	C32(al_eth_mac_stats, aFramesReceivedOK),
	C32(al_eth_mac_stats, aFramesTransmittedOK),
	C32(al_eth_mac_stats, etherStatsUndersizePkts),
	C32(al_eth_mac_stats, etherStatsFragments),
	C32(al_eth_mac_stats, etherStatsJabbers),
	C32(al_eth_mac_stats, etherStatsOversizePkts),
	C32(al_eth_mac_stats, aFrameCheckSequenceErrors),
	C32(al_eth_mac_stats, aAlignmentErrors),
	C32(al_eth_mac_stats, etherStatsDropEvents),
	C32(al_eth_mac_stats, aFrameTooLongErrors),
	C32(al_eth_mac_stats, aInRangeLengthErrors),
	C32(al_eth_mac_stats, aPAUSEMACCtrlFramesReceived),
	C32(al_eth_mac_stats, aPAUSEMACCtrlFramesTransmitted),
	C32(al_eth_mac_stats, VLANReceivedOK),
	C32(al_eth_mac_stats, VLANTransmittedOK),
	C32(al_eth_mac_stats, etherStatsPkts64Octets),
	C32(al_eth_mac_stats, etherStatsPkts65to127Octets),
	C32(al_eth_mac_stats, etherStatsPkts128to255Octets),
	C32(al_eth_mac_stats, etherStatsPkts256to511Octets),
	C32(al_eth_mac_stats, etherStatsPkts512to1023Octets),
	C32(al_eth_mac_stats, etherStatsPkts1024to1518Octets),
	C32(al_eth_mac_stats, etherStatsPkts1519toX),
	C32(al_eth_mac_stats, eee_in),
	C32(al_eth_mac_stats, eee_out),
};

/* The "where did the packet go" set - every EC counter meaning a drop or an
 * error. Printed first because it is the reason to run this command. */
static const struct alu_ctr ec_drop_ctrs[] = {
	C32(al_eth_ec_stats, faf_in_rx_short),
	C32(al_eth_ec_stats, faf_in_rx_long),
	C32(al_eth_ec_stats, faf_out_rx_short),
	C32(al_eth_ec_stats, faf_out_rx_long),
	C32(al_eth_ec_stats, faf_out_drop),
	C32(al_eth_ec_stats, rxf_in_fifo_err),
	C32(al_eth_ec_stats, lbf_in_fifo_err),
	C32(al_eth_ec_stats, rxf_out_drop_1_pkt),
	C32(al_eth_ec_stats, rxf_out_drop_2_pkt),
	C32(al_eth_ec_stats, rfw_in_vlan_drop),
	C32(al_eth_ec_stats, rfw_in_parse_drop),
	C32(al_eth_ec_stats, rfw_in_mac_drop),
	C32(al_eth_ec_stats, rfw_in_mac_ndet_drop),
	C32(al_eth_ec_stats, rfw_in_ctrl_drop),
	C32(al_eth_ec_stats, rfw_in_prot_i_drop),
};

/* The rest: stage-by-stage packet flow through the EC. */
static const struct alu_ctr ec_flow_ctrs[] = {
	C32(al_eth_ec_stats, faf_in_rx_pkt),
	C32(al_eth_ec_stats, faf_out_rx_pkt),
	C32(al_eth_ec_stats, rxf_in_rx_pkt),
	C32(al_eth_ec_stats, lbf_in_rx_pkt),
	C32(al_eth_ec_stats, rxf_out_rx_1_pkt),
	C32(al_eth_ec_stats, rxf_out_rx_2_pkt),
	C32(al_eth_ec_stats, rpe_1_in_rx_pkt),
	C32(al_eth_ec_stats, rpe_1_out_rx_pkt),
	C32(al_eth_ec_stats, rpe_2_in_rx_pkt),
	C32(al_eth_ec_stats, rpe_2_out_rx_pkt),
	C32(al_eth_ec_stats, rpe_3_in_rx_pkt),
	C32(al_eth_ec_stats, rpe_3_out_rx_pkt),
	C32(al_eth_ec_stats, tpe_in_tx_pkt),
	C32(al_eth_ec_stats, tpe_out_tx_pkt),
	C32(al_eth_ec_stats, tpm_tx_pkt),
	C32(al_eth_ec_stats, tfw_in_tx_pkt),
	C32(al_eth_ec_stats, tfw_out_tx_pkt),
	C32(al_eth_ec_stats, rfw_in_rx_pkt),
	C32(al_eth_ec_stats, rfw_in_mc),
	C32(al_eth_ec_stats, rfw_in_bc),
	C32(al_eth_ec_stats, rfw_in_vlan_exist),
	C32(al_eth_ec_stats, rfw_in_vlan_nexist),
	C32(al_eth_ec_stats, eee_in),
};

static const struct alu_ctr ec_udma_ctrs[] = {
	C32(al_eth_ec_stat_udma, rfw_out_drop),
	C32(al_eth_ec_stat_udma, msw_drop_q_full),
	C32(al_eth_ec_stat_udma, msw_drop_sop),
	C32(al_eth_ec_stat_udma, msw_drop_eop),
	C32(al_eth_ec_stat_udma, tpm_tx_spoof),
	C32(al_eth_ec_stat_udma, rfw_out_rx_pkt),
	C32(al_eth_ec_stat_udma, msw_in_rx_pkt),
	C32(al_eth_ec_stat_udma, msw_wr_eop),
	C32(al_eth_ec_stat_udma, msw_out_rx_pkt),
	C32(al_eth_ec_stat_udma, tso_no_tso_pkt),
	C32(al_eth_ec_stat_udma, tso_tso_pkt),
	C32(al_eth_ec_stat_udma, tso_seg_pkt),
	C32(al_eth_ec_stat_udma, tso_pad_pkt),
	C32(al_eth_ec_stat_udma, tmi_in_tx_pkt),
	C32(al_eth_ec_stat_udma, tmi_out_to_mac),
	C32(al_eth_ec_stat_udma, tmi_out_to_rx),
	C32(al_eth_ec_stat_udma, tx_q0_bytes),
	C32(al_eth_ec_stat_udma, tx_q1_bytes),
	C32(al_eth_ec_stat_udma, tx_q2_bytes),
	C32(al_eth_ec_stat_udma, tx_q3_bytes),
	C32(al_eth_ec_stat_udma, tx_q0_pkts),
	C32(al_eth_ec_stat_udma, tx_q1_pkts),
	C32(al_eth_ec_stat_udma, tx_q2_pkts),
	C32(al_eth_ec_stat_udma, tx_q3_pkts),
};

/* Running totals of the clear-on-read EC blocks, per port. Indexed by port, so
 * the two ports never share a total. */
static struct {
	struct al_eth_ec_stats		ec;
	struct al_eth_ec_stat_udma	udma[ALU_ETH_UDMAS_10G];
	bool				seen;
} alu_ec_total[ALU_ETH_PORT_10G + 1];

/* Add a just-read (delta) block into its running total, in place. Every field
 * of these two structs is uint32_t, so the walk is uniform. */
static void ec_accumulate(void *total, const void *delta, size_t bytes)
{
	u32 *t = total;
	const u32 *d = delta;
	size_t i;

	for (i = 0; i < bytes / sizeof(u32); i++)
		t[i] += d[i];
}

/* Every counter is printed, zero included: a name missing from the output must
 * never be readable as "zero" (#175). */
static void ctrs_print(const char *prefix, const void *base,
		       const struct alu_ctr *tbl, unsigned int n)
{
	unsigned int i;

	for (i = 0; i < n; i++) {
		const void *p = (const u8 *)base + tbl[i].off;

		if (tbl[i].width == 8)
			printf("%s%s: %llu\n", prefix, tbl[i].name,
			       (unsigned long long)*(const u64 *)p);
		else
			printf("%s%s: %u\n", prefix, tbl[i].name,
			       *(const u32 *)p);
	}
}

/* Which counter block the MAC keeps its numbers in, taken from the board params
 * so the command reports the port's configured mode rather than a guess. */
static enum al_eth_mac_mode stats_mac_mode(int port, void __iomem *mac)
{
	struct al_eth_board_params p;

	if (al_eth_board_params_get(mac, &p))
		return (port == ALU_ETH_PORT_1G) ? AL_ETH_MAC_MODE_RGMII :
						   AL_ETH_MAC_MODE_10GbE_Serial;

	switch (p.media_type) {
	case AL_ETH_BOARD_MEDIA_TYPE_RGMII:
		return AL_ETH_MAC_MODE_RGMII;
	case AL_ETH_BOARD_MEDIA_TYPE_SGMII:
	case AL_ETH_BOARD_MEDIA_TYPE_1000BASE_X:
		return AL_ETH_MAC_MODE_SGMII;
	case AL_ETH_BOARD_MEDIA_TYPE_SGMII_2_5G:
		return AL_ETH_MAC_MODE_SGMII_2_5G;
	default:
		return AL_ETH_MAC_MODE_10GbE_Serial;
	}
}

/* The Clause-49 PCS counters come from al_eth_link_status_get(), which is their
 * ONE reader - they are clear-on-read and a second read returns zero (#196). */
static void stats_pcs_print(struct al_hal_eth_adapter *a)
{
	struct al_eth_link_status st;
	int rc;

	memset(&st, 0, sizeof(st));
	rc = al_eth_link_status_get(a, &st);
	if (rc) {
		printf("pcs: unavailable (%d)\n", rc);
		return;
	}

	printf("pcs.link_up: %u\n", st.link_up);
	printf("pcs.local_fault: %u\n", st.local_fault);
	printf("pcs.remote_fault: %u\n", st.remote_fault);
	printf("pcs.errored_blocks: %u%s\n", st.pcs_errored_blocks,
	       (st.pcs_errored_blocks == 0xff) ? " (SATURATED - at least this many)" : "");
	printf("pcs.ber_count: %u%s\n", st.pcs_ber_count,
	       (st.pcs_ber_count == 0x3f) ? " (SATURATED - at least this many)" : "");
	printf("pcs.note: both counters are clear-on-read; this read zeroed them\n");
}

int alu_eth_stats_show(int port)
{
	struct al_hal_eth_adapter a;
	struct al_eth_ec_stat_udma us;
	struct al_eth_mac_stats ms;
	struct al_eth_ec_stats es;
	struct alu_eth_regs regs;
	unsigned int udma, nudma;
	char name[] = "eth-stats";
	int rc;

	rc = alu_eth_port_regs_get(port, &regs);
	if (rc)
		return rc;

	/* Software-only handle. The shared HAL's adapter is a plain struct - no
	 * vtable to dispatch - so the four fields the stats paths read are filled
	 * in directly and al_eth_adapter_init() (which configures hardware) is
	 * never called. */
	memset(&a, 0, sizeof(a));
	a.rev_id = AL_ETH_REV_ID_2;
	a.udma_regs_base = regs.udma;
	a.ec_regs_base = (struct al_ec_regs __iomem *)regs.ec;
	a.mac_regs_base = (struct al_eth_mac_regs __iomem *)regs.mac;
	a.name = name;
	a.mac_mode = stats_mac_mode(port, regs.mac);

	nudma = (port == ALU_ETH_PORT_1G) ? ALU_ETH_UDMAS_1G : ALU_ETH_UDMAS_10G;

	printf("alu_eth port %d (%s), mac_mode %u\n", port,
	       alu_eth_port_desc(port), a.mac_mode);

	/* MAC counters are free-running, so they print as read. */
	rc = al_eth_mac_stats_get(&a, &ms);
	if (rc)
		printf("mac: unavailable (%d)\n", rc);
	else
		ctrs_print("mac.", &ms, mac_ctrs, ARRAY_SIZE(mac_ctrs));

	/* EC counters are clear-on-read: accumulate, then print the total. */
	rc = al_eth_ec_stats_get(&a, &es);
	if (rc) {
		printf("ec: unavailable (%d)\n", rc);
	} else {
		ec_accumulate(&alu_ec_total[port].ec, &es, sizeof(es));
		alu_ec_total[port].seen = true;
		ctrs_print("ec.drop.", &alu_ec_total[port].ec, ec_drop_ctrs,
			   ARRAY_SIZE(ec_drop_ctrs));
		ctrs_print("ec.", &alu_ec_total[port].ec, ec_flow_ctrs,
			   ARRAY_SIZE(ec_flow_ctrs));
	}

	for (udma = 0; udma < nudma; udma++) {
		char prefix[16];

		rc = al_eth_ec_stat_udma_get(&a, udma, &us);
		if (rc) {
			printf("udma%u: unavailable (%d)\n", udma, rc);
			continue;
		}
		ec_accumulate(&alu_ec_total[port].udma[udma], &us, sizeof(us));
		snprintf(prefix, sizeof(prefix), "udma%u.", udma);
		ctrs_print(prefix, &alu_ec_total[port].udma[udma], ec_udma_ctrs,
			   ARRAY_SIZE(ec_udma_ctrs));
	}

	if (a.mac_mode == AL_ETH_MAC_MODE_10GbE_Serial)
		stats_pcs_print(&a);

	printf("ec.note: EC counters are clear-on-read; the totals above accumulate\n");
	printf("         every `eth stats` since power-on, not just this read.\n");

	return 0;
}

/* ---- the command ------------------------------------------------------- */

static int do_eth(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	int port = ALU_ETH_STATS_DEFAULT_PORT;
	int rc;

	if (argc < 2)
		return CMD_RET_USAGE;
	if (argc > 2)
		port = (int)dectoul(argv[2], NULL);

	if (!strcmp(argv[1], "stats"))
		rc = alu_eth_stats_show(port);
	else if (!strcmp(argv[1], "diag"))
		rc = alu_eth_diag_show(port);
	else
		return CMD_RET_USAGE;

	return rc ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

U_BOOT_CMD(eth, 3, 0, do_eth,
	   "alu_eth bring-up diagnostics + hardware counters",
	   "diag [<port>]   - one bring-up block: PCI BDF, the three BARs, MAC\n"
	   "                  address + source, board params decoded, and link\n"
	   "                  state (1G: PHY id + AN result; 10G: PCS block-lock,\n"
	   "                  SerDes grp/lane, TX equalisation taps in force).\n"
	   "eth stats [<port>]  - dump MAC + EC + per-UDMA counters.\n"
	   "                  Drop/error counters come first. Zero counters are\n"
	   "                  printed too. The EC counters are clear-on-read, so\n"
	   "                  they are accumulated across invocations; the MAC\n"
	   "                  counters are free-running. On a 10G port the\n"
	   "                  Clause-49 PCS errored-block/BER counters are\n"
	   "                  included - those saturate, so a pegged value means\n"
	   "                  \"at least this many\".\n"
	   "\n"
	   "<port> defaults to 2; 1 = 1G RJ45, 2 = 10G SFP+.");
