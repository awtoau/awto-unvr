/** @file
  PciSegmentInfoLib for the Ubiquiti UNVR: two flat, single-bus ECAM
  segments (Alpine V2 has no bus-number bits in either ECAM window's
  address decode - both are windowed by a separate register instead of
  standard MMCONFIG bus-shift addressing, see docs/hardware.md).

  Segment 0: internal PCIe, ECAM 0xFBC00000 (eth/dma/sata EPs, P1).
  Segment 1: external PCIe0, ECAM 0xFB600000 (ASM1042A xHCI, P1.5) -
  PciHostBridgeLib only adds this root bridge once it has confirmed the
  link is already up (see docs/hardware.md's "never retrain an
  already-linked external PCIe port" gotcha) and applied the matching
  config-space fixup, so by the time this segment is ever addressed
  it's known-safe to walk.

  Copyright (c) 2026, Awto / Daniel Tyrrell. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiDxe.h>
#include <Library/PciSegmentInfoLib.h>

STATIC PCI_SEGMENT_INFO mPciSegmentInfo[] = {
  { 0, 0xFBC00000, 0, 0 },  // internal PCIe
  { 1, 0xFB600000, 0, 0 },  // external PCIe0
};

PCI_SEGMENT_INFO *
EFIAPI
GetPciSegmentInfo (
  UINTN  *Count
  )
{
  *Count = ARRAY_SIZE (mPciSegmentInfo);
  return mPciSegmentInfo;
}
