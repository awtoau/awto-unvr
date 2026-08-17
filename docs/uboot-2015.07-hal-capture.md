# Stock U-Boot live capture — read-only (2026-08-17)

Live runtime dump of the running stock bootloader
`U-Boot 2015.07-alpine_db-2.21-HAL (Dec 16 2020)`. This is the **runtime-evidence**
companion to the static-RE boot-chain docs; it does not restate their analysis.

## Provenance

- Captured at the U-Boot prompt via `scripts/uboot-info.tcl` — **all commands read-only**.
- Reached U-Boot with the **SP805 watchdog reset** (`scripts/reboot-to-uboot.tcl`) —
  **no power-cycle** (see [reboot-driver-handover.md](reboot-driver-handover.md)).
- Verbatim dump (complete `printenv`/`help`/`bdinfo`/`nand`/`sf`/`i2c`/`pci`/`dm`/`usb`,
  incl. env MACs/IPs): **[uboot-2015.07-hal-capture.txt](uboot-2015.07-hal-capture.txt)**.

## Runtime-only findings (not derivable from static RE)

- **`dram_margins` crashed U-Boot with a Synchronous Abort** — the Annapurna SRAM agent blob
  is fragile; read-with-care. Folded into [uboot-ddr-port.md](uboot-ddr-port.md) §5.
- **`usb start` finds nothing** → the eMMC (over USB, `bootemmc`) is stranded on this unit,
  confirming the hostless-eMMC probe in [hardware.md](hardware.md).
- Live command set present/absent: `mmc`, `mtdparts`, `clocks`, `date` are **absent**;
  `reset` (SP805) present — a clean-reboot candidate vs the hung Linux path (#51).
- Live `i2c bus` mislabels the mux **`PCA9548@0x71` (8 ch)** — a generic driver mislabel;
  the part is a 4-channel PCA9546 ([hardware.md](hardware.md) I2C map is canonical).

## Where each captured fact is analysed (canonical docs — not repeated here)

| Captured output | Canonical analysis |
|---|---|
| `printenv` env, `bootcmd`, flash-write scripts (`kernelupd`/`dtupd`/`bootupd`/`delenv`) | [uboot-update-path.md](uboot-update-path.md), [nor-boot-chain.md](nor-boot-chain.md) §4 |
| TOC / boot chain / `flash_contents_*` | [nor-boot-chain.md](nor-boot-chain.md), [bootloader.md](bootloader.md) |
| `sf`/`nand` info (MX25U25635F, MT29F8G08) | [hardware.md](hardware.md), [nand-1.3.35.md](nand-1.3.35.md) |
| `i2c bus`, mux + expanders + RTC + ADT7475 map | [hardware.md](hardware.md) I2C map |
| `slowfan` / `fanspeed` fan control | [hardware.md](hardware.md), [gpio-switches-leds.md](gpio-switches-leds.md) |
| `pci` topology, `bdinfo` DRAM banks + MACs | [hardware.md](hardware.md), [nor-boot-chain.md](nor-boot-chain.md) |
| `cpu_set_speed` (downclock-only) / VCO overclock | [overclock-and-caps.md](overclock-and-caps.md) |
| `help` command inventory → porting parity | [uboot-hw-test-suite.md](uboot-hw-test-suite.md), [uboot-port-plan.md](uboot-port-plan.md) |
| `lcd_init` → LCD frame at 9600 on uart2 = RPS UART | [dt-gaps-hardware-of-record.md](dt-gaps-hardware-of-record.md), [rps-subsystem.md](rps-subsystem.md) |
