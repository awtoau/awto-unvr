/** @file
  Virtual memory map for the Ubiquiti UNVR (Alpine V2 SoC).

  Values from docs/uefi.md §3, cross-checked against the live DTB
  (docs/hw-reference/20260816-104601/live.dts) and `bdinfo` on the real
  unit (docs/uefi.md §5, 2026-09-02). DRAM is reported by U-Boot as 4
  separate 1 GiB banks (0x0, 0x40000000, 0x80000000, 0x200000000); the
  first three are contiguous, so P0 covers them as one 3 GiB region,
  matching the doc's simplification - not a bug, this is just merging
  adjacent banks the way the doc's own PCD table already does.

  P0 has no PCIe/USB/SATA/net, so the whole SoC device band
  (0xf0000000..0xff000000, covering GIC/PBS/UART/ECAM/integrated-EP MMIO)
  is one Device-nGnRnE region rather than the finer-grained split the doc
  lists for later phases - "simpler for P0; tighten later" per §3.

  Copyright (c) 2026, Awto / Daniel Tyrrell. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/ArmLib.h>
#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#define UNVR_DRAM0_BASE   0x0000000000ULL
#define UNVR_DRAM0_SIZE   0x00C0000000ULL  // 3 GiB (3x 1GiB banks, contiguous)
#define UNVR_DRAM1_BASE   0x0200000000ULL
#define UNVR_DRAM1_SIZE   0x0040000000ULL  // 1 GiB
#define UNVR_DEVICE_BASE  0x00F0000000ULL
#define UNVR_DEVICE_SIZE  0x000F000000ULL  // 0xf0000000..0xff000000

#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  4

/**
  Return the Virtual Memory Map of the platform.
**/
VOID
ArmPlatformGetVirtualMemoryMap (
  OUT ARM_MEMORY_REGION_DESCRIPTOR  **VirtualMemoryMap
  )
{
  ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;
  UINTN                         Index;

  ASSERT (VirtualMemoryMap != NULL);

  VirtualMemoryTable = AllocatePool (
                          sizeof (ARM_MEMORY_REGION_DESCRIPTOR) *
                          MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS
                          );
  if (VirtualMemoryTable == NULL) {
    DEBUG ((DEBUG_ERROR, "ArmPlatformGetVirtualMemoryMap: failed to allocate memory\n"));
    ASSERT (FALSE);
    return;
  }

  Index = 0;

  // DRAM0 (3 GiB)
  VirtualMemoryTable[Index].PhysicalBase = UNVR_DRAM0_BASE;
  VirtualMemoryTable[Index].VirtualBase  = UNVR_DRAM0_BASE;
  VirtualMemoryTable[Index].Length       = UNVR_DRAM0_SIZE;
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
  Index++;

  // DRAM1 (1 GiB, non-contiguous with DRAM0)
  VirtualMemoryTable[Index].PhysicalBase = UNVR_DRAM1_BASE;
  VirtualMemoryTable[Index].VirtualBase  = UNVR_DRAM1_BASE;
  VirtualMemoryTable[Index].Length       = UNVR_DRAM1_SIZE;
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
  Index++;

  // SoC device band: GIC, PBS (UART/I2C/SPI/GPIO/WDT), PCIe ECAM, integrated-EP MMIO
  VirtualMemoryTable[Index].PhysicalBase = UNVR_DEVICE_BASE;
  VirtualMemoryTable[Index].VirtualBase  = UNVR_DEVICE_BASE;
  VirtualMemoryTable[Index].Length       = UNVR_DEVICE_SIZE;
  VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
  Index++;

  // End of table sentinel
  VirtualMemoryTable[Index].PhysicalBase = 0;
  VirtualMemoryTable[Index].VirtualBase  = 0;
  VirtualMemoryTable[Index].Length       = 0;
  VirtualMemoryTable[Index].Attributes   = 0;

  *VirtualMemoryMap = VirtualMemoryTable;
}
