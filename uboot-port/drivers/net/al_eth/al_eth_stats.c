// SPDX-License-Identifier: GPL-2.0-or-later
/* `eth stats` - dump every al_eth hardware counter as plain text.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * U-Boot's answer to Linux's `ethtool -S`: MAC + EC + per-UDMA counters, plus
 * the Clause-49 PCS errored-block/BER counters on a 10G port. Counter walking
 * is table-driven ({name, offset}) so the same tables port to EDK2 (#39).
 *
 * Read-only. The adapter handle is built here (no al_eth_adapter_init) so the
 * command never disturbs a port that is already up.
 */

#include <command.h>
#include <errno.h>
#include <stdio.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <vsprintf.h>
#include <asm/io.h>

#include <al_hal_eth.h>
#include <al_hal_eth_mac.h>
#include <al_hal_eth_mac_regs.h>

#include "al_eth_port.h"

#define AL_ETH_STATS_DEFAULT_PORT	2

/* Clause-49 BASE-R Status 2 (MDIO dev 3, reg 0x21): bits 7:0 errored-block
 * count, 13:8 BER count. Both CLEAR-ON-READ and both SATURATE. */
#define AL_KR_PCS_ERR_BLK_MASK		0x00ff
#define AL_KR_PCS_ERR_BLK_MAX		0xff
#define AL_KR_PCS_BER_SHIFT		8
#define AL_KR_PCS_BER_MASK		0x3f
#define AL_KR_PCS_BER_MAX		0x3f
#define AL_KR_PCS_BLOCK_LOCK		AL_BIT(15)
#define AL_KR_PCS_HI_BER		AL_BIT(14)

struct al_eth_ctr {
	const char	*name;
	u16		off;
	u8		width;		/* 4 or 8 bytes */
};

#define C32(s, f)	{ #f, offsetof(struct s, f), 4 }
#define C64(s, f)	{ #f, offsetof(struct s, f), 8 }

static const struct al_eth_ctr mac_ctrs[] = {
	C64(al_eth_mac_stats, octets_rxed_ok),
	C64(al_eth_mac_stats, octets_txed_ok),
	C32(al_eth_mac_stats, pkts),
	C32(al_eth_mac_stats, octets),
	C32(al_eth_mac_stats, if_in_ucast_pkts),
	C32(al_eth_mac_stats, if_in_mcast_pkts),
	C32(al_eth_mac_stats, if_in_bcast_pkts),
	C32(al_eth_mac_stats, if_in_errors),
	C32(al_eth_mac_stats, if_out_ucast_pkts),
	C32(al_eth_mac_stats, if_out_mcast_pkts),
	C32(al_eth_mac_stats, if_out_bcast_pkts),
	C32(al_eth_mac_stats, if_out_errors),
	C32(al_eth_mac_stats, frames_rxed_ok),
	C32(al_eth_mac_stats, frames_txed_ok),
	C32(al_eth_mac_stats, undersize_pkts),
	C32(al_eth_mac_stats, fragments),
	C32(al_eth_mac_stats, jabbers),
	C32(al_eth_mac_stats, oversize_pkts),
	C32(al_eth_mac_stats, fcs_errors),
	C32(al_eth_mac_stats, alignment_errors),
	C32(al_eth_mac_stats, drop_events),
	C32(al_eth_mac_stats, frame_too_long_errs),
	C32(al_eth_mac_stats, in_range_length_errs),
	C32(al_eth_mac_stats, pause_mac_ctrl_frames_rxed),
	C32(al_eth_mac_stats, pause_mac_ctrl_frames_txed),
	C32(al_eth_mac_stats, vlan_rxed_ok),
	C32(al_eth_mac_stats, vlan_txed_ok),
	C32(al_eth_mac_stats, pkts_64_octets),
	C32(al_eth_mac_stats, pkts_65_to_127_octets),
	C32(al_eth_mac_stats, pkts_128_to_255_octets),
	C32(al_eth_mac_stats, pkts_256_to_511_octets),
	C32(al_eth_mac_stats, pkts_512_to_1023_octets),
	C32(al_eth_mac_stats, pkts_1024_to_1518_octets),
	C32(al_eth_mac_stats, pkts_1519_to_x),
	C32(al_eth_mac_stats, eee_in),
	C32(al_eth_mac_stats, eee_out),
};

/* The "where did the packet go" set - every EC counter that means a drop or an
 * error. Printed first because it is the reason to run this command. */
static const struct al_eth_ctr ec_drop_ctrs[] = {
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

/* The remaining EC counters: stage-by-stage packet flow through the EC. */
static const struct al_eth_ctr ec_flow_ctrs[] = {
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

static const struct al_eth_ctr ec_udma_ctrs[] = {
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

/* Every counter is printed, zero included: a name missing from the output must
 * never be readable as "zero" (#175). */
static void ctrs_print(const char *prefix, const void *base,
		       const struct al_eth_ctr *tbl, unsigned int n)
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

/* MAC counters live in either the 1G or the 10G block, selected by mac_mode.
 * Derived from the board params so the command reports the port's configured
 * mode, not a guess. */
static enum al_eth_mac_mode stats_mac_mode(int port, void __iomem *mac)
{
	struct al_eth_board_params p;

	if (al_eth_board_params_get(mac, &p))
		return (port == 1) ? AL_ETH_MAC_MODE_RGMII :
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

static void stats_pcs_print(void __iomem *mac)
{
	struct al_eth_mac_regs __iomem *m = (struct al_eth_mac_regs __iomem *)mac;
	u32 err_blk, ber, reg;

	writel(ETH_MAC_KR_PCS_BASE_R_STATUS2, &m->kr.pcs_addr);
	reg = readl(&m->kr.pcs_data);

	err_blk = reg & AL_KR_PCS_ERR_BLK_MASK;
	ber = (reg >> AL_KR_PCS_BER_SHIFT) & AL_KR_PCS_BER_MASK;

	printf("pcs.base_r_status2: 0x%04x\n", reg & 0xffff);
	printf("pcs.block_lock: %u\n", !!(reg & AL_KR_PCS_BLOCK_LOCK));
	printf("pcs.hi_ber: %u\n", !!(reg & AL_KR_PCS_HI_BER));
	printf("pcs.errored_blocks: %u%s\n", err_blk,
	       (err_blk == AL_KR_PCS_ERR_BLK_MAX) ? " (SATURATED - at least this many)" : "");
	printf("pcs.ber_count: %u%s\n", ber,
	       (ber == AL_KR_PCS_BER_MAX) ? " (SATURATED - at least this many)" : "");
	printf("pcs.note: both counters are clear-on-read; this read zeroed them\n");
}

static int al_eth_stats_show(int port)
{
	struct al_eth_adapter_handle_init_params hp;
	struct al_hal_eth_adapter a;
	struct al_eth_ec_stat_udma us;
	struct al_eth_mac_stats ms;
	struct al_eth_ec_stats es;
	struct al_eth_port_regs regs;
	unsigned int udma, nudma;
	char name[] = "eth-stats";
	int rc;

	rc = al_eth_port_regs_get(port, &regs);
	if (rc)
		return rc;

	/* Handle-only init: pure software, no register writes, so this never
	 * disturbs a port that is already up (al_eth_adapter_init is the one
	 * that configures hardware, and is deliberately NOT called). */
	memset(&hp, 0, sizeof(hp));
	hp.rev_id = AL_ETH_REV_ID_2;			/* Alpine V2 */
	hp.dev_id = (port % 2) ? AL_ETH_DEV_ID_STANDARD : AL_ETH_DEV_ID_ADVANCED;
	hp.udma_regs_base = regs.udma;
	hp.ec_regs_base = (struct al_ec_regs __iomem *)regs.ec;
	hp.mac_regs_base = (struct al_eth_mac_regs __iomem *)regs.mac;
	hp.common_mode = AL_ETH_COMMON_MODE_INVALID;
	hp.name = name;
	al_eth_adapter_handle_init(&a, &hp);

	/* stats_get picks the 1G or 10G counter block off mac_mode, which only
	 * al_eth_adapter_init would otherwise set. */
	a.mac_obj.mac_mode = stats_mac_mode(port, regs.mac);
	a.mac_mode = a.mac_obj.mac_mode;

	/* eth2 is the "advanced" function: 4 UDMAs. eth1 has 1. */
	nudma = (port % 2) ? 1 : 4;

	printf("al_eth port %d (%s), mac_mode %s\n", port, al_eth_port_desc(port),
	       al_eth_mac_mode_str(a.mac_mode));

	rc = al_eth_mac_stats_get(&a, &ms);
	if (rc)
		printf("mac: unavailable (%d)\n", rc);
	else
		ctrs_print("mac.", &ms, mac_ctrs, ARRAY_SIZE(mac_ctrs));

	rc = al_eth_ec_stats_get(&a, &es);
	if (rc) {
		printf("ec: unavailable (%d)\n", rc);
	} else {
		ctrs_print("ec.drop.", &es, ec_drop_ctrs, ARRAY_SIZE(ec_drop_ctrs));
		ctrs_print("ec.", &es, ec_flow_ctrs, ARRAY_SIZE(ec_flow_ctrs));
	}

	for (udma = 0; udma < nudma; udma++) {
		char prefix[16];

		rc = al_eth_ec_stat_udma_get(&a, udma, &us);
		if (rc) {
			printf("udma%u: unavailable (%d)\n", udma, rc);
			continue;
		}
		snprintf(prefix, sizeof(prefix), "udma%u.", udma);
		ctrs_print(prefix, &us, ec_udma_ctrs, ARRAY_SIZE(ec_udma_ctrs));
	}

	if (AL_ETH_IS_10G_MAC(a.mac_mode))
		stats_pcs_print(regs.mac);

	return 0;
}

/* al_eth_diag.c */
int al_eth_diag_show(int port);

static int do_eth(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	int port = AL_ETH_STATS_DEFAULT_PORT;
	int rc;

	if (argc < 2)
		return CMD_RET_USAGE;
	if (argc > 2)
		port = (int)dectoul(argv[2], NULL);

	if (!strcmp(argv[1], "stats"))
		rc = al_eth_stats_show(port);
	else if (!strcmp(argv[1], "diag"))
		rc = al_eth_diag_show(port);
	else
		return CMD_RET_USAGE;

	return rc ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

U_BOOT_CMD(eth, 3, 0, do_eth,
	   "al_eth bring-up diagnostics + hardware counters",
	   "diag [<port>]   - one bring-up block: PCI BDF, the three BARs, MAC\n"
	   "                  address + source, board params decoded, and link\n"
	   "                  state (1G: PHY id + AN result; 10G: PCS block-lock,\n"
	   "                  SerDes grp/lane, TX equalisation taps in force).\n"
	   "eth stats [<port>]  - dump MAC + EC + per-UDMA counters.\n"
	   "                  Drop/error counters come first. Zero counters are\n"
	   "                  printed too. On a 10G port the Clause-49 PCS\n"
	   "                  errored-block/BER counters are included - those two\n"
	   "                  are clear-on-read and saturate, so reading zeroes\n"
	   "                  them and a pegged value means \"at least this many\".\n"
	   "\n"
	   "<port> defaults to 2; 1 = 1G RJ45, 2 = 10G SFP+.");
