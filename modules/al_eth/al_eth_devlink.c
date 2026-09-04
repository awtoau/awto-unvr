// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * al_eth_devlink.c - devlink health reporter for the al_eth MAC cross-check
 *
 * Answers "is the MAC the bootloader programmed the one NOR says it should
 * be?" (#222). devlink rather than ethtool because the answer is a set of
 * addresses plus their provenance, not a boolean or a register dump; the
 * same reporter pattern is already in al_dma/al_ssm (#212).
 * ethtool -t exposes the pass/fail verdict for scripts - al_eth_main.c.
 *
 * Copyright (C) 2026 Awto / Daniel Tyrrell
 */

#include <linux/pci.h>
#include <net/devlink.h>

#include "al_eth.h"

static void al_eth_fmsg_mac(struct devlink_fmsg *fmsg, const char *name,
			    const u8 *addr)
{
	char buf[sizeof("xx:xx:xx:xx:xx:xx")];

	snprintf(buf, sizeof(buf), "%pM", addr);
	devlink_fmsg_string_pair_put(fmsg, name, buf);
}

static void al_eth_fmsg_mac_check(struct al_eth_adapter *adapter,
				  struct devlink_fmsg *fmsg)
{
	struct al_eth_mac_check c;

	al_eth_mac_verify(adapter, &c);

	devlink_fmsg_u32_pair_put(fmsg, "port_id", c.port_id);

	devlink_fmsg_string_pair_put(fmsg, "ec_source", c.ec_src);
	if (c.have_ec)
		al_eth_fmsg_mac(fmsg, "ec_mac", c.ec);

	devlink_fmsg_string_pair_put(fmsg, "nor_source", c.nor_src);
	if (c.have_nor) {
		al_eth_fmsg_mac(fmsg, "nor_mac", c.nor);
		if (c.derived) {
			al_eth_fmsg_mac(fmsg, "nor_base", c.nor_base);
			devlink_fmsg_u32_pair_put(fmsg, "nor_offset",
						  AL_ETH_10G_MAC_OFFSET);
		}
	} else if (c.nor_rc) {
		devlink_fmsg_u32_pair_put(fmsg, "nor_errno", -c.nor_rc);
	}

	/* Three states, not two: unknown is not a failure. */
	devlink_fmsg_string_pair_put(fmsg, "verdict",
				     !(c.have_ec && c.have_nor) ? "unknown" :
				     c.match ? "match" : "MISMATCH");
	devlink_fmsg_bool_pair_put(fmsg, "checked", c.have_ec && c.have_nor);
	devlink_fmsg_bool_pair_put(fmsg, "match", c.match);
}

static int al_eth_devlink_diagnose(struct devlink_health_reporter *reporter,
				   struct devlink_fmsg *fmsg,
				   struct netlink_ext_ack *extack)
{
	al_eth_fmsg_mac_check(devlink_health_reporter_priv(reporter), fmsg);
	return 0;
}

static int al_eth_devlink_dump(struct devlink_health_reporter *reporter,
			       struct devlink_fmsg *fmsg, void *priv_ctx,
			       struct netlink_ext_ack *extack)
{
	al_eth_fmsg_mac_check(devlink_health_reporter_priv(reporter), fmsg);
	return 0;
}

static const struct devlink_health_reporter_ops al_eth_mac_reporter_ops = {
	.name		= "mac",
	.diagnose	= al_eth_devlink_diagnose,
	.dump		= al_eth_devlink_dump,
};

static const struct devlink_ops al_eth_devlink_ops = {
};

int al_eth_devlink_init(struct al_eth_adapter *adapter)
{
	struct devlink *dl;
	int rc;

	dl = devlink_alloc(&al_eth_devlink_ops, 0, &adapter->pdev->dev);
	if (!dl)
		return -ENOMEM;

	adapter->mac_hr = devlink_health_reporter_create(dl,
							 &al_eth_mac_reporter_ops,
							 adapter);
	if (IS_ERR(adapter->mac_hr)) {
		rc = PTR_ERR(adapter->mac_hr);
		adapter->mac_hr = NULL;
		devlink_free(dl);
		return rc;
	}

	adapter->devlink = dl;
	devlink_register(dl);
	return 0;
}

void al_eth_devlink_fini(struct al_eth_adapter *adapter)
{
	if (!adapter->devlink)
		return;

	devlink_unregister(adapter->devlink);
	devlink_health_reporter_destroy(adapter->mac_hr);
	devlink_free(adapter->devlink);
	adapter->mac_hr = NULL;
	adapter->devlink = NULL;
}
