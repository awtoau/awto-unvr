# Source-of-truth audit — EDK2 PCIe glue vs U-Boot / Linux

Cross-reference audit of `Platform/Ubiquiti/UNVR/`'s Alpine PCIe register
glue (P1/P1.5, docs/uefi.md) against its two sources of truth: our own
U-Boot fork's `alpine.c` (the working, hardware-verified reference this
was ported from) and, where a real Linux equivalent exists, the mainline
kernel driver. Same method and table format as
[audit-board-shim-dts.md](audit-board-shim-dts.md) - this project keeps
register glue duplicated per-codebase (each needs its own idiomatic
style/build system) rather than shared-sourced, so an explicit,
maintained cross-check is what keeps the duplication honest instead of
silently drifting.

Date: 2026-09-03.

---

## Internal PCIe: SMCC/APP_CONTROL snoop fixup

`Platform/Ubiquiti/UNVR/Drivers/AlPcieSnoopFixDxe/AlPcieSnoopFixDxe.c`
vs `uboot-port/board/annapurna/alpine/alpine.c` vs Linux's
`pcie-al-internal.c` (the same three-way check
audit-board-shim-dts.md already ran for alpine.c alone - extended here
with the EDK2 column).

| our symbol/value | EDK2 file:line | alpine.c file:line | Linux (pcie-al-internal.c) | VERDICT |
|---|---|---|---|---|
| `AL_SMCC 0x110` | AlPcieSnoopFixDxe.c:27 | alpine.c:795 | `AL_ADAPTER_SMCC 0x110` @60 | **match** |
| `AL_SMCC_BUNDLE 0x20` | AlPcieSnoopFixDxe.c:28 | alpine.c:796 | `AL_ADAPTER_SMCC_BUNDLE_SIZE 0x20` @61 | **match** |
| `AL_SMCC_SNOOP 0x3` (OVR\|EN) | AlPcieSnoopFixDxe.c:29 | alpine.c:797 | `SNOOP_OVR BIT(0)\|SNOOP_EN BIT(1)=0x3` @62-65 | **match** |
| `AL_APP_CONTROL 0x220` | AlPcieSnoopFixDxe.c:30 | alpine.c:798 | `AL_ADAPTER_APP_CONTROL 0x220` @76 | **match** |
| `AL_APP_LO16 0x03ff` | AlPcieSnoopFixDxe.c:31 | alpine.c:799 | `AL_ADAPTER_APP_CONTROL_LO16 0x03ff` @77 | **match** |
| `AL_SLOT_THRESH 5` | AlPcieSnoopFixDxe.c:32 | alpine.c:800 | `AL_INTERNAL_SLOT_THRESHOLD 5` @85 | **match** |
| `AL_VENDOR_ID 0x1c36` | AlPcieSnoopFixDxe.c:24 | `AL_VENDOR 0x1c36` @801 | `PCI_VENDOR_ID_ANNAPURNA_LABS 0x1c36` @88 | **match** |
| SM1-3 loop `i=1;i<4` writes SM0 val | AlPcieSnoopFixDxe.c:47-51 | alpine.c:189-191 | `for i=1..3 write val` @153-159 | **match** |
| APP_CONTROL `(v&0xffff0000)\|lo16` | AlPcieSnoopFixDxe.c:53-55 | alpine.c:194 | identical RMW @167 | **match** |
| eth exclusion `1c36:0001`/`0002` | AlPcieSnoopFixDxe.c:99 (`AL_ETH_DEVICE_0`/`_1`) | alpine.c:836-844 (device_id 0x0001/0x0002) | n/a (Linux's own al_eth driver applies snoop from inside its own probe instead, not via this notifier) | **match vs alpine.c** |
| filter: vendor **AND** func==0 **AND** root-bus | AlPcieSnoopFixDxe.c:90-98 (`Function != 0`, `Bus != 0`, `Segment != 0` all checked via `GetLocation()`) | alpine.c:834 (**vendor only** - audit-board-shim-dts.md MUST-FIX #2, still open) | vendor **AND** `PCI_FUNC==0` **AND** root-bus @120,132,135 | **EDK2 matches Linux, stricter than alpine.c** (see note) |

**Note on the last row**: EDK2's filter is the *stricter*, Linux-matching
one (see `AlPcieSnoopFixDxe.c`'s `GetLocation()` check) - alpine.c's own
still-open MUST-FIX #2 (vendor-only filter, no func/root-bus guard) does
not apply here since this is new code, not ported from alpine.c's looser
version. No action needed on the EDK2 side; alpine.c's own gap is
tracked separately in audit-board-shim-dts.md.

---

## External PCIe0: LTSSM check + config-space fixup

`Platform/Ubiquiti/UNVR/Library/PciHostBridgeLib/PciHostBridgeLib.c`
vs `alpine.c`. No Linux mainline equivalent exists for this table -
`pcie-al.c` never touches these registers at all (see
docs/porting-roadmap.md §Phase 4 / `dw_pcie_host_init()`'s generic
link-training path) - the entire fixup is a U-Boot/vendor-HAL-specific
concept, so alpine.c is the *only* source of truth here.

| our symbol/value | EDK2 file:line | alpine.c file:line | VERDICT |
|---|---|---|---|
| `AL_PCIE_EXT0_BASE 0xfd800000` | PciHostBridgeLib.c:40 | alpine.c:117 | **match** |
| `AL_PCIE_EXT0_MASTER_ARCTL base+0x14` | PciHostBridgeLib.c:41 | alpine.c:193 | **match** |
| `AL_PCIE_EXT0_MASTER_AWCTL base+0x18` | PciHostBridgeLib.c:42 | alpine.c:194 | **match** |
| `AL_PCIE_AXI_SNOOP_MASK (3U<<26)` | PciHostBridgeLib.c:43 | alpine.c:196 | **match** |
| `AL_PCIE_EXT0_CFG_TARGET_BUS base+0x30` | PciHostBridgeLib.c:44 | alpine.c:144 | **match** |
| `AL_PCIE_CFG_TARGET_BUS_VAL 0xff` | PciHostBridgeLib.c:45 | alpine.c:145 | **match** |
| `AL_PCIE_EXT0_DEBUG_INFO_0 base+0x2080` (LTSSM) | PciHostBridgeLib.c:46 | alpine.c:171 | **match** |
| `AL_PCIE_LTSSM_STATE_MASK 0x1F8` | PciHostBridgeLib.c:47 | alpine.c:240 | **match** |
| `AL_PCIE_LTSSM_STATE_SHIFT 3` | PciHostBridgeLib.c:48 | alpine.c:241 | **match** |
| `AL_PCIE_LTSSM_STATE_L0 0x11` | PciHostBridgeLib.c:49 | alpine.c:248 | **match** |
| `AL_PCIE_EXT0_CONFIG_HEADER_CMD base+0x10004` | PciHostBridgeLib.c:50 | alpine.c:206 | **match** |
| `AL_PCIE_CMD_MEM_IO_BM_MASK 0x7` | PciHostBridgeLib.c:51 | alpine.c:207 | **match** |
| `AL_PCIE_EXT0_PORT_REGS_BASE base+0x10700` | PciHostBridgeLib.c:52 | alpine.c:163 | **match** |
| `AL_PCIE_EXT0_AXI_SLAVE_ERR_RESP regs+0x1D0` | PciHostBridgeLib.c:53 | alpine.c:216 | **match (constant defined, deliberately not applied - see below)** |
| `AL_PCIE_EXT0_PM_STATE_PF(n) base+0x24b0+n*4` | PciHostBridgeLib.c:55 | alpine.c:225 | **match** |
| `AL_PCIE_PM_STATE_DISABLE_MASK 0x180` | PciHostBridgeLib.c:56 | alpine.c:227 | **match** |
| RMW helper `(v & ~mask) \| (val & mask)` | PciHostBridgeLib.c `AlPcieRegSet()`:59-68 | alpine.c `al_pcie_reg_set()`:305-314 | **match** |
| LTSSM-gated: apply fixup only if `>= L0`, never train | PciHostBridgeLib.c:170-181 | alpine.c:443-465 (`al_pcie_ext0_bringup()`) | **match in spirit** (EDK2 goes further - never even adds the root bridge if link is down, vs alpine.c's cold-bringup fallback; see docs/hardware.md's "never retrain" gotcha) |
| `axi_slave_err_resp` applied | **not applied** (PciHostBridgeLib.c:97 comment) | applied, alpine.c:415-416 | **deliberate deviation** - confirmed live 2026-09-03 this turns a failed downstream access into a hard, uncatchable SError in EDK2 instead of alpine.c's context (where U-Boot's own read path handles it); see docs/uefi.md P1.5 |

**Deliberate deviation, not a bug**: `axi_slave_err_resp` is the one
place EDK2 intentionally diverges from alpine.c. Confirmed live: with it
applied (matching alpine.c exactly), a failed downstream PCI access
crashes the whole firmware with an unrecoverable SError instead of
returning the normal `0xFFFFFFFF` "no device" response, which is
strictly worse for diagnosing the devfn-aliasing issue P1.5 hit (see
docs/uefi.md). Revisit if/when that's fixed.

---

## Host-bridge / memory-map geometry

`Platform/Ubiquiti/UNVR/Library/PciHostBridgeLib/PciHostBridgeLib.c` +
`Library/PciSegmentInfoLib/PciSegmentInfoLib.c` +
`Library/PlatformLib/PlatformLibMem.c` vs `docs/hardware.md`'s own
register table (§PCIe / ECAM) - the canonical source for these, since
none of this has a direct alpine.c or Linux driver equivalent (it's
EDK2's own platform-description concept, not a HAL register table).

| our symbol/value | EDK2 file:line | hardware.md value | VERDICT |
|---|---|---|---|
| internal ECAM `0xfbc00000` | PciSegmentInfoLib.c (seg 0) | `pcie_int_ecam 0xfbc00000` | **match** |
| internal MMIO window `0xfe000000-0xfeffffff` | PciHostBridgeLib.c (segment-0 root bridge) | `pcie_int_ecam` note: integrated-EP window `0xfe000000` (0x1000000) | **match** |
| external ECAM `0xfb600000` | PciSegmentInfoLib.c (seg 1) | `pcie_ext0_win 0xfb600000` | **match** |
| external MMIO window `0xc0010000` size `0x7ff0000` | PciHostBridgeLib.c (segment-1 root bridge) + PlatformLibMem.c's `UNVR_PCIE_EXT0_MEM_BASE`/`_SIZE` | `pcie_ext0_mem 0xc0010000` size `0x7ff0000` | **match** |
| both segments bus range `0-0` | PciSegmentInfoLib.c | flat ECAM, no bus-shift bits (hardware.md's "Silicon gotcha" section + porting-reference.md gotcha #4) | **match** |

---

## External PCIe0: devfn-aliasing guard (`Library/PciSegmentLib/`)

Not a register cross-check like the tables above - this one has no
alpine.c or literal Linux-source equivalent to diff against (U-Boot
apparently never scans past devfn 0, so never needed a guard; Linux's
version lives in generic `pcie-designware-host.c` infrastructure, not
in a single copyable register table). Recorded here anyway since it's
the same class of "known hardware quirk, needs an explicit software
guard" fact the rest of this doc tracks.

| fact | EDK2 file:line | verified how |
|---|---|---|
| external PCIe0 (segment 1) has exactly one real device, at devfn 0/0 - any other devfn on that bus reads back the *same* device's identity instead of "no device" | `PciSegmentLibCommon.c`'s `PciSegmentLibGetEcamAddress()` guard, ~line 33-53 | confirmed live 2026-09-03: without the guard, `PciBus: Discovered PCI @ [00\|00\|00]` through `[00\|09\|00]` all reported identical `VID=0x1B21, DID=0x1142`; with it, exactly one entry, `pci` shell command confirms `Bus 01 Dev 00 Func 00` only |
| conceptually equivalent to Linux's `dw_pcie_rd_other_conf()` devfn-guard (every mainline `pcie-designware-host.c` platform has this) | same | design pattern, not a copied value - see docs/porting-roadmap.md §Phase 4 / `drivers/pci/controller/dwc/pcie-designware-host.c` |

---

## Summary

- **Mismatches: 0.**
- **Deliberate deviations: 2** - (1) EDK2's internal-snoop filter is
  *stricter* than alpine.c's (matches Linux instead, a strict
  improvement, not a gap); (2) `axi_slave_err_resp` intentionally not
  applied on the external path (diagnostic clarity, see above).
- **No Linux equivalent exists** for the external-PCIe0 fixup table -
  alpine.c is the sole source of truth there; flagged explicitly per
  row rather than left implicit.
- Everything else: **byte-for-byte match** against alpine.c, the
  hardware-verified reference this was ported from.
