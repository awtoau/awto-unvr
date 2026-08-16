# UNVR hardware — chips and connectors

Board: **`alpine_v2_ubnt one nas v5.0`**, BOM **113-2832-29**, sysid **0xea16**
("UNVR without eMMC"). Unit serial `74acb941a811`.

Evidence marks: ✅ read from the running unit (U-Boot banner, `/proc`, PCI
enumeration, MTD, EEPROM); 📄 from prior-art projects, not yet confirmed here;
❓ inferred, needs a physical check.

Sources for ✅ rows: the U-Boot boot banner captured at
`tmp/logs/unvr-console.log`, `scripts/analyse-mtd.py` output, and
`docs/nand-1.3.35.md`.

**Live capture 2026-08-16** (running kernel `4.19.152-alpine-unvr`, firmware
5.1.25) in [hw-reference/20260816-104601/](hw-reference/20260816-104601/) —
ground-truth for the mainline port. Mainline-driver mapping table at the
[bottom](#live-capture-mainline-driver-mapping). Port plan:
[porting-roadmap.md](porting-roadmap.md).

## Main silicon

| # | Part | Type | Detail | Evidence |
|---|---|---|---|---|
| U1 | **Annapurna Labs AL-324** | SoC | `Device ID = a324`, Alpine V2. Quad Cortex-A57 @ 1.7 GHz, ARMv8-A | ✅ U-Boot: `Device ID = a324` |
| — | — | SoC rev | `EEPROM Revision ID = 39`, `Device Info: v2sil-39-rc1` | ✅ |
| U? | DRAM | RAM | **4 GiB**. SPD read over I2C at address **0x57** | ✅ `DRAM: 4 GiB`, `SPD I2C Address: 57` |
| U? | **Macronix MX25U25635F** | SPI-NOR | **32 MiB**, page 256 B, erase 4 KiB, 1.8 V | ✅ `SF: Detected MX25U25635F` |
| U? | NAND flash | NAND | **1024 MiB**, erase block 256 KiB | ✅ `NAND: 1024 MiB`, MTD erasesize `0x40000` |

Note the SPI-NOR is 32 MiB but the MTD partitions on it total ~32 MiB
(mtd5–mtd12) — the whole chip is partitioned. NAND is 1024 MiB and holds
mtd0–mtd4.

## PCI devices — enumerated by U-Boot

| BDF | Vendor:Device | Class | What it is | Evidence |
|---|---|---|---|---|
| `00:01.0` | `1c36:0001` | Network controller | **al_eth1 [PRIME]** — integrated Ethernet | ✅ |
| `00:02.0` | `1c36:0002` | Network controller | **al_eth2** — second integrated Ethernet | ✅ |
| `00:04.0` | `1c36:0022` | Cryptographic device | Annapurna SSM crypto engine (`al_ssm`) | ✅ |
| `00:05.0` | `1c36:0022` | Mass storage | Annapurna SSM/RAID offload (`al_dma`) | ✅ |
| `00:08.0` | `1c36:0031` | Mass storage | SATA controller | ✅ |
| `00:09.0` | `1c36:0031` | Mass storage | SATA controller (second) | ✅ |
| `01:00.0` | `1b21:1142` | Serial bus controller | **ASMedia ASM1042A USB 3.0 xHCI host** — behind external-PCIe0 | ✅ live `lspci` subsystem string |

`1c36` is Annapurna Labs' PCI vendor ID. **Two SATA controllers** (`00:08.0`,
`00:09.0`) serve the four bays.

External PCIe link: **`PCIE_0: Link up. Speed 5GT/s Width x1`** ✅ — Gen2 x1 to
the ASMedia USB controller.

## Networking

| Item | Detail | Evidence |
|---|---|---|
| MACs | Base `74:ac:b9:41:a8:11`, **+2 allocated** (`mac: [74acb941a811] + [2]`) | ✅ |
| `al_eth1` | PRIME interface. Linux names it `enp0s1` | ✅ U-Boot + `ip addr` |
| `al_eth2` | Second interface | ✅ |
| 10G SFP+ | 1 × SFP+ cage — `enp0s2`/`al_eth2` (`1c36:0002`), SFP optic, LM mode `AL_ETH_LM_MODE_10G_OPTIC`, mdio 2500 kHz, no ext PHY | ✅ live dmesg |
| 1G RJ45 | 1 × RJ45 — `enp0s1`/`al_eth1` (`1c36:0001`), RGMII | ✅ live dmesg |
| 1G PHY | **Qualcomm Atheros AR8031** at MDIO addr 4 (`driver Atheros 8031`) → mainline `at803x` (needs `CONFIG_REGULATOR=y`) | ✅ live dmesg (corrects earlier "Marvell 10G" guess — that was the 10G port, which is SFP not a PHY) |
| SFP module | **NOT captured** — `ethtool` returned "No data available"; module EEPROM/vendor unconfirmed | ❓ read on hardware |

The base MAC and the "+2" allocation come from the EEPROM at offset `0x0000`.

## Storage subsystem

| Item | Detail | Evidence |
|---|---|---|
| Bays | 4 × 3.5"/2.5" hot-swap | 📄 |
| Controllers | 2 × Annapurna SATA (`1c36:0031`) | ✅ |
| Internal boot USB | On the ASM1142 USB host; enumerates as **`/dev/sdq`** | ✅ deliberately after the 16 possible `/dev/sd[a-p]` HDD slots |
| HDD power control | `ubnt-hdd-pwrctl` platform driver, GPIOs: **pwren 464, present 468, fault-led 476**, pwren delay 500 ms | ✅ kernel log |
| Drives fitted | 1 × 4 TB (`sda`, foreign), 2 × 8 TB (`sdb`,`sdc`, UniFi arrays) | ✅ |

## Sensors and misc

| Item | Detail | Evidence |
|---|---|---|
| Temperature | 3 sensors read at boot: `dtt_get_temp[0] 103`, `[1] 99`, `[2] 101`; SoC reported `temprature: 50 degrees` | ✅ |
| Fans | 3 ×, PWM, ADT7475 hwmon | 📄 linux-alpine-v2 |
| I2C | SPD at 0x57; `Setting bus to 4` at boot — at least 5 buses | ✅ |
| GPIO | `gpio: pin 37 (gpio 37) value is 0` read during boot | ✅ purpose unknown |
| GPIO expander | PCA9575 — pins 4, 5, 8 known, rest undocumented | 📄 linux-alpine-v2 |
| RTC | S35390A | 📄 |
| Watchdog | ARM SP805 | 📄 |
| LED | `ulogo_ctrl` with a `pattern` sysfs attribute (`2:500 1:500` = white/blue) | ✅ from `product-override` |

## Connectors

| Connector | Detail | Evidence |
|---|---|---|
| **UART** | 4-pin header **behind the SFP+ cage, mid-PCB**. Pins: GND, TXD, RXD, 3V3. **Use only the first three — do not connect 3V3.** 115200 8N1, 3.3 V TTL | 📄 pinout from NeccoNeko; ✅ baud/settings confirmed working |
| SFP+ | 1 × 10G cage | 📄 |
| RJ45 | 1 × 1G | 📄 |
| SATA | 4 × backplane, hot-swap | 📄 |
| Power | DC barrel; UNVR Pro instead has USP-RPS | 📄 |
| Reset | Front-panel button; hold ~10 s at power-on for recovery | 📄 |
| Internal USB | Glued-down flash stick between PSU and CPU. Heat gun or hair dryer + dental floss to free it | 📄 |

## Boot ROM chain

| Stage | Version | Detail | Evidence |
|---|---|---|---|
| stage2_loader | v2.22.3 | Reads SPD over I2C, brings up DRAM | ✅ |
| Stage 3 | v2.22.0 | commit `6088bc3`, CVOS `bac1d52`, HAL `61afa9c`, built **Sep 8 2020 11:40:22** | ✅ |
| agent_wakeup | v2.10 | Reads EEPROM identity | ✅ |
| Board config | `alpine_v2_ubnt one nas v5.0` | DT loaded to `0x01100000`, 26208 bytes; application 689728 bytes | ✅ |
| U-Boot | **2015.07-alpine_db-2.21-HAL** | Built **Dec 16 2020 05:54:51 +0800**, `jenkins-amaz-alpinev2-boot-master-161` | ✅ |

## Boot-time observations worth keeping

- **`*** Warning - bad CRC, using default environment`** ✅ — confirms `mtd6`/`mtd7`
  are zeroed and U-Boot runs on compiled-in defaults. See
  [nand-1.3.35.md](nand-1.3.35.md).
- **`eeprom_per_device_init: no valid information found!`** and
  **`power_init_board: EEPROM per device information is not valid - using defaults!`**
  ✅ — a *second* EEPROM region, separate from the identity block, is
  unpopulated. Power settings fall back to defaults. Not investigated.
- **`U-Boot script not found in TOC!`** ✅ — a TOC-based script slot exists and is
  empty. Possible customisation hook, unexplored.
- **`Boot: warm`** ✅ — distinguishes warm from cold reset; `reboot=cold` appears
  in the kernel cmdline.
- **`ubnt-hdd-pwrctl ... Failed to request gpio 468 pin: -517`** ✅ — `-517` is
  `-EPROBE_DEFER`, normal during probe ordering, not an error.

## Open questions

- Which physical chip provides the 10G PHY, and its MDIO address on this board.
- PCA9575 full pin map (only 4, 5, 8 known, from another project).
- What the second, unpopulated EEPROM region is for.
- The `chike` NAND partition holds a 2095×1692 JPEG with Exif — purpose unknown.
- Whether `dtt_get_temp[0..2]` maps to the three fan-adjacent sensors.
- SFP+ module type/vendor — `ethtool` gave no module data on the vendor kernel.

## MMIO and address map

**Single source of truth for the SoC address map** — all other docs link here, do
not re-table it. Verified against
[hw-reference/20260816-104601/live.dts](hw-reference/20260816-104601/live.dts)
(`reg=`/`compatible=`) + [iomem.txt](hw-reference/20260816-104601/iomem.txt)
(Linux `/proc/iomem`). Sizes are the DT `reg` span. HAL headers are in
`/mnt/2tb/unvr-port-refs/delroth-alpine_hal/` (see [ghidra.md](ghidra.md#4-register-naming--mechanical)).

### DRAM (two banks)

| region | base | size | notes |
|---|---|---|---|
| DRAM0 | `0x00000000` | `0xC0000000` (3 GiB) | bank 0; kernel/reserved carve-outs within |
| DRAM1 | `0x200000000` | `0x40000000` (1 GiB) | bank 1, above 4 GiB |

### On-SoC PBS peripherals (`compatible` → mainline driver → HAL header)

| region | base | size | compatible | driver | HAL header |
|---|---|---|---|---|---|
| i2c0 (i2c-pld) | `0xfd880000` | 0x1000 | `snps,designware-i2c` | i2c-designware | `pbs/al_hal_i2c_regs.h` |
| spi0 (NOR) | `0xfd882000` | 0x1000 | `amazon,alpine-dw-apb-ssi`,`snps,dw-spi-mmio`,`snps,dw-apb-ssi` | spi-dw + m25p80 | `pbs/al_hal_spi_regs.h` |
| uart0 | `0xfd883000` | 0x1000 | **`ns16550a`** (8250 DW, **NOT PL011**) | 8250_dw | `pbs/al_hal_uart_regs.h` |
| uart1 | `0xfd884000` | 0x1000 | `ns16550a` | " | " |
| uart2 | `0xfd885000` | 0x1000 | `ns16550a` (status okay) | " | " |
| uart3 | `0xfd886000` | 0x1000 | `ns16550a` (DT disabled; not in iomem) | " | " |
| gpio0 | `0xfd887000` | 0x1000 | `arm,pl061` | pl061 | `pbs/al_hal_gpio_regs.h` |
| gpio1 | `0xfd888000` | 0x1000 | `arm,pl061` | pl061 | " |
| gpio2 | `0xfd889000` | 0x1000 | `arm,pl061` | pl061 | " |
| gpio3 | `0xfd88a000` | 0x1000 | `arm,pl061` | pl061 | " |
| gpio4 | `0xfd88b000` | 0x1000 | `arm,pl061` | pl061 | " |
| wdt0..3 | `0xfd88c000` | 4×0x1000 | `arm,sp805`,`primecell` | sp805_wdt | `sys_services/al_hal_watchdog_regs.h` |
| timer0..3 | `0xfd890000` | 4×0x1000 | `arm,sp804`,`primecell` | sp804 | `sys_services/al_hal_timer_regs.h` |
| i2c1 | `0xfd894000` | 0x1000 | `snps,designware-i2c` | i2c-designware | `pbs/al_hal_i2c_regs.h` |
| otp_efuse | `0xfd896000` | 0x1000 | (not in DT/iomem — inferred) | — | `sys_services/al_hal_otp_regs.h` |
| gpio5 | `0xfd897000` | 0x1000 | `arm,pl061` | pl061 | `pbs/al_hal_gpio_regs.h` |
| pbs regfile / pinctrl | `0xfd8a8000` | 0x1000 | `annapurna-labs,al-pbs`,`alpine-pinctrl` | — | `pbs/al_hal_pbs_regs.h` |
| sgpo (bay LEDs) | `0xfd8b4000` | 0x5000 | `annapurna-labs,alpine-sgpo` | (custom al-sgpo) | `pbs/al_hal_sgpo_regs.h` |
| serdes | `0xfd8c0000` | 0x2400 | `annapurna-labs,al-serdes` | (custom al-serdes) | — |
| thermal | `0xfd860a00` | 0x100 | `annapurna-labs,al-thermal` | (custom al_thermal) | — |

> **eFuse modulus-hash** compared in preboot lives at **`0xfd89608c` (32 B, to
> 0xfd8960ac)** inside the `otp_efuse` block ([preboot-decompile.md](preboot-decompile.md)
> §RSA). `otp_efuse` @`0xfd896000` is **inferred** — not in live.dts or iomem; its
> base is chosen to cover `0xfd89608c` and its 0x1000 size is a guess (OTP is
> one-time-programmed). All other rows above are confirmed from DT + iomem.

### SoC service / fabric blocks

| region | base | size | compatible / role |
|---|---|---|---|
| GIC-v3 dist | `0xf0200000` | 0x10000 | `arm,gic-v3` (GICR @`0xf0280000` 0x200000; +f0100000/f0110000/f0120000) |
| nb-service | `0xf0070000` | 0x10000 | `annapurna-labs,al-nb-service`,`syscon` — **= preboot agent mailbox** |
| memctl | `0xf0080000` | 0x10000 | `annapurna-labs,alpine-mc` |
| ccu | `0xf0090000` | 0x10000 | `annapurna-labs,al-ccu` — **= preboot agent mailbox** |
| msix | `0xfbe00000` | 0x100000 | `annapurna-labs,alpine-msix`,`al,alpine-msix` |
| al-nand | `0xfa100000` | 0x202000 | `annapurna-labs,al-nand` (custom driver) |
| tdm | `0xf2300000` | 0x11000 | `annapurna-labs,al-tdm` |

### PCIe / ECAM

| region | base | size | compatible / role |
|---|---|---|---|
| pcie_int_ecam | `0xfbc00000` | 0x100000 | `annapurna-labs,alpine-internal-pcie`; integrated-EP window `0xfe000000` (0x1000000) |
| pcie_ext0_ctl | `0xfd800000` | 0x20000 | `annapurna-labs,alpine-external-pcie` (ECAM); `cfg-space-offset 0x10000` |
| pcie_ext1_ctl | `0xfd820000` | 0x20000 | `…external-pcie` (DT disabled) |
| pcie_ext2_ctl | `0xfd840000` | 0x20000 | `…external-pcie` (DT disabled) |
| pcie_ext3_ctl | `0xfd900000` | 0x20000 | `…external-pcie` (DT disabled) |
| pcie_ext0_win | `0xfb600000` | 0x100000 | external0 cfg/window (ext1/2/3 → fb700000/fb800000/fb900000) |
| pcie_ext0_mem | `0xc0010000` | 0x7ff0000 | external0 mem BAR space; **xHCI `1b21:1142` @`0xc0010000`** (0001:00:00.0) |

### Integrated-EP IO fabric (eth / dma / sata), under the internal-PCIe window

HAL: eth `drivers/eth/al_hal_eth_*_regs.h`; udma `include/udma/al_hal_udma_regs.h`;
adapter `include/io_fabric/al_hal_unit_adapter_regs.h`. Kernel copies:
`urnvr-kernel-4.19.152/drivers/net/ethernet/al/`. AHCI is stock (`ahci`). These are
PCI-enumerated EPs (bind by PCI ID, not DT); the fixed windows below are the MMIO the
EPs decode. (Bare `eth0..3` platform nodes at `0xfc000000`+ exist in DT but are
**unused** by the driver.)

| region | base | size | what |
|---|---|---|---|
| eth0 | `0xfe000000` | 0x20000 | `al_eth` |
| eth1 | `0xfe020000` | 0x20000 | `al_eth` |
| dma0 | `0xfe0e0000` | 0x20000 | `al_dma` (udma) |
| dma1 | `0xfe100000` | 0x20000 | `al_dma` |
| eth2 | `0xfe120000` | 0x10000 | `al_eth` |
| dma2 | `0xfe140000` | 0x10000 | `al_dma` |
| eth3 | `0xfe150000` | 0x4000 | `al_eth` |
| ahci0 | `0xfe154000` | 0x4000 | SATA (00:08.0, ata1–4) |
| ahci1 | `0xfe158000` | 0x4000 | SATA (00:09.0, ata5–8) |
| eth4 | `0xfe15c000` | 0x1000 | `al_eth` |
| eth5 | `0xfe15d000` | 0x1000 | `al_eth` |

### Preboot-only SoC regions (not in Linux DT)

From [preboot-decompile.md](preboot-decompile.md); Ghidra `--preboot` adds these.

| region | base | size | what |
|---|---|---|---|
| s2_sram | `0xf2200000` | 0x40000 | S2 first-stage link base (SRAM) |
| agent_mb0 | `0xf0070000` | 0x1000 | CVOS DDR agent mailbox = nb-service block above |
| agent_mb1 | `0xf0090000` | 0x1000 | CVOS agent mailbox = ccu block above |
| ddr_ready | `0xfbff4000` | 0x1000 | `_DAT_fbff4150 == 0x31415926` DDR-ready poll |

**Standard-IP notes (do not re-derive):** UART is 8250-style DesignWare
(`al_hal_uart_regs.h` = rbr_thr/dll/ier…), **not PL011** — use the 8250 layout. GPIO
is genuine `arm,pl061` (PrimeCell); the AL HAL wraps the PL061 block
(`gpiodata[0x100]`@0x0, `gpiodir`@0x400). Where iomem and DT disagree on presence
(uart3, wdt1..3, timers — DT-disabled, so absent from iomem), **trust DT** for block
layout; iomem lists only what a live driver claimed.

## Live capture mainline driver mapping

From [hw-reference/20260816-104601/](hw-reference/20260816-104601/), 2026-08-16.
Block | mainline driver | compatible / PCI ID | evidence file.

### CPU / interrupt / core

| Block | Mainline driver | Compatible / ID | Evidence |
|---|---|---|---|
| 4× Cortex-A57 @ arm64 | arch/arm64 + PSCI | `arm,cortex-a57`, `arm,psci-0.2` (smc) | cpuinfo.txt (`CPU part 0xd07`), dmesg (`PSCIv0.2`, 4 CPUs) |
| Generic timer | arch_timer | `arm,armv8-timer` | dmesg `cp15 timer 58.33MHz` |
| GIC | irq-gic-v3 | `arm,gic-v3` @ 0xf0200000 | dmesg `GICv3`, live.dts |
| MSI-X | irq-alpine-msi | `al,alpine-msix` @ 0xfbe00000 | interrupts.txt (MSIx), live.dts |
| RAM | — | 4 GiB (`0x40000000`+`0x200000000` banks) | meminfo (4040188 kB), live.dts memory |

### On-SoC peripherals

| Block | Mainline driver | Compatible / ID | Evidence |
|---|---|---|---|
| UART ×4 | 8250 (ns16550a) | `ns16550a` @ 0xfd883000, shift 2, 500MHz | interrupts.txt (ttyS0), live.dts |
| I2C ×2 | i2c-designware | `snps,designware-i2c` @ 0xfd880000 | i2c-devices.txt (`Synopsys DesignWare I2C adapter`) |
| SPI + NOR | spi-dw + m25p80 | `snps,dw-apb-ssi` @ 0xfd882000; mx25u25635f 32MB | dmesg (`found mx25u25635f`) |
| GPIO ×6 | pl061 | `arm,pl061` @ 0xfd887000+ | gpio.txt (gpiochip2–7) |
| I2C mux | pca954x | `pca9546` @ 0x71 | i2c-devices.txt |
| GPIO expanders ×3 | pca953x | `nxp,pca9575` @ 0x20/0x21/0x29 | i2c-devices.txt, gpio.txt |
| RTC | rtc-s35390a | `sii,s35390a` @ i2c 0x30 | dmesg (`rtc-s35390a 1-0030`) |
| Fan/hwmon | adt7475 | `adi,adt7475` @ i2c 0x2e | hwmon.txt, dmesg (rev 1) |
| Watchdog ×4 | sp805_wdt | `arm,sp805` @ 0xfd88c000+ | live.dts |
| Thermal | (custom) al_thermal | `annapurna-labs,al-thermal` @ 0xfd860a00 | thermal.txt (zone0 50°C) |
| NAND | (custom) al_nand | `annapurna-labs,al-nand` @ 0xfa100000; Micron MT29F8G08ABBCAH4 | dmesg, modules.txt |
| SGPO (bay LEDs) | (custom) al-sgpo | `annapurna-labs,alpine-sgpo` @ 0xfd8b4000 | gpio.txt (gpiochip8, 64 lines) |
| SerDes | (custom) al-serdes | `annapurna-labs,al-serdes` @ 0xfd8c0000 | live.dts |

### PCIe endpoints & controllers

| Block | Mainline driver | Compatible / ID | Evidence |
|---|---|---|---|
| Internal PCIe host | pcie-designware + Alpine glue | `annapurna-labs,alpine-internal-pcie` ECAM 0xfbc00000 | live.dts, iomem.txt |
| External PCIe0 host | pcie-designware + Alpine glue | `annapurna-labs,alpine-external-pcie` @ 0xfd800000, ECAM 0xfb600000 | dmesg (`link up Gen2 x1`) |
| 1G MAC | (custom/new) al_eth | PCI `1c36:0001` | lspci, dmesg |
| 10G SFP+ MAC | (custom/new) al_eth | PCI `1c36:0002` | lspci, dmesg |
| SATA ×2 | ahci (mainline) | PCI `1c36:0031`, AHCI 1.0, 4 ports ea | lspci, dmesg |
| USB xHCI | xhci_hcd (mainline) | PCI `1b21:1142` (ASM1042A) on ext-PCIe0 | lspci, dmesg |
| RAID/XOR | (custom) al_dma → dmaengine | PCI `1c36:0022` (RAID class) | lspci, dmesg |
| Crypto | (custom) al_ssm → crypto-engine | PCI `1c36:0022` (crypto class) | lspci |

### Storage state at capture

- Boot media: SanDisk Ultra 58.9GiB USB → `/dev/sdq` (blockdev.txt, lsusb.txt
  `0781:5581`). Overlay squashfs+ext4 (mounts.txt).
- **All 8 AHCI ports `SATA link down`** — no HDDs spun up at capture time (dmesg).

## From multi-boot logs (deltas beyond the single live capture)

New/corroborating detail swept from all ~20+ boots in `tmp/logs/unvr-console.log`
(cited by console timestamp). The live capture is one 5.1.25 boot with drives
spun down and reduced dmesg; the old 1.3.35 (`4.1.37-ubnt`) and first al324
(`4.19.152-ui-alpine`) boots carry things it never showed.

### Kernel generations seen (context for the deltas)

| Firmware | Kernel banner | Build | Console ts |
|---|---|---|---|
| 1.3.35 (arm64) | `4.1.37-ubnt` | Dec 16 2020, gcc 6.3 | 17:03:20 |
| 1.4.9 / 2.3.14 (al324) | `4.19.152-ui-alpine` | May 4 2021, gcc 6.3 | 18:36:16, 19:04:51 |
| 4.1.22 (al324) | `4.19.152-alpine-unvr` | Apr 14 2025, gcc 10.2 | 21:12:10 |
| 5.1.25 (al324) | `4.19.152-alpine-unvr` | Jul 10 2026, gcc 10.2 | 21:24:37 |

- 1.4.9 kernel prints `Machine model: Annapurna Labs Alpine V2 UBNT` (18:36:16).
- **2.3.14 and 3.1.16 boot dmesg never reached the log** — logging was at
  emergency-only during those flashes; only their login prompts appear (19:34, 20:38).
  No hardware lines from those two hops. Not a hardware fact — a capture gap.

### Drives fitted (the live capture had all links DOWN)

The 1.3.35 cold boot (17:03) spun up and identified all three drives — exact
models, not in the chip table before:

| ata | ctrl (BDF) | Linux | Model | Cap | FW | ts |
|---|---|---|---|---|---|---|
| ata1 | 00:08.0 (abar 0xfe154000) | sda | **Seagate ST4000DM000-1F2168** | 4 TB (7814037168 s) | CC52 | 17:03:38 |
| ata5 | 00:09.0 (abar 0xfe158000) | sdc | **WDC WD82PURZ-85TEUY0** (WD Purple) | 8 TB (15628053168 s) | 82.00A82 | 17:03:43 |
| ata7 | 00:09.0 | sdb | **WDC WD82PURZ-85TEUY0** (WD Purple) | 8 TB | 82.00A82 | 17:03:42 |

- Controller split confirmed: **00:08.0 = ata1–4** (abar `0xfe154000`), **00:09.0
  = ata5–8** (abar `0xfe158000`), 4 ports each, AHCI 0001.0300, 6 Gbps, all
  `UDMA/133`. Corroborates the "two SATA controllers, 4 bays" note.
- **ata5 & ata7 (the two 8 TB WD on 00:09.0) fail `COMRESET (errno=-16)` on every
  cold boot, then `SATA link up 6.0 Gbps` after ~22 s** — slow spin-up, seen
  17:03, 18:35, 19:05, 19:16, 19:44, 20:36, 20:45. Consistent, not a fault.
- Runtime PHY hiccup on ata1/sda under 1.3.35: `exception … SErr 0x10200 … hard
  resetting link` ×3 at 17:45. `failed to get fault-led -22` printed per controller
  (both), every boot.
- SR-IOV VF probe noise (both generations): `ahci 0001:00:00.0: writing to VF
  config space; probe of 0001:00:00.0 failed with error -22` (17:03:21), and `AL
  PCIe bridge quirk detected, disable MSI(X)` on bus 0001 (18:36:17). Benign.

### Ethernet — al_eth driver changes generation

- 1.3.35: `al_eth v0.2 (Mar 29, 2016)`, "unified 1GbE and 10GbE" (17:03:21).
- 1.4.9+: `al_eth v3.5.3 (Mar 14, 2019)` **with SR-IOV**, "1GbE/10GbE/25GbE
  integrated", `eth rev_id 2` (port1) / `rev_id 3` (port2), does an FLR on probe
  (18:36:17). Both log `Unable to find compatible OF node` — driver runs off PCI
  IDs, not DT.
- Board-info lines identical across all boots (corroborate the chip table):
  port1 `phy exist Yes, addr 4, mdio 1000 kHz, SFP No, media 1` (RGMII 1G, AR8031
  `driver Atheros 8031`, `phy[4]:supported 2ef adv 2ef`); port2 `phy No, addr 0,
  mdio 2500 kHz, SFP Yes, media 5` (10G optic). MAC `74:ac:b9:41:a8:11` (eth0) /
  `…:12` (eth1). RJ45 negotiates `100Mbps/Full` (17:03:28). SFP: `AL_ETH_LM_MODE
  _DISCONNECTED → 10G_OPTIC`, then `link established / wasn't established`
  flapping (17:11, 18:19).

### RAID6 / xor benchmark (not previously recorded)

md picks `neonx8` gen + `32regs` xor on every boot:

| Kernel / ts | neonx8 gen() | best xor | recovery |
|---|---|---|---|
| 4.1.37 (17:03:24) | 3891 MB/s | 32regs 7964 MB/s | intx1 |
| 4.19.152-ui (18:36:21) | 3777 MB/s (xor 3358, rmw) | 32regs 7967 MB/s | neon |
| 4.19.152-ui (19:04:57) | 3754 MB/s | **8regs 10572 MB/s** | neon |
| 5.1.25 (21:24:42 / 10:46:34) | 3630 MB/s (xor 3375, rmw) | 32regs 7725 MB/s | neon |

Full int64x1..x8 / neonx1..x8 tables are at those line ranges if needed. Numbers
are software-checksum micro-benches, not disk throughput.

### CPU / NAND / identity extras

- **CPU MIDR `0x411fd073`** = Cortex-A57 **r1p3** (18:36:16; chip table only had
  part `0xd07`). 4 cores, all start at EL2, PSCIv0.2, `arch_timer 58.33 MHz`,
  GICv3 redistributors at `f0280000` + `0x20000`/core.
- **NAND**: `Manufacturer ID 0x2c, Chip ID 0xa3` → `Micron MT29F8G08ABBCAH4`,
  `1024 MiB, SLC, erase 256 KiB, page 4096, OOB 224` (17:03:21). Under 1.3.35 the
  NAND is **actively ECC-correcting bitflips** on kernel-partition reads
  (`ecc_read_page: corrected increased to 1…6`, `ECC: 1 corrected bitflip(s) at
  offset 0x…`, 17:03:13+) — within ECC budget, but a wear signal worth watching.
- **U-Boot identity fields not in the chip table**: `hardware revision id =
  0x000b101d`, `instance_num = 0`, `subsystem id = 0xea16`, board `alpine_v2_ubnt
  one nas v5.0, 113-2832-29` (stage3/agent_wakeup, 17:03:08). stage2_loader
  v2.22.3, Stage 3 v2.22.0, agent_wakeup v2.10 confirmed each boot.
- Boot USB `SanDisk Ultra` → `sdq`, 63.2 GB / 58.9 GiB (123502592 s), USB3
  SuperSpeed on `xhci_hcd 0001:01:00.0` `hcc params 0x0200e081 hci v1.00` — matches
  live capture.

### Sensor cross-check / correction

- **`pca9575 @ 0x29` never probes** — `pca953x 0-0029: failed reading register` /
  `probe … failed with error -121` (EREMOTEIO) on **every** kernel **including the
  live 5.1.25 capture** (dmesg.txt:83). The i2c table lists 0x20/0x21/0x29 as
  present, but 0x29 is non-responsive (likely unpopulated or mux-hidden). Flag on
  the chip table's PCA9575 row.
- `adt7475 4-002e` rev 1 (`fan4 pwm2`), `rtc-s35390a 1-0030`, `pca9546 @ 0x71`
  (4 muxed busses) — all confirmed across boots, matching the live capture.
- `dtt_get_temp[0..2]` in U-Boot: cold-boot `103/99/101` (17:03), warm reboots
  `97–100`; SoC `temprature: 50 degrees`. Consistent with the chip table.

### ⚠ Contradiction to fix above

- The "Boot-time observations" note says *"`reboot=cold` appears in the kernel
  cmdline"*. **Wrong.** Every boot's cmdline is `… reboot=warm reboot=warm`
  (doubled) — 1.3.35 (17:03:13), 5.1.25 (21:24:37, 10:46:32), and the live
  `cmdline.txt` (`reboot=warm reboot=warm`). No `reboot=cold` anywhere. The
  U-Boot banner separately reports `Boot: warm` as the reset *type*; that is a
  different field from the cmdline `reboot=` action.

