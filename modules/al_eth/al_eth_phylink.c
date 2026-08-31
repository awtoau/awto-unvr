// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * al_eth 10G SFP+ port (eth2, PCI 1c36:0002): link management on mainline
 * phylink + drivers/net/phy/sfp.c.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 *
 * Division of labour:
 *   sfp.c     - cage/module presence, SFF-8472 EEPROM, interface selection, DDM
 *   phylink   - state machine, carrier, ethtool
 *   this file - phylink_pcs (SerDes bring-up + PCS/MAC link read) and the
 *               three MAC ops. The SerDes work itself is unchanged vendor HAL
 *               (al_hal_serdes_hssp.c + the al_init_eth_lm.c static parameter
 *               tables), only re-hosted under the pcs_* callbacks.
 *
 * Scope: this replaces al_init_eth_lm.c's detect/establish state machine and
 * al_eth_group_lm.c for the SFP+ port ONLY. eth1 (1G RGMII, PCI 1c36:0001) is
 * phy_exist=true and keeps the phylib path untouched.
 *
 * NOT covered yet:
 *   - 1G optics / SGMII: only 10GBASE-R is advertised. A 1G module is rejected
 *     by phylink with a clear message instead of silently mis-configuring the
 *     MAC. The vendor LM could do 1G; re-adding it needs mac_config to switch
 *     al_eth_mac_config()/SerDes mode live, which is untested on this board.
 *     #127.
 *   - passive DAC: sfp.c reports 10GBASE-R for both optic and direct-attach,
 *     so the DA-specific SerDes tables + rx_equal sweep are not selectable.
 *     The fitted module is an FS SFP-10GSR-85 (10GBASE-SR, 850nm), confirmed
 *     via its sfp.c-reported EEPROM string. #128.
 */

#include <linux/kconfig.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#include <linux/pci.h>
#include <linux/phylink.h>
#include <linux/property.h>

#include "al_hal_eth.h"
#include "al_init_eth_lm.h"
#include "al_eth.h"
#include "al_eth_phylink.h"

#if defined(CONFIG_ARCH_ALPINE) && !IS_ENABLED(CONFIG_PHYLINK)
/* Loud on purpose: this whole file compiles to nothing without PHYLINK and the
 * SFP+ port then comes up with no link management at all, which is invisible
 * until the box is booted. scripts/build-linux-fedora.py force-enables
 * PCS_XPCS/SFP so localmodconfig cannot drop it. */
#warning "CONFIG_PHYLINK is off: the 10G SFP+ port will have NO link management"
#endif

#if defined(CONFIG_ARCH_ALPINE) && IS_ENABLED(CONFIG_PHYLINK)

#include "alpine_serdes.h"

/*
 * Settle time after re-programming the SerDes group's electrical mode.
 * What: the group PLL re-locks and the lanes come out of reset.
 * Where from: the vendor's own al_eth_serdes_mode_set() used the same 1000 us
 * after alpine_serdes_eth_mode_set(); no datasheet lock time is published.
 * On expiry: nothing to report - it is a fixed settle, not a poll. The lane is
 * checked afterwards by pcs_get_state(), which reports link down if it is not
 * ready, and phylink retries on its 1 s poll.
 */
#define AL_ETH_SERDES_MODE_SETTLE_US	1000

struct al_eth_phylink {
	struct phylink_config	config;
	struct phylink		*phylink;
	struct phylink_pcs	pcs;
	struct al_eth_adapter	*adapter;
};

static struct al_eth_phylink *pcs_to_alp(struct phylink_pcs *pcs)
{
	return container_of(pcs, struct al_eth_phylink, pcs);
}

static struct al_eth_phylink *config_to_alp(struct phylink_config *config)
{
	return container_of(config, struct al_eth_phylink, config);
}

static struct phylink *al_eth_pl(struct al_eth_adapter *adapter)
{
	return adapter->plink ? adapter->plink->phylink : NULL;
}

/*************************** phylink_pcs ops *********************************/

/*
 * Runs before mac_config (phylink_major_config ordering), which is the order
 * the vendor group-LM used: SerDes group speed first, then al_eth_up() ->
 * al_eth_mac_config().
 */
static void al_eth_pcs_pre_config(struct phylink_pcs *pcs,
				  phy_interface_t interface)
{
	struct al_eth_adapter *adapter = pcs_to_alp(pcs)->adapter;
	int rc;

	rc = alpine_serdes_eth_mode_set(adapter->serdes_grp,
					ALPINE_SERDES_ETH_MODE_KR);
	if (rc) {
		netdev_err(adapter->netdev,
			   "serdes grp %u: KR mode set failed (%d)\n",
			   adapter->serdes_grp, rc);
		return;
	}

	al_udelay(AL_ETH_SERDES_MODE_SETTLE_US);

	/* Selects the optic TX/RX tables in al_init_eth_lm.c. Dirty flags force
	 * re-application after a mode set has reset the lane. */
	adapter->lm_context.mode = AL_ETH_LM_MODE_10G_OPTIC;
	adapter->lm_context.tx_param_dirty = 1;
	adapter->lm_context.rx_param_dirty = 1;
}

static int al_eth_pcs_enable(struct phylink_pcs *pcs)
{
	struct al_eth_adapter *adapter = pcs_to_alp(pcs)->adapter;

	if (!adapter->serdes_obj) {
		netdev_err(adapter->netdev, "no serdes object, cannot enable PCS\n");
		return -ENODEV;
	}

	return 0;
}

static int al_eth_pcs_config(struct phylink_pcs *pcs, unsigned int neg_mode,
			     phy_interface_t interface,
			     const unsigned long *advertising,
			     bool permit_pause_to_mac)
{
	struct al_eth_adapter *adapter = pcs_to_alp(pcs)->adapter;

	if (!adapter->serdes_obj)
		return -ENODEV;

	/* The lane's electrical tuning. Honours the serdes_[tr]x_* sysfs
	 * overrides when set, else the 10G-optic static tables. This is the
	 * code path under investigation for the TX side (#111). */
	al_eth_serdes_static_tx_params_set(&adapter->lm_context);
	al_eth_serdes_static_rx_params_set(&adapter->lm_context);

	/* 10GBASE-R has no in-band AN to restart */
	return 0;
}

static void al_eth_pcs_get_state(struct phylink_pcs *pcs, unsigned int neg_mode,
				 struct phylink_link_state *state)
{
	struct al_eth_adapter *adapter = pcs_to_alp(pcs)->adapter;
	struct al_eth_link_status status;

	state->link = false;
	state->an_complete = false;

	/* Clear the latched fault bits before reading, so a fault from a
	 * previous module/cable state cannot pin the link down for good. The
	 * vendor LM cached this in lm_context->link_state and needed a full
	 * driver reload to clear it (#98). */
	{
		int rc = al_eth_link_status_clear(&adapter->hal_adapter);

		if (rc)
			net_warn_ratelimited("%s: failed to clear link status latch (rc %d)\n",
					      adapter->netdev->name, rc);
	}

	if (al_eth_link_status_get(&adapter->hal_adapter, &status))
		return;

	if (status.link_up == AL_TRUE) {
		state->link = true;
		state->an_complete = true;
		state->speed = SPEED_10000;
		state->duplex = DUPLEX_FULL;
		return;
	}

	/* Down: local_fault = our own PCS has no block lock / is faulted;
	 * remote_fault = we are locked and the far end is telling us it is not.
	 * phylink only wants up/down, but the distinction is the whole
	 * diagnosis for this port, so keep it visible. */
	netdev_dbg(adapter->netdev, "pcs down: local_fault %d remote_fault %d\n",
		   status.local_fault, status.remote_fault);
}

static const struct phylink_pcs_ops al_eth_pcs_ops = {
	.pcs_pre_config	= al_eth_pcs_pre_config,
	.pcs_enable	= al_eth_pcs_enable,
	.pcs_config	= al_eth_pcs_config,
	.pcs_get_state	= al_eth_pcs_get_state,
};

/*************************** phylink_mac ops *********************************/

static struct phylink_pcs *al_eth_mac_select_pcs(struct phylink_config *config,
						 phy_interface_t interface)
{
	return &config_to_alp(config)->pcs;
}

/*
 * Nothing to do: the MAC is put in 10GBASE-R mode once by al_eth_up() ->
 * al_eth_hw_init() -> al_eth_mac_config(), and only one interface mode is
 * advertised, so phylink never asks for a different one. If SGMII/1000BASEX
 * is ever added to supported_interfaces this must reconfigure the MAC (and
 * needs mac_prepare/mac_finish to bracket it with mac_stop/mac_start).
 */
static void al_eth_plink_mac_config(struct phylink_config *config,
				    unsigned int mode,
				    const struct phylink_link_state *state)
{
	struct al_eth_adapter *adapter = config_to_alp(config)->adapter;

	if (adapter->mac_mode != AL_ETH_MAC_MODE_10GbE_Serial)
		netdev_warn(adapter->netdev,
			    "mac_config: mac_mode %d is not 10GbE_Serial\n",
			    adapter->mac_mode);
}

static void al_eth_plink_mac_link_up(struct phylink_config *config,
				     struct phy_device *phy, unsigned int mode,
				     phy_interface_t interface, int speed,
				     int duplex, bool tx_pause, bool rx_pause)
{
	struct al_eth_adapter *adapter = config_to_alp(config)->adapter;

	adapter->link_config.active_speed = speed;
	adapter->link_config.active_duplex = duplex;
	adapter->last_link = true;

	{
		int rc = al_eth_led_set(&adapter->hal_adapter, AL_TRUE);

		if (rc)
			netdev_warn(adapter->netdev, "%s: failed to set link LED (rc %d)\n",
				    __func__, rc);
	}
	al_eth_link_leds_set(adapter, speed);

	netdev_info(adapter->netdev, "link up, %d Mb/s %s duplex\n", speed,
		    duplex == DUPLEX_FULL ? "full" : "half");
}

static void al_eth_plink_mac_link_down(struct phylink_config *config,
				       unsigned int mode,
				       phy_interface_t interface)
{
	struct al_eth_adapter *adapter = config_to_alp(config)->adapter;

	adapter->last_link = false;

	{
		int rc = al_eth_led_set(&adapter->hal_adapter, AL_FALSE);

		if (rc)
			netdev_warn(adapter->netdev, "%s: failed to set link LED (rc %d)\n",
				    __func__, rc);
	}
	al_eth_link_leds_set(adapter, 0);

	netdev_info(adapter->netdev, "link down\n");
}

static const struct phylink_mac_ops al_eth_phylink_mac_ops = {
	.mac_select_pcs	= al_eth_mac_select_pcs,
	.mac_config	= al_eth_plink_mac_config,
	.mac_link_up	= al_eth_plink_mac_link_up,
	.mac_link_down	= al_eth_plink_mac_link_down,
};

/******************************* setup ***************************************/

/*
 * al_eth binds by PCI ID and reads no DT properties, so nothing links the PCI
 * device to its "annapurna-labs,al-eth" node. Match on port-id == PCI slot:
 * eth1 is 0000:00:01.0 / port-id 1, eth2 is 0000:00:02.0 / port-id 2
 * (docs/hardware.md:38-39).
 *
 * Returns a node with a reference held, released by al_eth_phylink_teardown().
 */
static struct device_node *al_eth_find_dt_node(struct pci_dev *pdev)
{
	struct device_node *np = NULL;
	u32 port_id;

	for_each_compatible_node(np, NULL, "annapurna-labs,al-eth") {
		if (!of_device_is_available(np))
			continue;
		if (of_property_read_u32(np, "port-id", &port_id))
			continue;
		if (port_id == PCI_SLOT(pdev->devfn))
			return np;
	}

	return NULL;
}

int al_eth_phylink_setup(struct al_eth_adapter *adapter)
{
	struct pci_dev *pdev = adapter->pdev;
	struct al_eth_phylink *alp;
	struct device_node *np;
	struct phylink *phylink;
	int rc;

	np = al_eth_find_dt_node(pdev);
	if (!np) {
		netdev_err(adapter->netdev,
			   "no DT node with port-id %u - cannot reach the sfp phandle\n",
			   PCI_SLOT(pdev->devfn));
		return -ENODEV;
	}

	if (!of_property_present(np, "sfp")) {
		netdev_err(adapter->netdev,
			   "DT node %pOF has no sfp phandle\n", np);
		of_node_put(np);
		return -ENODEV;
	}

	alp = devm_kzalloc(&pdev->dev, sizeof(*alp), GFP_KERNEL);
	if (!alp) {
		of_node_put(np);
		return -ENOMEM;
	}

	alp->adapter = adapter;

	/* sfp.c and phylink resolve their phandles from the PCI device's
	 * fwnode, which the PCI core leaves NULL for these internal devices. */
	device_set_node(&pdev->dev, of_fwnode_handle(np));
	adapter->dt_node = np;

	alp->pcs.ops = &al_eth_pcs_ops;
	/* No link-change interrupt on this MAC, so phylink must poll the PCS
	 * (1 s, phylink_pcs_poll_start) exactly as the vendor LM's own
	 * delayed work did. */
	alp->pcs.poll = true;
	__set_bit(PHY_INTERFACE_MODE_10GBASER, alp->pcs.supported_interfaces);

	alp->config.dev = &adapter->netdev->dev;
	alp->config.type = PHYLINK_NETDEV;
	alp->config.mac_capabilities = MAC_10000FD;
	__set_bit(PHY_INTERFACE_MODE_10GBASER, alp->config.supported_interfaces);

	phylink = phylink_create(&alp->config, of_fwnode_handle(np),
				 PHY_INTERFACE_MODE_10GBASER,
				 &al_eth_phylink_mac_ops);
	if (IS_ERR(phylink)) {
		rc = PTR_ERR(phylink);
		netdev_err(adapter->netdev, "phylink_create failed (%d)\n", rc);
		device_set_node(&pdev->dev, NULL);
		adapter->dt_node = NULL;
		of_node_put(np);
		return rc;
	}

	alp->phylink = phylink;
	adapter->plink = alp;

	netdev_info(adapter->netdev, "phylink + sfp link management on %pOF\n",
		    np);

	return 0;
}

void al_eth_phylink_teardown(struct al_eth_adapter *adapter)
{
	struct al_eth_phylink *alp = adapter->plink;

	if (!alp)
		return;

	phylink_destroy(alp->phylink);
	adapter->plink = NULL;

	device_set_node(&adapter->pdev->dev, NULL);
	of_node_put(adapter->dt_node);
	adapter->dt_node = NULL;
}

void al_eth_phylink_start(struct al_eth_adapter *adapter)
{
	if (adapter->plink)
		phylink_start(adapter->plink->phylink);
}

void al_eth_phylink_stop(struct al_eth_adapter *adapter)
{
	if (adapter->plink)
		phylink_stop(adapter->plink->phylink);
}

/***************************** ethtool ***************************************/

int al_eth_phylink_ksettings_get(struct al_eth_adapter *adapter,
				 struct ethtool_link_ksettings *cmd)
{
	return phylink_ethtool_ksettings_get(al_eth_pl(adapter), cmd);
}

int al_eth_phylink_ksettings_set(struct al_eth_adapter *adapter,
				 const struct ethtool_link_ksettings *cmd)
{
	return phylink_ethtool_ksettings_set(al_eth_pl(adapter), cmd);
}

int al_eth_phylink_nway_reset(struct al_eth_adapter *adapter)
{
	return phylink_ethtool_nway_reset(al_eth_pl(adapter));
}

/*
 * `ethtool -m` needs nothing from us: phylink publishes netdev->sfp_bus on
 * module attach (phylink.c:3641) and the ethtool core consults it ahead of
 * ethtool_ops (net/ethtool/ioctl.c:2854,2894). Adding get_module_info/eeprom
 * ops here would be dead code.
 */

#endif /* CONFIG_ARCH_ALPINE && CONFIG_PHYLINK */
