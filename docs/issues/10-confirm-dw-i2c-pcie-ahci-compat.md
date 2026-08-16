# Confirm DW I2C / DWC PCIe / AHCI register compatibility from the live DTB
labels: porting, verification
---

Nail down which "looks-mainline" blocks are actually register-compatible, using the
live DTB + running drivers, so the DT can use plain mainline compatibles with
confidence.
Roadmap: [target-architecture table](../porting-roadmap.md#target-architecture--strategy-vs-live-capture).

## Confirmed already (live)
- **DW I2C**: `snps,designware-i2c` @ 0xfd880000; live adapter name literally
  `Synopsys DesignWare I2C adapter` ([i2c-devices.txt](../hw-reference/20260816-104601/i2c-devices.txt)).
  Mainline `i2c-designware` should bind unchanged.
- **AHCI**: PCI `1c36:0031`, `prog-if 01 [AHCI 1.0]`, stock `ahci` driver bound,
  `AHCI 0001.0300` ([lspci.txt](../hw-reference/20260816-104601/lspci.txt)) —
  standard AHCI.
- **DW SPI**: `snps,dw-apb-ssi` @ 0xfd882000; `m25p80` bound mx25u25635f.

## To verify
- [ ] DW I2C: bring up under mainline `i2c-designware`; confirm the mux (pca9546 @
  0x71) + RTC/adt7475/pca9575 all enumerate.
- [ ] DWC PCIe: confirm mainline `pcie-designware` + Alpine glue (#05) matches the
  DBI/APP_CONTROL/snoop layout — the one block that is **not** plain-mainline.
- [ ] AHCI: confirm stock `ahci` binds the two controllers, 4 ports each, NCQ, with a
  disk fitted (all links were down at capture).
- [ ] Timing params in DT (`i2c-*-scl-*-raw`, `i2c-sda-hold-time-ns`) — decide
  whether mainline recomputes them from clock or needs them carried over.

## Acceptance criteria
- [ ] Each block marked "mainline-as-is" or "needs quirk", with the register
  evidence recorded in a finding doc.

Relates: #05, #08.
