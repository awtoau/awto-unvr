# UNVR hardware — chips and connectors

Board: **`alpine_v2_ubnt one nas v5.0`**, BOM **113-2832-29**, sysid **0xea16**
("UNVR without eMMC"). Unit serial `74acb941a811`.

Evidence marks: ✅ read from the running unit (U-Boot banner, `/proc`, PCI
enumeration, MTD, EEPROM); 📄 from prior-art projects, not yet confirmed here;
❓ inferred, needs a physical check.

Sources for ✅ rows: the U-Boot boot banner captured at
`tmp/logs/unvr-console.log`, `scripts/analyse-mtd.py` output, and
`docs/nand-1.3.35.md`.

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
| `01:00.0` | `1b21:1142` | Serial bus controller | **ASMedia ASM1142 USB 3.1 host** — behind PCIE_0 | ✅ |

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
| 10G SFP+ | 1 × SFP+ cage | 📄 |
| 1G RJ45 | 1 × RJ45 | 📄 |
| PHY | Marvell 10G (`marvell10g.ko` shipped in 5.1.25 modules) | 📄 not confirmed on this unit |

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
