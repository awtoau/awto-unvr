# UNVR chip reference — board BOM

Per-chip docs for the UNVR motherboard (Annapurna Alpine V2 / **AL-324**, sysid
`0xea16`, board `alpine_v2_ubnt one nas v5.0`, BOM 113-2832-29).

- Board overview + connectors + **SoC MMIO map**: [../hardware.md](../hardware.md) (single source of truth for addresses — these docs link, do not duplicate).
- Boot chain: [../nor-boot-chain.md](../nor-boot-chain.md), [../preboot-decompile.md](../preboot-decompile.md).
- Datasheet PDFs: [`../../sources/chips/`](../../sources/chips/) (tracked in git) + [manifest](../../sources/chips/README.md).

## Quick BOM

| Part | Function | Voltage | Bus / address | Doc | Datasheet |
|---|---|---|---|---|---|
| **AL-324** (Annapurna Alpine V2) | SoC — 4× Cortex-A57, all integrated I/O | SoC-internal; peripheral I/O 1.8 V | — | [al-324.md](al-324.md) | none (NDA) |
| **MX25U25635F** (Macronix) | SPI-NOR boot flash (32 MiB) | **1.8 V** | SPI0 `0xfd882000` | [mx25u25635f.md](mx25u25635f.md) | ✅ local v1.5 |
| **MT29F8G08ABBCAH4** (Micron) | SLC NAND — kernel+rootfs (1024 MiB) | **1.8 V** | al-nand `0xfa100000` | [mt29f8g08abbcah4.md](mt29f8g08abbcah4.md) | ⚠ 1-page card only |
| **AR8033** (Qualcomm/Atheros) | 1G PHY, RJ45 (enp0s1); driver reports "8031" (shared PHY ID) | core 1.0 V (int LDO); RGMII I/O 1.5/1.8/2.5 V | MDIO addr 4 (RGMII) | [ar8033.md](ar8033.md) | ✅ photo + live |
| **ASM1042A** (ASMedia) | USB 3.0 xHCI host (boot USB) | on-module (NDA) | ext-PCIe0, `1b21:1142` | [asm1042a.md](asm1042a.md) | none (NDA) |
| **PCA9575** ×3 (NXP) | GPIO expanders — HDD pwr/LED | 1.1–3.6 V | I²C 0x20/0x21/**0x29†** | [pca9575.md](pca9575.md) | ⚠ hotlink-blocked |
| **PCA9546A** (NXP) | 4-ch I²C mux | 1.65–5.5 V (1.8 V domain) | I²C 0x71 | [pca9546a.md](pca9546a.md) | ✅ local (TI 2nd-source) |
| **S-35390A** (ABLIC/Seiko) | RTC | 1.3–5.5 V | I²C 0x30 | [s-35390a.md](s-35390a.md) | ✅ local Rev 4.2 |
| **ADT7475** (ADI/onsemi) | fan/thermal controller | 3.0–5.5 V | I²C 0x2e | [adt7475.md](adt7475.md) | ✅ local Rev A |
| **DDR4 4 GiB** (unconfirmed) | main memory | 1.2 V | memctl `0xf0080000`; SPD I²C 0x57 | [dram-ddr4.md](dram-ddr4.md) | none (part unknown) |
| **SFP+ optic** (unconfirmed) | 10G module (enp0s2) | module | SerDes; EEPROM I²C 0x50/0x51 | [sfp-optic.md](sfp-optic.md) | none (part unknown) |

† PCA9575 @ **0x29 never probes** (EREMOTEIO on every kernel) — listed but non-responsive. See [pca9575.md](pca9575.md).

## hardware.md cross-check — flags

- **PCA9546 vs PCA9546A:** hardware.md writes "PCA9546"; the saved datasheet is PCA9546A. Board part is almost certainly the -A (register-identical). Confirm by silkscreen if it matters.
- **PCA9575 count:** hardware.md lists 3 (0x20/0x21/0x29); only 2 respond — the BOM row here marks 0x29 as phantom.
- **NOR page/erase:** hardware.md main-silicon table ("page 256 B, erase 4 KiB") matches the MX25U25635F datasheet. ✅ no conflict.
- **NAND OOB:** hardware.md and datasheet agree page 4096 + **224 B OOB**, erase 256 KiB. ✅
- **10G port:** hardware.md correctly records it as SFP optic, **not** a PHY (earlier "Marvell 10G" guess retracted there). AR8033 is the 1G port only.

## Unconfirmed parts (need a physical/host check)

- **DDR4 DRAM** — read SPD @ I²C 0x57 (`decode-dimms`) or photograph the silkscreen. See [dram-ddr4.md](dram-ddr4.md).
- **SFP+ optic** — `ethtool -m enp0s2` (mainline kernel) or read module EEPROM @ 0x50/0x51. See [sfp-optic.md](sfp-optic.md).
