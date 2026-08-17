# RPS (Redundant Power / PSE) subsystem

Reverse-engineered from `sbin/rpsd` (5.1.25) — full binary pin/protocol walk
(`scripts/rps_walk.py`), the live stock DTB, `RPS.jpg`, and Ubiquiti's USP-RPS
spec. **The RPS connector IS populated on this 4-bay UNVR (ea16)** (owner-confirmed).
2026-08-17.

## What it is

Redundant DC: the `RPS IN` connector (`JB4`) mates a **USP-RPS** (54 V + 12 V DC,
52 V @ 11.54 A = 600 W/port). RPS power is **ORed** with the main supply through an
on-board FET bank, then fed to the drive backplane. `rpsd` monitors/controls it.

## ea16 vs the USP-RPS *product* — the key distinction

`rpsd` is shared firmware. Its board table has a full 6-port PSE profile (the
`rps_pse` template) **and** the minimal NVR-side profile. **Our ea16 uses the
minimal one** — do not confuse them:

| | ea16 (this UNVR) | 6-port USP-RPS product (`rps_pse` template) |
|---|---|---|
| Ports | **1** | 6 |
| UART | **`/dev/ttyS2`** (SoC UART, RS-232 via MAX3221) | `/dev/ttyRPS1..6` |
| Functional pins | **2** (present + 12 V load sense) | 13/port (en/sw/oc/lp/psu_pg/guard/batt_guard/oring ×12 V,54 V) |
| Pin backend | **SoC PL061 GPIO** (gpiolib-sysfs) | PCA9575 expanders @0x20/22/23/24 (`pin<<32\|addr`) |
| ORing monitor | (see below — physical `U1`/`U48`) | ISL28022 @0x44-46, INA230 @0x40-42, INA237 @0x49 (i2c bus 11) |

## ea16 config (authoritative — `rpsd` config @ file 0x28990)

- **UART: `/dev/ttyS2`** — a **SoC UART**, taken to RS-232 levels by **MAX3221 (`U122`)**
  and out on `JB4`. (This is why our DTS `ttyS2` "Bluetooth CSR" label is wrong — it's
  the **RPS PHY UART**, not Bluetooth; there is no BT on this board.)
- **2 sense GPIOs, both SoC PL061 via gpiolib-sysfs** (hi32 of the 8-byte field = abs gpio):
  - **`RPS_PIN_RPS_PRNT` → gpio 33** (input, RPS-present detect) — flags 0x5
  - **`RPS_PIN_12V_LP` → gpio 34** (input, 12 V load/limit sense) — flags 0x1
- **No** 54V_EN/12V_EN/SW/OC/PSU_PG/guard/oring pins and **no PCA9575** for RPS on ea16.

> **Correction:** gpio 33/34 were earlier guessed to be SW1/SW2 (the two orphan SoC
> inputs in `gpio.txt`). The `rpsd` reverse shows they are **rps_prnt / 12v_lp** —
> exported by rpsd. So **SW1/SW2's real GPIO is unknown** (needs a continuity probe);
> see [gpio-switches-leds.md](gpio-switches-leds.md).

## Serial protocol (`ttyS2` → RPS PHY)

- **115200 baud, 8N1, raw, no flow control** (`rps_drv_uart_common_configure` @0x12790:
  CS8|CREAD|CLOCAL, CSIZE/CSTOPB/PARENB cleared, B115200, VTIME=1.0 s, TCIOFLUSH).
- **Line-delimited JSON (JSON-RPC style)** — NOT a binary/checksum frame:
  TX (`rps_port_do_command` @0x82b0) = `json_dumps()` + `\n`; RX splits on `\r\n`,
  `json_loads` each line, correlates by the `"status"` field, 5 retries.

## Power path (from `RPS.jpg`)

- **`JB4`** — large right-angle through-hole connector, **~15-16 gold bent-blade
  contacts**, single row, staggered 2-column via footprint (high-current). All blades
  identical → **54 V/12 V/GND rails are ganged across several adjacent pins**; a small
  group near `U122` (MAX3221) carries the **RS-232 UART TX/RX**; present/load sense =
  gpio 33/34. Exact per-blade rail assignment **needs continuity probing** (not
  resolvable from images).
- **`JB5` = "BP DCOUT1"** — 4-pin (2×2) **backplane DC-power output** (silk
  `HDD4_PWROFF`).
- **ORing / hot-swap:** FET bank **`Q536`/`Q537`/`Q14`/`Q59`/`Q590`** + **`VR5`** between
  JB4-in and JB5-out; **`D500`** = DPAK FET/diode in the ORing path.
- **ORing power-monitor IC:** physical candidate **`U48`** (~10-pin QFN) — marking
  illegible; need a clearer macro. (`U122` is the MAX3221 RS-232 xcvr, not the monitor.)
  ⚠ **`U1` UNRESOLVED:** the RPS-photo read called U1 a 16-pin monitor; the photo
  catalog read it as a Marvell 88E6xxx switch OR an ASM1061 SATA/USB bridge (marking
  worn) — see [photo-catalog.md](photo-catalog.md). Either way it's **not** the ORing
  monitor (that's U48/other). U1's true ID needs a macro.

## Power budgeting

`rpsd` tracks required vs remaining watts, detects PSU overload, enforces
"insufficient power" gating, battery-guard, fast-recovery; udev `999-rpsd.rules`
re-runs `rpsd_power_budget` on disk add/remove. (Most of the elaborate 12 V/54 V
budgeting logic is exercised by the 6-port product; ea16 does present/load-sense +
the ttyS2 JSON link.)

## Repurposing for remote reset / power-off

- **`gpio 33` (rps_prnt)** is the RPS-connection-detect input — a natural hook to sense
  an external signal and drive a reset (SP805 watchdog) or `poweroff`, though it's
  claimed by rpsd under the stock OS (free for us on Fedora).
- **Spare PCA9575@0x20 inputs (gpio 508/510/511)** — cleanest for a NEW trigger.
- **SW1/SW2** — physical buttons, GPIO TBD (probe) — ideal on-board momentary trigger.

## Open items

- **SW1/SW2 actual GPIO** (the gpio-33/34 assignment was the RPS pins) — continuity probe.
- **ORing power-monitor IC part** — read `U1`/`U48` markings (clearer shot / on-board).
- **`JB4` per-blade rail map** — continuity probe (54 V/12 V/GND/UART/sense).
- Whether ea16 populates any i2c power monitor at all (config lists none; physical U1/U48
  suggest one exists on the ORing path).
  - **Scan result (2026-08-17, `scripts/i2c-spi-scan.py`):** the *active* i2c bus
    (`i2c_pld` @0xfd880000) has **no monitor** in the INA/ISL range 0x40–0x49 — only
    mux/RTC/adt7475/pca9575/SFP. The **general bus `i2c_gen` @0xfd894000** (the stock
    "bus 11", where the INA230/ISL28022/INA237 monitors sit) is **`status="disabled"`** in
    the ea16 DTS, so nothing probes it. → To confirm ea16 has/lacks the monitor, **enable
    `i2c_gen` and rescan 0x40–0x49**. Unrelated: an unidentified 32-bit-register device sits
    at **0x57** on `i2c_pld` (not an INA/ISL monitor) — see #62.

Scripts: `scripts/rps_walk.py` (binary pin-table walk), `scripts/crop_rps.py`.
Full offsets/disasm/photo cites: `tmp/logs/rps-reverse.md`.
