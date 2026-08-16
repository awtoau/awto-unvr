# [Phase 7] Device Tree cleanup & hierarchy
labels: porting, devicetree, phase-7
---

Clean SoC/board DT hierarchy; describe hardware, not driver internals.
Roadmap: [Phase 7](../porting-roadmap.md#phase-7--device-tree-cleanup).

## Structure
- [ ] Extend mainline `arch/arm64/boot/dts/amazon/alpine-v2.dtsi`
- [ ] Optional shared Ubiquiti include
- [ ] `alpine-v2-ubnt-unvr.dts`
- [ ] Separate UDM-Pro and QNAP board files

## DT describes (only)
MMIO/interrupts, clocks/resets, PHY connections, SFP cages, GPIO ownership,
LEDs/fans, MTD partitions, PCIe topology, SATA ports, fixed regulators. It must NOT
encode driver impl details or values copied blindly from old board files.

## Get the partition map right from the start
- Live vendor DT already has the **corrected** NAND map: NO spurious `device_tree`
  partition and NO `partition@1` — `al_boot@0`, `linux_kernel@2` (0x300000),
  `rootfs@3` (0x1300000), `chike@4` (0x3ff00000). Confirms the persistent
  `fdt rm /soc/nand-flash/partition@1` fix is live. See
  [docs/nand-1.3.35.md](../nand-1.3.35.md).
- SPI-NOR map (mtd0–11) exact in
  [live.dts](../hw-reference/20260816-104601/live.dts) /
  [mtd.txt](../hw-reference/20260816-104601/mtd.txt).

## Known DT gotchas (RE'd)
- `al,alpine-msix` needs `interrupt-controller` + `#interrupt-cells` for
  `of_irq_init()`.
- `at803x` PHY `depends on REGULATOR` → `CONFIG_REGULATOR=y` or the PHY is dropped.
- Live msix SPI base 0xa1(161)/count 0x9e(158) differ from mainline dtsi 160/160.

## Acceptance criteria
- [ ] `alpine-v2-ubnt-unvr.dts` builds; nodes match the live MMIO map.
- [ ] No blindly-copied board values; each property traceable to hardware.

Feeds: #12, #13, #17.
