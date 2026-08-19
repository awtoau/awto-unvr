/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * `ddr` command — DDR config dump / ECC / training results / PHY BIST margins
 * for the Annapurna Labs Alpine V2 (AL-324) memory controller, backed by the
 * Annapurna DDR HAL running against the already-trained live controller.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 * Adapted to the modern U-Boot command API from the stock Annapurna U-Boot
 * board/annapurna-labs/common/cmd_ddr.c (Copyright (C) 2013 Annapurna Labs
 * Ltd, GPLv2). BIST/margins here use the HAL PHY DATX BIST family directly
 * (al_ddr_phy_datx_bist) — NOT the stock cmd_dram_margins.c prebuilt SRAM blob.
 *
 * Register bases: read from the DT node (compatible "annapurna-labs,al-ddr",
 * reg-names nb/ctrl/phy) via ofnode — NEVER hardcoded (#84). docs/hardware.md /
 * docs/uboot-ddr-port.md §4 (nb-service 0xf0070000, uMCTL2 ctrl 0xf0080000,
 * PUB PHY 0xf0088000; ddrc NULL on V2, channel 0).
 * Safe from a chainloaded U-Boot: no retrain, LOOPBACK BIST is PHY-internal.
 */

#include <command.h>
#include <vsprintf.h>
#include <linux/string.h>
#include <linux/types.h>
#include <dm/ofnode.h>

#include "al_hal_ddr.h"

#define AL_DDR_DT_COMPAT	"annapurna-labs,al-ddr"

/* Resolve one DDR register base by reg-name from the DT node (no #defines). */
static void __iomem *ddr_dt_base(ofnode node, const char *reg_name)
{
	int idx = ofnode_stringlist_search(node, "reg-names", reg_name);
	fdt_addr_t addr;

	if (idx < 0)
		return NULL;
	addr = ofnode_get_addr_index(node, idx);
	if (addr == FDT_ADDR_T_NONE)
		return NULL;
	return (void __iomem *)(uintptr_t)addr;
}

static int ddr_cfg_open(struct al_ddr_cfg *cfg)
{
	ofnode node = ofnode_by_compatible(ofnode_null(), AL_DDR_DT_COMPAT);
	void __iomem *nb, *ctrl, *phy;
	int err;

	if (!ofnode_valid(node)) {
		printf("ddr: no DT node (compatible \"%s\")\n", AL_DDR_DT_COMPAT);
		return -1;
	}

	nb   = ddr_dt_base(node, "nb");
	ctrl = ddr_dt_base(node, "ctrl");
	phy  = ddr_dt_base(node, "phy");
	if (!nb || !ctrl || !phy) {
		printf("ddr: DT node missing nb/ctrl/phy reg (nb=%p ctrl=%p phy=%p)\n",
		       nb, ctrl, phy);
		return -1;
	}

	err = al_ddr_cfg_init(nb, ctrl, phy, cfg);
	if (err) {
		printf("ddr: al_ddr_cfg_init failed (%d)\n", err);
		return -1;
	}
	return 0;
}

static const char *ddr_data_width_str(enum al_ddr_data_width w)
{
	switch (w) {
	case AL_DDR_DATA_WIDTH_16_BITS: return "16-bit";
	case AL_DDR_DATA_WIDTH_32_BITS: return "32-bit";
	case AL_DDR_DATA_WIDTH_64_BITS: return "64-bit";
	default:			return "?";
	}
}

static const char *ddr_op_mode_str(enum al_ddr_operating_mode m)
{
	switch (m) {
	case AL_DDR_OPERATING_MODE_INIT:	 return "init";
	case AL_DDR_OPERATING_MODE_NORMAL:	 return "normal";
	case AL_DDR_OPERATING_MODE_POWER_DOWN:	 return "power-down";
	case AL_DDR_OPERATING_MODE_SELF_REFRESH: return "self-refresh";
	default:				 return "?";
	}
}

static int do_ddr_info(void)
{
	struct al_ddr_cfg cfg;
	struct al_ddr_ecc_cfg ecc;
	unsigned int i;

	if (ddr_cfg_open(&cfg))
		return CMD_RET_FAILURE;

	printf("DDR controller (Alpine V2, ch %u) @ %p\n",
	       cfg.ch, cfg.ddr_ctrl_regs_base);
	printf(" operating mode : %s\n",
	       ddr_op_mode_str(al_ddr_operating_mode_get(&cfg)));
	printf(" data width     : %s\n",
	       ddr_data_width_str(al_ddr_data_width_get(&cfg)));
	printf(" active ranks   : %u (mask 0x%x)\n",
	       al_ddr_active_ranks_get(&cfg), al_ddr_active_ranks_mask_get(&cfg));
	printf(" active banks   : %u\n", al_ddr_active_banks_get(&cfg));
	printf(" active bankgrp : %u\n", al_ddr_active_bg_get(&cfg));
	printf(" active columns : %u\n", al_ddr_active_columns_get(&cfg));
	printf(" active rows    : %u\n", al_ddr_active_rows_get(&cfg));
	printf(" bits per rank  : %u\n", al_ddr_bits_per_rank_get(&cfg));

	al_ddr_ecc_cfg_get(&cfg, &ecc);
	printf(" ECC            : %s%s\n",
	       ecc.ecc_enabled ? "enabled" : "disabled",
	       (ecc.ecc_enabled && ecc.scrub_enabled) ? " (scrub on)" : "");

	for (i = 0; i <= 6; i++)
		printf(" MR%u            : 0x%04x\n", i,
		       al_ddr_mode_register_get(&cfg, i));

	return CMD_RET_SUCCESS;
}

static void ddr_ecc_stats_print(struct al_ddr_ecc_status *s, int corr)
{
	printf(" - ECC errors detected: %u\n", s->err_cnt);
	if (!s->err_cnt)
		return;
	printf(" - First bad read addr: rank %u bank %u row %u col %u\n",
	       s->rank, s->bank, s->row, s->col);
	printf(" - First bad data     : %08x:%08x:%02x\n",
	       s->syndromes_63_32, s->syndromes_31_0, s->syndromes_ecc);
	if (!corr)
		return;
	printf(" - Corrected data mask: %08x:%08x:%02x\n",
	       s->corr_bit_mask_63_32, s->corr_bit_mask_31_0,
	       s->corr_bit_mask_ecc);
	printf(" - Corrected bit num  : %u\n", s->ecc_corrected_bit_num);
}

static int do_ddr_ecc(int clear)
{
	struct al_ddr_cfg cfg;
	struct al_ddr_ecc_cfg ecc;
	struct al_ddr_ecc_status corr, uncorr;

	if (ddr_cfg_open(&cfg))
		return CMD_RET_FAILURE;

	al_ddr_ecc_cfg_get(&cfg, &ecc);
	if (!ecc.ecc_enabled) {
		printf("DDR ECC is disabled\n");
		return CMD_RET_SUCCESS;
	}

	if (al_ddr_ecc_status_get(&cfg, &corr, &uncorr)) {
		printf("ddr: al_ddr_ecc_status_get failed\n");
		return CMD_RET_FAILURE;
	}

	printf("Corrected:\n");
	ddr_ecc_stats_print(&corr, 1);
	printf("Uncorrected:\n");
	ddr_ecc_stats_print(&uncorr, 0);

	if (clear) {
		al_ddr_ecc_corr_count_clear(&cfg);
		al_ddr_ecc_corr_int_clear(&cfg);
		al_ddr_ecc_uncorr_count_clear(&cfg);
		al_ddr_ecc_uncorr_int_clear(&cfg);
		printf("ECC counters/interrupts cleared\n");
	}

	return CMD_RET_SUCCESS;
}

static int do_ddr_training(void)
{
	struct al_ddr_cfg cfg;

	if (ddr_cfg_open(&cfg))
		return CMD_RET_FAILURE;

	al_ddr_phy_training_results_print(&cfg);
	return CMD_RET_SUCCESS;
}

/*
 * PHY DATX BIST — the source-based data-eye/margins run (docs §5). LOOPBACK is
 * PHY-internal and non-destructive; DRAM mode drives the array (test range
 * only) and may fault a live U-Boot — opt in explicitly. VT calc is disabled
 * around the run via the _pre/_post pair as the HAL requires.
 */
static int do_ddr_bist(int dram_mode)
{
	struct al_ddr_cfg cfg;
	struct al_ddr_bist_params p;
	struct al_ddr_bist_err_status st;
	int err;

	if (ddr_cfg_open(&cfg))
		return CMD_RET_FAILURE;

	memset(&p, 0, sizeof(p));
	memset(&st, 0, sizeof(st));

	p.mode = dram_mode ? AL_DDR_BIST_MODE_DRAM : AL_DDR_BIST_MODE_LOOPBACK;
	p.pat = AL_DDR_BIST_PATTERN_LFSR;
	p.wc = 16;			/* words to generate (multiple of 4) */
	p.inc = 8;			/* burst-aligned address increment */
	p.col_min = 0;
	p.col_max = 0x38;		/* small burst-aligned column window */
	p.row_min = 0;
	p.row_max = 0;
	p.bank_min = 0;
	p.bank_max = 0;
	p.rank_min = 0;
	p.rank_max = 0;
	al_ddr_active_byte_lanes_get(&cfg, p.active_byte_lanes);
	p.all_lanes_active = AL_TRUE;
	p.inf = AL_FALSE;
	p.sonf = AL_FALSE;

	printf("ddr bist: %s mode, LFSR pattern, wc=%u\n",
	       dram_mode ? "DRAM" : "loopback", p.wc);

	al_ddr_phy_datx_bist_pre(&cfg);
	err = al_ddr_phy_datx_bist(&cfg, &p, &st);
	al_ddr_phy_datx_bist_post(&cfg);

	if (st.timeout) {
		printf("ddr bist: TIMEOUT waiting for BIST done\n");
		return CMD_RET_FAILURE;
	}
	if (err) {
		printf("ddr bist: FAILED — lane %d, word_err %u of %u words\n",
		       st.lane_err, st.word_err, st.word_count);
		return CMD_RET_FAILURE;
	}

	printf("ddr bist: PASS — %u words, no errors\n", st.word_count);
	return CMD_RET_SUCCESS;
}

static int do_ddr(struct cmd_tbl *cmdtp, int flag, int argc,
		  char *const argv[])
{
	if (argc < 2)
		return CMD_RET_USAGE;

	if (!strcmp(argv[1], "info"))
		return do_ddr_info();
	if (!strcmp(argv[1], "training"))
		return do_ddr_training();
	if (!strcmp(argv[1], "ecc"))
		return do_ddr_ecc(argc > 2 && !strcmp(argv[2], "clear"));
	if (!strcmp(argv[1], "bist"))
		return do_ddr_bist(argc > 2 && !strcmp(argv[2], "dram"));

	return CMD_RET_USAGE;
}

U_BOOT_CMD(
	ddr, 3, 0, do_ddr,
	"Alpine V2 DDR controller inspection / BIST margins (live controller)",
	"info               - dump controller/PHY config + mode registers\n"
	"ddr training           - print PHY training results (per-octet)\n"
	"ddr ecc [clear]        - ECC status counters (optionally clear)\n"
	"ddr bist [dram]        - PHY DATX BIST; loopback (default) or dram mode"
);
