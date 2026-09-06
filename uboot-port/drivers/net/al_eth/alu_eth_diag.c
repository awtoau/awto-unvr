// SPDX-License-Identifier: GPL-2.0-or-later
/* `eth diag` - one compact bring-up block per alu_eth port.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * Answers "why is this port not up" without a debug rebuild: PCI BDF + the
 * three BARs, the MAC address and its source, the board params decoded, and the
 * live link state - PHY id / AN result on 1G, PCS block lock + SerDes lane and
 * TX equalisation on 10G.
 *
 * Near read-only. Two writes, both stated:
 * - 1G re-asserts the MDIO master config; without it a bare handle routes reads
 *   at an unconfigured window and SErrors.
 * - the 10G link read clears the PCS errored-block/BER counters, which
 *   `eth stats` also reports. One reader only (#196).
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
#include <asm/io.h>

#include <al_hal_eth.h>
#include <al_hal_eth_mac_regs.h>

#include "alu_eth.h"

/* MDIO master clock. alu_eth_1g.c uses the same 1 MHz - the board-params
 * mdio_freq field is a 1/2.5 MHz flag, not a number. */
#define ALU_DIAG_MDIO_CLK_KHZ	1000

/* SerDes/EQ readback from drivers/phy/al_serdes - extern, no cross-dir -I. */
#if IS_ENABLED(CONFIG_AL_SERDES)
int al_serdes_10g_tx_params_get(unsigned int lane, unsigned int *override,
				unsigned int *c_minus_1, unsigned int *c_plus_1,
				unsigned int *c_plus_2, unsigned int *tdu,
				unsigned int *amp);
int al_serdes_10g_lane_status_get(unsigned int lane, unsigned int *sig_det,
				  unsigned int *version);
#endif

static const char *yn(al_bool v)
{
	return v ? "yes" : "no";
}

/* ---- identity: BDF + BARs + MAC ---------------------------------------- */

static void diag_ident(int port, const struct alu_eth_regs *r)
{
	printf("alu_eth port %d (%s)\n", port, alu_eth_port_desc(port));
	printf("  pci        : %04x:%04x @ %02x:%02x.%x\n", ALU_ETH_PCI_VENDOR,
	       alu_eth_port_devid(port), (unsigned int)PCI_BUS(r->bdf),
	       (unsigned int)PCI_DEV(r->bdf), (unsigned int)PCI_FUNC(r->bdf));
	printf("  bars       : udma(0) %p  ec(4) %p  mac(2) %p\n",
	       r->udma, r->ec, r->mac);
}

/* What the EC filter actually holds vs what NOR says it should - a mismatch is
 * the wrong-MAC bug Linux inherits (#222). */
static void diag_hwaddr(int port, const struct alu_eth_regs *r)
{
	uint8_t live[ARP_HLEN], want[ARP_HLEN];
	int rc;

	if (al_eth_mac_addr_read(r->ec, 0, live)) {
		printf("  mac        : unreadable (ec addr slot 0)\n");
		return;
	}

	printf("  mac        : %pM  <- EC filter slot 0%s\n", live,
	       is_zero_ethaddr(live) ? " (UNSET)" : "");

	rc = alu_eth_hwaddr_get(port, want);
	if (rc) {
		printf("  mac source : SPI-NOR unreadable (%d)\n", rc);
		return;
	}
	printf("  mac source : %pM (NOR base+%d) - %s\n", want,
	       port - ALU_ETH_PORT_1G,
	       memcmp(live, want, ARP_HLEN) ? "MISMATCH" : "match");
}

/* ---- board params (the MAC scratchpad Linux reads at probe) ------------- */

static int diag_board_params(int port, const struct alu_eth_regs *r,
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
	       alu_eth_media_name(p->media_type), p->ref_clk_freq);
	printf("               phy %s (addr %u, if %u, mdio %s), sfp %s\n",
	       yn(p->phy_exist), p->phy_mdio_addr, p->phy_if,
	       p->mdio_freq ? "1MHz" : "2.5MHz", yn(p->sfp_plus_module_exist));
	printf("               serdes grp %u lane %u, i2c-adapter %u\n",
	       p->serdes_grp, p->serdes_lane, p->i2c_adapter_id);
	printf("               an %s (%s), kr-lt %s, kr-fec %s, dac %s (len %u)\n",
	       yn(p->autoneg_enable), p->an_mode ? "in-band" : "out-of-band",
	       yn(p->kr_lt_enable), yn(p->kr_fec_enable), yn(p->dac), p->dac_len);
	printf("               dont_override_serdes %s, retimer %s\n",
	       yn(p->dont_override_serdes), yn(p->retimer_exist));
	return 0;
}

/* ---- link: 1G (MDIO PHY) ----------------------------------------------- */

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
	int rc;

	/* The one register write diag makes, and it is required: a bare handle
	 * leaves mdio_if at the 1G MAC, whose raw phy_regs_base window SErrors on
	 * a MAC the DM driver never started. */
	rc = al_eth_mdio_config(a, AL_ETH_MDIO_TYPE_CLAUSE_22, AL_TRUE,
				p->ref_clk_freq, ALU_DIAG_MDIO_CLK_KHZ);
	if (rc) {
		printf("  link       : mdio config failed (%d)\n", rc);
		return;
	}

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
	printf("  link       : %s\n", (bmsr & BMSR_LSTATUS) ? "UP" : "DOWN");
	printf("  autoneg    : %s, complete %s\n",
	       (bmcr & BMCR_ANENABLE) ? "enabled" : "disabled",
	       yn(!!(bmsr & BMSR_ANEGCOMPLETE)));

	/* Resolved speed/duplex, highest common ability first - the priority
	 * phylib's genphy_parse_link uses. */
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

/* ---- link: 10G (PCS block lock + SerDes lane) --------------------------- */

static void diag_link_10g(struct al_hal_eth_adapter *a,
			  const struct al_eth_board_params *p)
{
	struct al_eth_link_status st;
	int rc;

	memset(&st, 0, sizeof(st));
	rc = al_eth_link_status_get(a, &st);
	if (rc) {
		printf("  link       : status unavailable (%d)\n", rc);
	} else {
		printf("  link       : %s (10GBASE-R, fixed 10.3125G, no PHY)\n",
		       st.link_up ? "UP" : "DOWN");
		printf("  faults     : local %s, remote %s\n",
		       yn(st.local_fault), yn(st.remote_fault));
		printf("  pcs        : errored_blocks %u, ber_count %u\n",
		       st.pcs_errored_blocks, st.pcs_ber_count);
		printf("  note       : those two are clear-on-read; this read zeroed\n");
		printf("               them (`eth stats` reports the same pair)\n");
	}
	printf("  sfp        : board params say module %s\n",
	       yn(p->sfp_plus_module_exist));

#if IS_ENABLED(CONFIG_AL_SERDES)
	{
		unsigned int ov, cm1, cp1, cp2, tdu, amp, sig = 0, ver = 0;

		if (!al_serdes_10g_lane_status_get(p->serdes_lane, &sig, &ver))
			printf("  serdes     : HSSP grp %u lane %u, version 0x%08x, signal_detect %s\n",
			       p->serdes_grp, p->serdes_lane, ver, yn(sig));
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

int alu_eth_diag_show(int port)
{
	struct al_eth_board_params params;
	struct al_hal_eth_adapter a;
	struct alu_eth_regs regs;
	char name[] = "eth-diag";
	int rc;

	rc = alu_eth_port_regs_get(port, &regs);
	if (rc)
		return rc;

	diag_ident(port, &regs);
	diag_hwaddr(port, &regs);

	if (diag_board_params(port, &regs, &params))
		return 0;	/* the missing params ARE the diagnosis */

	/* Software-only handle: the shared HAL's adapter is a plain struct, so
	 * the fields the link paths read are filled in directly and
	 * al_eth_adapter_init() (which configures hardware) is never called. */
	memset(&a, 0, sizeof(a));
	a.rev_id = AL_ETH_REV_ID_2;
	a.udma_regs_base = regs.udma;
	a.ec_regs_base = (struct al_ec_regs __iomem *)regs.ec;
	a.mac_regs_base = (struct al_eth_mac_regs __iomem *)regs.mac;
	a.name = name;

	if (params.media_type == AL_ETH_BOARD_MEDIA_TYPE_RGMII) {
		a.mac_mode = AL_ETH_MAC_MODE_RGMII;
		diag_link_1g(&a, &params);
	} else {
		a.mac_mode = AL_ETH_MAC_MODE_10GbE_Serial;
		diag_link_10g(&a, &params);
	}

	return 0;
}
