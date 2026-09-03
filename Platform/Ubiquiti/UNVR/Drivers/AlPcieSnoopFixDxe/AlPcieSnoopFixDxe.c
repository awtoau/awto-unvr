/** @file
  Applies the Alpine internal-PCIe AXI-snoop/APP_CONTROL coherency
  fixup after PCI enumeration completes (docs/uefi.md P1). Mirrors
  al_pcie_snoop_fix()/al_snoop_one() in our U-Boot fork's
  board/annapurna/alpine/alpine.c, register-for-register (issue #74) -
  both include the same hal/pcie-al-alpine-regs.h, not just matching
  hand-typed values. See that header's own comment for the full
  three-way (Linux/U-Boot/EDK2) provenance.

  Deliberately excludes al_eth (1c36:0001/0002): applying this fixup to
  it broke UDMA TX and persisted across a warm reset (#74, #90) - U-Boot
  fixed that the same way, by skipping those two device IDs here. Not
  part of the shared header - Linux's al_eth applies its own snoop from
  inside its own probe instead, so there's no shared constant for it.

  Copyright (c) 2026, Awto / Daniel Tyrrell. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Protocol/PciIo.h>
#include <IndustryStandard/Pci.h>

#include "../../../../../hal/pcie-al-alpine-regs.h"

#define AL_ETH_DEVICE_0  0x0001
#define AL_ETH_DEVICE_1  0x0002

STATIC
EFI_STATUS
AlSnoopOneDevice (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINTN                Device
  )
{
  EFI_STATUS  Status;
  UINT32      Value;
  UINTN       Index;

  //
  // Every one of these config accesses used to be issued with its status
  // discarded, and `Value` is an uninitialised local - so a failed read
  // meant writing STACK GARBAGE into the SMCC snoop / APP_CONTROL
  // coherency registers, silently. That is the exact misconfiguration
  // this driver exists to prevent. Check all of them.
  //
  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, AL_ADAPTER_SMCC, 1, &Value);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Value |= AL_ADAPTER_SMCC_SNOOP_BITS;
  Status = PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, AL_ADAPTER_SMCC, 1, &Value);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Device <= AL_INTERNAL_SLOT_THRESHOLD) {
    for (Index = 1; Index < AL_ADAPTER_SMCC_NUM_SUBMASTERS; Index++) {
      Status = PciIo->Pci.Write (
                          PciIo, EfiPciIoWidthUint32,
                          AL_ADAPTER_SMCC + Index * AL_ADAPTER_SMCC_BUNDLE_SIZE,
                          1, &Value);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, AL_ADAPTER_APP_CONTROL, 1, &Value);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Value = (Value & 0xffff0000) | AL_ADAPTER_APP_CONTROL_LO16;
  return PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, AL_ADAPTER_APP_CONTROL, 1, &Value);
}

EFI_STATUS
EFIAPI
AlPcieSnoopFixEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_HANDLE            *Handles;
  UINTN                 Count;
  UINTN                 Index;
  UINTN                 Segment, Bus, Device, Function;
  UINT32                VendorDevice;
  UINT16                VendorId, DeviceId;
  UINTN                 Fixed;
  EFI_PCI_IO_PROTOCOL   *PciIo;

  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiPciIoProtocolGuid, NULL, &Count, &Handles);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "AlPcieSnoopFix: no PciIo handles (%r)\n", Status));
    return EFI_SUCCESS;
  }

  Fixed = 0;
  for (Index = 0; Index < Count; Index++) {
    Status = gBS->HandleProtocol (Handles[Index], &gEfiPciIoProtocolGuid, (VOID **)&PciIo);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);
    if (EFI_ERROR (Status) || Segment != 0 || Bus != 0 || Function != 0) {
      continue;
    }

    Status = PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, PCI_VENDOR_ID_OFFSET, 1, &VendorDevice);
    if (EFI_ERROR (Status)) {
      // Unchecked, this matched on uninitialised stack and could apply the
      // fixup to an unrelated device.
      DEBUG ((DEBUG_WARN, "AlPcieSnoopFix: vendor read failed on %d/%d/%d (%r) - skipping\n",
              (UINT32)Bus, (UINT32)Device, (UINT32)Function, Status));
      continue;
    }
    VendorId = (UINT16)VendorDevice;
    DeviceId = (UINT16)(VendorDevice >> 16);
    if (VendorId != PCI_VENDOR_ID_ANNAPURNA_LABS) {
      continue;
    }
    if ((DeviceId == AL_ETH_DEVICE_0) || (DeviceId == AL_ETH_DEVICE_1)) {
      continue;
    }

    Status = AlSnoopOneDevice (PciIo, Device);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "AlPcieSnoopFix: FAILED on %d/%d/%d (%r) - this device's "
              "DMA may not be cache-coherent\n",
              (UINT32)Bus, (UINT32)Device, (UINT32)Function, Status));
      continue;
    }
    Fixed++;
  }

  DEBUG ((DEBUG_INFO, "AlPcieSnoopFix: applied to %d internal PCIe device(s)\n", Fixed));
  FreePool (Handles);
  return EFI_SUCCESS;
}
