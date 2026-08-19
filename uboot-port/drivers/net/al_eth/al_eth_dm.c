// SPDX-License-Identifier: GPL-2.0-or-later
/* Annapurna Labs al_eth 1G RJ45 (RGMII) driver - modern U-Boot DM_ETH.
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 * Derived from Annapurna Labs HAL / vendor U-Boot al_eth.c (Copyright (C)
 * Annapurna Labs Ltd, GPLv2 OR BSD-3-Clause); reimplemented on U-Boot primitives.
 *
 * Scope: RGMII 1G only (eth1, PCI 1c36:0001, AR8033 PHY @ MDIO addr 4). NO serdes
 * / KR / PCS / link-management - those paths are for the 10G SFP+ port (eth2) and
 * are intentionally excluded (see hal/README.md). The init/send/recv sequence
 * mirrors the vendor al_eth.c (al_eth_adapter_init -> queue_config/enable ->
 * mac_config(RGMII) -> mac_link_config -> rx_buffer_add -> mac_start), re-expressed
 * as DM eth_ops.
 *
 * DMA coherency: relies on the #74 AXI SMCC snoop fix (board_late_init) that makes
 * the internal-PCIe units cache-coherent, so no explicit cache maintenance here -
 * same assumption as the vendor driver on this coherent SoC. If a HW bring-up shows
 * stale descriptors/buffers, add flush/invalidate around TX submit + RX consume.
 *
 * HARDWARE-TODO markers below flag values/paths that can only be finalised on the box.
 */

#include <dm.h>
#include <errno.h>
#include <malloc.h>
#include <memalign.h>
#include <miiphy.h>
#include <net.h>
#include <pci.h>
#include <phy.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <dm/device_compat.h>

#include <al_hal_eth.h>
#include <al_hal_udma.h>

/*
 * al_eth is a PCI-enumerated endpoint on the internal PCIe (bus 0), NOT a
 * platform/DT device - the bare eth0..3 nodes at 0xfc000000+ in the live DT are
 * UNUSED by the driver (docs/hardware.md). eth1 (1G RJ45) = vendor 0x1c36 dev
 * 0x0001. It exposes THREE separate BARs (non-contiguous windows): the UDMA and
 * MAC are not one base + offsets. BAR indices per the vendor al_hal_eth.h
 * (AL_ETH_UDMA_BAR/EC_BAR/MAC_BAR, matched by drivers/net/al_eth_pci.c):
 *   BAR0 -> UDMA regs   BAR4 -> EC (Ethernet Controller) regs   BAR2 -> MAC regs
 */
#define AL_ETH_PCI_VENDOR	0x1c36
#define AL_ETH_PCI_DEV_1G	0x0001
#define AL_ETH_BAR_UDMA		PCI_BASE_ADDRESS_0	/* AL_ETH_UDMA_BAR = 0 */
#define AL_ETH_BAR_EC		PCI_BASE_ADDRESS_4	/* AL_ETH_EC_BAR   = 4 */
#define AL_ETH_BAR_MAC		PCI_BASE_ADDRESS_2	/* AL_ETH_MAC_BAR  = 2 */

/* Ring sizing - one TX + one RX queue, mirror of the vendor driver. */
#define AL_ETH_RX_BUFFERS	PKTBUFSRX
#define AL_ETH_DESCS_PER_Q	(AL_ETH_RX_BUFFERS + 1)
#define AL_ETH_CDESC_SIZE	16
#define AL_ETH_Q_DESCS_SIZE	(AL_ETH_DESCS_PER_Q * AL_ETH_CDESC_SIZE)

#define AL_ETH_TX_SDESC_OFF	(0 * AL_ETH_Q_DESCS_SIZE)
#define AL_ETH_TX_CDESC_OFF	(1 * AL_ETH_Q_DESCS_SIZE)
#define AL_ETH_RX_SDESC_OFF	(2 * AL_ETH_Q_DESCS_SIZE)
#define AL_ETH_RX_CDESC_OFF	(3 * AL_ETH_Q_DESCS_SIZE)
#define AL_ETH_DESC_BLOCK_SIZE	(4 * AL_ETH_Q_DESCS_SIZE)

/* Poll bound for a single TX completion. Each spin ~= 1us (udelay(1)); a 1518-byte
 * frame at 1Gbps drains in ~12us, so 100000 (~100ms) is ~8000x worst case - a
 * generous cap that names itself in the log rather than hanging. HARDWARE-TODO:
 * tighten once real drain time is measured on the box. */
#define AL_ETH_TX_POLL_MAX	100000

/* Confirmed 1G board params (coordinator / live dmesg): AR8031/8033 (at803x) on
 * MDIO addr 4, MDIO 1000 kHz, eth ref clk 500 MHz. Set EXPLICITLY here rather than
 * via al_eth_board_params_get (MAC scratchpad) or the unused DT serdes/phy props -
 * robust for both chainload and standalone. */
#define AL_ETH_DEFAULT_PHY_ADDR	4
#define AL_ETH_MDIO_CLK_KHZ	1000
#define AL_ETH_REF_CLK		AL_ETH_REF_FREQ_500_MHZ

struct al_eth_dm_priv {
	struct al_hal_eth_adapter	adapter;
	void __iomem			*udma_regs;
	void __iomem			*ec_regs;
	void __iomem			*mac_regs;

	struct al_udma_q		*tx_q;
	struct al_udma_q		*rx_q;

	void				*desc_block;	/* descriptor rings */
	uchar				*rx_buf[AL_ETH_RX_BUFFERS];
	unsigned int			rx_head;	/* next RX buffer to reclaim */

	struct mii_dev			*mdio_bus;
	struct phy_device		*phy;
	unsigned int			phy_addr;
	phy_interface_t			phy_if;

	bool				started;
	char				name[16];
};

static int al_eth_dm_free_pkt(struct udevice *dev, uchar *packet, int length);
static int al_eth_dm_free_pkt_noop(struct udevice *dev);

/* ---- MDIO bus backed by the HAL clause-22 accessors -------------------- */

static int al_eth_dm_mdio_read(struct mii_dev *bus, int addr, int devad, int reg)
{
	struct al_eth_dm_priv *priv = bus->priv;
	uint16_t val = 0;
	int err;

	err = al_eth_mdio_read(&priv->adapter, addr, MDIO_DEVAD_NONE, reg, &val);
	if (err)
		return err;
	return val;
}

static int al_eth_dm_mdio_write(struct mii_dev *bus, int addr, int devad,
				int reg, u16 val)
{
	struct al_eth_dm_priv *priv = bus->priv;

	return al_eth_mdio_write(&priv->adapter, addr, MDIO_DEVAD_NONE, reg, val);
}

/* ---- DMA / adapter bring-up (mirror of vendor al_eth_dev_init) --------- */

static int al_eth_dm_dma_init(struct udevice *dev)
{
	struct al_eth_dm_priv *priv = dev_get_priv(dev);
	uintptr_t base = (uintptr_t)priv->desc_block;
	struct al_eth_adapter_params ap;
	struct al_udma_q_params txp, rxp;
	int i, err;

	memset(&ap, 0, sizeof(ap));
	ap.rev_id = AL_ETH_REV_ID_2;			/* Alpine V2 basic */
	ap.dev_id = AL_ETH_DEV_ID_STANDARD;		/* 1c36:0001 */
	ap.udma_id = 0;
	ap.enable_rx_parser = 0;
	ap.udma_regs_base = priv->udma_regs;
	ap.ec_regs_base = priv->ec_regs;
	ap.mac_regs_base = priv->mac_regs;
	ap.mac_common_regs = NULL;			/* MAC ver < 4 */
	ap.eth_common_regs_base = NULL;			/* rev < 4 */
	ap.name = priv->name;
	ap.serdes_lane = 0;
	ap.unit_adapter = NULL;	/* board_late_init already did PCI/snoop init */
	ap.common_mode = AL_ETH_COMMON_MODE_INVALID;

	err = al_eth_adapter_init(&priv->adapter, &ap);
	if (err) {
		dev_err(dev, "al_eth_adapter_init failed: %d\n", err);
		return err;
	}

	memset(&txp, 0, sizeof(txp));
	txp.size = AL_ETH_DESCS_PER_Q;
	txp.cdesc_size = AL_ETH_CDESC_SIZE;
	txp.adapter_rev_id = AL_ETH_REV_ID_2;
	txp.desc_base = (union al_udma_desc *)(base + AL_ETH_TX_SDESC_OFF);
	txp.desc_phy_base = base + AL_ETH_TX_SDESC_OFF;
	txp.cdesc_base = (uint8_t *)(base + AL_ETH_TX_CDESC_OFF);
	txp.cdesc_phy_base = base + AL_ETH_TX_CDESC_OFF;
	memset(txp.cdesc_base, 0, AL_ETH_Q_DESCS_SIZE);

	memset(&rxp, 0, sizeof(rxp));
	rxp.size = AL_ETH_DESCS_PER_Q;
	rxp.cdesc_size = AL_ETH_CDESC_SIZE;
	rxp.adapter_rev_id = AL_ETH_REV_ID_2;
	rxp.desc_base = (union al_udma_desc *)(base + AL_ETH_RX_SDESC_OFF);
	rxp.desc_phy_base = base + AL_ETH_RX_SDESC_OFF;
	rxp.cdesc_base = (uint8_t *)(base + AL_ETH_RX_CDESC_OFF);
	rxp.cdesc_phy_base = base + AL_ETH_RX_CDESC_OFF;
	memset(rxp.cdesc_base, 0, AL_ETH_Q_DESCS_SIZE);

	al_eth_queue_config(&priv->adapter, UDMA_TX, 0, &txp);
	al_eth_queue_enable(&priv->adapter, UDMA_TX, 0);
	al_eth_queue_config(&priv->adapter, UDMA_RX, 0, &rxp);
	al_eth_queue_enable(&priv->adapter, UDMA_RX, 0);
	al_udma_q_handle_get(&priv->adapter.tx_udma, 0, &priv->tx_q);
	al_udma_q_handle_get(&priv->adapter.rx_udma, 0, &priv->rx_q);

	/* RGMII 1G, external AR8033 PHY drives the link. */
	al_eth_mac_config(&priv->adapter, AL_ETH_MAC_MODE_RGMII);
	al_eth_mac_link_config(&priv->adapter,
			       AL_FALSE,	/* not 1000base-x (RGMII copper) */
			       AL_TRUE,		/* let PHY autoneg drive it */
			       1000, AL_TRUE);
	al_eth_rx_pkt_limit_config(&priv->adapter, 30, 1518);

	/* prime the RX ring */
	priv->rx_head = 0;
	for (i = 0; i < AL_ETH_RX_BUFFERS; i++) {
		struct al_buf buf = {
			.addr = (al_phys_addr_t)(uintptr_t)priv->rx_buf[i],
			.len = PKTSIZE_ALIGN,
		};

		err = al_eth_rx_buffer_add(priv->rx_q, &buf,
					   AL_ETH_RX_FLAGS_INT, NULL);
		if (err) {
			dev_err(dev, "rx_buffer_add[%d] failed: %d\n", i, err);
			return err;
		}
	}
	al_eth_rx_buffer_action(priv->rx_q, AL_ETH_RX_BUFFERS);

	al_eth_mac_start(&priv->adapter);

	/* MDIO clock config, then bring the PHY up via phylib. */
	err = al_eth_mdio_config(&priv->adapter, AL_ETH_MDIO_TYPE_CLAUSE_22,
				 AL_TRUE, AL_ETH_REF_CLK, AL_ETH_MDIO_CLK_KHZ);
	if (err) {
		dev_err(dev, "al_eth_mdio_config failed: %d\n", err);
		return err;
	}

	return 0;
}

/* ---- eth_ops ----------------------------------------------------------- */

static int al_eth_dm_start(struct udevice *dev)
{
	struct al_eth_dm_priv *priv = dev_get_priv(dev);
	int err;

	if (priv->started)
		return 0;

	err = al_eth_dm_dma_init(dev);
	if (err)
		return err;

	priv->phy = phy_connect(priv->mdio_bus, priv->phy_addr, dev,
				priv->phy_if);
	if (!priv->phy) {
		dev_err(dev, "phy_connect failed (addr %u)\n", priv->phy_addr);
		return -ENODEV;
	}
	priv->phy->supported &= (SUPPORTED_10baseT_Half | SUPPORTED_10baseT_Full |
				 SUPPORTED_100baseT_Half | SUPPORTED_100baseT_Full |
				 SUPPORTED_1000baseT_Full);
	priv->phy->advertising = priv->phy->supported;

	err = phy_config(priv->phy);
	if (err) {
		dev_err(dev, "phy_config failed: %d\n", err);
		return err;
	}
	err = phy_startup(priv->phy);
	if (err) {
		dev_err(dev, "phy_startup failed: %d\n", err);
		return err;
	}
	if (!priv->phy->link) {
		dev_err(dev, "no link\n");
		return -EIO;
	}

	/* HARDWARE-TODO: re-config the MAC to the PHY-negotiated speed/duplex
	 * (al_eth_mac_link_config) - the vendor driver left this as a TODO too. */
	priv->started = true;
	return 0;
}

static int al_eth_dm_send(struct udevice *dev, void *packet, int length)
{
	struct al_eth_dm_priv *priv = dev_get_priv(dev);
	struct al_eth_pkt pkt;
	int ndesc, done, poll = 0;

	memset(&pkt, 0, sizeof(pkt));
	pkt.num_of_bufs = 1;
	pkt.bufs[0].addr = (al_phys_addr_t)(uintptr_t)packet;
	pkt.bufs[0].len = length;

	ndesc = al_eth_tx_pkt_prepare(priv->tx_q, &pkt);
	if (!ndesc) {
		dev_err(dev, "tx_pkt_prepare produced 0 descriptors\n");
		return -EIO;
	}
	al_eth_tx_dma_action(priv->tx_q, ndesc);

	while (ndesc) {
		done = al_eth_comp_tx_get(priv->tx_q);
		ndesc -= done;
		if (!ndesc)
			break;
		udelay(1);
		if (++poll >= AL_ETH_TX_POLL_MAX) {
			dev_err(dev, "TX completion timeout: %d descs left after %d us\n",
				ndesc, poll);
			/* flush the TX datapath so a later send can recover */
			al_eth_mac_tx_flush_config(&priv->adapter, AL_TRUE);
			al_eth_mac_tx_flush_config(&priv->adapter, AL_FALSE);
			return -ETIMEDOUT;
		}
	}
	return 0;
}

static int al_eth_dm_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct al_eth_dm_priv *priv = dev_get_priv(dev);
	struct al_eth_pkt pkt;
	int ndesc;

	memset(&pkt, 0, sizeof(pkt));
	ndesc = al_eth_pkt_rx(priv->rx_q, &pkt);
	if (!ndesc)
		return -EAGAIN;

	if (pkt.flags & AL_ETH_RX_ERROR) {
		/* drop: recycle this buffer immediately */
		al_eth_dm_free_pkt_noop(dev);
		return -EIO;
	}

	*packetp = priv->rx_buf[priv->rx_head];
	return pkt.bufs[0].len;
}

static int al_eth_dm_free_pkt(struct udevice *dev, uchar *packet, int length)
{
	struct al_eth_dm_priv *priv = dev_get_priv(dev);
	struct al_buf buf;
	int err;

	buf.addr = (al_phys_addr_t)(uintptr_t)priv->rx_buf[priv->rx_head];
	buf.len = PKTSIZE_ALIGN;

	err = al_eth_rx_buffer_add(priv->rx_q, &buf, AL_ETH_RX_FLAGS_INT, NULL);
	if (err) {
		dev_err(dev, "rx_buffer_add (recycle) failed: %d\n", err);
		return err;
	}
	al_eth_rx_buffer_action(priv->rx_q, 1);

	if (++priv->rx_head == AL_ETH_RX_BUFFERS)
		priv->rx_head = 0;
	return 0;
}

static void al_eth_dm_stop(struct udevice *dev)
{
	struct al_eth_dm_priv *priv = dev_get_priv(dev);

	if (!priv->started)
		return;

	if (priv->phy)
		phy_shutdown(priv->phy);

	al_eth_mac_stop(&priv->adapter);
	udelay(10);	/* let the MAC FIFO drain (~10KB) into memory */
	al_eth_adapter_stop(&priv->adapter);
	priv->started = false;
}

static int al_eth_dm_write_hwaddr(struct udevice *dev)
{
	struct al_eth_dm_priv *priv = dev_get_priv(dev);
	struct eth_pdata *pdata = dev_get_plat(dev);

	return al_eth_mac_addr_store(priv->ec_regs, 0, pdata->enetaddr);
}

/* recv() drop-path helper: recycle the current RX buffer without handing it up. */
static int al_eth_dm_free_pkt_noop(struct udevice *dev)
{
	return al_eth_dm_free_pkt(dev, NULL, 0);
}

static const struct eth_ops al_eth_dm_ops = {
	.start		= al_eth_dm_start,
	.send		= al_eth_dm_send,
	.recv		= al_eth_dm_recv,
	.free_pkt	= al_eth_dm_free_pkt,
	.stop		= al_eth_dm_stop,
	.write_hwaddr	= al_eth_dm_write_hwaddr,
};

/* ---- bind / probe ------------------------------------------------------ */

static int al_eth_dm_probe(struct udevice *dev)
{
	struct al_eth_dm_priv *priv = dev_get_priv(dev);
	int i, err;

	strlcpy(priv->name, dev->name, sizeof(priv->name));

	/* Enable memory space + bus-mastering (DMA) on the endpoint. Our internal
	 * PCIe self-enumeration may not have set MASTER; the vendor driver set it
	 * explicitly too. */
	dm_pci_clrset_config16(dev, PCI_COMMAND, 0,
			       PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);

	/* Three separate BARs -> UDMA / EC / MAC register windows. */
	priv->udma_regs = dm_pci_map_bar(dev, AL_ETH_BAR_UDMA, 0, 0,
					 PCI_REGION_TYPE, PCI_REGION_MEM);
	priv->ec_regs = dm_pci_map_bar(dev, AL_ETH_BAR_EC, 0, 0,
				       PCI_REGION_TYPE, PCI_REGION_MEM);
	priv->mac_regs = dm_pci_map_bar(dev, AL_ETH_BAR_MAC, 0, 0,
					PCI_REGION_TYPE, PCI_REGION_MEM);
	if (!priv->udma_regs || !priv->ec_regs || !priv->mac_regs) {
		dev_err(dev, "failed to map BARs (udma=%p ec=%p mac=%p)\n",
			priv->udma_regs, priv->ec_regs, priv->mac_regs);
		return -EINVAL;
	}

	/* PHY on the internal MDIO bus - confirmed hardware-of-record: RGMII,
	 * AR8031/8033 (at803x) @ addr 4. No DT node backs this PCI endpoint. */
	priv->phy_if = PHY_INTERFACE_MODE_RGMII;
	priv->phy_addr = AL_ETH_DEFAULT_PHY_ADDR;

	/* descriptor rings - 1:1 mapped DRAM, so virt == phys. */
	priv->desc_block = memalign(ARCH_DMA_MINALIGN,
				    ALIGN(AL_ETH_DESC_BLOCK_SIZE, ARCH_DMA_MINALIGN));
	if (!priv->desc_block)
		return -ENOMEM;
	memset(priv->desc_block, 0, AL_ETH_DESC_BLOCK_SIZE);

	for (i = 0; i < AL_ETH_RX_BUFFERS; i++) {
		priv->rx_buf[i] = memalign(ARCH_DMA_MINALIGN,
					   ALIGN(PKTSIZE_ALIGN, ARCH_DMA_MINALIGN));
		if (!priv->rx_buf[i])
			return -ENOMEM;
	}

	priv->mdio_bus = mdio_alloc();
	if (!priv->mdio_bus)
		return -ENOMEM;
	priv->mdio_bus->read = al_eth_dm_mdio_read;
	priv->mdio_bus->write = al_eth_dm_mdio_write;
	priv->mdio_bus->priv = priv;
	strlcpy(priv->mdio_bus->name, dev->name, sizeof(priv->mdio_bus->name));
	err = mdio_register(priv->mdio_bus);
	if (err) {
		dev_err(dev, "mdio_register failed: %d\n", err);
		return err;
	}

	return 0;
}

U_BOOT_DRIVER(al_eth_dm) = {
	.name		= "al_eth_dm",
	.id		= UCLASS_ETH,
	.probe		= al_eth_dm_probe,
	.ops		= &al_eth_dm_ops,
	.priv_auto	= sizeof(struct al_eth_dm_priv),
	.plat_auto	= sizeof(struct eth_pdata),
};

/* Bind by PCI ID (internal PCIe EP), not a DT compatible. eth1 = 1G RJ45. */
static struct pci_device_id al_eth_pci_ids[] = {
	{ PCI_DEVICE(AL_ETH_PCI_VENDOR, AL_ETH_PCI_DEV_1G) },
	{ }
};

U_BOOT_PCI_DEVICE(al_eth_dm, al_eth_pci_ids);
