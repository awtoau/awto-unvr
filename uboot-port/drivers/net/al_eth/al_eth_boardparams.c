// SPDX-License-Identifier: GPL-2.0-or-later
/* al_eth board-params (MAC scratchpad) - the transport U-Boot uses to hand
 * SerDes/link policy to Linux's al_eth.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * Two jobs:
 * - SEED: build each port's params from /soc/board-cfg/ethernet/port<n> and
 *   write them at probe. Without this Linux al_eth fails probe outright
 *   ("board info not available"); today only stock U-Boot writes them, so a
 *   boot that does not chainload stock has no ethernet.
 * - EDIT: parity with stock's eth_freeze_serdes_settings / eth_mac_mode_set /
 *   eth_retimer_config (reversed in #198), driven from the `serdes` command.
 *
 * Bit layout is NOT re-derived here - al_eth_board_params_get/set()
 * (hal/eth/al_hal_eth_main.c) own it. The DT binding mirrors stock's own
 * board-cfg node names 1:1 so the two are diffable.
 *
 * Scratch regs (rev 1-3): reg1 = mac_1g.scratch, reg2 = mac_10g.scratch,
 * reg3 = mac_1g.mac_0. reg1 == 0 means "never written": al_eth_board_params_get()
 * returns -ENOENT and Linux's probe fails.
 */

#include <dm.h>
#include <errno.h>
#include <stdio.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <pci.h>
#include <vsprintf.h>
#include <dm/ofnode.h>
#include <asm/io.h>

#include <al_hal_eth.h>
#include <al_hal_eth_mac_regs.h>

#include "al_eth_boardparams.h"
#include "al_eth_port.h"

#define AL_ETH_BP_DT_PORT_FMT	"/soc/board-cfg/ethernet/port%d"

/* SB clock straps (board/annapurna/alpine/alpine.c) -> params.ref_clk_freq. */
u32 al_bootstrap_sb_clk_get(void);

static const char *const media_names[] = {
	"auto-detect", "RGMII", "10GBASE-SR (10G serial)", "SGMII",
	"1000BASE-X", "auto-detect-auto-speed", "SGMII-2.5G", "NBASE-T",
	"25G", "25G/10G-auto", "40G", "100G",
};

/* board-cfg "mode" string -> media_type, and whether it implies an SFP cage.
 * Mirrors dt_based_init_eth_params_port() in stock's board.c. */
static const struct {
	const char			*mode;
	enum al_eth_board_media_type	media;
	al_bool				sfp;
} bp_modes[] = {
	{ "rgmii",			AL_ETH_BOARD_MEDIA_TYPE_RGMII,			AL_FALSE },
	{ "sgmii",			AL_ETH_BOARD_MEDIA_TYPE_SGMII,			AL_FALSE },
	{ "sgmii-2.5g",			AL_ETH_BOARD_MEDIA_TYPE_SGMII_2_5G,		AL_FALSE },
	{ "10g-serial",			AL_ETH_BOARD_MEDIA_TYPE_10GBASE_SR,		AL_FALSE },
	{ "auto-detect-auto-speed",	AL_ETH_BOARD_MEDIA_TYPE_AUTO_DETECT_AUTO_SPEED,	AL_TRUE  },
	{ "auto-detect",		AL_ETH_BOARD_MEDIA_TYPE_AUTO_DETECT,		AL_TRUE  },
	{ "nbase-t",			AL_ETH_BOARD_MEDIA_TYPE_NBASE_T,		AL_FALSE },
	{ "25g",			AL_ETH_BOARD_MEDIA_TYPE_25G,			AL_FALSE },
	{ "25g-10g-auto-detect",	AL_ETH_BOARD_MEDIA_TYPE_25G_10G_AUTO,		AL_FALSE },
	{ "40g",			AL_ETH_BOARD_MEDIA_TYPE_40G,			AL_FALSE },
};

static const char *media_name(unsigned int t)
{
	return (t < ARRAY_SIZE(media_names)) ? media_names[t] : "?";
}

static const char *yn(al_bool v)
{
	return v ? "yes" : "no";
}

/* ---- DT (board-cfg) ----------------------------------------------------- */

static al_bool bp_prop_is(ofnode node, const char *name, const char *want)
{
	const char *s = ofnode_read_string(node, name);

	return (s && !strcmp(s, want)) ? AL_TRUE : AL_FALSE;
}

static ofnode bp_port_node(int port)
{
	char path[48];

	snprintf(path, sizeof(path), AL_ETH_BP_DT_PORT_FMT, port);
	return ofnode_path(path);
}

static void bp_parse_10g_serial(ofnode port_node, struct al_eth_board_params *p)
{
	ofnode n = ofnode_find_subnode(port_node, "10g-serial");
	u32 v;

	if (!ofnode_valid(n))
		return;

	p->dac = bp_prop_is(n, "dac", "enabled");
	/* Stock defaults an absent/out-of-range dac-length to 3. */
	p->dac_len = (!ofnode_read_u32(n, "dac-length", &v) && v > 0 && v < 16) ? v : 3;
	p->autoneg_enable = bp_prop_is(n, "auto-neg", "enabled");
	p->kr_lt_enable = bp_prop_is(n, "link-training", "enabled");
	p->kr_fec_enable = bp_prop_is(n, "fec", "enabled");
	p->auto_fec_enable = bp_prop_is(n, "auto-fec", "enabled");
	p->force_1000_base_x = bp_prop_is(n, "force-1000base-x", "enabled");
}

static void bp_parse_ext_phy(ofnode port_node, struct al_eth_board_params *p)
{
	ofnode n = ofnode_find_subnode(port_node, "ext_phy");
	u32 v;

	if (!ofnode_valid(n))
		return;

	p->phy_exist = AL_TRUE;
	if (ofnode_read_u32(n, "phy-addr", &v))
		return;			/* no phy-addr: stock leaves the rest default */
	p->phy_mdio_addr = v;

	if (bp_prop_is(n, "phy_mgmt_if", "xmdc-xmdio"))
		p->phy_if = AL_ETH_BOARD_PHY_IF_XMDIO;
	else if (bp_prop_is(n, "phy_mgmt_if", "i2c"))
		p->phy_if = AL_ETH_BOARD_PHY_IF_I2C;
	else
		p->phy_if = AL_ETH_BOARD_PHY_IF_MDIO;

	p->mdio_freq = bp_prop_is(n, "mdc-mdio-freq", "2.5Mhz") ?
		AL_ETH_BOARD_MDIO_FREQ_2_5_MHZ : AL_ETH_BOARD_MDIO_FREQ_1_MHZ;

	p->an_mode = bp_prop_is(n, "auto-neg-mode", "in-band") ?
		AL_ETH_BOARD_AUTONEG_IN_BAND : AL_ETH_BOARD_AUTONEG_OUT_OF_BAND;
}

static void bp_parse_1g_serial(ofnode port_node, struct al_eth_board_params *p)
{
	ofnode n = ofnode_find_subnode(port_node, "1g-serial");

	if (!ofnode_valid(n))
		return;

	p->an_disable = bp_prop_is(n, "auto-neg", "disabled");
	if (bp_prop_is(n, "speed", "100M"))
		p->speed = AL_ETH_BOARD_1G_SPEED_100M;
	else if (bp_prop_is(n, "speed", "10M"))
		p->speed = AL_ETH_BOARD_1G_SPEED_10M;
	else
		p->speed = AL_ETH_BOARD_1G_SPEED_1000M;
	p->half_duplex = bp_prop_is(n, "duplex", "half");
	p->force_1000_base_x = bp_prop_is(n, "force-1000base-x", "enabled");
}

static void bp_parse_retimer(ofnode port_node, struct al_eth_board_params *p)
{
	ofnode n = ofnode_find_subnode(port_node, "retimer");
	const char *ch;
	u32 v;

	if (!ofnode_valid(n) || !bp_prop_is(n, "exist", "enabled"))
		return;

	p->retimer_exist = AL_TRUE;
	if (bp_prop_is(n, "type", "br410"))
		p->retimer_type = AL_ETH_RETIMER_BR_410;
	else if (bp_prop_is(n, "type", "ds25"))
		p->retimer_type = AL_ETH_RETIMER_DS_25;
	else
		p->retimer_type = AL_ETH_RETIMER_BR_210;

	if (!ofnode_read_u32(n, "i2c-bus", &v))
		p->retimer_bus_id = v;
	if (!ofnode_read_u32(n, "i2c-addr", &v))
		p->retimer_i2c_addr = v;

	ch = ofnode_read_string(n, "channel");
	if (ch && ch[0] >= 'A' && ch[0] <= 'H')
		p->retimer_channel = ch[0] - 'A';
}

/* SB clock -> ref_clk_freq, same mapping stock uses. 428 MHz has no board-param
 * encoding in this HAL, so it reports as 375 like stock. */
static enum al_eth_ref_clk_freq bp_ref_clk_freq(void)
{
	u32 sb_clk = al_bootstrap_sb_clk_get();

	switch (sb_clk) {
	case 250000000:
		return AL_ETH_REF_FREQ_250_MHZ;
	case 500000000:
		return AL_ETH_REF_FREQ_500_MHZ;
	case 375000000:
		return AL_ETH_REF_FREQ_375_MHZ;
	default:
		printf("al_eth: unexpected SB clock %u Hz - board params say 375 MHz\n",
		       sb_clk);
		return AL_ETH_REF_FREQ_375_MHZ;
	}
}

/* Build one port's params from board-cfg. Returns 0 if the port is described
 * and enabled, -ENOENT / -EINVAL otherwise (nothing is written then). */
static int bp_from_dt(int port, struct al_eth_board_params *p)
{
	ofnode node = bp_port_node(port);
	const char *mode;
	unsigned int i;
	u32 v;

	memset(p, 0, sizeof(*p));
	p->ref_clk_freq = bp_ref_clk_freq();

	if (!ofnode_valid(node)) {
		printf("al_eth: no " AL_ETH_BP_DT_PORT_FMT " in the DT - board params\n",
		       port);
		printf("        NOT written; Linux al_eth would fail probe.\n");
		return -ENOENT;
	}

	if (!bp_prop_is(node, "status", "enabled"))
		return -ENOENT;

	mode = ofnode_read_string(node, "mode");
	for (i = 0; mode && i < ARRAY_SIZE(bp_modes); i++) {
		if (strcmp(mode, bp_modes[i].mode))
			continue;
		p->media_type = bp_modes[i].media;
		p->sfp_plus_module_exist = bp_modes[i].sfp;
		break;
	}
	if (!mode || i == ARRAY_SIZE(bp_modes)) {
		printf("al_eth: port %d board-cfg mode '%s' unknown\n", port,
		       mode ? mode : "<missing>");
		return -EINVAL;
	}

	/* RGMII has no SerDes/10G section - stock skips all of it. */
	if (p->media_type != AL_ETH_BOARD_MEDIA_TYPE_RGMII) {
		p->serdes_grp = ofnode_read_u32(node, "serdes-grp", &v) ? 3 : v;
		p->serdes_lane = ofnode_read_u32(node, "serdes-lane", &v) ? 3 - port : v;
		if (!ofnode_read_u32(node, "gpio-sfp-present", &v))
			p->gpio_sfp_present = v;
		bp_parse_10g_serial(node, p);
	}

	bp_parse_ext_phy(node, p);
	bp_parse_1g_serial(node, p);
	bp_parse_retimer(node, p);

	if (!ofnode_read_u32(node, "i2c-id", &v))
		p->i2c_adapter_id = v;

	p->dont_override_serdes = bp_prop_is(node, "freeze-serdes-params", "enable");

	return 0;
}

/* ---- register access ---------------------------------------------------- */

static void bp_raw_read(void __iomem *mac, u32 raw[3])
{
	struct al_eth_mac_regs __iomem *m = (struct al_eth_mac_regs __iomem *)mac;

	raw[0] = readl(&m->mac_1g.scratch);
	raw[1] = readl(&m->mac_10g.scratch);
	raw[2] = readl(&m->mac_1g.mac_0);
}

static void __iomem *al_eth_bp_mac_regs(int port)
{
	struct al_eth_port_regs r;

	return al_eth_port_regs_get(port, &r) ? NULL : r.mac;
}

/* Fetch params for a read/modify/write. Explains -ENOENT rather than just
 * returning it: an unwritten reg1 is the most common failure here. */
static int al_eth_bp_load(int port, void __iomem **mac_out,
			  struct al_eth_board_params *params)
{
	void __iomem *mac = al_eth_bp_mac_regs(port);
	int rc;

	if (!mac)
		return -ENODEV;

	rc = al_eth_board_params_get(mac, params);
	if (rc == -ENOENT) {
		printf("al_eth: port %d board params NOT set (mac_1g.scratch == 0).\n",
		       port);
		printf("        Nothing to modify. Write them first: `serdes boardparams write %d`\n",
		       port);
		return rc;
	}
	if (rc) {
		printf("al_eth: port %d board params load failed (%d)\n", port, rc);
		return rc;
	}

	*mac_out = mac;
	return 0;
}

static int al_eth_bp_store(int port, void __iomem *mac,
			   struct al_eth_board_params *params)
{
	int rc = al_eth_board_params_set(mac, params);

	if (rc)
		printf("al_eth: port %d board params store failed (%d)\n", port, rc);
	return rc;
}

/* ---- seed / write ------------------------------------------------------- */

int al_eth_bp_seed(int port, void __iomem *mac_regs)
{
	struct al_eth_board_params p;
	u32 before[3], after[3];
	int rc;

	if (!mac_regs)
		return -ENODEV;

	rc = bp_from_dt(port, &p);
	if (rc)
		return rc;

	bp_raw_read(mac_regs, before);
	rc = al_eth_bp_store(port, mac_regs, &p);
	if (rc)
		return rc;
	bp_raw_read(mac_regs, after);

	/* reg1 is the only validity marker Linux has. A zero here is fatal
	 * downstream, so it must be loud on the console, not a debug line. */
	if (!after[0]) {
		printf("al_eth: port %d ERROR board params reg1 wrote as 0 - Linux\n",
		       port);
		printf("        al_eth will fail probe (\"board info not available\")\n");
		return -EIO;
	}

	printf("al_eth: port %d board params 0x%08x 0x%08x 0x%08x (media %u, %s)\n",
	       port, after[0], after[1], after[2], p.media_type,
	       media_name(p.media_type));

	/* A chainload arrives with stock's values already in place; a difference
	 * is the parity check for what we build from the DT. */
	if ((before[0] || before[1] || before[2]) &&
	    (before[0] != after[0] || before[1] != after[1] || before[2] != after[2]))
		printf("al_eth: port %d overwrote 0x%08x 0x%08x 0x%08x (was already set)\n",
		       port, before[0], before[1], before[2]);

	return 0;
}

int al_eth_bp_write(int port)
{
	void __iomem *mac = al_eth_bp_mac_regs(port);

	return mac ? al_eth_bp_seed(port, mac) : -ENODEV;
}

/* ---- dump / edit -------------------------------------------------------- */

int al_eth_bp_dump(int port)
{
	struct al_eth_board_params p;
	void __iomem *mac;
	u32 raw[3];
	int rc;

	mac = al_eth_bp_mac_regs(port);
	if (!mac)
		return -ENODEV;
	bp_raw_read(mac, raw);

	printf("al_eth port %d (%s), MAC BAR2 @ %p\n", port,
	       al_eth_port_desc(port), mac);
	printf("  reg1 mac_1g.scratch  : 0x%08x%s\n", raw[0],
	       raw[0] ? "" : "   <-- UNSET");
	printf("  reg2 mac_10g.scratch : 0x%08x\n", raw[1]);
	printf("  reg3 mac_1g.mac_0    : 0x%08x\n", raw[2]);

	rc = al_eth_board_params_get(mac, &p);
	if (rc == -ENOENT) {
		printf("  decode: none - reg1 is the validity marker and reads 0.\n");
		printf("          Linux al_eth fails probe: \"board info not available\".\n");
		printf("          Write them: `serdes boardparams write %d`\n", port);
		return rc;
	}
	if (rc) {
		printf("  decode: failed (%d)\n", rc);
		return rc;
	}

	printf("  media_type           : %u (%s)\n", p.media_type,
	       media_name(p.media_type));
	printf("  phy_exist            : %s (mdio_addr %u, if %u, mdio_freq %s)\n",
	       yn(p.phy_exist), p.phy_mdio_addr, p.phy_if,
	       p.mdio_freq ? "1MHz" : "2.5MHz");
	printf("  sfp_plus_module_exist: %s\n", yn(p.sfp_plus_module_exist));
	printf("  i2c_adapter_id       : %u\n", p.i2c_adapter_id);
	printf("  autoneg_enable       : %s (an_mode %s)\n", yn(p.autoneg_enable),
	       p.an_mode ? "in-band" : "out-of-band");
	printf("  kr_lt_enable         : %s\n", yn(p.kr_lt_enable));
	printf("  kr_fec_enable        : %s (auto_fec %s)\n", yn(p.kr_fec_enable),
	       yn(p.auto_fec_enable));
	printf("  serdes_grp / lane    : %u / %u\n", p.serdes_grp, p.serdes_lane);
	printf("  ref_clk_freq         : %u\n", p.ref_clk_freq);
	printf("  dont_override_serdes : %s   <-- `serdes freeze`\n",
	       yn(p.dont_override_serdes));
	printf("  force_1000_base_x    : %s\n", yn(p.force_1000_base_x));
	printf("  1G an_disable        : %s (speed %u, half_duplex %s, fc_disable %s)\n",
	       yn(p.an_disable), p.speed, yn(p.half_duplex), yn(p.fc_disable));
	printf("  retimer_exist        : %s (bus %u, i2c 0x%02x, rx-ch %u, tx-ch %u, type %u)\n",
	       yn(p.retimer_exist), p.retimer_bus_id, p.retimer_i2c_addr,
	       p.retimer_channel, p.retimer_tx_channel, p.retimer_type);
	printf("  dac                  : %s (len %u)\n", yn(p.dac), p.dac_len);
	printf("  gpio sfp_present     : %u (spd_1g_10g %u, spd_25g %u)\n",
	       p.gpio_sfp_present, p.gpio_spd_1g_10g, p.gpio_spd_25g);
	printf("  common_mode          : %u\n", p.common_mode);

	return 0;
}

int al_eth_bp_freeze_set(int port, int enable)
{
	struct al_eth_board_params p;
	void __iomem *mac;
	int rc;

	rc = al_eth_bp_load(port, &mac, &p);
	if (rc)
		return rc;

	/* Stock's guard: in auto-speed mode the link manager re-runs SerDes setup
	 * per detected speed, so a frozen setting is overwritten. */
	if (p.media_type == AL_ETH_BOARD_MEDIA_TYPE_AUTO_DETECT_AUTO_SPEED) {
		printf("al_eth: port %d is in auto-speed mode - freeze would not take\n",
		       port);
		printf("        effect: the link manager re-programs the SerDes on every\n");
		printf("        detected speed change. Pick a fixed mode first, e.g.\n");
		printf("        `serdes macmode 2 %d` (10G serial).\n", port);
		return -EINVAL;
	}

	if (p.kr_lt_enable)
		printf("al_eth: WARNING port %d has KR link training enabled - LT can\n"
		       "        change the SerDes settings after the freeze.\n", port);

	p.dont_override_serdes = enable ? AL_TRUE : AL_FALSE;

	rc = al_eth_bp_store(port, mac, &p);
	if (rc)
		return rc;

	printf("al_eth: port %d dont_override_serdes = %s\n", port,
	       enable ? "1 - Linux keeps this bootloader's SerDes settings" :
			"0 - Linux programs its own SerDes table");
	return 0;
}

int al_eth_bp_mac_mode_set(int port, unsigned int mode)
{
	struct al_eth_board_params p;
	void __iomem *mac;
	int rc;

	if (mode > AL_ETH_BOARD_MEDIA_TYPE_AUTO_DETECT_AUTO_SPEED) {
		printf("al_eth: mac mode %u out of range (0-5)\n", mode);
		return -EINVAL;
	}

	rc = al_eth_bp_load(port, &mac, &p);
	if (rc)
		return rc;

	p.media_type = mode;

	rc = al_eth_bp_store(port, mac, &p);
	if (rc)
		return rc;

	printf("al_eth: port %d media_type = %u (%s)\n", port, mode,
	       media_name(mode));
	return 0;
}

int al_eth_bp_retimer_set(int port, int exist, int bus_id, int i2c_addr,
			  int channel)
{
	struct al_eth_board_params p;
	void __iomem *mac;
	int rc;

	if (exist > 1 || bus_id > 0xf || i2c_addr > 0x7f || channel > 1) {
		printf("al_eth: retimer arg out of range (exist 0-1, bus-id 0-15,\n");
		printf("        i2c-addr 0x00-0x7f, channel A|B)\n");
		return -EINVAL;
	}

	rc = al_eth_bp_load(port, &mac, &p);
	if (rc)
		return rc;

	if (exist >= 0)
		p.retimer_exist = exist ? AL_TRUE : AL_FALSE;
	if (bus_id >= 0)
		p.retimer_bus_id = bus_id;
	if (i2c_addr >= 0)
		p.retimer_i2c_addr = i2c_addr;
	if (channel >= 0)
		p.retimer_channel = channel;

	rc = al_eth_bp_store(port, mac, &p);
	if (rc)
		return rc;

	printf("al_eth: port %d retimer exist=%s bus=%u i2c=0x%02x channel=%c\n",
	       port, yn(p.retimer_exist), p.retimer_bus_id, p.retimer_i2c_addr,
	       'A' + (p.retimer_channel & 1));
	return 0;
}
