// SPDX-License-Identifier: GPL-2.0-or-later
/* alu_eth port index -> PCI function -> the three register windows.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * Shared by the two DM drivers and by `eth diag` / `eth stats`, which must work
 * on a port the DM driver never started.
 */

#include <dm.h>
#include <errno.h>
#include <pci.h>
#include <stdio.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <asm/io.h>

#include "alu_eth.h"

/* BAR indices from the vendor al_hal_eth.h. Not ascending: EC is BAR4. */
#define ALU_ETH_BAR_UDMA	PCI_BASE_ADDRESS_0
#define ALU_ETH_BAR_MAC		PCI_BASE_ADDRESS_2
#define ALU_ETH_BAR_EC		PCI_BASE_ADDRESS_4

static const struct {
	u16		devid;
	const char	*what;
} alu_eth_ports[] = {
	[0]			= { 0x0000, NULL },
	[ALU_ETH_PORT_1G]	= { ALU_ETH_PCI_DEV_1G,  "1G RJ45" },
	[ALU_ETH_PORT_10G]	= { ALU_ETH_PCI_DEV_10G, "10G SFP+" },
	[3]			= { 0x0000, NULL },
};

const char *alu_eth_port_desc(int port)
{
	if (port < 0 || port >= ARRAY_SIZE(alu_eth_ports))
		return NULL;
	return alu_eth_ports[port].what;
}

unsigned int alu_eth_port_devid(int port)
{
	if (port < 0 || port >= ARRAY_SIZE(alu_eth_ports))
		return 0;
	return alu_eth_ports[port].devid;
}

int alu_eth_port_regs_get(int port, struct alu_eth_regs *regs)
{
	struct udevice *dev;
	int rc;

	memset(regs, 0, sizeof(*regs));

	if (port < 0 || port >= ARRAY_SIZE(alu_eth_ports) ||
	    !alu_eth_ports[port].devid) {
		printf("alu_eth: no port %d on this board (1 = 1G RJ45, 2 = 10G SFP+)\n",
		       port);
		return -ENODEV;
	}

	rc = dm_pci_find_device(ALU_ETH_PCI_VENDOR, alu_eth_ports[port].devid, 0,
				&dev);
	if (rc) {
		printf("alu_eth: port %d (%04x:%04x) not on PCI (%d) - run `pci enum`\n",
		       port, ALU_ETH_PCI_VENDOR, alu_eth_ports[port].devid, rc);
		return rc;
	}

	dm_pci_clrset_config16(dev, PCI_COMMAND, 0, PCI_COMMAND_MEMORY);

	regs->dev = dev;
	regs->bdf = dm_pci_get_bdf(dev);
	regs->udma = dm_pci_map_bar(dev, ALU_ETH_BAR_UDMA, 0, 0,
				    PCI_REGION_TYPE, PCI_REGION_MEM);
	regs->ec = dm_pci_map_bar(dev, ALU_ETH_BAR_EC, 0, 0,
				  PCI_REGION_TYPE, PCI_REGION_MEM);
	regs->mac = dm_pci_map_bar(dev, ALU_ETH_BAR_MAC, 0, 0,
				   PCI_REGION_TYPE, PCI_REGION_MEM);

	if (!regs->udma || !regs->ec || !regs->mac) {
		printf("alu_eth: port %d BARs not mapped (udma=%p ec=%p mac=%p)\n",
		       port, regs->udma, regs->ec, regs->mac);
		return -EINVAL;
	}

	return 0;
}
