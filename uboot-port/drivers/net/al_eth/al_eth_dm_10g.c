// SPDX-License-Identifier: GPL-2.0-or-later
/* Annapurna Labs al_eth 10G SFP+ (eth2, PCI 1c36:0002) driver - modern U-Boot
 * DM_ETH. 10G-SPECIFIC front-end only: SerDes lane + 10GBASE-R PCS + MAC mode
 * AL_ETH_MAC_MODE_10GbE_Serial + no-PHY link (PCS block-lock).
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 * Derived from Annapurna Labs HAL / vendor U-Boot al_eth.c (Copyright (C)
 * Annapurna Labs Ltd, GPLv2 OR BSD-3-Clause); reimplemented on U-Boot primitives.
 *
 * Scope:
 * - eth2 shares the SAME UDMA/EC datapath as eth1 (al_eth_dm.c). The UDMA glue
 *   here (rings, FLR, snoop, send/recv, cache maintenance) is a MIRROR of that
 *   file - DEDUP into a shared al_eth_dm_core when #90 settles which arm wins.
 * - 10G deltas vs eth1: dev_id ADVANCED, SerDes/PCS bring-up, MAC mode
 *   10GbE_Serial, and no external PHY (link = PCS block-lock, not MDIO AN).
 * - Lane bring-up lives in drivers/phy/al_serdes (CONFIG_AL_SERDES), called by
 *   extern prototype so no cross-directory -I is needed.
 * - Fixed 10.3125 Gbps, KR AN + LT off, so no al_hal_eth_kr.c / LM FSM: the
 *   10GbE_Serial path is served entirely by al_hal_eth_mac_v1_v2.c (rev 2).
 *
 * Passes NO traffic until #90 (M2S never completes a TX descriptor) is fixed -
 * the same bug eth1 hits. Link bring-up is testable without traffic:
 * `eth diag 2` prints lane, PCS lock and the TX equalisation taps in force.
 */

#include <dm.h>
#include <errno.h>
#include <malloc.h>
#include <memalign.h>
#include <net.h>
#include <pci.h>
#include <cpu_func.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <dm/device_compat.h>

#include <al_hal_eth.h>
#include <al_hal_udma.h>

#include "al_eth_boardparams.h"
#include "al_eth_hwaddr.h"
#include "al_eth_rxfwd.h"

/* eth2 = the "advanced" al_eth function on the internal PCIe (vendor 0x1c36 dev
 * 0x0002). Same three-BAR layout as eth1 (UDMA=BAR0, EC=BAR4, MAC=BAR2). */
#define AL_ETH_PCI_VENDOR	0x1c36
#define AL_ETH_PCI_DEV_10G	0x0002		/* al_eth2 SFP+ cage */
#define AL_ETH_10G_PORT		2		/* stock eth_* port index (al_eth_port.c) */
#define AL_ETH_BAR_UDMA		PCI_BASE_ADDRESS_0	/* AL_ETH_UDMA_BAR = 0 */
#define AL_ETH_BAR_EC		PCI_BASE_ADDRESS_4	/* AL_ETH_EC_BAR   = 4 */
#define AL_ETH_BAR_MAC		PCI_BASE_ADDRESS_2	/* AL_ETH_MAC_BAR  = 2 */

/* Ring sizing - identical to eth1 (al_eth_dm.c). Power-of-2, >= AL_UDMA_MIN_Q_SIZE. */
#define AL_ETH_RX_BUFFERS	PKTBUFSRX
#define AL_ETH_DESCS_PER_Q	32
#define AL_ETH_CDESC_SIZE	16
#define AL_ETH_Q_DESCS_SIZE	(AL_ETH_DESCS_PER_Q * AL_ETH_CDESC_SIZE)

#define AL_ETH_TX_SDESC_OFF	(0 * AL_ETH_Q_DESCS_SIZE)
#define AL_ETH_TX_CDESC_OFF	(1 * AL_ETH_Q_DESCS_SIZE)
#define AL_ETH_RX_SDESC_OFF	(2 * AL_ETH_Q_DESCS_SIZE)
#define AL_ETH_RX_CDESC_OFF	(3 * AL_ETH_Q_DESCS_SIZE)
#define AL_ETH_DESC_BLOCK_SIZE	(4 * AL_ETH_Q_DESCS_SIZE)

/* One TX completion: 1us/spin, so 100000 = ~100ms. A 1518B frame at 10Gbps
 * drains in ~1.2us, making this ~80000x worst case - far past the repo's 1.25x
 * rule, kept only until #90 makes a completion observable enough to measure.
 * On expiry: log descs-left + elapsed, flush the MAC TX, fail the send. */
#define AL_ETH_TX_POLL_MAX	100000

/* PCS block-lock after lane bring-up: 1ms/iter, so 3000 = ~3s. Matches stock's
 * LM budget (~30 x 100ms) with a partner already up. On expiry: warn "no 10G
 * link" and continue - a later start() retries. */
#define AL_ETH_10G_LINK_POLL_MAX	3000

/* al_eth "advanced" function id for the 10G port (al_hal_eth.h:111). */
#ifndef AL_ETH_DEV_ID_ADVANCED
#define AL_ETH_DEV_ID_ADVANCED	0x0002
#endif

/* SerDes/PCS bring-up from drivers/phy/al_serdes (CONFIG_AL_SERDES). Referenced
 * by extern prototype so we need no cross-directory include path. */
#if IS_ENABLED(CONFIG_AL_SERDES)
extern int al_serdes_10g_init(void);
extern void al_serdes_10g_status(void);
#else
static inline int al_serdes_10g_init(void) { return -ENOSYS; }
static inline void al_serdes_10g_status(void) { }
#endif

struct al_eth_10g_priv {
	struct al_hal_eth_adapter	adapter;
	void __iomem			*udma_regs;
	void __iomem			*ec_regs;
	void __iomem			*mac_regs;

	struct al_udma_q		*tx_q;
	struct al_udma_q		*rx_q;

	void				*desc_block;	/* low DRAM (see al_eth_10g_dma_low_alloc) */
	uchar				*rx_buf[AL_ETH_RX_BUFFERS];	/* low DRAM */
	uchar				*tx_bounce;	/* low-DRAM TX bounce buffer */
	unsigned int			rx_head;

	bool				started;
	char				name[16];
};

/* ---- cache maintenance (mirror of al_eth_dm.c) ------------------------- */

static void al_eth_cache_flush(void *p, size_t len)
{
	uintptr_t s = rounddown((uintptr_t)p, ARCH_DMA_MINALIGN);
	uintptr_t e = roundup((uintptr_t)p + len, ARCH_DMA_MINALIGN);

	flush_dcache_range(s, e);
}

static void al_eth_cache_inval(void *p, size_t len)
{
	uintptr_t s = rounddown((uintptr_t)p, ARCH_DMA_MINALIGN);
	uintptr_t e = roundup((uintptr_t)p + len, ARCH_DMA_MINALIGN);

	invalidate_dcache_range(s, e);
}

/* PCI config accessors for al_eth_flr_rmn (mirror of al_eth_dm.c). */
static int al_eth_10g_cfg_read(void *handle, int where, uint32_t *val)
{
	return dm_pci_read_config32((struct udevice *)handle, where, val);
}

static int al_eth_10g_cfg_write(void *handle, int where, uint32_t val)
{
	return dm_pci_write_config32((struct udevice *)handle, where, val);
}

/* Low-DRAM allocator for al_udma-visible memory (see al_eth_dm.c). Own window
 * at 0x02100000 so it does not collide with the 1G driver's 0x02000000. */
#define AL_ETH_10G_DMA_LOW_BASE	0x02100000UL
#define AL_ETH_10G_DMA_LOW_SIZE	0x00100000UL
static uintptr_t al_eth_10g_dma_low_next = AL_ETH_10G_DMA_LOW_BASE;

static void *al_eth_10g_dma_low_alloc(size_t size)
{
	uintptr_t p = ALIGN(al_eth_10g_dma_low_next, ARCH_DMA_MINALIGN);
	uintptr_t end = p + ALIGN(size, ARCH_DMA_MINALIGN);

	if (end > AL_ETH_10G_DMA_LOW_BASE + AL_ETH_10G_DMA_LOW_SIZE)
		return NULL;
	al_eth_10g_dma_low_next = end;
	return (void *)p;
}

/* Replicate al_unit_adapter_init() on THIS function's PCI config space (mirror
 * of al_eth_dm_unit_adapter_setup in al_eth_dm.c): snoop, error-track, ROB. All
 * three are reset by FLR/adapter_init, so they run post-adapter_init here.
 * NB error_track is DISABLED here and left ENABLED on eth1 - eth1 needs the
 * latched AXI-error attributes for the #90 dump. */
#define AL_ETH_SMCC		0x110
#define AL_ETH_SMCC_BUNDLE	0x20
#define AL_ETH_SMCC_SNOOP	0x3	/* SNOOP_OVR|SNOOP_EN */
#define AL_ETH_SMCC_CONF_2	0x114
#define AL_ETH_DIS_ERR_TRACK	0x100	/* SMCC_CONF_2 bit 8 */
#define AL_ETH_APP_CONTROL	0x220
#define AL_ETH_APP_LO16		0x3ff
#define AL_ETH_GEN_CTL_19	0x240	/* ROB config */
#define AL_ETH_RD_ROB_EN	0x00000001	/* bit 0 */
#define AL_ETH_WR_ROB_EN	0x00010000	/* bit 16 */
#define AL_ETH_RD_ROB_RESET	0x00008000	/* bit 15 */
#define AL_ETH_WR_ROB_RESET	0x80000000	/* bit 31 */

static void al_eth_10g_snoop_enable(struct udevice *dev)
{
	u32 v;
	int i;

	/* 1. snoop_enable */
	dm_pci_read_config32(dev, AL_ETH_SMCC, &v);
	v |= AL_ETH_SMCC_SNOOP;
	dm_pci_write_config32(dev, AL_ETH_SMCC, v);
	for (i = 1; i < 4; i++)
		dm_pci_write_config32(dev, AL_ETH_SMCC + i * AL_ETH_SMCC_BUNDLE, v);

	dm_pci_read_config32(dev, AL_ETH_APP_CONTROL, &v);
	v = (v & 0xffff0000) | AL_ETH_APP_LO16;
	dm_pci_write_config32(dev, AL_ETH_APP_CONTROL, v);

	/* 2. error_track: disable (func 0) */
	dm_pci_read_config32(dev, AL_ETH_SMCC_CONF_2, &v);
	v |= AL_ETH_DIS_ERR_TRACK;
	dm_pci_write_config32(dev, AL_ETH_SMCC_CONF_2, v);

	/* 3. rob_cfg: reset both ROBs, then enable read+write ROB */
	dm_pci_read_config32(dev, AL_ETH_GEN_CTL_19, &v);
	v |= AL_ETH_RD_ROB_RESET | AL_ETH_WR_ROB_RESET;
	dm_pci_write_config32(dev, AL_ETH_GEN_CTL_19, v);
	v &= ~(AL_ETH_RD_ROB_RESET | AL_ETH_WR_ROB_RESET);
	dm_pci_write_config32(dev, AL_ETH_GEN_CTL_19, v);
	v |= AL_ETH_RD_ROB_EN | AL_ETH_WR_ROB_EN;
	dm_pci_write_config32(dev, AL_ETH_GEN_CTL_19, v);
}

/* ---- 10G link-up gate: 10GBASE-R PCS block-lock ------------------------ */

/* Poll the MAC's KR-PCS BASE-R status (via al_eth_link_status_get, served by
 * al_hal_eth_mac_v1_v2.c for 10GbE_Serial) until link_up, or the bound expires.
 * No MDIO/phylib: the SFP optic drives the line and the PCS reports block-lock.
 * Returns 0 on link, -ETIMEDOUT otherwise. */
static int al_eth_10g_wait_link(struct udevice *dev)
{
	struct al_eth_10g_priv *priv = dev_get_priv(dev);
	struct al_eth_link_status status;
	int poll = 0;

	do {
		memset(&status, 0, sizeof(status));
		if (!al_eth_link_status_get(&priv->adapter, &status) &&
		    status.link_up)
			return 0;
		udelay(1000);	/* 1ms/iter; bound = AL_ETH_10G_LINK_POLL_MAX ms */
	} while (++poll < AL_ETH_10G_LINK_POLL_MAX);

	dev_warn(dev, "no 10G link after %d ms (PCS block-lock not set)\n", poll);
	return -ETIMEDOUT;
}

/* ---- DMA / adapter + 10G front-end bring-up ---------------------------- */

static int al_eth_10g_dma_init(struct udevice *dev)
{
	struct al_eth_10g_priv *priv = dev_get_priv(dev);
	uintptr_t base = (uintptr_t)priv->desc_block;
	struct al_eth_adapter_params ap;
	struct al_udma_q_params txp, rxp;
	int i, err;

	memset(&ap, 0, sizeof(ap));
	ap.rev_id = AL_ETH_REV_ID_2;		/* Alpine V2 basic (rev 2) */
	ap.dev_id = AL_ETH_DEV_ID_ADVANCED;	/* 10G: 1c36:0002 */
	ap.udma_id = 0;
	ap.enable_rx_parser = 0;
	ap.udma_regs_base = priv->udma_regs;
	ap.ec_regs_base = priv->ec_regs;
	ap.mac_regs_base = priv->mac_regs;
	ap.mac_common_regs = NULL;		/* MAC ver < 4 */
	ap.eth_common_regs_base = NULL;		/* rev < 4 */
	ap.name = priv->name;
	/* HSSP group D (serdes-grp 3), lane 0 - the DT is hardware-of-record and
	 * the group-D retarget is what stopped the SError (commit eac280a). Stock's
	 * CODE default (3 - port_idx = lane 1) is wrong for this board. */
	ap.serdes_lane = 0;
	ap.unit_adapter = NULL;	/* board_late_init already did PCI/snoop init */
	ap.common_mode = AL_ETH_COMMON_MODE_INVALID;

	/* FLR to a clean state before init (mirror of al_eth_dm.c - THE missing
	 * step that otherwise wedges UDMA TX after chainload). */
	al_eth_flr_rmn(al_eth_10g_cfg_read, al_eth_10g_cfg_write, dev,
		       priv->mac_regs);

	err = al_eth_adapter_init(&priv->adapter, &ap);
	if (err) {
		dev_err(dev, "al_eth_adapter_init failed: %d\n", err);
		return err;
	}

	/* Re-enable DMA snoop post-FLR/adapter-init (they clear it). */
	al_eth_10g_snoop_enable(dev);

	/* SerDes lane before MAC config, so the electrical lane is live when the
	 * MAC selects the KR mux (stock's order). PCS has a single owner - this
	 * driver, via al_eth_mac_config below; al_serdes_10g_init() only does the
	 * lane (group D fixed-10G + optic EQ) and no longer touches the PCS. */
	err = al_serdes_10g_init();
	if (err)
		dev_warn(dev, "al_serdes_10g_init failed: %d (continuing; MAC may still mux)\n",
			 err);

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

	/* Steer RX to UDMA0/Q0. Without it the EC drops every frame before the
	 * S2M ring - TX went out, replies never arrived (#234). */
	al_eth_rxfwd_config(&priv->adapter);

	/* MAC mode 10GbE_Serial, served entirely by al_hal_eth_mac_v1_v2.c - no
	 * serdes/KR closure. NOT al_eth_mac_link_config: stock skips it here (that
	 * path is SGMII AN only) and speed is fixed, driven by the SerDes/PCS. */
	err = al_eth_mac_config(&priv->adapter, AL_ETH_MAC_MODE_10GbE_Serial);
	if (err) {
		dev_err(dev, "al_eth_mac_config(10GbE_Serial) failed: %d\n", err);
		return err;
	}
	al_eth_rx_pkt_limit_config(&priv->adapter, 30, 1518);

	/* prime the RX ring (mirror of al_eth_dm.c) */
	priv->rx_head = 0;
	for (i = 0; i < AL_ETH_RX_BUFFERS; i++) {
		struct al_buf buf = {
			.addr = (al_phys_addr_t)(uintptr_t)priv->rx_buf[i],
			.len = PKTSIZE_ALIGN,
		};

		err = al_eth_rx_buffer_add(priv->rx_q, &buf,
					   AL_ETH_RX_FLAGS_INT |
					   AL_ETH_RX_FLAGS_NO_SNOOP, NULL);
		if (err) {
			dev_err(dev, "rx_buffer_add[%d] failed: %d\n", i, err);
			return err;
		}
	}
	al_eth_rx_buffer_action(priv->rx_q, AL_ETH_RX_BUFFERS);

	for (i = 0; i < AL_ETH_RX_BUFFERS; i++)
		al_eth_cache_inval(priv->rx_buf[i], PKTSIZE_ALIGN);
	al_eth_cache_flush(priv->desc_block, AL_ETH_DESC_BLOCK_SIZE);

	err = al_eth_mac_start(&priv->adapter);
	if (err) {
		/* Was ignored: the MAC's TX/RX enable can fail (-ENOSYS for a
		 * missing mac_start_stop_adv, or the vtable's own error) and the
		 * UDMA was then kicked at a MAC that never enabled its transmit
		 * path - descriptors consumed, no completion ever posted. */
		dev_err(dev, "al_eth_mac_start failed: %d\n", err);
		return err;
	}

	return 0;
}

/* ---- eth_ops ----------------------------------------------------------- */

static int al_eth_10g_start(struct udevice *dev)
{
	struct al_eth_10g_priv *priv = dev_get_priv(dev);
	int err;

	if (priv->started)
		return 0;

	err = al_eth_10g_dma_init(dev);
	if (err)
		return err;

	/* No phylib: link = 10GBASE-R PCS block-lock. start() does NOT hard-fail on
	 * no-link, so the port is probeable with nothing plugged; a send then just
	 * does not complete. OPEN: whether that should instead fail start() waits
	 * on #90 - today every send fails anyway, so no-link is indistinguishable. */
	if (al_eth_10g_wait_link(dev))
		dev_warn(dev, "10G link not up (no SFP/partner?) - continuing\n");
	else if (IS_ENABLED(CONFIG_AL_SERDES))
		al_serdes_10g_status();		/* dump lane + PCS lock on link */

	priv->started = true;
	return 0;
}

static int al_eth_10g_send(struct udevice *dev, void *packet, int length)
{
	struct al_eth_10g_priv *priv = dev_get_priv(dev);
	struct al_eth_pkt pkt;
	int ndesc, done, poll = 0;

	memset(&pkt, 0, sizeof(pkt));
	pkt.num_of_bufs = 1;
	/* NO_SNOOP TX - same rationale as eth1 (al_eth_dm.c): the M2S engine reads
	 * descriptor+buffer as plain DRAM reads serviced by our flush, not snoop
	 * reads that stall forever in this chainloaded U-Boot. */
	pkt.flags = AL_ETH_TX_FLAGS_NO_SNOOP;
	/* Bounce TX through low DRAM (caller's packet is high heap, master can't
	 * reach it - see al_eth_10g_dma_low_alloc). */
	if (length > PKTSIZE_ALIGN)
		length = PKTSIZE_ALIGN;
	memcpy(priv->tx_bounce, packet, length);
	pkt.bufs[0].addr = (al_phys_addr_t)(uintptr_t)priv->tx_bounce;
	pkt.bufs[0].len = length;

	ndesc = al_eth_tx_pkt_prepare(priv->tx_q, &pkt);
	if (!ndesc) {
		dev_err(dev, "tx_pkt_prepare produced 0 descriptors\n");
		return -EIO;
	}
	al_eth_cache_flush(priv->tx_bounce, length);
	al_eth_cache_flush(priv->desc_block, AL_ETH_DESC_BLOCK_SIZE);
	al_eth_tx_dma_action(priv->tx_q, ndesc);

	while (ndesc) {
		al_eth_cache_inval(priv->desc_block, AL_ETH_DESC_BLOCK_SIZE);
		done = al_eth_comp_tx_get(priv->tx_q);
		ndesc -= done;
		if (!ndesc)
			break;
		udelay(1);
		if (++poll >= AL_ETH_TX_POLL_MAX) {
			dev_err(dev, "TX completion timeout: %d descs left after %d us\n",
				ndesc, poll);
			al_eth_mac_tx_flush_config(&priv->adapter, AL_TRUE);
			al_eth_mac_tx_flush_config(&priv->adapter, AL_FALSE);
			return -ETIMEDOUT;
		}
	}
	return 0;
}

static int al_eth_10g_free_pkt(struct udevice *dev, uchar *packet, int length);

static int al_eth_10g_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct al_eth_10g_priv *priv = dev_get_priv(dev);
	struct al_eth_pkt pkt;
	int ndesc;

	memset(&pkt, 0, sizeof(pkt));
	al_eth_cache_inval(priv->desc_block, AL_ETH_DESC_BLOCK_SIZE);
	al_eth_cache_inval(priv->rx_buf[priv->rx_head], PKTSIZE_ALIGN);
	ndesc = al_eth_pkt_rx(priv->rx_q, &pkt);
	if (!ndesc)
		return -EAGAIN;

	if (pkt.flags & AL_ETH_RX_ERROR) {
		al_eth_10g_free_pkt(dev, NULL, 0);
		return -EIO;
	}

	*packetp = priv->rx_buf[priv->rx_head];
	return pkt.bufs[0].len;
}

static int al_eth_10g_free_pkt(struct udevice *dev, uchar *packet, int length)
{
	struct al_eth_10g_priv *priv = dev_get_priv(dev);
	struct al_buf buf;
	int err;

	buf.addr = (al_phys_addr_t)(uintptr_t)priv->rx_buf[priv->rx_head];
	buf.len = PKTSIZE_ALIGN;

	al_eth_cache_inval(priv->rx_buf[priv->rx_head], PKTSIZE_ALIGN);

	/* Same flags as the initial priming above - a recycled buffer that
	 * dropped NO_SNOOP would be written back through a different path
	 * than the one our invalidate assumes. */
	err = al_eth_rx_buffer_add(priv->rx_q, &buf,
				   AL_ETH_RX_FLAGS_INT |
				   AL_ETH_RX_FLAGS_NO_SNOOP, NULL);
	if (err) {
		dev_err(dev, "rx_buffer_add (recycle) failed: %d\n", err);
		return err;
	}
	al_eth_rx_buffer_action(priv->rx_q, 1);
	al_eth_cache_flush(priv->desc_block, AL_ETH_DESC_BLOCK_SIZE);

	if (++priv->rx_head == AL_ETH_RX_BUFFERS)
		priv->rx_head = 0;
	return 0;
}

static void al_eth_10g_stop(struct udevice *dev)
{
	struct al_eth_10g_priv *priv = dev_get_priv(dev);

	if (!priv->started)
		return;

	al_eth_mac_stop(&priv->adapter);
	udelay(10);	/* let the MAC FIFO drain into memory */
	al_eth_adapter_stop(&priv->adapter);
	priv->started = false;
}

static int al_eth_10g_write_hwaddr(struct udevice *dev)
{
	struct al_eth_10g_priv *priv = dev_get_priv(dev);
	struct eth_pdata *pdata = dev_get_plat(dev);

	return al_eth_mac_addr_store(priv->ec_regs, 0, pdata->enetaddr);
}

/* 10G MAC = NOR base + 1 (al_eth_hwaddr.c). The eth uclass calls this when env
 * has no ethaddr; probe has already committed the same value to the EC filter. */
static int al_eth_10g_read_rom_hwaddr(struct udevice *dev)
{
	struct eth_pdata *pdata = dev_get_plat(dev);
	int ret = al_eth_hwaddr_get(AL_ETH_10G_PORT, pdata->enetaddr);

	if (ret)
		dev_dbg(dev, "no MAC from NOR: %d\n", ret);
	return ret;
}

static const struct eth_ops al_eth_10g_ops = {
	.start		= al_eth_10g_start,
	.send		= al_eth_10g_send,
	.recv		= al_eth_10g_recv,
	.free_pkt	= al_eth_10g_free_pkt,
	.stop		= al_eth_10g_stop,
	.write_hwaddr	= al_eth_10g_write_hwaddr,
	.read_rom_hwaddr = al_eth_10g_read_rom_hwaddr,
};

/* ---- probe ------------------------------------------------------------- */

static int al_eth_10g_probe(struct udevice *dev)
{
	struct al_eth_10g_priv *priv = dev_get_priv(dev);
	int i;

	strlcpy(priv->name, dev->name, sizeof(priv->name));

	dm_pci_clrset_config16(dev, PCI_COMMAND, 0,
			       PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);

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

	/* Hand the board params to Linux via the MAC scratchpad. Stock U-Boot
	 * does this too; without it Linux al_eth fails probe outright. */
	al_eth_bp_seed(AL_ETH_10G_PORT, priv->mac_regs);

	/* MAC into the EC filter here, not from the uclass's lazy write_hwaddr:
	 * U-Boot never activates this port, so Linux would otherwise inherit the
	 * previous bootloader's wrong value (#222). */
	al_eth_hwaddr_commit(AL_ETH_10G_PORT, priv->ec_regs);

	/* al_udma-visible memory MUST be low DRAM (master can't reach U-Boot's ~3GB
	 * heap - mirrors al_eth_dm.c's al_eth_dma_low_alloc; own 0x02100000 window). */
	priv->desc_block = al_eth_10g_dma_low_alloc(AL_ETH_DESC_BLOCK_SIZE);
	if (!priv->desc_block)
		return -ENOMEM;
	memset(priv->desc_block, 0, AL_ETH_DESC_BLOCK_SIZE);

	for (i = 0; i < AL_ETH_RX_BUFFERS; i++) {
		priv->rx_buf[i] = al_eth_10g_dma_low_alloc(PKTSIZE_ALIGN);
		if (!priv->rx_buf[i])
			return -ENOMEM;
	}

	priv->tx_bounce = al_eth_10g_dma_low_alloc(PKTSIZE_ALIGN);
	if (!priv->tx_bounce)
		return -ENOMEM;

	/* 10G: no MDIO bus / no PHY - the SFP optic + PCS drive the link. */
	return 0;
}

U_BOOT_DRIVER(al_eth_dm_10g) = {
	.name		= "al_eth_dm_10g",
	.id		= UCLASS_ETH,
	.probe		= al_eth_10g_probe,
	.ops		= &al_eth_10g_ops,
	.priv_auto	= sizeof(struct al_eth_10g_priv),
	.plat_auto	= sizeof(struct eth_pdata),
};

/* Bind by PCI ID (internal PCIe EP). eth2 = 10G SFP+ = 1c36:0002. */
static struct pci_device_id al_eth_10g_pci_ids[] = {
	{ PCI_DEVICE(AL_ETH_PCI_VENDOR, AL_ETH_PCI_DEV_10G) },
	{ }
};

U_BOOT_PCI_DEVICE(al_eth_dm_10g, al_eth_10g_pci_ids);
