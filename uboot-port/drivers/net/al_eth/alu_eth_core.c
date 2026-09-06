// SPDX-License-Identifier: GPL-2.0-or-later
/* alu_eth datapath core - the UDMA/EC half the 1G and 10G ports share.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * Runs on the SHARED Linux HAL (modules/al_eth, staged by scripts/stage_hal.py),
 * so every fix landed there is already here. Every HAL entry point is
 * __must_check, so every return is handled.
 *
 * DMA coherency: this driver enables SMCC snoop on the eth function itself,
 * post-adapter-init - board_late_init's pass skips eth because the reset would
 * wipe it. Explicit flush/invalidate is kept as a safety net.
 */

#include <dm.h>
#include <errno.h>
#include <memalign.h>
#include <net.h>
#include <pci.h>
#include <stdio.h>
#include <cpu_func.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <asm/io.h>
#include <dm/device_compat.h>

#include <al_hal_eth.h>
#include <al_hal_udma.h>

#include "alu_eth.h"
#include "alu_eth_core.h"

/* One TX completion, 1us per spin. A 1518B frame drains in ~12us at 1Gbps and
 * ~1.2us at 10Gbps, so this is far past the 1.25x rule; it stays only while
 * #253 keeps a 10G completion from being observable enough to measure.
 * On expiry: log descs-left + elapsed, dump the M2S state, fail the send. */
#define ALU_ETH_TX_POLL_MAX	100000

/* RX frame length window handed to the EC. 30 is the vendor minimum; 1518 is a
 * standard untagged frame with FCS. */
#define ALU_ETH_RX_MIN_LEN	30
#define ALU_ETH_RX_MAX_LEN	1518

/* Low DRAM the al_udma master can reach: 1MB at 32MB, below relocated U-Boot,
 * shared by both ports. gd->ram_size stays 3GB. #90. */
#define ALU_ETH_DMA_LOW_BASE	0x02000000UL
#define ALU_ETH_DMA_LOW_SIZE	0x00100000UL
static uintptr_t alu_eth_dma_low_next = ALU_ETH_DMA_LOW_BASE;

/* Unit-adapter registers in this function's PCI config space. */
#define ALU_ETH_SMCC		0x110	/* GEN_CTL: SMCC sub-master 0 */
#define ALU_ETH_SMCC_BUNDLE	0x20
#define ALU_ETH_SMCC_COHERENT	0x3	/* SNOOP_OVR | SNOOP_EN */
#define ALU_ETH_SMCC_CONF_2	0x114
#define ALU_ETH_DIS_ERR_TRACK	0x100	/* SMCC_CONF_2 bit 8 */
#define ALU_ETH_APP_CONTROL	0x220	/* GEN_CTL_11: axcache */
#define ALU_ETH_APP_LO16	0x3ff
#define ALU_ETH_GEN_CTL_19	0x240	/* ROB config */
#define ALU_ETH_RD_ROB_EN	0x00000001
#define ALU_ETH_WR_ROB_EN	0x00010000
#define ALU_ETH_RD_ROB_RESET	0x00008000
#define ALU_ETH_WR_ROB_RESET	0x80000000

/* M2S engine + Q0 ring pointers, straight off BAR0. NEVER read 0x1038
 * (drtp_inc is write-only and data-aborts), nor Q0 cfg/pref/tdrbp
 * (0x1000/0x1020/0x1028..0x1030) for the same reason on this rev.
 * m2s.state nibbles: 1 = NORMAL/active, 2 = ABORT; 0 = healthy idle. */
void alu_eth_dump_tx(struct alu_eth_priv *priv, const char *tag)
{
	void __iomem *u = priv->udma_regs;

	printf("alu_eth[%s]: m2s.state=%08x tx0 drhp=%08x drtp=%08x dcp=%08x crhp=%08x qpkt=%08x\n",
	       tag, readl(u + 0x200), readl(u + 0x1034), readl(u + 0x103c),
	       readl(u + 0x1040), readl(u + 0x104c), readl(u + 0x10c0));
}

/* Unit-adapter AXI-master READ error attributes. addr_to = never routed to a
 * slave; comp_to = routed, no answer. Needs error tracking left ENABLED. */
#define ALU_ETH_RD_ERR_ATTR	0x1B8
#define ALU_ETH_RD_ERR_LO	0x1C8
#define ALU_ETH_RD_ERR_HI	0x1CC
#define ALU_ETH_AXI_MSTR_TO	0x1D0

static void alu_eth_dump_axi_err(struct udevice *dev)
{
	u32 attr = 0, lo = 0, hi = 0, to = 0;

	dm_pci_read_config32(dev, ALU_ETH_RD_ERR_ATTR, &attr);
	dm_pci_read_config32(dev, ALU_ETH_RD_ERR_LO, &lo);
	dm_pci_read_config32(dev, ALU_ETH_RD_ERR_HI, &hi);
	dm_pci_read_config32(dev, ALU_ETH_AXI_MSTR_TO, &to);
	printf("alu_eth[axi-rd-err]: attr=%08x addr_to=%d comp_err=%d comp_to=%d "
	       "err_blk=%d rd_parity=%d mstr_id=%u comp_stat=%u fail_addr=%08x%08x "
	       "mstr_to(rd)=%04x\n",
	       attr, !!(attr & 0x100), !!(attr & 0x200), !!(attr & 0x400),
	       !!(attr & 0x800), !!(attr & 0x1000), (attr >> 2) & 0x7, attr & 0x3,
	       hi, lo, to & 0xffff);
}

/* ---- low-DRAM allocation ---------------------------------------------- */

void *alu_eth_dma_low_alloc(size_t size)
{
	uintptr_t p = ALIGN(alu_eth_dma_low_next, ARCH_DMA_MINALIGN);
	uintptr_t end = p + ALIGN(size, ARCH_DMA_MINALIGN);

	if (end > ALU_ETH_DMA_LOW_BASE + ALU_ETH_DMA_LOW_SIZE)
		return NULL;
	alu_eth_dma_low_next = end;
	return (void *)p;
}

int alu_eth_bufs_alloc(struct alu_eth_priv *priv)
{
	int i;

	priv->desc_block = alu_eth_dma_low_alloc(ALU_ETH_DESC_BLOCK_SIZE);
	if (!priv->desc_block)
		return -ENOMEM;
	memset(priv->desc_block, 0, ALU_ETH_DESC_BLOCK_SIZE);

	for (i = 0; i < ALU_ETH_RX_BUFFERS; i++) {
		priv->rx_buf[i] = alu_eth_dma_low_alloc(PKTSIZE_ALIGN);
		if (!priv->rx_buf[i])
			return -ENOMEM;
	}

	priv->tx_bounce = alu_eth_dma_low_alloc(PKTSIZE_ALIGN);
	return priv->tx_bounce ? 0 : -ENOMEM;
}

/* ---- cache maintenance ------------------------------------------------- */

void alu_eth_cache_flush(void *p, size_t len)
{
	uintptr_t s = rounddown((uintptr_t)p, ARCH_DMA_MINALIGN);
	uintptr_t e = roundup((uintptr_t)p + len, ARCH_DMA_MINALIGN);

	flush_dcache_range(s, e);
}

void alu_eth_cache_inval(void *p, size_t len)
{
	uintptr_t s = rounddown((uintptr_t)p, ARCH_DMA_MINALIGN);
	uintptr_t e = roundup((uintptr_t)p + len, ARCH_DMA_MINALIGN);

	invalidate_dcache_range(s, e);
}

/* ---- unit adapter ------------------------------------------------------ */

void alu_eth_unit_adapter_setup(struct udevice *dev, bool err_track)
{
	u32 v;
	int i;

	/* Coherent AXI master on all four sub-masters, plus axcache - the same
	 * settings the working AHCI uses on this fabric. */
	dm_pci_read_config32(dev, ALU_ETH_SMCC, &v);
	v |= ALU_ETH_SMCC_COHERENT;
	dm_pci_write_config32(dev, ALU_ETH_SMCC, v);
	for (i = 1; i < 4; i++)
		dm_pci_write_config32(dev, ALU_ETH_SMCC + i * ALU_ETH_SMCC_BUNDLE, v);

	dm_pci_read_config32(dev, ALU_ETH_APP_CONTROL, &v);
	v = (v & 0xffff0000) | ALU_ETH_APP_LO16;
	dm_pci_write_config32(dev, ALU_ETH_APP_CONTROL, v);

	/* Error tracking latches the attributes a failed M2S read leaves behind
	 * (alu_eth_dump_axi_err). Stock disables it. */
	dm_pci_read_config32(dev, ALU_ETH_SMCC_CONF_2, &v);
	if (err_track)
		v &= ~ALU_ETH_DIS_ERR_TRACK;
	else
		v |= ALU_ETH_DIS_ERR_TRACK;
	dm_pci_write_config32(dev, ALU_ETH_SMCC_CONF_2, v);

	/* Reset both ROBs, then enable read + write ROB. */
	dm_pci_read_config32(dev, ALU_ETH_GEN_CTL_19, &v);
	v |= ALU_ETH_RD_ROB_RESET | ALU_ETH_WR_ROB_RESET;
	dm_pci_write_config32(dev, ALU_ETH_GEN_CTL_19, v);
	v &= ~(ALU_ETH_RD_ROB_RESET | ALU_ETH_WR_ROB_RESET);
	dm_pci_write_config32(dev, ALU_ETH_GEN_CTL_19, v);
	v |= ALU_ETH_RD_ROB_EN | ALU_ETH_WR_ROB_EN;
	dm_pci_write_config32(dev, ALU_ETH_GEN_CTL_19, v);
}

/* ---- FLR ---------------------------------------------------------------- */

static int alu_eth_cfg_read(void *handle, int where, uint32_t *val)
{
	return dm_pci_read_config32((struct udevice *)handle, where, val);
}

static int alu_eth_cfg_write(void *handle, int where, uint32_t val)
{
	return dm_pci_write_config32((struct udevice *)handle, where, val);
}

/* al_eth_flr_rmn() alone WIPES the MAC scratchpad board params and the EC MAC
 * filter, leaving Linux with "board info not available" and the wrong address
 * (#253). The HAL's own restore wrapper saves and restores both, which is what
 * Linux's al_eth_function_reset() does. */
static int alu_eth_flr(struct udevice *dev, struct alu_eth_priv *priv)
{
	return al_eth_flr_rmn_restore_params(alu_eth_cfg_read, alu_eth_cfg_write,
					     dev, priv->mac_regs, priv->ec_regs,
					     1);
}

/* ---- bring-up ----------------------------------------------------------- */

static void alu_eth_q_params(struct al_udma_q_params *p, uintptr_t base,
			     unsigned int sdesc_slot, unsigned int cdesc_slot)
{
	uintptr_t sdesc = base + sdesc_slot * ALU_ETH_Q_DESCS_SIZE;
	uintptr_t cdesc = base + cdesc_slot * ALU_ETH_Q_DESCS_SIZE;

	memset(p, 0, sizeof(*p));
	p->size = ALU_ETH_DESCS_PER_Q;
	p->cdesc_size = ALU_ETH_CDESC_SIZE;
	p->adapter_rev_id = AL_ETH_REV_ID_2;
	p->desc_base = (union al_udma_desc *)sdesc;
	p->desc_phy_base = sdesc;
	p->cdesc_base = (uint8_t *)cdesc;
	p->cdesc_phy_base = cdesc;
	memset(p->cdesc_base, 0, ALU_ETH_Q_DESCS_SIZE);
}

/* Both the initial priming and every recycle use these flags. A recycled buffer
 * that dropped NO_SNOOP would be written back through a different path than the
 * one our invalidate assumes. */
#define ALU_ETH_RX_FLAGS	(AL_ETH_RX_FLAGS_INT | AL_ETH_RX_FLAGS_NO_SNOOP)

static int alu_eth_rx_ring_prime(struct udevice *dev,
				 struct alu_eth_priv *priv)
{
	int i, rc;

	priv->rx_head = 0;
	for (i = 0; i < ALU_ETH_RX_BUFFERS; i++) {
		struct al_buf buf = {
			.addr = (al_phys_addr_t)(uintptr_t)priv->rx_buf[i],
			.len = PKTSIZE_ALIGN,
		};

		rc = al_eth_rx_buffer_add(priv->rx_q, &buf, ALU_ETH_RX_FLAGS,
					  NULL);
		if (rc) {
			dev_err(dev, "rx_buffer_add[%d] failed: %d\n", i, rc);
			return rc;
		}
	}

	al_eth_rx_buffer_action(priv->rx_q, ALU_ETH_RX_BUFFERS);

	/* RX buffers are about to be DMA-written: drop stale dirty lines, and
	 * publish the just-posted descriptors before the MAC pulls them. */
	for (i = 0; i < ALU_ETH_RX_BUFFERS; i++)
		alu_eth_cache_inval(priv->rx_buf[i], PKTSIZE_ALIGN);
	alu_eth_cache_flush(priv->desc_block, ALU_ETH_DESC_BLOCK_SIZE);

	return 0;
}

int alu_eth_core_init(struct udevice *dev, struct alu_eth_priv *priv)
{
	uintptr_t base = (uintptr_t)priv->desc_block;
	struct al_eth_adapter_params ap;
	struct al_udma_q_params txp, rxp;
	int rc;

	rc = alu_eth_flr(dev, priv);
	if (rc) {
		dev_err(dev, "FLR failed: %d\n", rc);
		return rc;
	}

	memset(&ap, 0, sizeof(ap));
	ap.rev_id = AL_ETH_REV_ID_2;		/* Alpine V2 basic */
	ap.udma_id = 0;
	ap.enable_rx_parser = 0;
	ap.udma_regs_base = priv->udma_regs;
	ap.ec_regs_base = priv->ec_regs;
	ap.mac_regs_base = priv->mac_regs;
	ap.name = priv->name;
	/* HSSP group D lane 0 for the 10G port; unused at RGMII. The DT is
	 * hardware-of-record here - stock's code default (3 - port) is wrong for
	 * this board and cost an SError (eac280a). */
	ap.serdes_lane = 0;

	rc = al_eth_adapter_init(&priv->adapter, &ap);
	if (rc) {
		dev_err(dev, "al_eth_adapter_init failed: %d\n", rc);
		return rc;
	}

	/* adapter_init resets SMCC to 0, so snoop/ROB come after it. Error
	 * tracking stays on: it is the only latch for a failed M2S read. */
	alu_eth_unit_adapter_setup(dev, true);

	alu_eth_q_params(&txp, base, 0, 1);
	alu_eth_q_params(&rxp, base, 2, 3);

	rc = al_eth_queue_config(&priv->adapter, UDMA_TX, 0, &txp);
	if (!rc)
		rc = al_eth_queue_enable(&priv->adapter, UDMA_TX, 0);
	if (!rc)
		rc = al_eth_queue_config(&priv->adapter, UDMA_RX, 0, &rxp);
	if (!rc)
		rc = al_eth_queue_enable(&priv->adapter, UDMA_RX, 0);
	if (rc) {
		dev_err(dev, "queue config/enable failed: %d\n", rc);
		return rc;
	}

	rc = al_udma_q_handle_get(&priv->adapter.tx_udma, 0, &priv->tx_q);
	if (!rc)
		rc = al_udma_q_handle_get(&priv->adapter.rx_udma, 0, &priv->rx_q);
	if (rc) {
		dev_err(dev, "udma_q_handle_get failed: %d\n", rc);
		return rc;
	}

	/* Steer RX to UDMA0/Q0. Without it the EC drops every frame before the
	 * S2M ring - TX goes out, replies never arrive (#234). */
	rc = alu_eth_rxfwd_config(&priv->adapter);
	if (rc) {
		dev_err(dev, "rx forwarding config failed: %d\n", rc);
		return rc;
	}

	/* No al_eth_mac_link_config: stock skips it for external-PHY RGMII (the
	 * AR8033 drives the link and the MAC follows via RGMII in-band) and the
	 * 10G speed is fixed by the SerDes/PCS. Forcing it wedged the UDMA TX. */
	rc = al_eth_mac_config(&priv->adapter, priv->mac_mode);
	if (rc) {
		dev_err(dev, "al_eth_mac_config failed: %d\n", rc);
		return rc;
	}

	rc = al_eth_rx_pkt_limit_config(&priv->adapter, ALU_ETH_RX_MIN_LEN,
					ALU_ETH_RX_MAX_LEN);
	if (rc) {
		dev_err(dev, "rx_pkt_limit_config failed: %d\n", rc);
		return rc;
	}

	rc = alu_eth_rx_ring_prime(dev, priv);
	if (rc)
		return rc;

	/* The MAC's TX/RX enable can fail on its own; kicking the UDMA at a MAC
	 * that never enabled transmit consumes descriptors and posts no
	 * completion, which reads as a TX hang. */
	rc = al_eth_mac_start(&priv->adapter);
	if (rc) {
		dev_err(dev, "al_eth_mac_start failed: %d\n", rc);
		return rc;
	}

	return 0;
}

/* ---- eth_ops ------------------------------------------------------------ */

int alu_eth_core_send(struct udevice *dev, void *packet, int length)
{
	struct alu_eth_priv *priv = dev_get_priv(dev);
	struct al_eth_pkt pkt;
	int ndesc, done, poll = 0;

	memset(&pkt, 0, sizeof(pkt));
	pkt.num_of_bufs = 1;
	/* NO_SNOOP: the M2S engine reads descriptor + buffer as plain DRAM reads
	 * (serviced by the flush below) instead of snoop reads, which stall
	 * forever here - the fabric never answers the snoop. */
	pkt.flags = AL_ETH_TX_FLAGS_NO_SNOOP;

	/* The caller's packet is in U-Boot's high heap, which the al_udma master
	 * cannot reach; bounce it through the low window. */
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

	alu_eth_cache_flush(priv->tx_bounce, length);
	alu_eth_cache_flush(priv->desc_block, ALU_ETH_DESC_BLOCK_SIZE);
	al_eth_tx_dma_action(priv->tx_q, ndesc);

	while (ndesc) {
		alu_eth_cache_inval(priv->desc_block, ALU_ETH_DESC_BLOCK_SIZE);
		done = al_eth_comp_tx_get(priv->tx_q);
		ndesc -= done;
		if (!ndesc)
			break;
		udelay(1);
		if (++poll >= ALU_ETH_TX_POLL_MAX) {
			dev_err(dev, "TX completion timeout: %d descs left after %d us\n",
				ndesc, poll);
			alu_eth_dump_tx(priv, "timeout");
			alu_eth_dump_axi_err(dev);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

int alu_eth_core_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct alu_eth_priv *priv = dev_get_priv(dev);
	struct al_eth_pkt pkt;
	int ndesc;

	memset(&pkt, 0, sizeof(pkt));
	alu_eth_cache_inval(priv->desc_block, ALU_ETH_DESC_BLOCK_SIZE);
	alu_eth_cache_inval(priv->rx_buf[priv->rx_head], PKTSIZE_ALIGN);

	ndesc = al_eth_pkt_rx(priv->rx_q, &pkt);
	if (!ndesc)
		return -EAGAIN;

	if (pkt.flags & AL_ETH_RX_ERROR) {
		alu_eth_core_free_pkt(dev, NULL, 0);	/* recycle, drop */
		return -EIO;
	}

	*packetp = priv->rx_buf[priv->rx_head];
	return pkt.bufs[0].len;
}

int alu_eth_core_free_pkt(struct udevice *dev, uchar *packet, int length)
{
	struct alu_eth_priv *priv = dev_get_priv(dev);
	struct al_buf buf;
	int rc;

	buf.addr = (al_phys_addr_t)(uintptr_t)priv->rx_buf[priv->rx_head];
	buf.len = PKTSIZE_ALIGN;

	/* About to be DMA-written again: drop stale lines before re-posting. */
	alu_eth_cache_inval(priv->rx_buf[priv->rx_head], PKTSIZE_ALIGN);

	rc = al_eth_rx_buffer_add(priv->rx_q, &buf, ALU_ETH_RX_FLAGS, NULL);
	if (rc) {
		dev_err(dev, "rx_buffer_add (recycle) failed: %d\n", rc);
		return rc;
	}

	al_eth_rx_buffer_action(priv->rx_q, 1);
	alu_eth_cache_flush(priv->desc_block, ALU_ETH_DESC_BLOCK_SIZE);

	if (++priv->rx_head == ALU_ETH_RX_BUFFERS)
		priv->rx_head = 0;
	return 0;
}

void alu_eth_core_stop(struct udevice *dev)
{
	struct alu_eth_priv *priv = dev_get_priv(dev);
	int rc;

	if (!priv->started)
		return;

	rc = al_eth_mac_stop(&priv->adapter);
	if (rc)
		dev_err(dev, "al_eth_mac_stop failed: %d\n", rc);

	udelay(10);	/* let the MAC FIFO (~10KB) drain into memory */

	rc = al_eth_adapter_stop(&priv->adapter);
	if (rc)
		dev_err(dev, "al_eth_adapter_stop failed: %d\n", rc);

	priv->started = false;
}

int alu_eth_core_write_hwaddr(struct udevice *dev)
{
	struct alu_eth_priv *priv = dev_get_priv(dev);
	struct eth_pdata *pdata = dev_get_plat(dev);

	return al_eth_mac_addr_store(priv->ec_regs, 0, pdata->enetaddr);
}

/* Per-unit MAC from the SPI-NOR identity blob. The eth uclass calls this when
 * env carries no ethaddr (#89); probe already committed the same value to the
 * EC filter. */
int alu_eth_core_read_rom_hwaddr(struct udevice *dev)
{
	struct alu_eth_priv *priv = dev_get_priv(dev);
	struct eth_pdata *pdata = dev_get_plat(dev);
	int rc = alu_eth_hwaddr_get(priv->port, pdata->enetaddr);

	if (rc)
		dev_dbg(dev, "no MAC from NOR: %d\n", rc);
	return rc;
}
