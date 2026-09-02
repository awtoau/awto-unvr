/** @file
  ArmPlatformLib implementation for the Ubiquiti UNVR (Annapurna Labs
  Alpine V2, sysid 0xea16).

  Unlike the MikroTik CCR2004 reference this was adapted from, EDK2 here
  is chainloaded by our own U-Boot AFTER full board bring-up (DDR
  training, PBS pin mux, UART) - see docs/uefi.md. So, deliberately,
  ArmPlatformInitialize() does none of that: no MUIO mux writes, no I2C/
  GPIO expander init. Doing it again would be redundant at best; U-Boot's
  own values are the ones already proven to work on this exact unit.

  Copyright (c) 2026, Awto / Daniel Tyrrell. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/ArmLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/SerialPortLib.h>
#include <Guid/ArmMpCoreInfo.h>
#include <Ppi/ArmMpCoreInfo.h>

//
// UNVR (Alpine V2) has 4x Cortex-A57 in a single cluster.
// MPIDR Aff0 values: 0x0, 0x1, 0x2, 0x3 (docs/uefi.md §3).
//
STATIC ARM_CORE_INFO  mCoreInfoTable[] = {
  { 0x000 },  // Cluster 0, Core 0
  { 0x001 },  // Cluster 0, Core 1
  { 0x002 },  // Cluster 0, Core 2
  { 0x003 },  // Cluster 0, Core 3
};

STATIC
EFI_STATUS
EFIAPI
GetMpCoreInfo (
  OUT UINTN          *CoreCount,
  OUT ARM_CORE_INFO  **ArmCoreTable
  )
{
  *CoreCount    = ARRAY_SIZE (mCoreInfoTable);
  *ArmCoreTable = mCoreInfoTable;
  return EFI_SUCCESS;
}

STATIC ARM_MP_CORE_INFO_PPI  mMpCoreInfoPpi = {
  GetMpCoreInfo
};

STATIC EFI_PEI_PPI_DESCRIPTOR  mPlatformPpiTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gArmMpCoreInfoPpiGuid,
    &mMpCoreInfoPpi
  }
};

// =====================================================================
// ArmPlatformLib interface
// =====================================================================

EFI_BOOT_MODE
ArmPlatformGetBootMode (
  VOID
  )
{
  return BOOT_WITH_FULL_CONFIGURATION;
}

/**
  Called by PeilessSec after the serial port is initialized (P0: EDK2
  skips re-initializing it - U-Boot already left it at 115200, see
  docs/uefi.md §6 "UART re-init"). Board bring-up (DDR/pin-mux/clocks)
  is already done by U-Boot's own boot chain before EDK2 is entered, so
  there is nothing left for this to do at P0.
**/
RETURN_STATUS
ArmPlatformInitialize (
  IN  UINTN  MpId
  )
{
  SerialPortWrite ((UINT8 *)"SEC: UNVR ArmPlatformInitialize (P0 - no board init needed)\n\r", 63);
  return RETURN_SUCCESS;
}

VOID
ArmPlatformGetPlatformPpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  )
{
  *PpiListSize = sizeof (mPlatformPpiTable);
  *PpiList     = mPlatformPpiTable;
}
