# [Patch 4-5] DWC PCIe Alpine quirks + internal PCIe controller
labels: porting, pcie, patch
---

The two PCIe glue patches (order #4 DWC quirks, #5 internal controller). Implements
Phase 4 in code.
Roadmap: [Phase 4](../porting-roadmap.md#phase-4--pcie),
[patch order](../porting-roadmap.md#patch-order).

## Patch 4 — DWC PCIe Alpine quirks
- [ ] DBI base offset = controller_base + 0x10000 (external PCIe / xHCI path).
- [ ] AXI coherency/snoop fixup on AL PCI devices.
- [ ] Build on mainline `drivers/pci/controller/dwc/pcie-al.c`.

## Patch 5 — Alpine internal PCIe controller
- [ ] Internal host driver (integrated EPs: eth/dma/ahci/crypto).
- [ ] AXI SMCC snoop regs 0x110/0x130/0x150/0x170 (`SNOOP_OVR|SNOOP_ENABLE`).
- [ ] APP_CONTROL 0x220 = 0x03FF.
- [ ] MSI routing via `al,alpine-msix`.
- [ ] Reset/clock sequencing.

## Evidence
- Internal ECAM `0xfbc00000`, EP window `0xfe000000`; external0 @ `0xfd800000`,
  ECAM `0xfb600000`, `link up Gen2 x1`
  ([live.dts](../hw-reference/20260816-104601/live.dts),
  [dmesg.txt](../hw-reference/20260816-104601/dmesg.txt)).
- RE details: [docs/porting-reference.md](../porting-reference.md) §"Hardware gotchas".

## Acceptance criteria
- [ ] Coherent DMA verified (no stale-cache TX) once eth/dma driver present.
- [ ] Resembles other `pcie-designware-host` platform drivers.

Depends: #12. Blocks: #05, #02, #15, #19, #20.
