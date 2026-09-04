/** @file
  ArmPlatformLib implementation for the Ubiquiti UNVR (Annapurna Labs
  Alpine V2, sysid 0xea16).

  Unlike the MikroTik CCR2004 reference this was adapted from, EDK2 here
  is chainloaded by our own U-Boot AFTER full board bring-up (DDR
  training, PBS pin mux, UART) - see docs/uefi.md. So, deliberately,
  ArmPlatformInitialize() does none of that: no MUIO mux writes, no I2C/
  GPIO expander init. Doing it again would be redundant at best; U-Boot's
  own values are the ones already proven to work on this exact unit.

  The one exception is the CCU coherency enable below - see its own
  comment for why inheriting that particular register is not safe.

  Copyright (c) 2026, Awto / Daniel Tyrrell. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/ArmLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
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

//
// CCU (Cache Coherency Unit) coherency enable - the EDK2 counterpart of
// alpine.c's al_ccu_early_coherency_enable(). Registers/values recovered
// in docs/nor-reference/uboot-ccu-coherency.md; base and io_coherency=1
// are hardcoded because there is no DTB at this stage (#140, #97).
//
#define UNVR_CCU_BASE              0xF0090000ULL  // /soc/ccu reg, both DTS sources
#define UNVR_CCU_SPECULATION       0x4            // speculation_ctrl_register_v1_v2
#define UNVR_CCU_SLAVE3_SNOOP      0x4000         // slaves[3], cluster 0
#define UNVR_CCU_SLAVE4_SNOOP      0x5000         // slaves[4], cluster 1
#define UNVR_CCU_SPECULATION_VAL   7
#define UNVR_CCU_SNOOP_EN          1              // vendor writes 1, not the HAL's SNOOP_EN|DVMS (3)

/**
  Enable CCU cluster snooping, matching what U-Boot already does.

  Without this EDK2 runs its whole PCIe/xHCI/AHCI stack with the
  interconnect-level coherency gate in whatever state it was inherited
  in, so any EDK2-vs-U-Boot differential test (docs/uefi.md P1.5, #140)
  is not comparing equivalent machine state.
**/
STATIC
VOID
UnvrCcuEnableCoherency (
  VOID
  )
{
  MmioWrite32 (UNVR_CCU_BASE + UNVR_CCU_SLAVE3_SNOOP, UNVR_CCU_SNOOP_EN);
  MmioWrite32 (UNVR_CCU_BASE + UNVR_CCU_SLAVE4_SNOOP, UNVR_CCU_SNOOP_EN);
  MmioWrite32 (UNVR_CCU_BASE + UNVR_CCU_SPECULATION, UNVR_CCU_SPECULATION_VAL);
}

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
  //
  // Length must match the literal exactly: it was 63 for a 61-byte
  // string, reading 2 bytes past the NUL every boot.
  //
  SerialPortWrite ((UINT8 *)"SEC: UNVR ArmPlatformInitialize (P0 - no board init needed)\n\r", 61);

  //
  // The one exception to "U-Boot already did it": CCU snoop is applied
  // by alpine.c's board_init(), but EDK2 is entered by `go`, and any
  // later warm path could leave it unset. Idempotent, so re-apply.
  //
  UnvrCcuEnableCoherency ();
  SerialPortWrite ((UINT8 *)"SEC: UNVR CCU coherency enabled (slave3/4 snoop, speculation)\n\r", 63);

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
