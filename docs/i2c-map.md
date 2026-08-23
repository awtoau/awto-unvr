# I2C map — UNVR ea16 (live, 2026-08-18)

Two DesignWare controllers + a 4-channel mux. Scanned live from Fedora (kernel sysfs =
authoritative; `i2cdetect` is unreliable here — the DW adapter has no SMBus-quick-write so
it silently *skips* 0x00-0x02/0x28-0x2f/0x40-0x4f unless `-r` read-probe is forced).

## Buses

`i2c_gen` (0xfd894000) is disabled, matching stock (both 1.3.35 and 5.1.25 firmware leave it
disabled - no RPS monitor driver exists in any Ubiquiti firmware, #64; U48 is very likely a
pure analog part with no digital interface at all - MUIO pins 30/31, i2c_gen's SCL/SDA, are
actually muxed to ETH-LED and ulogo_blue per the live 48-ball read, docs/gpio-map.md - there
was never a real i2c bus there regardless). Enabling it was a mistake that shifted every
board-params-derived i2c adapter number by one and broke the SFP+ EEPROM read (#98).

| Linux bus | Controller / source | Devices |
|-----------|---------------------|---------|
| i2c-0 | DW @0xfd880000 "i2c-pld" (our dts `i2c_pld`) | 0x20, 0x21 PCA9575; 0x57 24C64 EEPROM; 0x71 PCA9546 mux |
| i2c-1 | mux ch0 | 0x30 s35390a **RTC** (0x30-0x37, its own multi-address quirk) |
| i2c-2 | mux ch1 | 0x50 **SFP module EEPROM** (al_eth, not in DT) |
| i2c-3 | mux ch2 | empty |
| i2c-4 | mux ch3 | 0x2e **adt7475** — the **PWM fan controller** (temp/voltage sense + fan drive) |

`i2c-gen` (0xfd894000) is disabled - no adapter number allocated for it.

## Chips (physical, photo-confirmed)

- **UB1 + U10 = 2× PCA9575PW** (24-pin TSSOP) → i2c 0x20 / 0x21. **Only two.** The DT's
  third `@0x29` is the 6-bay **UDM-Pro's** second bay-expander; on 4-bay ea16 it is
  **unpopulated** ("pca953x 0-0029: failed reading register"). Photos `20260816_232614`
  (UB1), `_231415` (U10).
- **0x21** = bay control: pwren lines 0-3 (gpio-hog output-high), presence 4-7, fault LEDs 12-15.
- **0x20** = straps + `sfp_1g` LED, pin 2, ACTIVE_HIGH (per live stock board-cfg capture).
  Chip/pin/polarity confirmed correct; pin still produces no visible light under direct
  GPIO test (#98) - possible hardware population/wiring issue, not a software fix.
- **adt7475** (U27, behind mux ch3) = the **PWM fan controller** — 3 temp + 2 voltage inputs
  drive 3 PWM fan outputs (stock env `slowfan` pokes 0x2e regs 0x30-0x32/0x5c-0x5e). rev 2.
  SENSORS_ADT7475. NOT the RPS current monitor.
- **No RPS i2c monitor exists on this bus.** Originally hypothesized as 2× ORing power
  monitors (INA/ISL class) on `i2c_gen` (fd894000), one per rail (12 V / 54 V) — disproven
  by #64: pins 30/31 (i2c_gen's would-be SCL/SDA) are physically muxed to ETH-LED/ulogo_blue
  on this board's live 48-ball read, not to any i2c bus, and no Ubiquiti firmware carries an
  RPS i2c-monitor driver. **U48** is very likely a pure analog part with no digital interface.
  ORing FET path (`Q536/Q537/Q14/Q59/Q590`) still worth a look via `docs/rps-subsystem.md` if
  RPS monitoring is wanted, just not via i2c.
- **s35390a** RTC (behind mux ch0), coin cell SII MS621.
- **24C64** EEPROM @0x57 = DDR-config blob (reads 0x1c36-repeating, not plain SPD) — #67.
- Bay-activity LEDs are **not** i2c: SGPO `fd8b4000` → external **74VHC595 (UB20)** shift
  register, sgpo lines 16-23. See [gpio-switches-leds.md](gpio-switches-leds.md).

## KNOWN BUG — mux ch0 (s35390a RTC) wedges the bus

Selecting ch0 + touching any address holds SDA low and wedges the whole pld bus (s35390a RTC
@0x30 strongly implicated). The SFP EEPROM @0x50 is on ch1, a separate bus, uninvolved in this
wedge. Likely cause = a
dropped `i2c-sda-hold-time-ns`, fixed by restoring it — **full analysis, fix, and recovery in
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
