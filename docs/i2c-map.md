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

```
AL-324 SoC
├─ DW i2c-pld @0xfd880000  ── i2c-0 (PARENT)
│    ├─ 0x20  PCA9575   (SFP LED + straps)
│    ├─ 0x21  PCA9575   (bay power / presence / fault LEDs)
│    ├─ 0x57  24C64     (DDR-config EEPROM)
│    └─ 0x71  PCA9546 4-ch MUX
│               ├─ ch0 → i2c-1 ── 0x30-0x37 s35390a RTC (bound as rtc0)
│               ├─ ch1 → i2c-2 ── 0x50 SFP+ module EEPROM
│               ├─ ch2 → i2c-3 ── (empty)
│               └─ ch3 → i2c-4 ── 0x2e adt7475 fan controller
└─ DW i2c-gen @0xfd894000  ── disabled, no adapter, NO REAL BUS (#64)
     pins 30/31 (would-be SCL/SDA) are muxed to ETH-LED/ulogo_blue instead -
     nothing to scan here regardless of enable state.
```

## Chips (physical, photo-confirmed)

- **UB1 + U10 = 2× PCA9575PW** (24-pin TSSOP) → i2c 0x20 / 0x21. **Only two.** The DT's
  third `@0x29` is the 6-bay **UDM-Pro's** second bay-expander; on 4-bay ea16 it is
  **unpopulated** ("pca953x 0-0029: failed reading register"). Photos `20260816_232614`
  (UB1), `_231415` (U10).
- **0x21** = bay control: pwren lines 0-3, presence 4-7, fault LEDs 12-15. pwren is now
  4× `regulator-fixed` nodes (per bay) instead of a gpio-hog, giving AHCI a real probe-order
  dependency via `fw_devlink` (#104) - landed in the DTS but **not yet hardware-tested**
  (no boot-with-drives verification run yet).
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

## ch0 wedge — FIXED (2026-09-04)

Cause was mainline's `IC_ENABLE.ABORT` terminating transfers mid-byte, which the s35390a answers
by holding SDA low. Suppressed via `snps,no-enable-abort` on `i2c_pld`. A full
`i2cdetect -y -r 1` of ch0 now completes with 0 timeouts; the bus runs at 400 kHz.
Full analysis in [rtc-s35390a-fault.md](rtc-s35390a-fault.md).

## Re-scan recipe

- Linux: `i2cdetect -l`; per-bus map `for d in /sys/bus/i2c/devices/*; do echo $(basename $d) $(cat $d/name); done`.
  Read-probe every bus including ch0: `i2cdetect -y -r <bus>`. Safe since the ABORT fix.
- U-Boot: `i2c bus` lists the pld bus and the four mux child buses; `i2c dev <n>; i2c probe`
  scans each. U-Boot never issued `IC_ENABLE.ABORT`, so ch0 was always safe there.
