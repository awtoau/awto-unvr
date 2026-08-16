# [Patch 3] UNVR board DT (alpine-v2-ubnt-unvr.dts)
labels: porting, devicetree, patch
---

The board file that turns the SoC dtsi into a bootable UNVR.
Roadmap: [patch order](../porting-roadmap.md#patch-order).

## Tasks
- [ ] `alpine-v2-ubnt-unvr.dts` including the SoC dtsi (#12).
- [ ] `chosen` bootargs / stdout-path `ttyS0,115200`.
- [ ] memory: 4 GiB in the two banks from
  [live.dts](../hw-reference/20260816-104601/live.dts) memory node.
- [ ] I2C topology: pca9546 mux @ 0x71 → s35390a (0x30), adt7475 (0x2e); pca9575
  expanders @ 0x20/0x21/0x29.
- [ ] MTD partitions: NAND (corrected map, no `partition@1`/`device_tree`) + SPI-NOR
  (mtd0–11). Exact offsets in [mtd.txt](../hw-reference/20260816-104601/mtd.txt).
- [ ] LEDs (`gpio-leds`: ulogo blue/white), reset button (`gpio-keys`), ramoops
  reserved-memory @ 0x92000000.
- [ ] HDD power control + SATA bay LEDs (SGPO) — see #18.
- [ ] Ethernet + SFP DT — split into #17.

## Acceptance criteria
- [ ] Board DT boots 6.18 (#02); peripherals enumerate matching the live capture.
- [ ] No blindly-copied vendor board values (Phase 7 rule).

Depends: #11, #12. Feeds: #02, #17.
