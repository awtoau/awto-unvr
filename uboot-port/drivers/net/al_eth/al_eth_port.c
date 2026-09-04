// SPDX-License-Identifier: GPL-2.0-or-later
/* al_eth port index -> PCI function -> register windows, shared by the
 * board-params and stats commands.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 */

#include <dm.h>
#include <errno.h>
#include <stdio.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <pci.h>
#include <vsprintf.h>
#include <asm/io.h>

#include "al_eth_port.h"

#define AL_ETH_PCI_VENDOR	0x1c36
#define AL_ETH_PORT_BAR_UDMA	PCI_BASE_ADDRESS_0	/* AL_ETH_UDMA_BAR = 0 */
#define AL_ETH_PORT_BAR_MAC	PCI_BASE_ADDRESS_2	/* AL_ETH_MAC_BAR  = 2 */
#define AL_ETH_PORT_BAR_EC	PCI_BASE_ADDRESS_4	/* AL_ETH_EC_BAR   = 4 */

/* al_eth adapter index -> PCI function. Ports 0 and 3 exist in the SoC iomap
 * but have no PCI function fitted on the UNVR. Numbering matches stock's
 * eth_* <port> argument (AL_ETH_IS_ADV: even idx = 0x0002 "advanced"). */
static const struct {
	u16		devid;
	const char	*what;
} al_eth_ports[] = {
	{ 0x0000, NULL },
	{ 0x0001, "1G RJ45" },
	{ 0x0002, "10G SFP+" },
	{ 0x0000, NULL },
};

const char *al_eth_port_desc(int port)
{
	if (port < 0 || port >= ARRAY_SIZE(al_eth_ports))
		return NULL;
	return al_eth_ports[port].what;
}

unsigned int al_eth_port_devid(int port)
{
	if (port < 0 || port >= ARRAY_SIZE(al_eth_ports))
		return 0;
	return al_eth_ports[port].devid;
}

int al_eth_port_regs_get(int port, struct al_eth_port_regs *regs)
{
	struct udevice *dev;
	int rc;

	memset(regs, 0, sizeof(*regs));

	if (port < 0 || port >= ARRAY_SIZE(al_eth_ports) ||
	    !al_eth_ports[port].devid) {
		printf("al_eth: no port %d on this board (1 = 1G RJ45, 2 = 10G SFP+)\n",
		       port);
		return -ENODEV;
	}

	rc = dm_pci_find_device(AL_ETH_PCI_VENDOR, al_eth_ports[port].devid, 0,
				&dev);
	if (rc) {
		printf("al_eth: port %d (%04x:%04x) not on PCI (%d) - run `pci enum`\n",
		       port, AL_ETH_PCI_VENDOR, al_eth_ports[port].devid, rc);
		return rc;
	}

	dm_pci_clrset_config16(dev, PCI_COMMAND, 0, PCI_COMMAND_MEMORY);

	regs->dev = dev;
	regs->bdf = dm_pci_get_bdf(dev);

	regs->udma = dm_pci_map_bar(dev, AL_ETH_PORT_BAR_UDMA, 0, 0,
				    PCI_REGION_TYPE, PCI_REGION_MEM);
	regs->ec = dm_pci_map_bar(dev, AL_ETH_PORT_BAR_EC, 0, 0,
				  PCI_REGION_TYPE, PCI_REGION_MEM);
	regs->mac = dm_pci_map_bar(dev, AL_ETH_PORT_BAR_MAC, 0, 0,
				   PCI_REGION_TYPE, PCI_REGION_MEM);

	if (!regs->udma || !regs->ec || !regs->mac) {
		printf("al_eth: port %d BARs not mapped (udma=%p ec=%p mac=%p)\n",
		       port, regs->udma, regs->ec, regs->mac);
		return -EINVAL;
	}

	return 0;
}
