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

## How the part was confirmed

- Board-photo top-marking (`SEC 013 / K4A8G16 / 5WB-BCRC`) on all 4 packages, cross-checked
  against U-Boot `DRAM: 4 GiB` / `SPD I2C Address: 57` and live SPD decode
  ([ddr-config-reverse.md](../ddr-config-reverse.md)). Same method as
  [components.md](../components.md) and [hardware.md](../hardware.md#physical-chip-ids--board-photo-catalog).

## Datasheet

- **None saved yet.** Fetch the Samsung K4A8G165WB-BCRC datasheet if exact timing
  parameters are needed for a from-scratch DRAM re-init.

## RE / repurpose notes

- Nothing to reprogram (RAM). The actionable item is **reading the SPD at 0x57** to close [hardware.md](../hardware.md) open questions and to know the exact timings if ever re-initialising DRAM outside the stock preboot.
