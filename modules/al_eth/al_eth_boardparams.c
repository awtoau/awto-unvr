// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * al_eth_boardparams.c - board params: validate, dump, self-seed from DT (#221)
 *
 * Board params = MAC scratchpad regs (not fuses): undefined on cold boot,
 * kept across warm reset. Absent or invalid -> seed from the DT eth node
 * instead of failing probe. Never touches NOR/EEPROM.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 */

#include <linux/debugfs.h>
#include <linux/etherdevice.h>
#include <linux/of.h>
#include <linux/pci.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

#include "al_hal_eth.h"
#include "al_hal_eth_mac_regs.h"
#include "al_eth.h"

#define BP_MAX_ISSUES	12
#define BP_ISSUE_LEN	80
/* Same fixed 500 MHz SB clock the NIC_V2 branches assume; DT sbclk agrees. */
#define BP_SEED_REF_CLK	AL_ETH_REF_FREQ_500_MHZ

struct bp_issues {
	int	n;
	char	msg[BP_MAX_ISSUES][BP_ISSUE_LEN];
};

static __printf(2, 3) void bp_issue(struct bp_issues *is, const char *fmt, ...)
{
	va_list ap;

	if (is->n >= BP_MAX_ISSUES)
		return;
	va_start(ap, fmt);
	vsnprintf(is->msg[is->n++], BP_ISSUE_LEN, fmt, ap);
	va_end(ap);
}

static const char *const bp_media_names[] = {
	"auto-detect", "RGMII", "10GBASE-SR", "SGMII", "1000BASE-X",
	"auto-detect-auto-speed", "SGMII-2.5G", "NBASE-T", "25G",
	"25G/10G-auto", "40G",
};

static const char *bp_media_name(unsigned int t)
{
	return t < ARRAY_SIZE(bp_media_names) ? bp_media_names[t] : "?";
}

/* Field-level sanity of a decoded set; each failure names field + expected. */
static void bp_check(const struct al_eth_board_params *p, struct bp_issues *is)
{
	bool rgmii = p->media_type == AL_ETH_BOARD_MEDIA_TYPE_RGMII;

	if (p->media_type > AL_ETH_BOARD_MEDIA_TYPE_40G)
		bp_issue(is, "media_type %u: expected 0..%u", p->media_type,
			 AL_ETH_BOARD_MEDIA_TYPE_40G);
	if (p->phy_exist == AL_TRUE && p->phy_mdio_addr > 31)
		bp_issue(is, "phy_mdio_addr %u: expected 0..31", p->phy_mdio_addr);
	if (p->phy_if > AL_ETH_BOARD_PHY_IF_I2C)
		bp_issue(is, "phy_if %u: expected 0..2", p->phy_if);
	if (p->mdio_freq > AL_ETH_BOARD_MDIO_FREQ_1_MHZ)
		bp_issue(is, "mdio_freq %u: expected 0..1", p->mdio_freq);
	if (p->ref_clk_freq > AL_ETH_REF_FREQ_428_MHZ)
		bp_issue(is, "ref_clk_freq %u: expected 0..4", p->ref_clk_freq);
	if (p->speed > AL_ETH_BOARD_1G_SPEED_10M)
		bp_issue(is, "1g speed %u: expected 0..2", p->speed);
	if (!rgmii && p->serdes_lane > 3)
		bp_issue(is, "serdes_lane %u: expected 0..3", p->serdes_lane);
	if (!rgmii && p->serdes_grp > 3)
		bp_issue(is, "serdes_grp %u: expected 0..3", p->serdes_grp);
	if (rgmii && p->phy_exist != AL_TRUE)
		bp_issue(is, "RGMII media but phy_exist=0: expected an ext PHY");
	if (p->retimer_exist == AL_TRUE &&
	    (p->retimer_type >= AL_ETH_RETIMER_TYPE_MAX ||
	     p->retimer_channel >= AL_ETH_RETIMER_CHANNEL_MAX))
		bp_issue(is, "retimer type %u channel %u: expected <%u / <%u",
			 p->retimer_type, p->retimer_channel,
			 AL_ETH_RETIMER_TYPE_MAX, AL_ETH_RETIMER_CHANNEL_MAX);
}

/* ---- DT -> params -------------------------------------------------------- */

static bool bp_dt_is(struct device_node *np, const char *prop, const char *val)
{
	return of_property_match_string(np, prop, val) >= 0;
}

static int bp_from_dt(struct al_eth_adapter *adapter,
		      struct al_eth_board_params *p)
{
	struct device *dev = &adapter->pdev->dev;
	struct device_node *np = al_eth_mac_of_node(adapter->pdev);
	const char *mode = NULL;
	u32 v;

	memset(p, 0, sizeof(*p));
	if (!np) {
		dev_err(dev, "no DT ethernet node for PCI slot %u\n",
			PCI_SLOT(adapter->pdev->devfn));
		return -ENOENT;
	}

	of_property_read_string(np, "phy-mode", &mode);
	p->ref_clk_freq = BP_SEED_REF_CLK;

	if (mode && !strncmp(mode, "rgmii", 5)) {
		p->media_type = AL_ETH_BOARD_MEDIA_TYPE_RGMII;
	} else if (mode && !strcmp(mode, "10gbase-r")) {
		if (of_property_present(np, "sfp")) {
			p->media_type = AL_ETH_BOARD_MEDIA_TYPE_AUTO_DETECT_AUTO_SPEED;
			p->sfp_plus_module_exist = AL_TRUE;
		} else {
			p->media_type = AL_ETH_BOARD_MEDIA_TYPE_10GBASE_SR;
		}
	} else if (mode && !strcmp(mode, "sgmii")) {
		p->media_type = AL_ETH_BOARD_MEDIA_TYPE_SGMII;
	} else {
		dev_err(dev, "DT phy-mode '%s' has no board-params media type\n",
			mode ? mode : "<missing>");
		of_node_put(np);
		return -EINVAL;
	}

	if (p->media_type != AL_ETH_BOARD_MEDIA_TYPE_RGMII) {
		p->serdes_grp = of_property_read_u32(np, "serdes-grp", &v) ? 3 : v;
		p->serdes_lane = of_property_read_u32(np, "serdes-lane", &v) ?
				 0 : v;
		p->autoneg_enable = bp_dt_is(np, "auto-neg", "enabled");
		p->kr_lt_enable = bp_dt_is(np, "link-training", "enabled");
		p->kr_fec_enable = bp_dt_is(np, "fec", "enabled");
		p->force_1000_base_x = bp_dt_is(np, "force-1000base-x", "enabled");
		/* Stock defaults an absent/out-of-range dac-length to 3. */
		p->dac_len = (!of_property_read_u32(np, "dac-length", &v) &&
			      v > 0 && v < 16) ? v : 3;
	}

	if (!of_property_read_u32(np, "phy-addr", &v)) {
		p->phy_exist = AL_TRUE;
		p->phy_mdio_addr = v;
		p->phy_if = AL_ETH_BOARD_PHY_IF_MDIO;
		p->mdio_freq = (!of_property_read_u32(np, "mdio-freq-khz", &v) &&
				v == 2500) ? AL_ETH_BOARD_MDIO_FREQ_2_5_MHZ :
					     AL_ETH_BOARD_MDIO_FREQ_1_MHZ;
		p->an_mode = bp_dt_is(np, "auto-neg-mode", "in-band") ?
			AL_ETH_BOARD_AUTONEG_IN_BAND :
			AL_ETH_BOARD_AUTONEG_OUT_OF_BAND;
	}

	of_node_put(np);
	return 0;
}

/* Registers win over DT; differences are reported, not "fixed". */
static void bp_compare_dt(const struct al_eth_board_params *r,
			  const struct al_eth_board_params *d,
			  struct bp_issues *is)
{
#define CMP(f) do { if (r->f != d->f) \
	bp_issue(is, #f ": regs %u, DT %u", (unsigned int)r->f, \
		 (unsigned int)d->f); } while (0)
	CMP(media_type);
	CMP(phy_exist);
	CMP(phy_mdio_addr);
	CMP(sfp_plus_module_exist);
	if (r->media_type != AL_ETH_BOARD_MEDIA_TYPE_RGMII) {
		CMP(serdes_grp);
		CMP(serdes_lane);
		CMP(autoneg_enable);
		CMP(kr_lt_enable);
		CMP(kr_fec_enable);
		CMP(force_1000_base_x);
	}
#undef CMP
}

/* ---- load / self-seed ---------------------------------------------------- */

static void bp_log_issues(struct device *dev, const char *what,
			  const struct bp_issues *is)
{
	int i;

	for (i = 0; i < is->n; i++)
		dev_err(dev, "board params %s: %s\n", what, is->msg[i]);
}

int al_eth_board_params_load(struct al_eth_adapter *adapter,
			     struct al_eth_board_params *p)
{
	struct device *dev = &adapter->pdev->dev;
	struct al_eth_board_params dt;
	struct bp_issues *iss = kcalloc(2, sizeof(*iss), GFP_KERNEL);
	struct bp_issues *is = &iss[0], *dis = &iss[1];
	const char *why;
	int rc;

	if (!iss)
		return -ENOMEM;
	rc = al_eth_board_params_get(adapter->mac_base, p);
	if (!rc) {
		bp_check(p, is);
		if (!is->n) {
			adapter->bp_origin = "registers";
			if (!bp_from_dt(adapter, &dt)) {
				bp_compare_dt(p, &dt, dis);
				bp_log_issues(dev, "DT mismatch (regs used)", dis);
			}
			kfree(iss);
			return 0;
		}
		bp_log_issues(dev, "INVALID", is);
		why = "invalid";
	} else if (rc == -ENOENT) {
		why = "absent (reg1 == 0)";
	} else {
		dev_err(dev, "board params read failed (%d)\n", rc);
		kfree(iss);
		return rc;
	}

	rc = bp_from_dt(adapter, p);
	if (!rc) {
		bp_check(p, dis);
		if (dis->n) {
			bp_log_issues(dev, "DT-derived set INVALID", dis);
			rc = -EINVAL;
		}
	}
	kfree(iss);
	if (rc) {
		dev_err(dev, "board params %s and DT seed failed (%d)\n", why, rc);
		return rc;
	}

	dev_warn(dev, "BOARD PARAMS %s - SEEDED FROM DT (no bootloader of ours ran?): media %s serdes %u/%u phy %u@%u\n",
		 why, bp_media_name(p->media_type), p->serdes_grp,
		 p->serdes_lane, p->phy_exist == AL_TRUE, p->phy_mdio_addr);
	adapter->bp_origin = "seeded from DT";

	/* MAC scratch regs only (volatile), so FLR save/restore and ethtool see them. */
	rc = al_eth_board_params_set(adapter->mac_base, p);
	if (rc)
		dev_warn(dev, "board params seed write-back failed (%d); using in-memory copy\n",
			 rc);
	return 0;
}

/* ---- debugfs dump -------------------------------------------------------- */

static int bp_show(struct seq_file *m, void *unused)
{
	struct al_eth_adapter *a = m->private;
	struct al_eth_mac_regs __iomem *r = a->mac_base;
	struct al_eth_board_params p, dt;
	struct bp_issues *iss = kcalloc(2, sizeof(*iss), GFP_KERNEL);
	struct bp_issues *is = &iss[0], *dis = &iss[1];
	struct al_eth_mac_check mc;
	int rc, i;

	if (!iss)
		return -ENOMEM;

	seq_printf(m, "port %u pci %s origin: %s\n", PCI_SLOT(a->pdev->devfn),
		   pci_name(a->pdev), a->bp_origin ? a->bp_origin : "unknown");
	seq_printf(m, "raw: reg1(mac_1g.scratch)=0x%08x reg2(mac_10g.scratch)=0x%08x reg3(mac_1g.mac_0)=0x%08x\n",
		   readl(&r->mac_1g.scratch), readl(&r->mac_10g.scratch),
		   readl(&r->mac_1g.mac_0));

	rc = al_eth_board_params_get(a->mac_base, &p);
	if (rc) {
		seq_printf(m, "decode: FAILED (%d)%s\n", rc,
			   rc == -ENOENT ? " reg1 == 0, never written" : "");
		kfree(iss);
		return 0;
	}

	seq_printf(m, "media_type %u (%s) ref_clk %u mdio_freq %u\n",
		   p.media_type, bp_media_name(p.media_type), p.ref_clk_freq,
		   p.mdio_freq);
	seq_printf(m, "phy_exist %u phy_mdio_addr %u phy_if %u an_mode %u sfp_plus %u i2c_adapter %u\n",
		   p.phy_exist == AL_TRUE, p.phy_mdio_addr, p.phy_if, p.an_mode,
		   p.sfp_plus_module_exist == AL_TRUE, p.i2c_adapter_id);
	seq_printf(m, "serdes grp %u lane %u an %u lt %u fec %u dont_override %u force_1000bx %u\n",
		   p.serdes_grp, p.serdes_lane, p.autoneg_enable == AL_TRUE,
		   p.kr_lt_enable == AL_TRUE, p.kr_fec_enable == AL_TRUE,
		   p.dont_override_serdes == AL_TRUE, p.force_1000_base_x == AL_TRUE);
	seq_printf(m, "1g an_disable %u speed %u half_duplex %u fc_disable %u\n",
		   p.an_disable == AL_TRUE, p.speed, p.half_duplex == AL_TRUE,
		   p.fc_disable == AL_TRUE);
	seq_printf(m, "dac %u dac_len %u retimer exist %u type %u bus %u addr 0x%02x ch %u\n",
		   p.dac == AL_TRUE, p.dac_len, p.retimer_exist == AL_TRUE,
		   p.retimer_type, p.retimer_bus_id, p.retimer_i2c_addr,
		   p.retimer_channel);

	bp_check(&p, is);
	if (!is->n)
		seq_puts(m, "validate: OK\n");
	for (i = 0; i < is->n; i++)
		seq_printf(m, "validate: FAIL %s\n", is->msg[i]);

	if (bp_from_dt(a, &dt)) {
		seq_puts(m, "dt: no usable DT node\n");
	} else {
		bp_compare_dt(&p, &dt, dis);
		if (!dis->n)
			seq_puts(m, "dt: agrees\n");
		for (i = 0; i < dis->n; i++)
			seq_printf(m, "dt: DIFF %s\n", dis->msg[i]);
	}

	al_eth_mac_verify(a, &mc);
	seq_printf(m, "mac: ec %pM (%s) valid %u; nor %pM (%s) valid %u; %s\n",
		   mc.ec, mc.ec_src, mc.have_ec, mc.nor, mc.nor_src,
		   mc.have_nor,
		   !(mc.have_ec && mc.have_nor) ? "unknown" :
		   mc.match ? "match" : "MISMATCH");
	kfree(iss);
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(bp);

void al_eth_bp_debugfs_init(struct al_eth_adapter *adapter)
{
	char name[32];

	snprintf(name, sizeof(name), "al_eth_%s", pci_name(adapter->pdev));
	adapter->bp_dbg = debugfs_create_dir(name, NULL);
	debugfs_create_file("board_params", 0444, adapter->bp_dbg, adapter,
			    &bp_fops);
}

void al_eth_bp_debugfs_fini(struct al_eth_adapter *adapter)
{
	debugfs_remove_recursive(adapter->bp_dbg);
	adapter->bp_dbg = NULL;
}
