/** @file
  PciHostBridgeLib for the Ubiquiti UNVR's internal PCIe (Alpine V2).

  Bus 0 only, flat ECAM at PcdPciExpressBaseAddress (0xFBC00000, set in
  Unvr.dsc), no I/O space, MMIO window 0xFE000000-0xFEFFFFFF - values
  cross-checked against docs/hardware.md's register table and confirmed
  identical to imbushuo/ccr2004-uefi's own PciHostBridgeLib (same SoC
  family, same internal-PCIe layout). External PCIe0 (ASM1042A xHCI) is
  out of scope for P1 - see docs/uefi.md.

  Copyright (c) 2026, Awto / Daniel Tyrrell. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Protocol/PciRootBridgeIo.h>

GLOBAL_REMOVE_IF_UNREFERENCED
STATIC CHAR16 CONST * CONST mPciHostBridgeLibAcpiAddressSpaceTypeStr[] = {
  L"Mem", L"I/O", L"Bus"
};

#pragma pack(1)
typedef struct {
  ACPI_HID_DEVICE_PATH     AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;
#pragma pack ()

STATIC EFI_PCI_ROOT_BRIDGE_DEVICE_PATH mEfiPciRootBridgeDevicePath = {
  {
    {
      ACPI_DEVICE_PATH,
      ACPI_DP,
      {
        (UINT8)(sizeof (ACPI_HID_DEVICE_PATH)),
        (UINT8)((sizeof (ACPI_HID_DEVICE_PATH)) >> 8)
      }
    },
    EISA_PNP_ID (0x0A08),  // PCIe
    0
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      END_DEVICE_PATH_LENGTH,
      0
    }
  }
};

//
// Alpine V2 internal PCIe: bus 0 only, 16MB MMIO at 0xFE000000, no I/O space
//
STATIC PCI_ROOT_BRIDGE mPciRootBridge = {
  0,                                              // Segment
  0,                                              // Supports
  0,                                              // Attributes
  TRUE,                                           // DmaAbove4G
  FALSE,                                          // NoExtendedConfigSpace
  FALSE,                                          // ResourceAssigned
  EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM,           // AllocationAttributes
  {
    // Bus 0-0
    0,
    0
  },
  {
    // I/O: none (set empty range)
    MAX_UINT64,
    0
  },
  {
    // Mem: 0xFE000000 - 0xFEFFFFFF (16MB)
    0xFE000000,
    0xFEFFFFFF
  },
  {
    // MemAbove4G: none
    MAX_UINT64,
    0
  },
  {
    // PMem: none
    MAX_UINT64,
    0
  },
  {
    // PMemAbove4G: none
    MAX_UINT64,
    0
  },
  (EFI_DEVICE_PATH_PROTOCOL *)&mEfiPciRootBridgeDevicePath
};

PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeGetRootBridges (
  UINTN  *Count
  )
{
  *Count = 1;
  return &mPciRootBridge;
}

VOID
EFIAPI
PciHostBridgeFreeRootBridges (
  PCI_ROOT_BRIDGE  *Bridges,
  UINTN            Count
  )
{
}

VOID
EFIAPI
PciHostBridgeResourceConflict (
  EFI_HANDLE  HostBridgeHandle,
  VOID        *Configuration
  )
{
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Descriptor;
  UINTN                              RootBridgeIndex;

  DEBUG ((DEBUG_ERROR, "PciHostBridge: Resource conflict!\n"));

  RootBridgeIndex = 0;
  Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)Configuration;
  while (Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    DEBUG ((DEBUG_ERROR, "RootBridge[%d]:\n", RootBridgeIndex++));
    for (; Descriptor->Desc == ACPI_ADDRESS_SPACE_DESCRIPTOR; Descriptor++) {
      ASSERT (Descriptor->ResType <
              ARRAY_SIZE (mPciHostBridgeLibAcpiAddressSpaceTypeStr));
      DEBUG ((DEBUG_ERROR, " %s: Length/Alignment = 0x%lx / 0x%lx\n",
              mPciHostBridgeLibAcpiAddressSpaceTypeStr[Descriptor->ResType],
              Descriptor->AddrLen, Descriptor->AddrRangeMax));
    }

    ASSERT (Descriptor->Desc == ACPI_END_TAG_DESCRIPTOR);
    Descriptor = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *)(
                   (EFI_ACPI_END_TAG_DESCRIPTOR *)Descriptor + 1
                   );
  }
}
