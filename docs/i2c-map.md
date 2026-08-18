# I2C map — UNVR ea16 (live, 2026-08-18)

Two DesignWare controllers + a 4-channel mux. Scanned live from Fedora (kernel sysfs =
authoritative; `i2cdetect` is unreliable here — the DW adapter has no SMBus-quick-write so
it silently *skips* 0x00-0x02/0x28-0x2f/0x40-0x4f unless `-r` read-probe is forced).

## Buses

| Linux bus | Controller / source | Devices |
|-----------|---------------------|---------|
| i2c-0 | DW @0xfd880000 "i2c-pld" (our dts `i2c_pld`) | 0x20, 0x21 PCA9575; 0x57 24C64 EEPROM; 0x71 PCA9546 mux |
| i2c-1 | DW @0xfd894000 "i2c-gen" (our dts `i2c_gen`) | Live bus. Carries the **2× RPS ORing power monitors** (12 V + 54 V rails — U48-area, powered off the mainboard ORing FET path, **on** regardless of RPS module). A plain scan reads empty because these are INA/ISL-class parts at **0x40–0x49** — exactly the range a DW quick-write probe **skips** (see header note). Use a **read-probe** (`i2cdetect -r` / `i2c md <addr>`) to see them. Exact addr + part TBD (#64). |
| i2c-2 | mux ch0 | 0x30 s35390a **RTC**; 0x50 **SFP module EEPROM** (al_eth, not in DT) |
| i2c-3 | mux ch1 | empty |
| i2c-4 | mux ch2 | empty |
| i2c-5 | mux ch3 | 0x2e **adt7475** — the **PWM fan controller** (temp/voltage sense + fan drive) |

## Chips (physical, photo-confirmed)

- **UB1 + U10 = 2× PCA9575PW** (24-pin TSSOP) → i2c 0x20 / 0x21. **Only two.** The DT's
  third `@0x29` is the 6-bay **UDM-Pro's** second bay-expander; on 4-bay ea16 it is
  **unpopulated** ("pca953x 0-0029: failed reading register"). Photos `20260816_232614`
  (UB1), `_231415` (U10).
- **0x21** = bay control: pwren lines 0-3 (gpio-hog output-high), presence 4-7, fault LEDs 12-15.
- **0x20** = SFP+ 1G link LED (pin 2) + straps.
- **adt7475** (U27, behind mux ch3) = the **PWM fan controller** — 3 temp + 2 voltage inputs
  drive 3 PWM fan outputs (stock env `slowfan` pokes 0x2e regs 0x30-0x32/0x5c-0x5e). rev 2.
  SENSORS_ADT7475. NOT the RPS current monitor.
- **2× RPS ORing power monitors** (INA/ISL class, V·I·P per rail) on the `i2c_gen` bus
  (fd894000): one for the **12 V** rail, one for the **54 V** rail. Watch the on-board ORing
  FET path (`Q536/Q537/Q14/Q59/Q590`, `docs/rps-subsystem.md`), so they are **powered and
  present whether or not an external RPS module is plugged in** — the RPS input just ORs into
  the same rail. Physical candidate **U48** (~10-pin QFN, RPS area — `docs/photo-catalog.md`).
  Addr + exact part still open (#64); reachable only once pins 30/31 mux to I2C_GEN.
- **s35390a** RTC (behind mux ch0), coin cell SII MS621.
- **24C64** EEPROM @0x57 = DDR-config blob (reads 0x1c36-repeating, not plain SPD) — #67.
- Bay-activity LEDs are **not** i2c: SGPO `fd8b4000` → external **74VHC595 (UB20)** shift
  register, sgpo lines 16-23. See [gpio-switches-leds.md](gpio-switches-leds.md).

## KNOWN BUG — mux ch0 (s35390a RTC) wedges the bus

Selecting ch0 + touching any address holds SDA low and wedges the whole pld bus (RTC @0x30 is
the culprit; SFP EEPROM @0x50 is collateral). Root cause = a dropped `i2c-sda-hold-time-ns`,
fixed by restoring it — **full analysis, fix, and recovery in
[rtc-s35390a-fault.md](rtc-s35390a-fault.md)**. Behind-mux is otherwise fine (ch3 adt7475 reads
clean).

## Re-scan recipe

- Linux: `i2cdetect -l`; per-bus map `for d in /sys/bus/i2c/devices/*; do echo $(basename $d) $(cat $d/name); done`.
  Force read-probe for hidden devices: `i2cdetect -y -r <bus>` (behind-mux buses can stall on
  per-address timeouts — bounded ranges only).
- U-Boot: `i2c dev <n>; i2c probe`. ch3 is reachable via `i2c mw 0x71 0 8 1` then
  `i2c md 0x2e 3d 1` (adt7475 = 0x75). Do **not** select ch0 (`i2c mw 0x71 0 1 1`) + read —
  it wedges the bus and needs an SP805 reset. (Our U-Boot LED blink is now solid-on so a
  wedge no longer floods — see defconfig PREBOOT.)
