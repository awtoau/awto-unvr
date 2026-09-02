/** @file
  Applies the Alpine internal-PCIe AXI-snoop/APP_CONTROL coherency
  fixup after PCI enumeration completes (docs/uefi.md P1). Mirrors
  al_pcie_snoop_fix()/al_snoop_one() in our U-Boot fork's
  board/annapurna/alpine/alpine.c, register-for-register (issue #74).

  Deliberately excludes al_eth (1c36:0001/0002): applying this fixup to
  it broke UDMA TX and persisted across a warm reset (#74, #90) - U-Boot
  fixed that the same way, by skipping those two device IDs here.

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

#define AL_VENDOR_ID     0x1c36
#define AL_ETH_DEVICE_0  0x0001
#define AL_ETH_DEVICE_1  0x0002
#define AL_SMCC          0x110
#define AL_SMCC_BUNDLE   0x20
#define AL_SMCC_SNOOP    0x3    // SNOOP_OVR | SNOOP_ENABLE
#define AL_APP_CONTROL   0x220
#define AL_APP_LO16      0x03ff
#define AL_SLOT_THRESH   5

STATIC
VOID
AlSnoopOneDevice (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINTN                Device
  )
{
  UINT32  Value;
  UINTN   Index;

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, AL_SMCC, 1, &Value);
  Value |= AL_SMCC_SNOOP;
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, AL_SMCC, 1, &Value);
  if (Device <= AL_SLOT_THRESH) {
    for (Index = 1; Index < 4; Index++) {
      PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, AL_SMCC + Index * AL_SMCC_BUNDLE, 1, &Value);
    }
  }

  PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, AL_APP_CONTROL, 1, &Value);
  Value = (Value & 0xffff0000) | AL_APP_LO16;
  PciIo->Pci.Write (PciIo, EfiPciIoWidthUint32, AL_APP_CONTROL, 1, &Value);
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

    PciIo->Pci.Read (PciIo, EfiPciIoWidthUint32, PCI_VENDOR_ID_OFFSET, 1, &VendorDevice);
    VendorId = (UINT16)VendorDevice;
    DeviceId = (UINT16)(VendorDevice >> 16);
    if (VendorId != AL_VENDOR_ID) {
      continue;
    }
    if ((DeviceId == AL_ETH_DEVICE_0) || (DeviceId == AL_ETH_DEVICE_1)) {
      continue;
    }

    AlSnoopOneDevice (PciIo, Device);
    Fixed++;
  }

  DEBUG ((DEBUG_INFO, "AlPcieSnoopFix: applied to %d internal PCIe device(s)\n", Fixed));
  FreePool (Handles);
  return EFI_SUCCESS;
}
