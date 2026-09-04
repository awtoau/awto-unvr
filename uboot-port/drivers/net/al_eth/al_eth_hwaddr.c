// SPDX-License-Identifier: GPL-2.0-or-later
/* al_eth per-unit MAC: SPI-NOR identity blob -> port address -> EC filter.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * The identity partition (mtd4 "EEPROM", NOR 0x1f0000) holds ONE base MAC at
 * +0x0. Stock U-Boot prints "mac: [<base>] + [2]" - a COUNT of allocated
 * addresses, not an offset: port 1 is base+0, port 2 is base+1 (#222, #223).
 * The 6 bytes at +0x6 are locally administered, so not a globally unique MAC;
 * nothing here reads them.
 *
 * Committed at PROBE, not from the eth uclass's lazy write_hwaddr: the EC
 * filter register is part of the handoff to Linux, and a boot that only ever
 * uses the 1G port would otherwise leave the 10G port holding whatever the
 * previous bootloader wrote (#222).
 */

#include <dm.h>
#include <errno.h>
#include <net.h>
#include <spi_flash.h>
#include <stdio.h>
#include <linux/string.h>
#include <vsprintf.h>
#include <asm/io.h>

#include <al_hal_eth.h>

#include "al_eth_hwaddr.h"

/* Same flash as our env; see docs/mtd.md. */
#define AL_ETH_MAC_ROM_OFFSET	0x1f0000

int al_eth_hwaddr_get(int port, unsigned char *addr)
{
	struct udevice *flash;
	u32 nic;
	int ret;

	ret = uclass_first_device_err(UCLASS_SPI_FLASH, &flash);
	if (ret)
		return ret;
	ret = spi_flash_read_dm(flash, AL_ETH_MAC_ROM_OFFSET, ARP_HLEN, addr);
	if (ret)
		return ret;
	if (!is_valid_ethaddr(addr))
		return -EINVAL;

	/* Carry runs through the 24-bit NIC part only, so +1 stays in the OUI. */
	nic = (addr[3] << 16) | (addr[4] << 8) | addr[5];
	nic = (nic + (port - 1)) & 0xffffff;
	addr[3] = nic >> 16;
	addr[4] = nic >> 8;
	addr[5] = nic;

	return 0;
}

int al_eth_hwaddr_commit(int port, void __iomem *ec_regs)
{
	unsigned char addr[ARP_HLEN], prev[ARP_HLEN];
	int have_prev, ret;

	ret = al_eth_hwaddr_get(port, addr);
	if (ret) {
		printf("al_eth%d: no MAC from NOR 0x%06x (%d) - EC filter left as found\n",
		       port, AL_ETH_MAC_ROM_OFFSET, ret);
		return ret;
	}

	have_prev = !al_eth_mac_addr_read(ec_regs, 0, prev);

	ret = al_eth_mac_addr_store(ec_regs, 0, addr);
	if (ret) {
		printf("al_eth%d: MAC %pM store failed (%d)\n", port, addr, ret);
		return ret;
	}

	/* A different prior value is the previous bootloader's - the only place
	 * it is ever visible. */
	if (have_prev && !is_zero_ethaddr(prev) && memcmp(prev, addr, ARP_HLEN))
		printf("al_eth%d: MAC %pM (NOR base+%d), replacing pre-existing %pM\n",
		       port, addr, port - 1, prev);
	else
		printf("al_eth%d: MAC %pM (NOR base+%d)\n", port, addr, port - 1);

	return 0;
}
