/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 *
 * al_eth 10G SFP+ port (eth2, PCI 1c36:0002): link management via mainline
 * phylink + drivers/net/phy/sfp.c instead of Annapurna's LM state machine.
 * The 1G RGMII port (eth1) keeps the phylib path and is not touched.
 */
#ifndef AL_ETH_PHYLINK_H
#define AL_ETH_PHYLINK_H

#include <linux/kconfig.h>

struct al_eth_adapter;
struct ethtool_link_ksettings;

#if defined(CONFIG_ARCH_ALPINE) && IS_ENABLED(CONFIG_PHYLINK)

/* probe: match a DT node, attach it to the PCI device, create phylink+PCS */
int al_eth_phylink_setup(struct al_eth_adapter *adapter);
void al_eth_phylink_teardown(struct al_eth_adapter *adapter);

/* ndo_open / ndo_stop: phylink owns carrier, the datapath is brought up by
 * the caller before start / after stop */
void al_eth_phylink_start(struct al_eth_adapter *adapter);
void al_eth_phylink_stop(struct al_eth_adapter *adapter);

/* ethtool passthroughs — keep <linux/phylink.h> out of al_eth_main.c */
int al_eth_phylink_ksettings_get(struct al_eth_adapter *adapter,
				 struct ethtool_link_ksettings *cmd);
int al_eth_phylink_ksettings_set(struct al_eth_adapter *adapter,
				 const struct ethtool_link_ksettings *cmd);
int al_eth_phylink_nway_reset(struct al_eth_adapter *adapter);

#else /* !CONFIG_ARCH_ALPINE || !CONFIG_PHYLINK */

static inline int al_eth_phylink_setup(struct al_eth_adapter *adapter)
{
	return -EOPNOTSUPP;
}
static inline void al_eth_phylink_teardown(struct al_eth_adapter *adapter) { }
static inline void al_eth_phylink_start(struct al_eth_adapter *adapter) { }
static inline void al_eth_phylink_stop(struct al_eth_adapter *adapter) { }
static inline int al_eth_phylink_ksettings_get(struct al_eth_adapter *adapter,
					       struct ethtool_link_ksettings *cmd)
{
	return -EOPNOTSUPP;
}
static inline int al_eth_phylink_ksettings_set(struct al_eth_adapter *adapter,
					       const struct ethtool_link_ksettings *cmd)
{
	return -EOPNOTSUPP;
}
static inline int al_eth_phylink_nway_reset(struct al_eth_adapter *adapter)
{
	return -EOPNOTSUPP;
}

#endif

#endif /* AL_ETH_PHYLINK_H */
