/** @file
  Registers one trivial HII string package - nothing in P0's component
  list otherwise registers any HII package at all (docs/uefi.md: no
  SetupBrowserDxe/GraphicsConsoleDxe/UiApp, none of TerminalDxe/
  ConSplitterDxe/ConPlatformDxe/BdsDxe do it either), so the database is
  genuinely empty by the time BdsDxe's first ReadyToBoot fires -
  HiiGetDatabaseInfo() (HiiDatabaseDxe/Database.c) asserts on that ("Not
  Found" instead of the expected EFI_BUFFER_TOO_SMALL from an empty
  export). A DXE_DRIVER (not an application - those only register once
  actually launched as a boot option, which is after ReadyToBoot
  already fired) that registers ANY package, even this trivial one,
  fixes it.

  Copyright (c) 2026, Awto / Daniel Tyrrell. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/HiiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

extern UINT8  MinimalHiiDxeStrings[];

EFI_STATUS
EFIAPI
MinimalHiiDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HII_HANDLE  HiiHandle;

  HiiHandle = HiiAddPackages (
                &gEfiCallerIdGuid,
                ImageHandle,
                MinimalHiiDxeStrings,
                NULL
                );
  if (HiiHandle == NULL) {
    DEBUG ((DEBUG_ERROR, "MinimalHiiDxe: HiiAddPackages failed\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}
