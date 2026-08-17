# DRAM — 4 GiB DDR4 (Samsung K4A8G165WB-BCRC ×4)

- **Part: Samsung K4A8G165WB-BCRC** ×4 — 8 Gb ×16 DDR4, soldered. **Photo-confirmed**
  (`SEC 013 / K4A8G16 / 5WB-BCRC`, refs U3/U4 + 2). 4 × 8 Gb = 4 GiB.
- **Function:** main system memory.
- **Confirmed:** U-Boot `DRAM: 4 GiB`, `SPD I2C Address: 57`; stage2_loader reads SPD to bring up DRAM. ✅ live.

## What is known

- **Total: 4 GiB DDR4.** Two banks (from live.dts):
  - DRAM0 `0x00000000`, 3 GiB (`0xC0000000`) — kernel/reserved carve-outs within.
  - DRAM1 `0x200000000`, 1 GiB (`0x40000000`) — above 4 GiB.
- **Voltage:** DDR4 nominal **1.2 V** (VDD/VDDQ), 2.5 V VPP — standard, not board-programmable.
- **Controller:** on-SoC `annapurna-labs,alpine-mc` (memctl @ `0xf0080000`) — [hardware.md#mmio-and-address-map](../hardware.md#mmio-and-address-map).
- **SPD: present, in a config EEPROM at I²C 0x57** (matches U-Boot `SPD I2C Address: 57`). 0x57 is a **16-bit-addressed multi-record store** (records at base `0x400`, magic-guarded: `0xAA` SPD-pointer, `0xBB` DRAM-voltage GPIO, `0xCC` impedance override) that **contains the JEDEC DDR4 SPD** — live-confirmed (`i2ctransfer -y 0 w2@0x57 0x04 0x00 r128` → byte2 `0x0C` = DDR4). A naive 1-byte dump at offset 0 reads garbage (do NOT conclude 'no SPD'). Full decode path + proofs: [ddr-config-reverse.md](../ddr-config-reverse.md), the S2 loader reads this at boot; also cross-checkable via live-controller readback (`al_ddr_cfg_init`).

## How to confirm the part

1. **Read the SPD** (best): `i2cdump`/`decode-dimms` against the bus carrying 0x57 (may be behind the [PCA9546A](pca9546a.md) mux). SPD bytes give manufacturer (JEDEC ID), density, organisation, speed bin, week/year.
2. **Silkscreen / package photo:** read the DDR4 chip top-marking directly and decode (Samsung K4A…, SK Hynix H5A…, Micron MT40A…).
3. Count devices × width to cross-check 4 GiB (e.g. 2× 16 Gbit ×16, or 4× 8 Gbit ×8).

## Candidates (typical Alpine V2 NAS BOM — to be confirmed, not asserted)

- Samsung K4A8G/K4AAG DDR4.
- SK Hynix H5AN DDR4.
- Micron MT40A DDR4.

## Datasheet

- **None saved** — part unconfirmed, so no specific datasheet. Fetch once the SPD/silkscreen identifies it.

## RE / repurpose notes

- Nothing to reprogram (RAM). The actionable item is **reading the SPD at 0x57** to close [hardware.md](../hardware.md) open questions and to know the exact timings if ever re-initialising DRAM outside the vendor preboot.
