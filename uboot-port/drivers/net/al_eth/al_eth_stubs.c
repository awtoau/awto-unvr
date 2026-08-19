// SPDX-License-Identifier: GPL-2.0-or-later
/* al_eth HAL stubs - 10G/25G (MAC v3/v4) symbols the RGMII 1G path never calls.
 * Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
 * Co-authored with Claude (Anthropic).
 *
 * al_hal_eth_mac.c dispatches al_eth_mac_handle_init() to v1_v2/v3/v4 by rev_id.
 * eth1 (RGMII) is rev_id 2 -> v1_v2 only, but the v3/v4 handle-init symbols must
 * still resolve at link. Rather than pull in mac_v3.c/mac_v4.c (which drag the
 * whole serdes/KR closure we deliberately excluded), stub them: they warn and
 * return an error if ever reached. Populate with more v3/v4-only symbols only if
 * the link reports them undefined.
 */
#include <linux/printk.h>
#include <al_hal_eth.h>
#include <al_hal_unit_adapter.h>

int al_eth_mac_v3_handle_init(struct al_eth_mac_obj *obj,
			      struct al_eth_mac_obj_init_params *params);
int al_eth_mac_v4_handle_init(struct al_eth_mac_obj *obj,
			      struct al_eth_mac_obj_init_params *params);

int al_eth_mac_v3_handle_init(struct al_eth_mac_obj *obj,
			      struct al_eth_mac_obj_init_params *params)
{
	pr_err("al_eth: MAC v3 (10G) not supported in the RGMII-only build\n");
	return -ENOTSUPP;
}

int al_eth_mac_v4_handle_init(struct al_eth_mac_obj *obj,
			      struct al_eth_mac_obj_init_params *params)
{
	pr_err("al_eth: MAC v4 (25G) not supported in the RGMII-only build\n");
	return -ENOTSUPP;
}

/* al_eth_adapter_init() emits a call to this in the (unit_adapter != NULL)
 * branch. We always pass unit_adapter = NULL (board_late_init already did the
 * PCI/snoop init), so it is referenced-but-never-called. Stub to satisfy the
 * linker without pulling in the whole io_fabric/unit_adapter closure. */
void al_unit_adapter_init(struct al_unit_adapter *unit_adapter)
{
	pr_err("al_eth: al_unit_adapter_init stub reached (unexpected)\n");
}
