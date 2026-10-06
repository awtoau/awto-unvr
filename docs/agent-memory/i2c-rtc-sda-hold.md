---
name: i2c-rtc-sda-hold
description: s35390a RTC wedges the pld i2c bus; root cause dropped SDA-hold; current state + how to recover
metadata: 
  node_type: memory
  type: project
  originSessionId: 0edb0769-b7b5-4d90-bc3a-84dd99588559
  modified: 2026-08-18T22:08:39.038Z
---

The `i2c_designware fd880000.i2c: controller timed out` flood + `lost arbitration` = the **s35390a RTC (U5050, 0x30, behind PCA9546 mux ch0) holding SDA low** and wedging the whole pld bus. Full writeup: `docs/rtc-s35390a-fault.md`, `docs/i2c-map.md`.

**Root cause (found by independent review):** our Fedora DTB dropped the stock i2c timing tuning. Fixed by adding `i2c-sda-hold-time-ns = <300>` to `i2c@fd880000` (commit `5338bdc`) → applied register value `DW_IC_SDA_HOLD` (0xfd88007c) = 0x96 = 300 ns @500 MHz. NOT a dead RTC (it read fine on stock 4.19) and NOT sbclk (board DTS overrides to 500 MHz).

**Build footgun (why "bugs kept coming back"):** `build-linux-71-fedora.py` compiled whatever stale DTS was in the kernel tree. Fixed — it now `stage_dts()`s the repo-tracked `dts/` (+ now-tracked `dts/alpine-v2.dtsi`).

**Current state (not fully solved):**
- SDA-hold cut arb-loss from a flood to ~1/boot, but even 1 wedges the RTC (holds SDA).
- **A power-cycle did NOT clear it** (battery disconnected too) → the held-SDA needs the datasheet **63-SCL-clock recovery** (Fig 46), which THIS SoC can't generate (pld SCL/SDA are dedicated PBS pads, no GPIO mux; DW core v1.20 has no HW SDA-recovery) → needs a **bench tool** on the RTC pins, or a true VDD cut.
- **adt7475 monitor reads fine in OUR chainloaded U-Boot** (select ch3 directly, 0x3d=0x75) but **hangs in Linux** (mux-driver + idle-disconnect + stuck ch0). So the monitor is reachable; the Linux hang is the mux interaction, not the chip.

Related: [[our-uboot-assume-nothing]], issue #72 (RTC INT wiring). User's primary want = the "current monitor" (adt7475), not the RTC (NTP is fine).
