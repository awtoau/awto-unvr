/* SPDX-License-Identifier: GPL-2.0-or-later */
/* al_eth board-params (MAC scratchpad) - the U-Boot -> Linux transport.
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 */

#ifndef __AL_ETH_BOARDPARAMS_H__
#define __AL_ETH_BOARDPARAMS_H__

/* Build port <port>'s params from /soc/board-cfg/ethernet/port<port> and write
 * them to the already-mapped MAC BAR. Called from each al_eth probe; prints a
 * before/after diff when it changes what was already there (e.g. a stock
 * chainload). Returns 0 on success, negative errno otherwise. */
int al_eth_bp_seed(int port, void __iomem *mac_regs);

/* Same, resolving the MAC BAR from PCI itself (prompt-driven). */
int al_eth_bp_write(int port);

/* Decode + print all three scratch regs for one port. */
int al_eth_bp_dump(int port);

/* dont_override_serdes ("freeze"): keep the SerDes settings this bootloader
 * left, instead of letting Linux program its own table. */
int al_eth_bp_freeze_set(int port, int enable);

/* media_type: 0 auto-detect, 1 RGMII, 2 10G-serial, 3 SGMII, 4 1000BASE-X,
 * 5 auto-detect-auto-speed. */
int al_eth_bp_mac_mode_set(int port, unsigned int mode);

/* Retimer fields; any argument < 0 is left as read. */
int al_eth_bp_retimer_set(int port, int exist, int bus_id, int i2c_addr,
			  int channel);

#endif /* __AL_ETH_BOARDPARAMS_H__ */
