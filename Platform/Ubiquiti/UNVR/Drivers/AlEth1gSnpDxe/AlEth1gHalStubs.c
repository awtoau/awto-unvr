/** @file
  HAL stubs - MAC v3/v4 and unit-adapter symbols the RGMII 1G path never calls.

  al_hal_eth_mac.c dispatches al_eth_mac_handle_init() to v1_v2/v3/v4 by rev_id.
  The 1G RJ45 is rev_id 2 -> v1_v2 only, but the v3/v4 handle-init symbols must
  still resolve at link. Pulling in mac_v3.c/mac_v4.c would drag the whole
  SerDes/KR closure AlpineHalLib deliberately excludes, so they are stubbed and
  return an error if ever reached.

  al_unit_adapter_init() is referenced from al_eth_adapter_init()'s
  (unit_adapter != NULL) branch; we always pass NULL, so it is
  referenced-but-never-called.

  Same three symbols, same rationale, as our U-Boot driver's al_eth_stubs.c.

  Copyright (c) 2026, Awto / Daniel Tyrrell. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>

#include "al_hal_eth.h"
#include "al_hal_unit_adapter.h"

int
al_eth_mac_v3_handle_init (
  struct al_eth_mac_obj              *obj,
  struct al_eth_mac_obj_init_params  *params
  );

int
al_eth_mac_v4_handle_init (
  struct al_eth_mac_obj              *obj,
  struct al_eth_mac_obj_init_params  *params
  );

int
al_eth_mac_v3_handle_init (
  struct al_eth_mac_obj              *obj,
  struct al_eth_mac_obj_init_params  *params
  )
{
  DEBUG ((DEBUG_ERROR, "AlEth1g: MAC v3 (10G) not in this RGMII-only build\n"));
  return -EINVAL;
}

int
al_eth_mac_v4_handle_init (
  struct al_eth_mac_obj              *obj,
  struct al_eth_mac_obj_init_params  *params
  )
{
  DEBUG ((DEBUG_ERROR, "AlEth1g: MAC v4 (25G) not in this RGMII-only build\n"));
  return -EINVAL;
}

void
al_unit_adapter_init (
  struct al_unit_adapter  *unit_adapter
  )
{
  DEBUG ((DEBUG_ERROR, "AlEth1g: al_unit_adapter_init stub reached\n"));
}
