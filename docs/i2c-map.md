# I2C map — UNVR ea16 (live, 2026-08-18)

Two DesignWare controllers + a 4-channel mux. Scanned live from Fedora (kernel sysfs =
authoritative; `i2cdetect` is unreliable here — the DW adapter has no SMBus-quick-write so
it silently *skips* 0x00-0x02/0x28-0x2f/0x40-0x4f unless `-r` read-probe is forced).

## Buses

| Linux bus | Controller / source | Devices |
|-----------|---------------------|---------|
| i2c-0 | DW @0xfd880000 "i2c-pld" | 0x20, 0x21 PCA9575; 0x57 24C64 EEPROM; 0x71 PCA9546 mux |
| i2c-1 | DW @0xfd894000 "i2c-gen" | **empty** AND unusable — pins 30/31 muxed to ETH-LED/GPIO, not I2C_GEN (func2). Needs a pinmux change to use (#64) |
| i2c-2 | mux ch0 | 0x30 s35390a **RTC**; 0x50 **SFP module EEPROM** (al_eth, not in DT) |
| i2c-3 | mux ch1 | empty |
| i2c-4 | mux ch2 | empty |
| i2c-5 | mux ch3 | 0x2e **adt7475** fan/temp/voltage monitor |

## Chips (physical, photo-confirmed)

- **UB1 + U10 = 2× PCA9575PW** (24-pin TSSOP) → i2c 0x20 / 0x21. **Only two.** The DT's
  third `@0x29` is the 6-bay **UDM-Pro's** second bay-expander; on 4-bay ea16 it is
  **unpopulated** ("pca953x 0-0029: failed reading register"). Photos `20260816_232614`
  (UB1), `_231415` (U10).
- **0x21** = bay control: pwren lines 0-3 (gpio-hog output-high), presence 4-7, fault LEDs 12-15.
- **0x20** = SFP+ 1G link LED (pin 2) + straps.
- **adt7475** (U27, behind mux ch3) = the board **voltage/temp/fan monitor** ("the current
  monitor"). rev 2. SENSORS_ADT7475.
- **s35390a** RTC (behind mux ch0), coin cell SII MS621.
- **24C64** EEPROM @0x57 = DDR-config blob (reads 0x1c36-repeating, not plain SPD) — #67.
- Bay-activity LEDs are **not** i2c: SGPO `fd8b4000` → external **74VHC595 (UB20)** shift
  register, sgpo lines 16-23. See [gpio-switches-leds.md](gpio-switches-leds.md).

## KNOWN BUG — behind-mux i2c times out on mainline 7.1

- **Every device behind the PCA9546 mux times out**: RTC (ch0) probe `-5`, SFP EEPROM 0x50
  (ch0) "Read failed", adt7475 (ch3) probe `-110` (reads "ADT7475 rev 2" then times out).
  Direct bus-0 devices (0x20/0x21/0x57/0x71) work.
- **Stock 5.1 reads all of these fine** → mainline-7.1 regression in the i2c-designware /
  pca954x path, not hardware. DT is identical to the reference (plain `nxp,pca9546`, no
  `i2c-mux-idle-disconnect`).
- **Symptom = console flood.** al_eth's link manager polls the SFP EEPROM (0x50, ch0) every
  ~2 s (`al_eth_module_detect` → `al_eth_i2c_byte_read`); each poll times out → endless
  `i2c_designware fd880000.i2c: controller timed out`. Confirmed by ftrace stack.
- **Mitigation:** keep the SFP port (`enp0s2`) **down** until fixed — stops the flood (0 new
  timeouts), but the SFP is unusable meanwhile. It re-floods when the port is up.
- **Candidate fixes (untested):** add `i2c-mux-idle-disconnect` to the mux node + rebuild the
  Fedora DTB (mux-channel race under al_eth's concurrent polling); or an al_eth SFP-i2c
  retry/backoff so an unreadable module doesn't hammer at 2 s. Also breaks RTC + hwmon fan
  control, so worth a real fix.

## Re-scan recipe

- Linux: `i2cdetect -l`; per-bus map `for d in /sys/bus/i2c/devices/*; do echo $(basename $d) $(cat $d/name); done`.
  Force read-probe for hidden devices: `i2cdetect -y -r <bus>` (behind-mux buses can stall on
  per-address timeouts — bounded ranges only).
- U-Boot: `i2c dev <n>; i2c probe`. Do **not** hand-force the mux channel (`i2c mw 0x71 …`)
  while LED blink is running — it hangs the DW bus (needs SP805 reset). Use a mux DT node instead.
