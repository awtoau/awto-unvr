# RPS (Redundant Power / PSE) subsystem

Reverse-engineered from `sbin/rpsd` (5.1.25 firmware, strings + ELF layout), the
live vendor DTB, and Ubiquiti's USP-RPS spec. **The RPS connector IS populated on
this 4-bay UNVR (ea16)** — owner-confirmed; it is *not* Pro-only. 2026-08-17.

## What it is

Redundant DC power: the UNVR's `RPS IN` connector mates a **USP-RPS** (54 V + 12 V
DC, 52 V @ 11.54 A = 600 W/port, 6 ports). RPS power is **ORed** with the main
supply so the box keeps running if one source fails. `rpsd` (+`rps-ctrl`,
`rpsd.service`, enabled) manages it and does **power budgeting** (decides what can be
powered given available RPS watts vs. the fitted-disk load).

## Architecture — three interfaces

1. **Per-port UART to the RPS PHY** — `rpsd` drives up to **8 ports** `phy-1..phy-8`
   over `/dev/ttyRPS1..8` (`uart-common`). Functions: `rps_drv_uart_init`,
   `rps_command_read`/`rps_command_write`, `rps_port_do_command`,
   `rps_read_uart_message`, `rps_drv_uart_common_configure/write`. A **heartbeat**
   watches liveness (`RPS heartbeat no response than %d seconds` → recovery mode).
   On ea16 the board profile group `{ea16,ea1a,ea20,ea51,ea67}` sits immediately
   before the `phy-N`/`ttyRPSN` string block in `rpsd .rodata` (file 0x20024).
2. **ORing power-monitor over I²C (dedicated controller)** — an on-board **power-path
   ORing + monitor IC**, exposed as hwmon: `hwmon12v_oring`, `hwmon54v_oring`, with
   `oring_12v/54v_voltage`, `_current`, `_power`, `_power_max`, `_oc`, and
   `_power_crit`(+`_enable`) critical-alert registers. `rpsd` reads it via
   `i2c:%d-0x%x … register 0x%x`. This is the "dedicated controller" — it measures V/I/P
   per rail and raises OC/over-power alerts. **Chip part not yet identified on the board
   (find its I²C addr + marking).**
3. **Control/sense GPIO on a PCA9575 expander** — the enable/switch/sense pins (below),
   resolved at runtime to Linux sysfs GPIOs (`i2c_gpioexp_find_base`).

## Pin functions (the `rps_pin_*` set)

Per rail (**12 V** and **54 V**):
| Pin | Dir | Function |
|---|---|---|
| `*_en` | out | rail enable (turn rail on/off) |
| `*_sw` | out | rail switch |
| `*_oc` | in | over-current sense |
| `*_lp` | in | load/limit sense |
| `*_psu_pg` | in | PSU power-good (`54V_STBY_PG` = 54 V standby PG) |
| `*_guard` | out | AC power guard (protection) |
| `*_batt_guard` | out | battery power guard |
| `*_out_oc_oring` | in | ORing-output over-current |

Board-level:
| Pin | Dir | Function |
|---|---|---|
| `rps_pin_rps_prnt` | in | **RPS present** — is a USP-RPS connected |
| `rps_pin_pd_prnt` | in | powered-device present |
| `rps_pin_uart_tx/rx` | — | UART to the RPS PHY controller |

## Serial protocol (ttyRPS)

- Per-port UART (`/dev/ttyRPS1..8`), register read/write command model
  (`rps_command_read`/`write` → `rps_port_do_command`), message parser
  (`rps_read_uart_message`, `unknown uart message: %s`), JSON status responses
  (`rps_json_response_ok_set_new`), and a liveness **heartbeat**.
- **Exact framing/baud not yet extracted** — needs a deeper `rpsd`/`libubnt` reverse or
  a live `ttyRPS` capture. (Contrast the LCD-MCU path which uses a fixed frame
  `a0 0a 08 <cmd> 0b b0` over UART2 — the RPS PHY protocol is its own.)

## Power budgeting / behaviour

`rpsd` tracks `required_power_12v/54v` vs remaining, detects PSU overload
(`PSU 12V/54V overload … remaining power %d mW`), enforces `Insufficient power … <
PD req. power` (won't power a device it can't sustain), runs **battery power-guard**
protection, and a **fast-recovery** mode after overload (`reinit pins to recover PSU
overload`). udev `999-rpsd.rules` re-runs `rpsd_power_budget` on disk add/remove.

## Pin resolution — where the exact GPIO numbers live

- **NOT in the EEPROM.** Identity EEPROM (SPI-NOR mtd04 @0x1f0000) holds only sysid
  0xea16 / hwrev 0x0777 / MAC / serial 113-02832-29 / RSA key — no pin table.
- Numbers are **compiled into `rpsd`** (per-board port/pin table, `.data.rel.ro`/`.rodata`),
  resolved at runtime to real **PL061/PCA9575** sysfs GPIOs via `i2c_gpioexp_find_base`.
- **Get the exact ea16 numbers, two ways:**
  - **Live (vendor OS):** `cat /var/run/rpsd.conf` + `cat /sys/kernel/debug/gpio`.
  - **Static (offline):** walk `rpsd` — the `{ea16,…}` board_ids array is at file
    **0x20024** (32-bit LE sysids, `0xffff0000` terminators); the group descriptor
    (in `.data.rel.ro`) carries the name/desc/ttyRPS + the port/pin sub-table.
    **Status: WIP** — the descriptor→pin-table pointer walk isn't finished (the ptr at
    file 0x27868→0x20010 is the *other* group `{ea2c,ea32}`, not ours).

## Repurposing for remote reset / power-off (owner goal)

RPS sense pins are Linux-pollable inputs → drive a reset (SP805 watchdog) or poweroff.
Ranked: (1) `rps_pin_rps_prnt`; (2) spare PCA9575@0x20 inputs gpio 508/510/511;
(3) dormant SW1/SW2 (gpio 33/34). See
[gpio-switches-leds.md](gpio-switches-leds.md#repurposing-the-rps-connector-for-remote-reset--poweroff).

## Open items

- Exact ea16 `rps_pin_* → gpio` numbers (finish the static `rpsd` walk, or read
  `/var/run/rpsd.conf` under the vendor OS).
- Identify the **ORing power-monitor IC** (I²C addr + board marking).
- RPS PHY **UART framing/baud** (deeper reverse or live capture).
- **Physical `RPS IN` connector pinout** — needs a straight-down macro shot of the
  contacts; power blades (54 V/12 V/GND) + logic (3.3 V) + UART not yet pin-mapped.
