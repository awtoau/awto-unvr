// SPDX-License-Identifier: GPL-2.0-or-later
/* `eth diag` - one compact bring-up block per al_eth port.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * Answers "why is this port not up" without a debug rebuild: PCI BDF + the
 * three BARs, the MAC address and its source, the board params decoded (not
 * raw hex), and the live link state - PHY id / AN result on 1G, PCS block-lock
 * + SerDes lane and TX equalisation taps on 10G.
 *
 * Read-only: the adapter handle is built here (never al_eth_adapter_init), and
 * MDIO/SerDes reads have no side effects, so a live port is never disturbed.
 * One exception, printed as a note in the output: reading BASE-R Status 2 on
 * 10G clears its errored-block/BER fields, which `eth stats` also reports.
 */

#include <command.h>
#include <errno.h>
#include <miiphy.h>
#include <net.h>
#include <pci.h>
#include <stdio.h>
#include <linux/kernel.h>
#include <linux/mii.h>
#include <linux/string.h>
#include <vsprintf.h>
#include <asm/io.h>

#include <al_hal_eth.h>
#include <al_hal_eth_mac.h>
#include <al_hal_eth_mac_regs.h>

#include "al_eth_port.h"

/* Bit 15 of KR-PCS BASE-R Status 2 (Clause-45 dev 3 reg 0x21) = block lock. */
#define AL_DIAG_PCS_BASE_R_STATUS2	0x21
#define AL_DIAG_PCS_BLOCK_LOCK		AL_BIT(15)
#define AL_DIAG_PCS_HI_BER		AL_BIT(14)

/* Per-unit MAC in the SPI-NOR identity partition; port 1 is base+0, port 2 is
 * base+1 (docs/mtd.md, #223). Mirrors what the two DM drivers program. */
#define AL_DIAG_MAC_ROM_OFFSET		0x1f0000

/* SerDes/EQ readback from drivers/phy/al_serdes (extern - no cross-dir -I). */
#if IS_ENABLED(CONFIG_AL_SERDES)
int al_serdes_10g_tx_params_get(unsigned int lane, unsigned int *override,
				unsigned int *c_minus_1, unsigned int *c_plus_1,
				unsigned int *c_plus_2, unsigned int *tdu,
				unsigned int *amp);
int al_serdes_10g_lane_status_get(unsigned int lane, unsigned int *sig_det,
				  unsigned int *version);
#endif

static const char *const diag_media[] = {
	"auto-detect", "RGMII", "10GBASE-SR (10G serial)", "SGMII",
	"1000BASE-X", "auto-detect-auto-speed", "SGMII-2.5G", "NBASE-T",
	"25G", "25G/10G-auto", "40G", "100G",
};

static const char *diag_media_name(unsigned int t)
{
	return (t < ARRAY_SIZE(diag_media)) ? diag_media[t] : "?";
}

static const char *diag_yn(al_bool v)
{
	return v ? "yes" : "no";
}

/* ---- identity: BDF + BARs + MAC ---------------------------------------- */

static void diag_ident(int port, const struct al_eth_port_regs *r)
{
	printf("al_eth port %d (%s)\n", port, al_eth_port_desc(port));
	printf("  pci        : %04x:%04x @ %02x:%02x.%x\n", 0x1c36,
	       al_eth_port_devid(port), (unsigned int)PCI_BUS(r->bdf),
	       (unsigned int)PCI_DEV(r->bdf), (unsigned int)PCI_FUNC(r->bdf));
	printf("  bars       : udma(0) %p  ec(4) %p  mac(2) %p\n",
	       r->udma, r->ec, r->mac);
}

/* The MAC the EC filter is actually programmed with, plus what the NOR would
 * give - a mismatch is the "wrong MAC" bug, and both being zero means the port
 * was never started. */
static void diag_hwaddr(int port, const struct al_eth_port_regs *r)
{
	uint8_t live[ARP_HLEN];

	if (al_eth_mac_addr_read(r->ec, 0, live)) {
		printf("  mac        : unreadable (ec addr slot 0)\n");
		return;
	}

	printf("  mac        : %pM  <- EC filter slot 0", live);
	if (is_zero_ethaddr(live))
		printf(" (UNSET - port never started)");
	printf("\n");
	printf("  mac source : SPI-NOR 0x%06x + %d (#223)\n",
	       AL_DIAG_MAC_ROM_OFFSET, port - 1);
}

/* ---- board params (the MAC scratchpad Linux reads at probe) ------------- */

static int diag_board_params(int port, const struct al_eth_port_regs *r,
			     struct al_eth_board_params *p)
{
	struct al_eth_mac_regs __iomem *m =
		(struct al_eth_mac_regs __iomem *)r->mac;
	u32 reg1 = readl(&m->mac_1g.scratch);
	int rc;

	rc = al_eth_board_params_get(r->mac, p);
	if (rc) {
		printf("  boardparams: reg1=0x%08x NOT SET - Linux al_eth fails\n",
		       reg1);
		printf("               probe (\"board info not available\").\n");
		printf("               Write them: `serdes boardparams write %d`\n",
		       port);
		return rc;
	}

	printf("  boardparams: media %u (%s), ref_clk %u\n", p->media_type,
	       diag_media_name(p->media_type), p->ref_clk_freq);
	printf("               phy %s (addr %u, if %u, mdio %s), sfp %s\n",
	       diag_yn(p->phy_exist), p->phy_mdio_addr, p->phy_if,
	       p->mdio_freq ? "1MHz" : "2.5MHz",
	       diag_yn(p->sfp_plus_module_exist));
	printf("               serdes grp %u lane %u, i2c-adapter %u\n",
	       p->serdes_grp, p->serdes_lane, p->i2c_adapter_id);
	printf("               an %s (%s), kr-lt %s, kr-fec %s, dac %s (len %u)\n",
	       diag_yn(p->autoneg_enable),
	       p->an_mode ? "in-band" : "out-of-band", diag_yn(p->kr_lt_enable),
	       diag_yn(p->kr_fec_enable), diag_yn(p->dac), p->dac_len);
	printf("               dont_override_serdes %s, retimer %s\n",
	       diag_yn(p->dont_override_serdes), diag_yn(p->retimer_exist));
	return 0;
}

/* ---- link: 1G (MDIO PHY) ----------------------------------------------- */

/* Clause-22 read straight off the MAC's MDIO master, so diag works whether or
 * not the DM driver ever started (it owns the only mii_dev). */
static int diag_mdio_read(struct al_hal_eth_adapter *a, unsigned int addr,
			  unsigned int reg, uint16_t *val)
{
	*val = 0;
	return al_eth_mdio_read(a, addr, MDIO_DEVAD_NONE, reg, val);
}

static void diag_link_1g(struct al_hal_eth_adapter *a,
			 const struct al_eth_board_params *p)
{
	uint16_t id1, id2, bmsr, bmcr, lpa, stat1000;
	unsigned int addr = p->phy_exist ? p->phy_mdio_addr : 4;

	if (diag_mdio_read(a, addr, MII_PHYSID1, &id1) ||
	    diag_mdio_read(a, addr, MII_PHYSID2, &id2)) {
		printf("  link       : MDIO addr %u unreadable\n", addr);
		return;
	}
	if (id1 == 0xffff || (!id1 && !id2)) {
		printf("  link       : no PHY at MDIO addr %u (id %04x%04x)\n",
		       addr, id1, id2);
		return;
	}

	diag_mdio_read(a, addr, MII_BMSR, &bmsr);
	diag_mdio_read(a, addr, MII_BMSR, &bmsr);	/* latch-low: read twice */
	diag_mdio_read(a, addr, MII_BMCR, &bmcr);
	diag_mdio_read(a, addr, MII_LPA, &lpa);
	diag_mdio_read(a, addr, MII_STAT1000, &stat1000);

	printf("  phy        : addr %u, id %04x%04x\n", addr, id1, id2);
	printf("  link       : %s\n",
	       (bmsr & BMSR_LSTATUS) ? "UP" : "DOWN");
	printf("  autoneg    : %s, complete %s\n",
	       (bmcr & BMCR_ANENABLE) ? "enabled" : "disabled",
	       diag_yn(!!(bmsr & BMSR_ANEGCOMPLETE)));

	/* Resolved speed/duplex from the AN result registers, highest common
	 * ability first - the same priority phylib's genphy_parse_link uses. */
	if (stat1000 & (LPA_1000FULL | LPA_1000HALF))
		printf("  speed      : 1000M %s\n",
		       (stat1000 & LPA_1000FULL) ? "full" : "half");
	else if (lpa & (LPA_100FULL | LPA_100HALF))
		printf("  speed      : 100M %s\n",
		       (lpa & LPA_100FULL) ? "full" : "half");
	else if (lpa & (LPA_10FULL | LPA_10HALF))
		printf("  speed      : 10M %s\n",
		       (lpa & LPA_10FULL) ? "full" : "half");
	else
		printf("  speed      : unresolved (lpa %04x stat1000 %04x)\n",
		       lpa, stat1000);
}

/* ---- link: 10G (PCS block-lock + SerDes lane) --------------------------- */

static void diag_link_10g(const struct al_eth_port_regs *r,
			  const struct al_eth_board_params *p)
{
	struct al_eth_mac_regs __iomem *m =
		(struct al_eth_mac_regs __iomem *)r->mac;
	u32 status2;

	writel(AL_DIAG_PCS_BASE_R_STATUS2, &m->kr.pcs_addr);
	status2 = readl(&m->kr.pcs_data);

	printf("  pcs        : base_r_status2 0x%04x, block_lock %s, hi_ber %s\n",
	       status2 & 0xffff, diag_yn(!!(status2 & AL_DIAG_PCS_BLOCK_LOCK)),
	       diag_yn(!!(status2 & AL_DIAG_PCS_HI_BER)));
	printf("  link       : %s (10GBASE-R, fixed 10.3125G, no PHY)\n",
	       (status2 & AL_DIAG_PCS_BLOCK_LOCK) ? "UP" : "DOWN");
	printf("  sfp        : board params say module %s\n",
	       diag_yn(p->sfp_plus_module_exist));
	printf("  note       : the errored-block/BER fields of base_r_status2 are\n");
	printf("               clear-on-read; this read zeroed them (`eth stats`)\n");

#if IS_ENABLED(CONFIG_AL_SERDES)
	{
		unsigned int ov, cm1, cp1, cp2, tdu, amp, sig = 0, ver = 0;

		if (!al_serdes_10g_lane_status_get(p->serdes_lane, &sig, &ver))
			printf("  serdes     : HSSP grp %u lane %u, version 0x%08x, signal_detect %s\n",
			       p->serdes_grp, p->serdes_lane, ver,
			       diag_yn(sig));
		else
			printf("  serdes     : HSSP grp %u lane %u - lane status unavailable\n",
			       p->serdes_grp, p->serdes_lane);

		if (!al_serdes_10g_tx_params_get(p->serdes_lane, &ov, &cm1,
						 &cp1, &cp2, &tdu, &amp)) {
			printf("  tx eq      : c_minus_1 %u, c_plus_1 %u, c_plus_2 %u,\n",
			       cm1, cp1, cp2);
			printf("               total_driver_units %u -> main cursor %d, amp %u\n",
			       tdu, (int)tdu - (int)(cm1 + cp1 + cp2), amp);
			printf("  tx eq src  : %s\n", ov ?
			       "REGISTER OVERRIDE - our static params are in force" :
			       "external pins - static params NOT applied");
		} else {
			printf("  tx eq      : readback unavailable\n");
		}
	}
#else
	printf("  serdes     : CONFIG_AL_SERDES off - no lane/EQ readback\n");
#endif
}

/* ---- the command ------------------------------------------------------- */

int al_eth_diag_show(int port)
{
	struct al_eth_adapter_handle_init_params hp;
	struct al_eth_board_params params;
	struct al_hal_eth_adapter a;
	struct al_eth_port_regs regs;
	char name[] = "eth-diag";
	int rc;

	rc = al_eth_port_regs_get(port, &regs);
	if (rc)
		return rc;

	diag_ident(port, &regs);
	diag_hwaddr(port, &regs);

	if (diag_board_params(port, &regs, &params))
		return 0;	/* the missing params ARE the diagnosis */

	/* Handle-only init: software state, no register writes, so a port that
	 * is already up is untouched (al_eth_adapter_init is what configures
	 * hardware and is deliberately never called). */
	memset(&hp, 0, sizeof(hp));
	hp.rev_id = AL_ETH_REV_ID_2;			/* Alpine V2 */
	hp.dev_id = (port % 2) ? AL_ETH_DEV_ID_STANDARD : AL_ETH_DEV_ID_ADVANCED;
	hp.udma_regs_base = regs.udma;
	hp.ec_regs_base = (struct al_ec_regs __iomem *)regs.ec;
	hp.mac_regs_base = (struct al_eth_mac_regs __iomem *)regs.mac;
	hp.common_mode = AL_ETH_COMMON_MODE_INVALID;
	hp.name = name;
	al_eth_adapter_handle_init(&a, &hp);

	if (params.media_type == AL_ETH_BOARD_MEDIA_TYPE_RGMII) {
		a.mac_obj.mac_mode = AL_ETH_MAC_MODE_RGMII;
		a.mac_mode = AL_ETH_MAC_MODE_RGMII;
		diag_link_1g(&a, &params);
	} else {
		a.mac_obj.mac_mode = AL_ETH_MAC_MODE_10GbE_Serial;
		a.mac_mode = AL_ETH_MAC_MODE_10GbE_Serial;
		diag_link_10g(&regs, &params);
	}

	return 0;
}
