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
- **GPIO: UNKNOWN — correction.** gpio 33/34 were earlier guessed to be SW1/SW2 (the
  two orphan `sysfs`-owned inputs in gpio.txt), but the `rpsd` binary reverse proves
  **gpio 33 = `rps_prnt` (RPS present)** and **gpio 34 = `12v_lp` (12 V load sense)** —
  those are the RPS sense inputs (see [rps-subsystem.md](rps-subsystem.md)), exported by
  rpsd, not the buttons. **So SW1/SW2's actual GPIO is unknown — needs a continuity probe.**
- **Function:** no boot-mode/strap read in U-Boot or preboot, and no daemon polls
  buttons on gpio 33/34 (those are RPS). SW1/SW2 look like factory-test / debug buttons,
  but until we know their pins that's provisional. NOT boot-select/recovery/factory-reset.

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

## RPS connector (PSE) — summary

Populated **PSE** power port on this 4-bay board (owner-confirmed, **not** Pro-only):
redundant **54 V + 12 V** ORed with the main supply. On ea16, `rpsd` uses only
**`/dev/ttyS2`** (RS-232 via MAX3221 `U122`) + two SoC-GPIO sense inputs — **gpio 33 =
`rps_prnt`**, **gpio 34 = `12v_lp`** (these are the two `sysfs` inputs once mis-assigned
to SW1/SW2). The rear `RPS POWER ON` LED is a hardwired rail indicator, not GPIO.

**Full mechanism, protocol, pin map, connector, and remote-reset/poweroff repurposing
→ [rps-subsystem.md](rps-subsystem.md).**

## Full GPIO / LED / expander map

**SoC PL061:**
| Global | Bank.pin | Function | Dir |
|---|---|---|---|
| 0 | gpio0.0 | SFP+ speed-indication LED (al_eth `"25g"`) | out |
| 3 | gpio0.3 | U-Boot drives output-low; Linux unclaimed; **unknown** | out |
| 31 | gpio3.7 | `ulogo_blue` LED, active-low, default-on | out |
| **33** | gpio4.1 | **RPS present** (`rps_prnt`, rpsd input) — NOT SW1/SW2 | in |
| **34** | gpio4.2 | **12 V load sense** (`12v_lp`, rpsd input) — NOT SW1/SW2 | in |
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
