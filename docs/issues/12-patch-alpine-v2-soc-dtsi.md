# [Patch 2] Alpine V2 SoC .dtsi (extend the existing mainline dtsi)
labels: porting, devicetree, patch
---

Extend `arch/arm64/boot/dts/amazon/alpine-v2.dtsi` with the SoC peripherals mainline
currently omits. Also covers **patch 6** (clock/reset additions, if required).
Roadmap: [patch order](../porting-roadmap.md#patch-order),
[Phase 7](../porting-roadmap.md#phase-7--device-tree-cleanup).

## Mainline gap (what dtsi already has vs. needs)
Mainline dtsi has: CPUs (`arm,cortex-a57`), PSCI, GIC-v3, arch timer, PMU,
`pci-host-ecam-generic` @ 0xfbc00000, `al,alpine-msix`, 4× UART, 4× SP804 timer.
Add, from [live.dts](../hw-reference/20260816-104601/live.dts):

- [ ] `snps,designware-i2c` @ 0xfd880000 (+ 0xfd894000)
- [ ] `snps,dw-apb-ssi` SPI @ 0xfd882000
- [ ] `arm,pl061` GPIO ×6 @ 0xfd887000+
- [ ] `arm,sp805` watchdog ×4 @ 0xfd88c000+
- [ ] `annapurna-labs,al-thermal` @ 0xfd860a00 + thermal-zones
- [ ] `annapurna-labs,al-nand` @ 0xfa100000
- [ ] `annapurna-labs,alpine-sgpo` @ 0xfd8b4000
- [ ] `annapurna-labs,al-serdes` @ 0xfd8c0000
- [ ] internal + external PCIe host nodes (see #14)
- [ ] fixed-clocks: refclk 100MHz, sbclk 500MHz, nbclk, cpuclk (no CCF controller
  node observed → **patch 6 likely reduces to fixed-clocks**)
- [ ] SoC service blocks as needed: al-ccu @ 0xf0090000, al-nb-service @ 0xf0070000,
  al-pbs @ 0xfd8a8000, alpine-mc @ 0xf0080000

## Fix vs vendor dtsi
- CPU compatible → `arm,cortex-a57` (mainline), not vendor's generic `arm,armv8`.
- msix SPI base/count are per-board — set from live (0xa1 / 0x9e), not the dtsi
  default 160/160.

## Acceptance criteria
- [ ] dtsi builds; `dtbs_check` clean against #11.
- [ ] Board DT (#13) can reference every SoC node it needs.

Depends: #11. Feeds: #13, #14, #17, #18.
