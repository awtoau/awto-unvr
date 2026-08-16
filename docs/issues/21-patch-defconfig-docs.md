# [Patch 14] defconfig & docs
labels: porting, defconfig, docs, patch
---

Final patch: a UNVR/Alpine V2 defconfig and the porting documentation.
Roadmap: [patch order](../porting-roadmap.md#patch-order).

## Tasks
- [ ] `alpine_v2_ubnt_unvr_defconfig` (or fold into a shared arm64 config) enabling:
  `ARCH_ALPINE`, PSCI, GIC-v3, `i2c-designware`, `spi-dw`+`m25p80`, `pl061`,
  `sp805_wdt`, `rtc-s35390a`, `adt7475`, `pca953x`+`pca954x`, `ahci`, `xhci`,
  `pcie-designware`, `at803x` (**+ `REGULATOR`**), `sfp`+`phylink`, MD/raid456 +
  async_tx, and the new al_eth/al_dma/al_ssm/al_nand/al-sgpo/al-thermal drivers.
- [ ] Boot-tested defconfig for both 6.18 and 7.1.
- [ ] Update [docs/porting-roadmap.md](../porting-roadmap.md) + this repo's docs with
  final build/flash instructions.

## Acceptance criteria
- [ ] `make <defconfig> Image dtbs modules` produces a booting image.
- [ ] Docs let a fresh builder reproduce the image.

Depends: all.
