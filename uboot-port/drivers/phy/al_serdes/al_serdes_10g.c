// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Annapurna Labs 25G-SerDes + 10GBASE-R PCS bring-up for the UNVR SFP+ port
 * (eth2). Fixed 10.3125 Gbps, KR auto-neg + link-training DISABLED.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 * Derived from Annapurna Labs HAL (Copyright (C) Annapurna Labs Ltd, GPLv2 OR
 * BSD-3-Clause); reimplemented on U-Boot primitives.
 *
 * Scope: SerDes PMA (25G HAL for status; HSSP HAL for the group-D SFP+ lane's
 * optic TX/RX EQ, #111) + the 10GBASE-R PCS + the fixed-10G link-mgmt bits. It
 * does NOT touch the MAC/UDMA/DM_ETH driver (a separate effort). The SerDes
 * electrical mode here is the HAL's "KR" = 10.3125G NRZ; Clause-73 AN /
 * Clause-72 LT are a MAC-KR-FSM feature that is deliberately NOT invoked, so
 * the lane comes up at a fixed rate (10G_OPTIC / passive-DAC use case).
 *
 * COMPILE-VERIFIED ONLY. A human iterates the real lane bring-up on the box.
 * Hardware-iteration points are tagged "HW:" below.
 */

#include <command.h>
#include <dm/ofnode.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <vsprintf.h>

#include "al_hal_serdes_25g.h"		/* pulls interface + common + shim */
#include "al_hal_serdes_hssp.h"		/* HSSP group HAL - the REAL group-D driver */
#include "al_serdes_10g.h"

/* DT: our U-Boot dts is the hardware-of-record (docs/hardware.md). Bases are
 * read via ofnode by reg-name, never hardcoded.
 *   reg-name "serdes" -> SerDes PMA  (fd8c0000, size 0x2400)
 *   reg-name "pcs"    -> eth2 MAC-adapter window that holds the 10GBASE-R PCS
 *                        sub-block (OPTIONAL; shared with the eth/MAC agent's
 *                        node — HW: confirm the exact base on the box).
 */
#define AL_SERDES_DT_COMPAT	"annapurna-labs,al-serdes-25g"

/*
 * The SFP+ lane. HW: confirm which physical 25G lane the SFP+ TX/RX pair is
 * wired to on the AL-324 (LN0 vs LN1) before trusting per-lane status.
 */
#define AL_SFP_LANE		AL_SRDS_LANE_0

/*
 * 10GBASE-R PCS register offsets inside the "pcs" (MAC-adapter) window.
 * Transcribed from al_hal_eth_mac_regs.h so we do NOT pull in the MAC HAL:
 *   kr        @ 0xa00 : pcs_addr +0x00, pcs_data +0x04 (Clause-45 indirect)
 *   gen_v3    @ 0xe00 : pcs_10g_ll_cfg +0x38, pcs_10g_ll_status +0x3c
 * PCS block-lock (10G serial) = KR-PCS BASE-R Status 2 (reg 0x21), bit 15.
 */
#define AL_PCS_KR_ADDR		0xa00	/* kr.pcs_addr  */
#define AL_PCS_KR_DATA		0xa04	/* kr.pcs_data  */
#define AL_PCS_10G_LL_CFG	0xe38	/* gen_v3.pcs_10g_ll_cfg    */
#define AL_PCS_10G_LL_STATUS	0xe3c	/* gen_v3.pcs_10g_ll_status */
#define AL_PCS_10G_LL_STATUS_FEC_LOCKED	(1u << 0)
#define AL_KR_PCS_BASE_R_STATUS2	0x21	/* Clause-45 dev3 reg 0x21 */
#define AL_KR_PCS_BLOCK_LOCK		(1u << 15)
/* 10GbE-Serial PCS cfg value (al_hal_eth_main 10GbE_Serial path). HW: verify. */
#define AL_PCS_10G_LL_CFG_10GR	0x00000050

/*
 * 10G optic TX/RX equaliser params — mirror al_init_eth_lm.c's 10G_OPTIC static
 * values (optic_tx_params / optic_rx_params). Applied in al_serdes_10g_init()
 * via obj.tx_advanced_params_set()/rx_advanced_params_set() (#111). HW: these
 * are the vendor optic defaults; retune per the actual SFP/DAC on the box
 * (rx_equalization sweep).
 */
static struct al_serdes_adv_tx_params optic_tx_params = {
	.override		= AL_TRUE,
	.amp			= 0x1,
	.total_driver_units	= 0x13,
	.c_plus_1		= 0x2,
	.c_plus_2		= 0,
	.c_minus_1		= 0,
	.slew_rate		= 0,
};

static struct al_serdes_adv_rx_params optic_rx_params = {
	.override		= AL_TRUE,
	.dcgain			= 0x0,
	.dfe_3db_freq		= 0x7,
	.dfe_gain		= 0x0,
	.dfe_first_tap_ctrl	= 0x0,
	.dfe_secound_tap_ctrl	= 0x8,
	.dfe_third_tap_ctrl	= 0x0,
	.dfe_fourth_tap_ctrl	= 0x8,
	.low_freq_agc_gain	= 0x7,
	.precal_code_sel	= 0,
	.high_freq_agc_boost	= 0x4,
};

/* Resolve a reg base by reg-name on the serdes DT node. Returns NULL if absent. */
static void __iomem *al_serdes_dt_base(const char *reg_name)
{
	ofnode node;
	int idx;
	fdt_addr_t addr;

	node = ofnode_by_compatible(ofnode_null(), AL_SERDES_DT_COMPAT);
	if (!ofnode_valid(node))
		return NULL;

	idx = ofnode_stringlist_search(node, "reg-names", reg_name);
	if (idx < 0)
		return NULL;

	addr = ofnode_get_addr_index(node, idx);
	if (addr == FDT_ADDR_T_NONE)
		return NULL;

	return (void __iomem *)(uintptr_t)addr;
}

/* ---- HSSP SerDes group D (the REAL SFP+ 10G lane) -------------------------
 * CORRECTION (root cause of the 10G SError): the UNVR SFP+ 10G is on the HSSP
 * SerDes complex, GROUP D (serdes-grp 3, lane 0), NOT the 25G complex that
 * al_hal_serdes_25g targets. On this board the 25G complex (group E) is board-
 * cfg SKIP / powered-down, so the 25G indirect reg model external-aborts on the
 * first access. al_boot preboot already muxes + powers + clock-routes + KR/10G-
 * configures group D (al_serdes_init_cores @ PBS 0xfd8a8000) before our U-Boot
 * runs, so we RETARGET to group D and read status via the HSSP reg model - no
 * firmware blob, no heavy re-init needed.
 * Group base = complex base + offset; groups {A,B,C,D,E} = {0,0x400,0x800,0xc00,
 * 0x2000}. HSSP regs: gen.version@0x0, gen.reg_addr@0x10, gen.reg_data@0x14. */
#define AL_HSSP_GRP_D_OFF	0xc00		/* serdes-grp 3 (GRP_D) */
#define AL_HSSP_GEN_VERSION	0x00
#define AL_HSSP_GEN_REG_ADDR	0x10
#define AL_HSSP_GEN_REG_DATA	0x14
/* SRDS_CORE_REG_ADDR(page,type,offset) = (page<<13)|(type<<12)|offset */
#define AL_HSSP_ADDR(page, type, off)	(((page) << 13) | ((type) << 12) | (off))
#define AL_HSSP_TYPE_PMA	0
#define AL_HSSP_RXRANDET_REG	41	/* PMA reg 41: RX random-data detect */
#define AL_HSSP_RXRANDET_STAT	0x20	/* bit 5 set = signal detected */

/* Group-D base (SFP+ lane) from the DT complex "serdes" base. */
static void __iomem *al_hssp_grp_d_base(void)
{
	void __iomem *sbase = al_serdes_dt_base("serdes");	/* complex base */

	return sbase ? sbase + AL_HSSP_GRP_D_OFF : NULL;
}

/* Indirect HSSP register read: lane page, PMA/PCS type, 8-bit reg number. */
static u8 al_hssp_reg_read(void __iomem *grp, unsigned page, unsigned type,
			   u16 off)
{
	writel(AL_HSSP_ADDR(page, type, off), grp + AL_HSSP_GEN_REG_ADDR);
	return (u8)readl(grp + AL_HSSP_GEN_REG_DATA);
}

/*
 * Build a per-call HAL object bound to the HSSP group-D base (complex base +
 * 0xc00 - see al_hssp_grp_d_base()). No HW side effects.
 *
 * Uses al_serdes_hssp_handle_init(), NOT al_serdes_25g_handle_init(): group D
 * is an HSSP group (see the block comment above al_hssp_grp_d_base()), and
 * al_serdes_hssp_handle_init() takes the GROUP base directly (obj->regs_base
 * = serdes_regs_base, no further offset applied) - matching how Linux's
 * alpine_serdes_grp_objs_init() calls al_serdes_handle_grp_init() with
 * alpine_serdes_resource_get(group) = complex_base + serdes_grp_offset[group]
 * (modules/al_eth/alpine_serdes.c). The 25G handle_init is for the 25G-complex
 * group (E on this board, powered-down/SKIP) and is the wrong model here.
 */
static int al_serdes_obj_get(struct al_serdes_grp_obj *obj, void __iomem **base)
{
	void __iomem *grp = al_hssp_grp_d_base();

	if (!grp) {
		printf("serdes: no '%s' DT node / 'serdes' reg\n",
		       AL_SERDES_DT_COMPAT);
		return -ENODEV;
	}
	al_serdes_hssp_handle_init(grp, obj);
	if (base)
		*base = grp;
	return 0;
}

/*
 * 10GBASE-R PCS bring-up (optional). Selects the 10G-serial PCS via the MAC
 * adapter's pcs_10g_ll_cfg. HW: the MAC agent owns the full adapter datapath +
 * reset ordering; this is the minimal PCS-mode write for standalone bring-up
 * and must be reconciled with the MAC config at merge / verified on the box.
 */
static void al_serdes_pcs_10gr_config(void __iomem *pcs)
{
	writel(AL_PCS_10G_LL_CFG_10GR, pcs + AL_PCS_10G_LL_CFG);
	/* PCS reset settle. 1 us: al_hal_eth_main AL_ETH_KR_PCS_RESET_DELAY. */
	udelay(1);
}

/* Indirect Clause-45 read of a KR-PCS register through the MAC adapter. */
static u16 al_serdes_kr_pcs_read(void __iomem *pcs, u16 reg)
{
	writel(reg, pcs + AL_PCS_KR_ADDR);
	return (u16)readl(pcs + AL_PCS_KR_DATA);
}

int al_serdes_10g_init(void)
{
	struct al_serdes_grp_obj obj;
	void __iomem *base;
	void __iomem *pcs;
	int rc;

	void __iomem *grp = al_hssp_grp_d_base();
	u32 version;
	u8 rxdet;

	if (!grp) {
		printf("serdes: no '%s' DT node / 'serdes' reg\n",
		       AL_SERDES_DT_COMPAT);
		return -ENODEV;
	}

	/*
	 * Go/no-go: read HSSP group-D gen.version. On the OLD (wrong) 25G target
	 * this first read external-aborts (SError -> box reset). On group D - which
	 * al_boot preboot already powered/clocked/KR-configured - it returns the
	 * SerDes revision. A plausible value here == the retarget is correct.
	 */
	version = readl(grp + AL_HSSP_GEN_VERSION);
	printf("serdes: HSSP group D (SFP+ lane %d) @ %p, gen.version=0x%08x\n",
	       AL_SFP_LANE, grp, version);

	/*
	 * preboot already ran al_serdes_hssp_group_cfg_eth_kr_mode(156MHz) on this
	 * group, so the lane is in fixed-10G KR. We do NOT re-init here - just read
	 * the PMA RX-signal-detect for the SFP+ lane via the HSSP indirect model.
	 */
	rxdet = al_hssp_reg_read(grp, AL_SFP_LANE, AL_HSSP_TYPE_PMA,
				 AL_HSSP_RXRANDET_REG);
	printf("serdes: lane %d signal_detect=%s (pma rxrandet=0x%02x)\n",
	       AL_SFP_LANE, (rxdet & AL_HSSP_RXRANDET_STAT) ? "yes" : "no", rxdet);

	/*
	 * Apply the 10G-optic TX/RX EQ (#111). Real HSSP HAL vtable call - the
	 * same path Linux's al_eth_serdes_static_tx/rx_params_set() uses
	 * (al_init_eth_lm.c), now that al_hal_serdes_hssp.c is ported here.
	 * HW: unverified - vendor 10G_OPTIC defaults, not yet confirmed to
	 * help (or not hurt) link training on this box.
	 */
	rc = al_serdes_obj_get(&obj, &base);
	if (rc) {
		printf("serdes: HSSP obj_get failed (%d); EQ params NOT applied\n", rc);
	} else {
		obj.tx_advanced_params_set(&obj, AL_SFP_LANE, &optic_tx_params);
		obj.rx_advanced_params_set(&obj, AL_SFP_LANE, &optic_rx_params);
		printf("serdes: lane %d TX/RX optic EQ applied (HSSP group D)\n",
		       AL_SFP_LANE);
	}

	/*
	 * PCS is DEFERRED to the MAC driver. al_eth_dm_10g configures the 10GBASE-R
	 * PCS as part of the MAC ("Configured MAC to KR mode", 10GbE_Serial path in
	 * al_hal_eth_mac_v1_v2). Poking the "pcs" MAC-adapter window directly here
	 * (base 0xfe120000, unconfirmed) external-aborts on read (0xfe120a04) and is
	 * redundant with the MAC path - so the serdes driver no longer touches it.
	 */
	(void)pcs;
	printf("serdes: 10G init done (HSSP group D); PCS owned by the MAC driver\n");
	return 0;
}

void al_serdes_10g_status(void)
{
	void __iomem *grp = al_hssp_grp_d_base();
	u8 rxdet;

	if (!grp) {
		printf("serdes: no 'serdes' DT base\n");
		return;
	}

	printf("serdes: HSSP group D @ %p, SFP+ lane %d:\n", grp, AL_SFP_LANE);
	printf("  gen.version  : 0x%08x\n", readl(grp + AL_HSSP_GEN_VERSION));

	rxdet = al_hssp_reg_read(grp, AL_SFP_LANE, AL_HSSP_TYPE_PMA,
				 AL_HSSP_RXRANDET_REG);
	printf("  signal_detect: %s (pma rxrandet=0x%02x)\n",
	       (rxdet & AL_HSSP_RXRANDET_STAT) ? "yes" : "no", rxdet);

	/* PCS block-lock / FEC is read by the MAC driver, not here - the "pcs"
	 * MAC-adapter window aborts on direct read (0xfe120a04). See al_eth_dm_10g. */
}

static int do_serdes(struct cmd_tbl *cmdtp, int flag, int argc,
		     char *const argv[])
{
	const char *sub = (argc > 1) ? argv[1] : "all";

	if (!strcmp(sub, "init"))
		return al_serdes_10g_init() ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
	if (!strcmp(sub, "status")) {
		al_serdes_10g_status();
		return CMD_RET_SUCCESS;
	}
	if (!strcmp(sub, "all")) {
		int rc = al_serdes_10g_init();

		al_serdes_10g_status();
		return rc ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
	}
	return CMD_RET_USAGE;
}

U_BOOT_CMD(serdes, 2, 0, do_serdes,
	   "bring up the SFP+ 25G-SerDes lane at fixed 10G + read lane status",
	   "init    - configure the lane for fixed 10GBASE-R (KR/AN/LT off)\n"
	   "serdes status  - read PLL/signal/CDR/rx-valid + PCS block-lock\n"
	   "serdes         - init then status");
