# UNVR chip reference — per-chip datasheet docs

Detailed per-chip notes + datasheet status for the UNVR mainboard (Annapurna Alpine
V2 **AL-324**, sysid `0xea16`, BOM 113-2832-29).

- **Full component / connector / test-point catalog → [../components.md](../components.md)** (the master BOM — single source; this folder is just the per-chip deep-dives + datasheets).
- Board overview + **SoC MMIO map** → [../hardware.md](../hardware.md).
- Datasheet PDFs → [`../../sources/chips/`](../../sources/chips/) + [manifest](../../sources/chips/README.md).

## Datasheet index

| Part | Function | Doc | Datasheet |
|---|---|---|---|
| **AL-324** (Annapurna Alpine V2) | SoC — 4× Cortex-A57 + integrated I/O | [al-324.md](al-324.md) | none (NDA) |
| **MX25U25635F** (Macronix) | SPI-NOR boot flash, 32 MiB, 1.8 V | [mx25u25635f.md](mx25u25635f.md) | ✅ local v1.5 |
| **MT29F8G08ABBCAH4** (Micron) | SLC NAND, 1 GiB, 1.8 V | [mt29f8g08abbcah4.md](mt29f8g08abbcah4.md) | ⚠ 1-page card only |
| **AR8033** (Qualcomm/Atheros) | 1G PHY, RJ45; driver reports "8031" (shared PHY ID) | [ar8033.md](ar8033.md) | ✅ photo + live |
| **ASM1042A** (ASMedia) | USB 3.0 xHCI host (boot USB) | [asm1042a.md](asm1042a.md) | none (NDA) |
| **PCA9575** ×2 (NXP) | GPIO expanders @0x20/0x21 (`0x29` phantom) | [pca9575.md](pca9575.md) | ⚠ hotlink-blocked |
| **PCA9546A / TCA9546A** (NXP/TI) | 4-ch I²C mux @0x71 | [pca9546a.md](pca9546a.md) | ✅ local |
| **S-35390A** (ABLIC/Seiko) | RTC @0x30 | [s-35390a.md](s-35390a.md) | ✅ local Rev 4.2 |
| **ADT7475** (ADI/onsemi) | fan/thermal @0x2e | [adt7475.md](adt7475.md) | ✅ local Rev A |
| **Samsung K4A8G165WB** ×4 | DDR4, 4 GiB total, 1.2 V | [dram-ddr4.md](dram-ddr4.md) | none |
| **Finisar FTLX8571D3BCL** | 10G-SR SFP+ optic (EEPROM 0x50/0x51) | [sfp-optic.md](sfp-optic.md) | SFF-8472 (read live) |

Per-chip docs carry the full specs, pinout and RE notes; the BOM columns (bus/voltage/
location) live in [../components.md](../components.md) — not duplicated here.

## Resolved (were "unconfirmed" in the first draft)

- **DDR4 = Samsung K4A8G165WB** — photo-confirmed ×4. The **DDR4 SPD IS present** in a
  **config EEPROM at I²C 0x57** — a 16-bit-addressed, magic-guarded multi-record store
  (records @ base `0x400`: `0xAA` SPD-pointer, `0xBB` voltage, `0xCC` impedance) that holds
  the JEDEC SPD (live-confirmed, byte2 `0x0C` = DDR4). A 1-byte dump at offset 0 misreads it.
  Decode path + proofs: [../ddr-config-reverse.md](../ddr-config-reverse.md). (0x57 is NOT the
  identity EEPROM — that was a separate item, #62, closed.)
- **SFP+ = Finisar FTLX8571D3BCL** 10G-SR — read live (0x50 = SFF-8079 EEPROM, 0x51 =
  SFF-8472 DDM). Behind PCA9546 channel 1.
- **1G PHY = AR8033**, not AR8031/Marvell; the 10G port is an **SFP optic, no PHY**.
- **I²C mux is 4-channel** (PCA9546/TCA9546A); PCA9575 **@0x29 never probes** (EREMOTEIO —
  Pro-only 8-bay node, unpopulated on this 4-bay board, correctly absent; node removed
  from the DTS — see [kernel-boot-fixes.md](../kernel-boot-fixes.md) Bug 3).
