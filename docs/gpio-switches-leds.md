# GPIO / switches / LEDs / RPS map

Reverse-engineered from the vendor GPL U-Boot + kernel (4.1.37), the live vendor
DTB (`hw-reference/20260816-104601/`), the extracted firmware userland
(`UNVR-5.1.25.bin` rootfs), and board photos (`photos/`). 2026-08-17.

GPIO bases (from `live.dts` baseidx): gpio0=0, gpio1=8, gpio2=16, gpio3=24,
gpio4=32, gpio5=40, sgpo=48 (64 lines), pca9575@0x29=464, @0x21=480, @0x20=496.

## SW1 / SW2 — the mystery tactile switches (dormant)

- **Physical:** two momentary tactiles by the RTC (`photos/20260816_231647.jpg`),
  silk `SW2` (left) / `SW1` (right), next to the S-35390A (U5050) + coin cell
  (`RTC BATT1`, MS621), between C135/C136. No function silkscreen.
- **GPIO:** the two orphan SoC inputs on gpio4 — **GPIO 33 (gpio4.1)** and
  **GPIO 34 (gpio4.2)**, both `in`, owner `sysfs` (gpio.txt). Exact SW→pin needs a
  continuity probe.
- **Function: NONE in shipping firmware.** No boot-mode/strap read in U-Boot
  (`board.c` reads no GPIO strap; recovery = UART/XMODEM or TOC fallback only), no
  preboot strap, no userland daemon polls them. Exported to sysfs (likely by
  `ubnthal.ko`) but unused → **factory-test / debug buttons, dormant in production.**
  NOT boot-select, NOT recovery, NOT factory-reset.

## Reset button — press-duration semantics

- **HW:** front button = `gpio-keys` → **GPIO 38 (gpio4.6)**, active-low, IRQ,
  `KEY_RESTART` (408).
- **Handler:** `sbin/infctld` (→ `ubnt-tools`), reads `/dev/input/event*`:
  - **short press → reboot** (only if a config flag is set)
  - **hold ~5 s → 119 s → factory reset** (`(tv_sec−5) ≤ 114` → `/sbin/reset2defaults`;
    119 s upper bound guards a stuck button). `reset2defaults` writes
    `/boot/reset2defaults` + `/persistent/system/reset_reason`, runs reset hooks.
  - re-entry guarded ("RESET in progress, ignoring..").
- The **~10 s-at-power-on recovery is a separate U-Boot function**.
- Disable: `ubnt-systool` / `infctld -n`.

## RPS / UPS-connector LED

- Rear `RPS POWER ON` LED by the `RPS IN` connector (`photos/20260816_225150.jpg`),
  with fuse `FD1` + series resistor `R12800`.
- **`RPS POWER ON` LED = hardwired rail power-present indicator** (FD1 + R12800),
  not SoC-GPIO-driven (no RPS LED in the DTB or `libuled`).
- **RPS = PSE (power-sourcing), not mains-UPS** (no nut/upsd). `rpsd`+`rps-ctrl`
  (`rpsd.service`): 12 V/54 V rail-enable + PSU-power-good + over-current + present-
  sense over GPIO, + disk-driven budgeting (`999-rpsd.rules`→`rpsd_power_budget`).
  Signals `rps_pin_*`: 12v/54v ×(en,sw,oc,lp,psu_pg,guard,batt_guard,out_oc_oring),
  `rps_prnt`, `pd_prnt`, `uart_tx/rx`. UART smart-module path (`ttyRPS*`) is UDM/UXG
  only — not on ea16. **The RPS connector IS populated on this 4-bay board
  (owner-confirmed) — NOT Pro-only.** Full detail in [rps-subsystem.md](rps-subsystem.md).
- **Connector:** mates the standard **USP-RPS** (54 V+12 V DC, 52 V@11.54 A=600 W/port)
  → large: paralleled 54 V+GND power blades + 12 V + 3.3 V-CMOS logic + a 3.3 V-TTL UART.
- **Pin numbers are compiled into `rpsd`, NOT the EEPROM.** Identity EEPROM (mtd04
  @0x1f0000) holds only sysid 0xea16 / hwrev 0x0777 / MAC / serial 113-02832-29 / RSA
  key — no pin table. ea16 ∈ rpsd board group `{ea16,ea1a,ea20,ea51,ea67}` (UNVR/UNAS).
  `rpsd`/`libubnt.so.1` resolves each `rps_pin_*` → a real PL061/PCA9575 sysfs GPIO at
  runtime (`i2c_gpioexp_find_base`). **Exact ea16 numbers:** on a running *vendor* unit,
  `cat /var/run/rpsd.conf` + `cat /sys/kernel/debug/gpio`.

### Repurposing the RPS connector for remote reset / poweroff

All RPS sense pins are Linux-pollable inputs → an external edge → hard reset (SP805
watchdog) or graceful poweroff. Ranked candidates:
1. **`rps_pin_rps_prnt`** (RPS-present input, real connector contact, 3.3 V CMOS) → poll
   `/sys/class/gpio/gpioN/value` → `systemctl poweroff` / `echo b >/proc/sysrq-trigger`.
2. **PCA9575@0x20 pins 12/14/15 (gpio 508/510/511)** — spare board inputs, not tied to
   bay control → cleanest for a NEW external trigger.
3. **SW1/SW2 (gpio 33/34)** — dormant IRQ-capable PL061 inputs → on-board momentary
   trigger (add a `gpio-keys` node emitting `KEY_POWER`/`KEY_RESTART`).
Missing: a straight-down macro of the `RPS IN` contacts for a per-pin physical table.

## Full GPIO / LED / expander map

**SoC PL061:**
| Global | Bank.pin | Function | Dir |
|---|---|---|---|
| 0 | gpio0.0 | SFP+ speed-indication LED (al_eth `"25g"`) | out |
| 3 | gpio0.3 | U-Boot drives output-low; Linux unclaimed; **unknown** | out |
| 31 | gpio3.7 | `ulogo_blue` LED, active-low, default-on | out |
| **33** | gpio4.1 | **SW1/SW2 (dormant input)** | in |
| **34** | gpio4.2 | **SW1/SW2 (dormant input)** | in |
| 37 | gpio4.5 | **`ulogo_white` LED**, active-low (U-Boot `resetled=gpio clear 37`) | out |
| 38 | gpio4.6 | Reset button, `KEY_RESTART`, active-low, IRQ | in |
| 42 | gpio5.2 | HDD `force-power-on-wa`, active-high | out |

**SGPO** (base 48 → external 74VHC595 `UB20`): lines 16–23 (global 64–71) = SATA
activity/presence LEDs, 8 ports (host0/host1 × 0–3); ea16 maps the 4 populated
(sgpo 16/18/20/22).

**PCA9575 expanders (I2C `i2c-pld` @fd880000):**
| Chip | Base | Function |
|---|---|---|
| @0x20 | 496 | pin2 (498)=SFP+ **1G link LED** (`sfp_1g`); inputs pins 3,7,9,10,12,14,15 (mask 0xd688); userland exports 12/14/15 (508/510/511)=board straps/status/present-sense |
| @0x21 | 480 | pins0–3 (480–483)=**bay1–4 pwr-enable** (hogged high); 4–7 (484–487)=**bay present** (active-low); 12–15 (492–495)=**bay fault LEDs** |
| @0x29 | 464 | Pro **bays5–8**: pwren 464–467, present 468–471, fault 476–479 — **unpopulated on the 4-bay** (probe EREMOTEIO) |

**I2C mux `pca9546@0x71`:** ch0 = RTC **S-35390A@0x30** (U5050); ch3 = **ADT7475@0x2e**.
**Fan:** ADT7475 PWM over I2C (not GPIO); userland `ustd` PID (`fan_ctrl`); U-Boot
`slowfan` env sets a slow default.

**Corrections to `hardware.md`:** `gpio 37` = `ulogo_white` (was "unknown"); the
`pwren 464 / present 468 / fault 476` quoted for HDD control are the **@0x29 (Pro)**
expander — the **populated** bay control is **@0x21 (480/484/492)**.
