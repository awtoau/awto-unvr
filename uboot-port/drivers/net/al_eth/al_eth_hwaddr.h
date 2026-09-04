/* SPDX-License-Identifier: GPL-2.0-or-later */
/* al_eth per-unit MAC: SPI-NOR identity blob -> port address -> EC filter.
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 */

#ifndef __AL_ETH_HWADDR_H__
#define __AL_ETH_HWADDR_H__

/* Read the NOR base MAC and derive port <port>'s address into <addr>
 * (ARP_HLEN bytes): port 1 = base+0, port 2 = base+1 (#222, #223).
 * Returns 0, or -errno if the flash read failed or the base is not a valid
 * unicast address. */
int al_eth_hwaddr_get(int port, unsigned char *addr);

/* Program port <port>'s NOR-derived MAC into EC filter slot 0 at <ec_regs>.
 * Called from probe so the address is right for Linux even when U-Boot never
 * activates the port (#222). Prints what it wrote, and the prior value when it
 * differed - that value is the previous bootloader's and is visible nowhere
 * else. Returns 0 on success, negative errno otherwise. */
int al_eth_hwaddr_commit(int port, void __iomem *ec_regs);

#endif /* __AL_ETH_HWADDR_H__ */
