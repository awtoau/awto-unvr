/* SPDX-License-Identifier: GPL-2.0-or-later */
/* al_eth port index -> PCI function -> register windows.
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 */

#ifndef __AL_ETH_PORT_H__
#define __AL_ETH_PORT_H__

struct udevice;

/* The three al_eth BARs: UDMA = BAR0, EC = BAR4, MAC = BAR2. `dev`/`bdf` name
 * the PCI function they came from (diagnostics; NULL/0 if unresolved). */
struct al_eth_port_regs {
	void __iomem	*udma;
	void __iomem	*ec;
	void __iomem	*mac;
	struct udevice	*dev;
	unsigned long	bdf;
};

/* Human name for a port, or NULL if the board has no such function. */
const char *al_eth_port_desc(int port);

/* The PCI device id this board fits at <port>, or 0 if none. */
unsigned int al_eth_port_devid(int port);

/* Map port <port>'s three register windows. Probes the PCI function if needed,
 * which is what assigns the BARs - the al_eth DM driver need not have started.
 * Returns 0 on success; prints the reason and returns -errno otherwise. */
int al_eth_port_regs_get(int port, struct al_eth_port_regs *regs);

#endif /* __AL_ETH_PORT_H__ */
