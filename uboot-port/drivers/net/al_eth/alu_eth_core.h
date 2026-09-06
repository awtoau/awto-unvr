/* SPDX-License-Identifier: GPL-2.0-or-later */
/* alu_eth datapath core - the UDMA/EC half both ports share.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * The 1G and 10G ports differ only in their front end (RGMII + AR8033 via
 * phylib, vs SerDes lane + 10GBASE-R PCS). Rings, adapter init, send, recv and
 * cache maintenance are identical, so they live here once.
 */

#ifndef __ALU_ETH_CORE_H__
#define __ALU_ETH_CORE_H__

#include <net.h>
#include <linux/types.h>

#include <al_hal_eth.h>
#include <al_hal_udma.h>

struct udevice;

/* Ring depth: a power of 2 and >= AL_UDMA_MIN_Q_SIZE (32), which the HAL
 * enforces. Independent of how many RX buffers we prime (PKTBUFSRX). */
#define ALU_ETH_RX_BUFFERS	PKTBUFSRX
#define ALU_ETH_DESCS_PER_Q	32
#define ALU_ETH_CDESC_SIZE	16
#define ALU_ETH_Q_DESCS_SIZE	(ALU_ETH_DESCS_PER_Q * ALU_ETH_CDESC_SIZE)
#define ALU_ETH_DESC_BLOCK_SIZE	(4 * ALU_ETH_Q_DESCS_SIZE)

struct alu_eth_priv {
	struct al_hal_eth_adapter	adapter;
	void __iomem			*udma_regs;
	void __iomem			*ec_regs;
	void __iomem			*mac_regs;

	struct al_udma_q		*tx_q;
	struct al_udma_q		*rx_q;

	void				*desc_block;	/* low DRAM */
	uchar				*rx_buf[ALU_ETH_RX_BUFFERS];
	uchar				*tx_bounce;
	unsigned int			rx_head;

	int				port;		/* ALU_ETH_PORT_* */
	enum al_eth_mac_mode		mac_mode;
	bool				started;
	char				name[16];
};

/* al_udma master DRAM reach: it decodes only the LOW window, and U-Boot's heap
 * relocates to the top of the 3GB bank, so memalign() hands the engine
 * addresses it hangs on. Every al_udma-visible buffer comes from here. #90. */
void *alu_eth_dma_low_alloc(size_t size);

/* Allocate this port's descriptor block, RX buffers and TX bounce buffer out of
 * the low window. Call from probe. */
int alu_eth_bufs_alloc(struct alu_eth_priv *priv);

/* Cache maintenance around every descriptor/buffer handoff. Both ends round to
 * ARCH_DMA_MINALIGN so a neighbouring line is never touched. */
void alu_eth_cache_flush(void *p, size_t len);
void alu_eth_cache_inval(void *p, size_t len);

/* Replicate al_unit_adapter_init() on this function's PCI config space: snoop,
 * error tracking, ROB. All three are cleared by FLR and by adapter_init, so
 * this runs AFTER both. <err_track> enables the AXI error-attribute latch. */
void alu_eth_unit_adapter_setup(struct udevice *dev, bool err_track);

/* FLR that preserves board params + EC MAC address, then adapter init, queue
 * config/enable, RX forwarding, MAC mode, RX ring priming and MAC start.
 * <mac_mode> selects RGMII or 10GbE_Serial. */
int alu_eth_core_init(struct udevice *dev, struct alu_eth_priv *priv);

/* eth_ops bodies, shared verbatim by both ports. */
int alu_eth_core_send(struct udevice *dev, void *packet, int length);
int alu_eth_core_recv(struct udevice *dev, int flags, uchar **packetp);
int alu_eth_core_free_pkt(struct udevice *dev, uchar *packet, int length);
void alu_eth_core_stop(struct udevice *dev);
int alu_eth_core_write_hwaddr(struct udevice *dev);
int alu_eth_core_read_rom_hwaddr(struct udevice *dev);

/* EC RX forwarding -> UDMA0/Q0 (#234). */
int alu_eth_rxfwd_config(struct al_hal_eth_adapter *adapter);

/* M2S engine + TX ring state, the #90/#253 diagnostic. */
void alu_eth_dump_tx(struct alu_eth_priv *priv, const char *tag);

#endif /* __ALU_ETH_CORE_H__ */
