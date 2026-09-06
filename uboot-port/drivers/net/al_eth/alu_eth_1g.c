// SPDX-License-Identifier: GPL-2.0-or-later
/* alu_eth 1G RJ45 (RGMII, eth1, PCI 1c36:0001) - DM_ETH front end.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * 1G-specific half only: the MDIO master, the AR8033 via phylib, and RGMII MAC
 * mode. Rings, adapter init, send and recv are in alu_eth_core.c.
 *
 * Binds by PCI ID, not a DT compatible - the bare eth0..3 platform nodes at
 * 0xfc000000+ are unused (docs/hardware.md).
 */

#include <dm.h>
#include <errno.h>
#include <miiphy.h>
#include <net.h>
#include <pci.h>
#include <phy.h>
#include <linux/string.h>
#include <dm/device_compat.h>

#include <al_hal_eth.h>

#include "alu_eth.h"
#include "alu_eth_core.h"

/* Confirmed board facts (coordinator / live dmesg): AR8031/8033 (at803x) on
 * MDIO addr 4, MDIO master 1 MHz, eth ref clk 500 MHz. Set explicitly rather
 * than read back from the scratchpad - robust for chainload and standalone. */
#define ALU_ETH_1G_PHY_ADDR	4
#define ALU_ETH_1G_MDIO_CLK_KHZ	1000
#define ALU_ETH_1G_REF_CLK	AL_ETH_REF_FREQ_500_MHZ

struct alu_eth_1g_priv {
	struct alu_eth_priv	core;		/* MUST be first: dev_get_priv() */
	struct mii_dev		*mdio_bus;
	struct phy_device	*phy;
};

/* ---- MDIO bus over the HAL's clause-22 accessors ----------------------- */

static int alu_eth_1g_mdio_read(struct mii_dev *bus, int addr, int devad, int reg)
{
	struct alu_eth_priv *priv = bus->priv;
	uint16_t val = 0;
	int rc;

	rc = al_eth_mdio_read(&priv->adapter, addr, MDIO_DEVAD_NONE, reg, &val);
	return rc ? rc : val;
}

static int alu_eth_1g_mdio_write(struct mii_dev *bus, int addr, int devad,
				 int reg, u16 val)
{
	struct alu_eth_priv *priv = bus->priv;

	return al_eth_mdio_write(&priv->adapter, addr, MDIO_DEVAD_NONE, reg, val);
}

/* ---- eth_ops ----------------------------------------------------------- */

static int alu_eth_1g_start(struct udevice *dev)
{
	struct alu_eth_1g_priv *p = dev_get_priv(dev);
	int rc;

	if (p->core.started)
		return 0;

	rc = alu_eth_core_init(dev, &p->core);
	if (rc)
		return rc;

	/* MDIO master config comes after adapter init: it programs the MAC's own
	 * MDIO window, which adapter_init resets. */
	rc = al_eth_mdio_config(&p->core.adapter, AL_ETH_MDIO_TYPE_CLAUSE_22,
				AL_TRUE, ALU_ETH_1G_REF_CLK,
				ALU_ETH_1G_MDIO_CLK_KHZ);
	if (rc) {
		dev_err(dev, "al_eth_mdio_config failed: %d\n", rc);
		return rc;
	}

	/* RGMII_ID, not plain RGMII: the AR8033 must apply its internal RX/TX
	 * clock delays. Without them the MAC samples RX on the wrong edge and
	 * drops every frame - 213 in, 213 if_in_errors, 0 fcs_errors (30e7c65).
	 * Linux uses RGMII_ID here too. */
	p->phy = phy_connect(p->mdio_bus, ALU_ETH_1G_PHY_ADDR, dev,
			     PHY_INTERFACE_MODE_RGMII_ID);
	if (!p->phy) {
		dev_err(dev, "phy_connect failed (addr %u)\n",
			ALU_ETH_1G_PHY_ADDR);
		return -ENODEV;
	}

	p->phy->supported &= (SUPPORTED_10baseT_Half | SUPPORTED_10baseT_Full |
			      SUPPORTED_100baseT_Half | SUPPORTED_100baseT_Full |
			      SUPPORTED_1000baseT_Full);
	p->phy->advertising = p->phy->supported;

	rc = phy_config(p->phy);
	if (rc) {
		dev_err(dev, "phy_config failed: %d\n", rc);
		return rc;
	}
	rc = phy_startup(p->phy);
	if (rc) {
		dev_err(dev, "phy_startup failed: %d\n", rc);
		return rc;
	}
	if (!p->phy->link) {
		dev_err(dev, "no link\n");
		return -EIO;
	}

	/* No al_eth_mac_link_config: the external AR8033 drives the link and the
	 * MAC follows via RGMII in-band. Forcing it wedged the UDMA TX. */
	p->core.started = true;
	return 0;
}

static void alu_eth_1g_stop(struct udevice *dev)
{
	struct alu_eth_1g_priv *p = dev_get_priv(dev);

	if (p->phy)
		phy_shutdown(p->phy);
	alu_eth_core_stop(dev);
}

static const struct eth_ops alu_eth_1g_ops = {
	.start		= alu_eth_1g_start,
	.send		= alu_eth_core_send,
	.recv		= alu_eth_core_recv,
	.free_pkt	= alu_eth_core_free_pkt,
	.stop		= alu_eth_1g_stop,
	.write_hwaddr	= alu_eth_core_write_hwaddr,
	.read_rom_hwaddr = alu_eth_core_read_rom_hwaddr,
};

/* ---- bind / probe ------------------------------------------------------ */

static int alu_eth_1g_probe(struct udevice *dev)
{
	struct alu_eth_1g_priv *p = dev_get_priv(dev);
	struct alu_eth_regs regs;
	int rc;

	strlcpy(p->core.name, dev->name, sizeof(p->core.name));
	p->core.port = ALU_ETH_PORT_1G;
	p->core.mac_mode = AL_ETH_MAC_MODE_RGMII;

	rc = alu_eth_port_regs_get(ALU_ETH_PORT_1G, &regs);
	if (rc)
		return rc;
	p->core.udma_regs = regs.udma;
	p->core.ec_regs = regs.ec;
	p->core.mac_regs = regs.mac;

	/* Bus mastering for DMA - our internal-PCIe self-enumeration may not
	 * have set it. */
	dm_pci_clrset_config16(dev, PCI_COMMAND, 0,
			       PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);

	/* Board params + MAC into the scratchpad and the EC filter at PROBE, not
	 * from the uclass's lazy write_hwaddr: Linux inherits both whether or not
	 * U-Boot ever activates the port (#222). */
	alu_eth_bp_seed(ALU_ETH_PORT_1G, p->core.mac_regs);
	alu_eth_hwaddr_commit(ALU_ETH_PORT_1G, p->core.ec_regs);

	rc = alu_eth_bufs_alloc(&p->core);
	if (rc)
		return rc;

	p->mdio_bus = mdio_alloc();
	if (!p->mdio_bus)
		return -ENOMEM;
	p->mdio_bus->read = alu_eth_1g_mdio_read;
	p->mdio_bus->write = alu_eth_1g_mdio_write;
	p->mdio_bus->priv = &p->core;
	strlcpy(p->mdio_bus->name, dev->name, sizeof(p->mdio_bus->name));

	rc = mdio_register(p->mdio_bus);
	if (rc) {
		dev_err(dev, "mdio_register failed: %d\n", rc);
		return rc;
	}

	return 0;
}

/* priv persists across probe/remove, so the bus pointer must be cleared or the
 * next probe fails with "non unique device name". */
static int alu_eth_1g_remove(struct udevice *dev)
{
	struct alu_eth_1g_priv *p = dev_get_priv(dev);

	if (p->mdio_bus) {
		mdio_unregister(p->mdio_bus);
		mdio_free(p->mdio_bus);
		p->mdio_bus = NULL;
	}
	return 0;
}

U_BOOT_DRIVER(alu_eth_1g) = {
	.name		= "alu_eth_1g",
	.id		= UCLASS_ETH,
	.probe		= alu_eth_1g_probe,
	.remove		= alu_eth_1g_remove,
	.ops		= &alu_eth_1g_ops,
	.priv_auto	= sizeof(struct alu_eth_1g_priv),
	.plat_auto	= sizeof(struct eth_pdata),
};

static struct pci_device_id alu_eth_1g_pci_ids[] = {
	{ PCI_DEVICE(ALU_ETH_PCI_VENDOR, ALU_ETH_PCI_DEV_1G) },
	{ }
};

U_BOOT_PCI_DEVICE(alu_eth_1g, alu_eth_1g_pci_ids);
