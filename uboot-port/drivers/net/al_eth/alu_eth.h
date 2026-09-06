/* SPDX-License-Identifier: GPL-2.0-or-later */
/* alu_eth - U-Boot glue for the Annapurna al_eth MAC, on the SHARED Linux HAL.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * `alu_` = the U-Boot side. Everything `al_` is the shared HAL, staged from
 * modules/al_eth/ (scripts/stage_hal.py) - never a second copy (#256).
 */

#ifndef __ALU_ETH_H__
#define __ALU_ETH_H__

#include <linux/types.h>

struct udevice;

/* --- ports ------------------------------------------------------------- */

/* al_eth port index == stock's eth_* <port> argument. 1 = 1G RJ45
 * (1c36:0001), 2 = 10G SFP+ (1c36:0002). 0 and 3 exist in the SoC iomap but
 * have no PCI function fitted on this board. */
#define ALU_ETH_PORT_1G		1
#define ALU_ETH_PORT_10G	2

#define ALU_ETH_PCI_VENDOR	0x1c36
#define ALU_ETH_PCI_DEV_1G	0x0001
#define ALU_ETH_PCI_DEV_10G	0x0002

/* The three al_eth register windows. Not one base + offsets, and NOT in
 * ascending BAR order: UDMA = BAR0, MAC = BAR2, EC = BAR4. */
struct alu_eth_regs {
	void __iomem	*udma;
	void __iomem	*ec;
	void __iomem	*mac;
	struct udevice	*dev;
	unsigned long	bdf;
};

/* Human name for a port, or NULL if this board has no such function. */
const char *alu_eth_port_desc(int port);

/* PCI device id fitted at <port>, or 0 if none. */
unsigned int alu_eth_port_devid(int port);

/* Map <port>'s three windows, probing the PCI function if needed (probing is
 * what assigns the BARs, so the DM driver need not have started). Returns 0,
 * or prints the reason and returns -errno. */
int alu_eth_port_regs_get(int port, struct alu_eth_regs *regs);

/* --- per-unit MAC address ---------------------------------------------- */

/* Port <port>'s MAC from the SPI-NOR identity blob: base+0 for the 1G port,
 * base+1 for the 10G (stock's printed "[2]" is a COUNT, not an offset -
 * #222/#223). <addr> is ARP_HLEN bytes. */
int alu_eth_hwaddr_get(int port, unsigned char *addr);

/* Program that address into EC filter slot 0. Called from probe, not from the
 * uclass's lazy write_hwaddr: Linux inherits this register whether or not
 * U-Boot ever activates the port (#222). */
int alu_eth_hwaddr_commit(int port, void __iomem *ec_regs);

/* --- board params (the MAC scratchpad Linux reads at probe) ------------- */

/* Build <port>'s params from /soc/board-cfg/ethernet/port<n> and write them to
 * the already-mapped MAC window. Without them Linux al_eth fails probe with
 * "board info not available". */
int alu_eth_bp_seed(int port, void __iomem *mac_regs);

/* Same, resolving the MAC window from PCI (prompt-driven). */
int alu_eth_bp_write(int port);

/* Decode + print all three scratch regs for one port. */
int alu_eth_bp_dump(int port);

/* dont_override_serdes: keep this bootloader's SerDes settings instead of
 * letting Linux program its own table. */
int alu_eth_bp_freeze_set(int port, int enable);

/* media_type: 0 auto-detect, 1 RGMII, 2 10G-serial, 3 SGMII, 4 1000BASE-X,
 * 5 auto-detect-auto-speed. */
int alu_eth_bp_mac_mode_set(int port, unsigned int mode);

/* Retimer fields; any argument < 0 is left as read. */
int alu_eth_bp_retimer_set(int port, int exist, int bus_id, int i2c_addr,
			   int channel);

/* Human name for a board-params media_type. */
const char *alu_eth_media_name(unsigned int media_type);

/* --- diagnostics -------------------------------------------------------- */

int alu_eth_stats_show(int port);
int alu_eth_diag_show(int port);

#endif /* __ALU_ETH_H__ */
