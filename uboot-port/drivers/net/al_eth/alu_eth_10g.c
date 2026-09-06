// SPDX-License-Identifier: GPL-2.0-or-later
/* alu_eth 10G SFP+ (eth2, PCI 1c36:0002) - DM_ETH front end.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * 10G-specific half only: the SerDes lane, MAC mode 10GbE_Serial and the
 * no-PHY link gate (10GBASE-R PCS block lock). Rings, adapter init, send and
 * recv are in alu_eth_core.c.
 *
 * Fixed 10.3125 Gbps with KR AN and LT off, so no link-management FSM: the
 * mode is served by the shared HAL's rev-2 MAC path.
 */

#include <dm.h>
#include <errno.h>
#include <net.h>
#include <pci.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <dm/device_compat.h>

#include <al_hal_eth.h>

#include "alu_eth.h"
#include "alu_eth_core.h"

/* PCS block lock after lane bring-up: 1ms per iteration. Matches stock's link
 * budget (~30 x 100ms) against a partner already up. On expiry: warn and
 * continue - a later start() retries, and a port with nothing plugged must
 * still probe. */
#define ALU_ETH_10G_LINK_POLL_MAX	3000

/* SerDes lane bring-up lives in drivers/phy/al_serdes (CONFIG_AL_SERDES),
 * reached by extern prototype so no cross-directory -I is needed. */
#if IS_ENABLED(CONFIG_AL_SERDES)
int al_serdes_10g_init(void);
void al_serdes_10g_status(void);
#else
static inline int al_serdes_10g_init(void) { return -ENOSYS; }
static inline void al_serdes_10g_status(void) { }
#endif

struct alu_eth_10g_priv {
	struct alu_eth_priv	core;		/* MUST be first: dev_get_priv() */
};

/* Link = PCS block lock, reported through the HAL's link_status. No MDIO: the
 * SFP optic drives the line. */
static int alu_eth_10g_wait_link(struct udevice *dev)
{
	struct alu_eth_10g_priv *p = dev_get_priv(dev);
	struct al_eth_link_status status;
	int poll = 0;

	do {
		memset(&status, 0, sizeof(status));
		if (!al_eth_link_status_get(&p->core.adapter, &status) &&
		    status.link_up)
			return 0;
		udelay(1000);
	} while (++poll < ALU_ETH_10G_LINK_POLL_MAX);

	dev_warn(dev, "no 10G link after %d ms (PCS block lock not set)\n", poll);
	return -ETIMEDOUT;
}

/* ---- eth_ops ----------------------------------------------------------- */

static int alu_eth_10g_start(struct udevice *dev)
{
	struct alu_eth_10g_priv *p = dev_get_priv(dev);
	int rc;

	if (p->core.started)
		return 0;

	/* SerDes lane before the core's MAC config, so the electrical lane is
	 * live when the MAC selects the KR mux - stock's order. The PCS has a
	 * single owner (al_eth_mac_config); this call does the lane only. */
	rc = al_serdes_10g_init();
	if (rc)
		dev_warn(dev, "al_serdes_10g_init failed: %d (continuing)\n", rc);

	rc = alu_eth_core_init(dev, &p->core);
	if (rc)
		return rc;

	if (alu_eth_10g_wait_link(dev))
		dev_warn(dev, "10G link not up (no SFP/partner?) - continuing\n");
	else if (IS_ENABLED(CONFIG_AL_SERDES))
		al_serdes_10g_status();

	p->core.started = true;
	return 0;
}

static const struct eth_ops alu_eth_10g_ops = {
	.start		= alu_eth_10g_start,
	.send		= alu_eth_core_send,
	.recv		= alu_eth_core_recv,
	.free_pkt	= alu_eth_core_free_pkt,
	.stop		= alu_eth_core_stop,
	.write_hwaddr	= alu_eth_core_write_hwaddr,
	.read_rom_hwaddr = alu_eth_core_read_rom_hwaddr,
};

/* ---- probe ------------------------------------------------------------- */

static int alu_eth_10g_probe(struct udevice *dev)
{
	struct alu_eth_10g_priv *p = dev_get_priv(dev);
	struct alu_eth_regs regs;
	int rc;

	strlcpy(p->core.name, dev->name, sizeof(p->core.name));
	p->core.port = ALU_ETH_PORT_10G;
	p->core.mac_mode = AL_ETH_MAC_MODE_10GbE_Serial;

	rc = alu_eth_port_regs_get(ALU_ETH_PORT_10G, &regs);
	if (rc)
		return rc;
	p->core.udma_regs = regs.udma;
	p->core.ec_regs = regs.ec;
	p->core.mac_regs = regs.mac;

	dm_pci_clrset_config16(dev, PCI_COMMAND, 0,
			       PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);

	/* At PROBE, so Linux inherits the right values even though U-Boot
	 * normally never activates this port (#222). */
	alu_eth_bp_seed(ALU_ETH_PORT_10G, p->core.mac_regs);
	alu_eth_hwaddr_commit(ALU_ETH_PORT_10G, p->core.ec_regs);

	return alu_eth_bufs_alloc(&p->core);
}

U_BOOT_DRIVER(alu_eth_10g) = {
	.name		= "alu_eth_10g",
	.id		= UCLASS_ETH,
	.probe		= alu_eth_10g_probe,
	.ops		= &alu_eth_10g_ops,
	.priv_auto	= sizeof(struct alu_eth_10g_priv),
	.plat_auto	= sizeof(struct eth_pdata),
};

static struct pci_device_id alu_eth_10g_pci_ids[] = {
	{ PCI_DEVICE(ALU_ETH_PCI_VENDOR, ALU_ETH_PCI_DEV_10G) },
	{ }
};

U_BOOT_PCI_DEVICE(alu_eth_10g, alu_eth_10g_pci_ids);
