/** @file
  PciHostBridgeLib for the Ubiquiti UNVR (Alpine V2): internal PCIe
  (P1) plus external PCIe0 (P1.5, ASM1042A xHCI).

  Internal: bus 0 only, flat ECAM at 0xFBC00000, no I/O space, MMIO
  window 0xFE000000-0xFEFFFFFF - cross-checked against docs/hardware.md
  and confirmed identical to imbushuo/ccr2004-uefi's own
  PciHostBridgeLib (same SoC family, same internal-PCIe layout).

  External PCIe0: bus 0 only (same flat-ECAM shape as internal - see
  Platform/Ubiquiti/UNVR/Library/PciSegmentInfoLib), MMIO window
  0xC0010000-0xC7FFFFFF. Per docs/hardware.md's "never retrain an
  already-linked external PCIe port" gotcha (issue #140): this library
  reads LTSSM state first and ONLY adds the external root bridge (and
  applies the matching config-space fixup - CFG_TARGET_BUS, AXI snoop,
  RC-mode COMMAND, PM-state disable, register-for-register identical to
  al_pcie_ext0_port_config_fixup() in our U-Boot fork's
  board/annapurna/alpine/alpine.c) if the link is already up. It never
  attempts link training - if LTSSM is below L0, the external segment
  is simply not presented to PciBusDxe at all.

  Copyright (c) 2026, Awto / Daniel Tyrrell. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>
#include <Protocol/PciRootBridgeIo.h>

//
// External PCIe0 (Alpine V2) - register offsets/values match
// AL_PCIE_EXT0_* in uboot-port/board/annapurna/alpine/alpine.c 1:1,
// so the two can be diffed directly. See docs/uefi.md P1.5.
//
#define AL_PCIE_EXT0_BASE                0xfd800000UL
#define AL_PCIE_EXT0_MASTER_ARCTL        (AL_PCIE_EXT0_BASE + 0x14)
#define AL_PCIE_EXT0_MASTER_AWCTL        (AL_PCIE_EXT0_BASE + 0x18)
#define AL_PCIE_AXI_SNOOP_MASK           (3U << 26)
#define AL_PCIE_EXT0_CFG_TARGET_BUS      (AL_PCIE_EXT0_BASE + 0x30)
#define AL_PCIE_CFG_TARGET_BUS_VAL       0x000000ffU  // mask=0xff, target_bus=0
#define AL_PCIE_EXT0_DEBUG_INFO_0        (AL_PCIE_EXT0_BASE + 0x2080)
#define AL_PCIE_LTSSM_STATE_MASK         0x1F8U
#define AL_PCIE_LTSSM_STATE_SHIFT        3
#define AL_PCIE_LTSSM_STATE_L0           0x11U
#define AL_PCIE_EXT0_CONFIG_HEADER_CMD   (AL_PCIE_EXT0_BASE + 0x10004)  // DBI+4
#define AL_PCIE_CMD_MEM_IO_BM_MASK       0x7U
#define AL_PCIE_EXT0_PORT_REGS_BASE      (AL_PCIE_EXT0_BASE + 0x10700)
#define AL_PCIE_EXT0_AXI_SLAVE_ERR_RESP  (AL_PCIE_EXT0_PORT_REGS_BASE + 0x1D0)
#define AL_PCIE_AXI_SLAVE_ERR_RESP_ALL_MAPPING  (1U << 0)
#define AL_PCIE_EXT0_PM_STATE_PF(n)      (AL_PCIE_EXT0_BASE + 0x24b0 + (n) * 4)
#define AL_PCIE_PM_STATE_DISABLE_MASK    0x180U

STATIC
VOID
AlPcieRegSet (
  IN UINTN   Reg,
  IN UINT32  Mask,
  IN UINT32  Value
  )
{
  UINT32  Current;

  Current = MmioRead32 (Reg);
  MmioWrite32 (Reg, (Current & ~Mask) | (Value & Mask));
}

STATIC
UINT32
AlPcieExt0LtssmState (
  VOID
  )
{
  return (MmioRead32 (AL_PCIE_EXT0_DEBUG_INFO_0) & AL_PCIE_LTSSM_STATE_MASK) >>
         AL_PCIE_LTSSM_STATE_SHIFT;
}

//
// Mirrors al_pcie_ext0_port_config_fixup() in alpine.c exactly - no
// link-state registers touched, config-space/AXI-wrapper glue only.
//
STATIC
VOID
AlPcieExt0PortConfigFixup (
  VOID
  )
{
  AlPcieRegSet (AL_PCIE_EXT0_CFG_TARGET_BUS, 0xffffffffU, AL_PCIE_CFG_TARGET_BUS_VAL);
  AlPcieRegSet (AL_PCIE_EXT0_MASTER_ARCTL, AL_PCIE_AXI_SNOOP_MASK, AL_PCIE_AXI_SNOOP_MASK);
  AlPcieRegSet (AL_PCIE_EXT0_MASTER_AWCTL, AL_PCIE_AXI_SNOOP_MASK, AL_PCIE_AXI_SNOOP_MASK);
  AlPcieRegSet (AL_PCIE_EXT0_CONFIG_HEADER_CMD, AL_PCIE_CMD_MEM_IO_BM_MASK, AL_PCIE_CMD_MEM_IO_BM_MASK);
  //
  // Deliberately NOT setting AL_PCIE_EXT0_AXI_SLAVE_ERR_RESP here (unlike
  // alpine.c's port_config_fixup): it turns a failed downstream PCI
  // access into a hard SError instead of the normal "no device"
  // 0xFFFFFFFF - confirmed live 2026-09-03, crashed EDK2 outright before
  // PciBusDxe could even report what it found. Leaving it off lets
  // enumeration fail soft and actually tell us something.
  AlPcieRegSet (AL_PCIE_EXT0_PM_STATE_PF (1), AL_PCIE_PM_STATE_DISABLE_MASK, 0);
  AlPcieRegSet (AL_PCIE_EXT0_PM_STATE_PF (2), AL_PCIE_PM_STATE_DISABLE_MASK, 0);
  AlPcieRegSet (AL_PCIE_EXT0_PM_STATE_PF (3), AL_PCIE_PM_STATE_DISABLE_MASK, 0);
}

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

STATIC EFI_PCI_ROOT_BRIDGE_DEVICE_PATH mPciRootBridgeDevicePath[2] = {
  {
    {
      { ACPI_DEVICE_PATH, ACPI_DP, { (UINT8)(sizeof (ACPI_HID_DEVICE_PATH)), (UINT8)((sizeof (ACPI_HID_DEVICE_PATH)) >> 8) } },
      EISA_PNP_ID (0x0A08),  // PCIe
      0
    },
    { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { END_DEVICE_PATH_LENGTH, 0 } }
  },
  {
    {
      { ACPI_DEVICE_PATH, ACPI_DP, { (UINT8)(sizeof (ACPI_HID_DEVICE_PATH)), (UINT8)((sizeof (ACPI_HID_DEVICE_PATH)) >> 8) } },
      EISA_PNP_ID (0x0A08),  // PCIe
      1
    },
    { END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, { END_DEVICE_PATH_LENGTH, 0 } }
  },
};

STATIC PCI_ROOT_BRIDGE mPciRootBridges[2] = {
  {
    // Segment 0: internal PCIe - bus 0-0, 16MB MMIO at 0xFE000000, no I/O
    0, 0, 0, TRUE, FALSE, FALSE, EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM,
    { 0, 0 },                    // Bus 0-0
    { MAX_UINT64, 0 },           // I/O: none
    { 0xFE000000, 0xFEFFFFFF },  // Mem: 16MB
    { MAX_UINT64, 0 },           // MemAbove4G: none
    { MAX_UINT64, 0 },           // PMem: none
    { MAX_UINT64, 0 },           // PMemAbove4G: none
    (EFI_DEVICE_PATH_PROTOCOL *)&mPciRootBridgeDevicePath[0]
  },
  {
    // Segment 1: external PCIe0 - bus 0-0, ~128MB MMIO at 0xC0010000, no I/O
    1, 0, 0, TRUE, FALSE, FALSE, EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM,
    { 0, 0 },                    // Bus 0-0
    { MAX_UINT64, 0 },           // I/O: none
    { 0xC0010000, 0xC7FFFFFF },  // Mem: ~128MB
    { MAX_UINT64, 0 },           // MemAbove4G: none
    { MAX_UINT64, 0 },           // PMem: none
    { MAX_UINT64, 0 },           // PMemAbove4G: none
    (EFI_DEVICE_PATH_PROTOCOL *)&mPciRootBridgeDevicePath[1]
  },
};

PCI_ROOT_BRIDGE *
EFIAPI
PciHostBridgeGetRootBridges (
  UINTN  *Count
  )
{
  UINT32  Ltssm;

  Ltssm = AlPcieExt0LtssmState ();
  if (Ltssm >= AL_PCIE_LTSSM_STATE_L0) {
    DEBUG ((DEBUG_INFO, "PciHostBridge: external PCIe0 link already up (LTSSM 0x%x) - "
      "applying config-space fixup, not touching link training\n", Ltssm));
    AlPcieExt0PortConfigFixup ();
    *Count = 2;
  } else {
    DEBUG ((DEBUG_WARN, "PciHostBridge: external PCIe0 link NOT up (LTSSM 0x%x) - "
      "skipping this segment entirely (never cold-retrain, see docs/hardware.md)\n", Ltssm));
    *Count = 1;
  }

  return mPciRootBridges;
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
