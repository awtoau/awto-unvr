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
 * Scope: the SFP+ lane only - HSSP group D status + the optic TX/RX EQ (#111).
 * The PCS and MAC belong to al_eth_dm_10g.c. Electrical mode is the HAL's "KR"
 * = 10.3125G NRZ; Clause-73 AN / Clause-72 LT live in the MAC KR FSM and are
 * deliberately never invoked, so the lane comes up at a fixed rate.
 *
 * The group-D retarget is hardware-verified (it is what stopped the SError,
 * commit eac280a). The EQ tap VALUES are not - see optic_tx_params below.
 * `eth diag 2` reads back what is actually in force.
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

/* DT is hardware-of-record (docs/hardware.md); the complex base comes from
 * reg-name "serdes" (fd8c0000, size 0x2400) via ofnode, never hardcoded. */
#define AL_SERDES_DT_COMPAT	"annapurna-labs,al-serdes-25g"

/* SFP+ lane: group D lane 0, per the DT's serdes-grp 3 / serdes-lane 0. */
#define AL_SFP_LANE		AL_SRDS_LANE_0

/* 10G optic TX/RX equaliser params - al_init_eth_lm.c's 10G_OPTIC statics,
 * applied via the HSSP vtable in al_serdes_10g_init() (#111).
 * c_plus_1 is 0x5, not the vendor's 0x2: the vendor "optic" table is really the
 * direct-attach copper one and under-equalises a real optic (#121, #207).
 * Chosen under Linux; untested in U-Boot - there is no throughput path here. */
static struct al_serdes_adv_tx_params optic_tx_params = {
	.override		= AL_TRUE,
	.amp			= 0x1,
	.total_driver_units	= 0x13,
	.c_plus_1		= 0x5,
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

/* ---- HSSP SerDes group D (the SFP+ 10G lane) ------------------------------
 * The SFP+ 10G is on the HSSP complex, group D - NOT the 25G complex, whose
 * group E is board-cfg SKIP / powered down here and external-aborts on first
 * access (that was the 10G SError). preboot already muxes/powers/clocks and
 * KR-configures group D, so we only read status; no re-init needed.
 * Group base = complex + {A,B,C,D,E} = {0,0x400,0x800,0xc00,0x2000}. */
#define AL_HSSP_GRP_D_OFF	0xc00		/* serdes-grp 3 (GRP_D) */
#define AL_HSSP_RXRANDET_REG	41	/* PMA reg 41: RX random-data detect */
#define AL_HSSP_RXRANDET_STAT	0x20	/* bit 5 set = signal detected */

/* gen.version / gen.reg_addr / gen.reg_data come from struct al_serdes_regs,
 * NOT hand-rolled offsets: gen is at [0x100], so the old 0x00/0x10/0x14 read
 * the reserved window (#110). Compiler-computed, so it cannot drift again. */
#define AL_HSSP_GEN(grp)	(&((struct al_serdes_regs __iomem *)(grp))->gen)

/* Group-D base (SFP+ lane) from the DT complex "serdes" base. */
static void __iomem *al_hssp_grp_d_base(void)
{
	void __iomem *sbase = al_serdes_dt_base("serdes");	/* complex base */

	return sbase ? sbase + AL_HSSP_GRP_D_OFF : NULL;
}

/* Indirect HSSP register read via the HAL's own vtable - it owns the addr/data
 * sequencing, so no address arithmetic is duplicated here.
 * The HAL has no settling wait between the addr write and the data read: a
 * failed read returns 0xFF, indistinguishable from a real value (#239/#228). */
static u8 al_hssp_reg_read(struct al_serdes_grp_obj *obj, unsigned page,
			   enum al_serdes_reg_type type, u16 off)
{
	uint8_t data = 0xff;

	if (obj->reg_read)
		obj->reg_read(obj, (enum al_serdes_reg_page)page, type, off,
			      &data);
	return data;
}

/* Per-call HAL object bound to the group-D base. No HW side effects.
 * al_serdes_hssp_handle_init(), NOT the 25G one: it takes the GROUP base
 * directly (no further offset), matching Linux's alpine_serdes_grp_objs_init().
 * The 25G handle_init models the 25G complex and is the wrong model here. */
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

int al_serdes_10g_init(void)
{
	struct al_serdes_grp_obj obj;
	void __iomem *grp;
	u8 rxdet;
	int rc;

	rc = al_serdes_obj_get(&obj, &grp);
	if (rc)
		return rc;

	/* gen.version doubles as the go/no-go: a plausible value means the group-D
	 * base is right (the wrong 25G target external-aborts on this first read). */
	printf("serdes: HSSP group D (SFP+ lane %d) @ %p, gen.version=0x%08x\n",
	       AL_SFP_LANE, grp, readl(&AL_HSSP_GEN(grp)->version));

	/* preboot already ran group_cfg_eth_kr_mode(156MHz) here, so the lane is
	 * in fixed-10G KR - we do not re-init, only read PMA signal-detect. */
	rxdet = al_hssp_reg_read(&obj, AL_SFP_LANE, AL_SRDS_REG_TYPE_PMA,
				 AL_HSSP_RXRANDET_REG);
	printf("serdes: lane %d signal_detect=%s (pma rxrandet=0x%02x)\n",
	       AL_SFP_LANE, (rxdet & AL_HSSP_RXRANDET_STAT) ? "yes" : "no", rxdet);

	/* Apply the 10G-optic TX/RX EQ (#111) - the same HSSP vtable path Linux's
	 * al_eth_serdes_static_tx/rx_params_set() uses. Whether these tap VALUES
	 * help this board's optic is still open (#207); `eth diag 2` reads back
	 * what actually landed. */
	obj.tx_advanced_params_set(&obj, AL_SFP_LANE, &optic_tx_params);
	obj.rx_advanced_params_set(&obj, AL_SFP_LANE, &optic_rx_params);
	printf("serdes: lane %d TX/RX optic EQ applied (HSSP group D)\n",
	       AL_SFP_LANE);

	/* PCS is the MAC driver's: al_eth_dm_10g's al_eth_mac_config(10GbE_Serial)
	 * configures it. Poking the MAC-adapter window from here external-aborts
	 * and is redundant, so this driver does not touch it. */
	printf("serdes: 10G init done (HSSP group D); PCS owned by the MAC driver\n");
	return 0;
}

void al_serdes_10g_status(void)
{
	struct al_serdes_grp_obj obj;
	void __iomem *grp;
	u8 rxdet;

	if (al_serdes_obj_get(&obj, &grp))
		return;

	printf("serdes: HSSP group D @ %p, SFP+ lane %d:\n", grp, AL_SFP_LANE);
	printf("  gen.version  : 0x%08x\n", readl(&AL_HSSP_GEN(grp)->version));

	rxdet = al_hssp_reg_read(&obj, AL_SFP_LANE, AL_SRDS_REG_TYPE_PMA,
				 AL_HSSP_RXRANDET_REG);
	printf("  signal_detect: %s (pma rxrandet=0x%02x)\n",
	       (rxdet & AL_HSSP_RXRANDET_STAT) ? "yes" : "no", rxdet);

	/* PCS block-lock / FEC is read by the MAC driver, not here - the "pcs"
	 * MAC-adapter window aborts on direct read (0xfe120a04). See al_eth_dm_10g. */
}

/* ---- readback for `eth diag` (al_eth_diag.c, extern - no cross-dir -I) ----
 * The taps ACTUALLY in force, read back off the lane, not the static table we
 * meant to write - the two differ whenever the apply failed or Linux/LT moved
 * them. Read-only. */

int al_serdes_10g_tx_params_get(unsigned int lane, unsigned int *override,
				unsigned int *c_minus_1, unsigned int *c_plus_1,
				unsigned int *c_plus_2, unsigned int *tdu,
				unsigned int *amp)
{
	struct al_serdes_adv_tx_params tx;
	struct al_serdes_grp_obj obj;
	int rc;

	rc = al_serdes_obj_get(&obj, NULL);
	if (rc)
		return rc;
	if (!obj.tx_advanced_params_get)
		return -ENOSYS;

	memset(&tx, 0, sizeof(tx));
	obj.tx_advanced_params_get(&obj, (enum al_serdes_lane)lane, &tx);

	*override = tx.override;
	*c_minus_1 = tx.c_minus_1;
	*c_plus_1 = tx.c_plus_1;
	*c_plus_2 = tx.c_plus_2;
	*tdu = tx.total_driver_units;
	*amp = tx.amp;
	return 0;
}

int al_serdes_10g_lane_status_get(unsigned int lane, unsigned int *sig_det,
				  unsigned int *version)
{
	struct al_serdes_grp_obj obj;
	void __iomem *grp;
	u8 rxdet;
	int rc;

	rc = al_serdes_obj_get(&obj, &grp);
	if (rc)
		return rc;

	*version = readl(&AL_HSSP_GEN(grp)->version);
	rxdet = al_hssp_reg_read(&obj, lane, AL_SRDS_REG_TYPE_PMA,
				 AL_HSSP_RXRANDET_REG);
	*sig_det = !!(rxdet & AL_HSSP_RXRANDET_STAT);
	return 0;
}

/* TX equaliser fields settable from the prompt, so a tap can be swept without a
 * rebuild. Main cursor = total_driver_units - (c_minus_1 + c_plus_1 + c_plus_2),
 * so raising a tap costs amplitude - too high and the lane dies (#121). */
static const struct {
	const char	*name;
	uint8_t		*field;
} tx_fields[] = {
	{ "amp",	&optic_tx_params.amp },
	{ "tdu",	&optic_tx_params.total_driver_units },
	{ "c_plus_1",	&optic_tx_params.c_plus_1 },
	{ "c_plus_2",	&optic_tx_params.c_plus_2 },
	{ "c_minus_1",	&optic_tx_params.c_minus_1 },
	{ "slew",	&optic_tx_params.slew_rate },
};

static void serdes_tx_params_show(void)
{
	int i;

	printf("optic TX params (hex):");
	for (i = 0; i < ARRAY_SIZE(tx_fields); i++)
		printf(" %s=%x", tx_fields[i].name, *tx_fields[i].field);
	printf("\n  main cursor = %d of %d units\n",
	       optic_tx_params.total_driver_units -
		       (optic_tx_params.c_minus_1 + optic_tx_params.c_plus_1 +
			optic_tx_params.c_plus_2),
	       optic_tx_params.total_driver_units);
}

static int do_serdes_tx(int argc, char *const argv[])
{
	int i;

	if (argc < 4) {
		serdes_tx_params_show();
		return CMD_RET_SUCCESS;
	}

	for (i = 0; i < ARRAY_SIZE(tx_fields); i++) {
		if (strcmp(argv[2], tx_fields[i].name))
			continue;
		*tx_fields[i].field = (uint8_t)hextoul(argv[3], NULL);
		serdes_tx_params_show();
		/* Params are consumed during lane setup, so re-init to apply. */
		return al_serdes_10g_init() ? CMD_RET_FAILURE : CMD_RET_SUCCESS;
	}

	printf("unknown TX field '%s'\n", argv[2]);
	return CMD_RET_USAGE;
}

/* ---- board params (the U-Boot -> Linux SerDes/link policy transport) -----
 * Stock's eth_freeze_serdes_settings / eth_mac_mode_set / eth_retimer_config
 * (#198) all read/modify/write the same MAC scratchpad. The implementation
 * lives with the eth HAL (drivers/net/al_eth/al_eth_boardparams.c) and is
 * called by extern prototype - same convention al_eth_dm_10g.c uses for us.
 */
#if IS_ENABLED(CONFIG_AL_ETH)

#define AL_SERDES_BP_PORT	2		/* the SFP+ port */

extern int al_eth_bp_dump(int port);
extern int al_eth_bp_write(int port);
extern int al_eth_bp_freeze_set(int port, int enable);
extern int al_eth_bp_mac_mode_set(int port, unsigned int mode);
extern int al_eth_bp_retimer_set(int port, int exist, int bus_id, int i2c_addr,
				 int channel);

/* Optional trailing <port>; defaults to the SFP+ port. */
static int serdes_port_arg(int argc, char *const argv[], int idx)
{
	return (argc > idx) ? (int)dectoul(argv[idx], NULL) : AL_SERDES_BP_PORT;
}

static int do_serdes_boardparams(int argc, char *const argv[])
{
	if (argc > 2 && !strcmp(argv[2], "write"))
		return al_eth_bp_write(serdes_port_arg(argc, argv, 3)) ?
			CMD_RET_FAILURE : CMD_RET_SUCCESS;

	return al_eth_bp_dump(serdes_port_arg(argc, argv, 2)) ?
		CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

static int do_serdes_freeze(int argc, char *const argv[])
{
	int enable;

	if (argc < 3)
		return CMD_RET_USAGE;
	if (!strcmp(argv[2], "enable"))
		enable = 1;
	else if (!strcmp(argv[2], "disable"))
		enable = 0;
	else
		return CMD_RET_USAGE;

	return al_eth_bp_freeze_set(serdes_port_arg(argc, argv, 3), enable) ?
		CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

static int do_serdes_macmode(int argc, char *const argv[])
{
	if (argc < 3)
		return CMD_RET_USAGE;

	return al_eth_bp_mac_mode_set(serdes_port_arg(argc, argv, 3),
				      (unsigned int)dectoul(argv[2], NULL)) ?
		CMD_RET_FAILURE : CMD_RET_SUCCESS;
}

static int do_serdes_retimer(int argc, char *const argv[])
{
	int port = AL_SERDES_BP_PORT;
	int exist = -1, bus = -1, addr = -1, ch = -1;
	int i;

	for (i = 2; i + 1 < argc; i += 2) {
		const char *k = argv[i], *v = argv[i + 1];

		if (!strcmp(k, "--port"))
			port = (int)dectoul(v, NULL);
		else if (!strcmp(k, "--exist"))
			exist = (int)dectoul(v, NULL);
		else if (!strcmp(k, "--bus-id"))
			bus = (int)dectoul(v, NULL);
		else if (!strcmp(k, "--i2c-addr"))
			addr = (int)simple_strtoul(v, NULL, 0);
		else if (!strcmp(k, "--channel") && (*v == 'A' || *v == 'B'))
			ch = *v - 'A';
		else
			return CMD_RET_USAGE;
	}
	if (i != argc)
		return CMD_RET_USAGE;

	return al_eth_bp_retimer_set(port, exist, bus, addr, ch) ?
		CMD_RET_FAILURE : CMD_RET_SUCCESS;
}
#endif /* CONFIG_AL_ETH */

static int do_serdes(struct cmd_tbl *cmdtp, int flag, int argc,
		     char *const argv[])
{
	const char *sub = (argc > 1) ? argv[1] : "all";

	if (!strcmp(sub, "tx"))
		return do_serdes_tx(argc, argv);
#if IS_ENABLED(CONFIG_AL_ETH)
	if (!strcmp(sub, "boardparams"))
		return do_serdes_boardparams(argc, argv);
	if (!strcmp(sub, "freeze"))
		return do_serdes_freeze(argc, argv);
	if (!strcmp(sub, "macmode"))
		return do_serdes_macmode(argc, argv);
	if (!strcmp(sub, "retimer"))
		return do_serdes_retimer(argc, argv);
#endif
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

U_BOOT_CMD(serdes, 12, 0, do_serdes,
	   "SFP+ SerDes lane bring-up, status, and the al_eth board params",
	   "init    - configure the lane for fixed 10GBASE-R (KR/AN/LT off)\n"
	   "serdes status  - group-D version + PMA signal-detect. PCS block-lock\n"
	   "                 and the taps in force are in `eth diag 2`.\n"
	   "serdes tx      - show the optic TX equaliser params\n"
	   "serdes tx <field> <hex>  - set one and re-init the lane\n"
	   "                 fields: amp tdu c_plus_1 c_plus_2 c_minus_1 slew\n"
	   "serdes         - init then status\n"
	   "\n"
	   "board params (MAC scratchpad - what Linux al_eth reads at probe).\n"
	   "<port> is optional and defaults to 2; 1 = 1G RJ45, 2 = 10G SFP+.\n"
	   "serdes boardparams [<port>]        - decode + print all three regs\n"
	   "serdes boardparams write [<port>]  - rebuild from the DT board-cfg\n"
	   "serdes freeze <enable|disable> [<port>]\n"
	   "                 dont_override_serdes: enable = Linux keeps this\n"
	   "                 bootloader's SerDes settings. Refused in auto-speed\n"
	   "                 mode (mode 5) - set a fixed mode first.\n"
	   "serdes macmode <0-5> [<port>]\n"
	   "                 0 auto-detect, 1 RGMII, 2 10G-serial, 3 SGMII,\n"
	   "                 4 1000BASE-X, 5 auto-detect-auto-speed\n"
	   "serdes retimer [--port <n>] [--exist <0|1>] [--bus-id <n>]\n"
	   "               [--i2c-addr <n>] [--channel <A|B>]\n"
	   "                 NO retimer is fitted on this board (#202: 0x56 NAKs\n"
	   "                 on all five i2c buses) - here for other boards.");
