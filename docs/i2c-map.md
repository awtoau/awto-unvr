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

## KNOWN BUG — mux **ch0 (s35390a RTC)** wedges the i2c bus

Corrected 2026-08-18 (an earlier note wrongly said "all behind-mux fails" — it doesn't).

- **Behind-mux WORKS.** ch3 **adt7475 reads fine at 100 kHz** (`0x3d = 0x75`), and a
  nonexistent address on ch3 NAKs cleanly + the bus survives (deselect + main-bus read both
  OK after). So the mux, DW i2c, timing, and NAK-handling are all fine.
- **Only ch0 is bad.** Selecting ch0 and touching *any* address (even a nonexistent one)
  **holds SDA low at the pin level** and wedges the whole bus — `i2c reset` can't clear it,
  only a SoC reset. ch0's device is the **s35390a RTC** (0x30); the SFP EEPROM (0x50) shares
  ch0, so it's collateral. Reproduces in stock U-Boot, our U-Boot, and Linux 7.1.
- **A healthy s35390a does not hold SDA** — a browned-out one does. Prime suspect: a
  **discharged MS621 rechargeable backup cell** leaving the RTC in a bad state. Fits every
  observation (RTC-only, pin-level hold, unrecoverable by re-init, all three i2c stacks).
- **No software recovery available in-controller:** the AL-324 DW i2c core is **v1.20**
  (IC_COMP_VERSION `0x3132302a`); it lacks hardware SDA-stuck-recovery (IC_ENABLE[3], a
  v2.00a+ feature — bit doesn't stick), and the driver refuses to clock a bus it sees busy
  (IC_STATUS idle, TX_ABRT_SOURCE 0, yet transfers return -121/EREMOTEIO). Only a GPIO
  bit-bang deblock of the pld SCL/SDA could free it — and the pld bus isn't in the MUIO GPIO
  map, so that route likely doesn't exist.
- **Symptom = console flood.** al_eth's link manager polls the SFP EEPROM (0x50, ch0) every
  ~2 s (`al_eth_module_detect` → `al_eth_i2c_byte_read`); each poll hits the wedged ch0→
  endless `i2c_designware fd880000.i2c: controller timed out`. Confirmed by ftrace stack.
- **Mitigation:** unbind al_eth from the SFP port — `echo 0000:00:02.0 > /sys/bus/pci/drivers/al_eth/unbind`
  (or keep `enp0s2` down). Stops the flood (0 new timeouts); RJ45/SSH unaffected. Not persistent.
- **Real fix path:** confirm/charge the MS621 cell first (path 2 — costs nothing, box charges
  it while running). If the cell is fine and ch0 still wedges → genuine s35390a fault, then
  invest in a GPIO bit-bang i2c deblock. Also add `i2c-mux-idle-disconnect` regardless, so a
  wedged ch0 can't poison the main bus / other channels.

## Re-scan recipe

- Linux: `i2cdetect -l`; per-bus map `for d in /sys/bus/i2c/devices/*; do echo $(basename $d) $(cat $d/name); done`.
  Force read-probe for hidden devices: `i2cdetect -y -r <bus>` (behind-mux buses can stall on
  per-address timeouts — bounded ranges only).
- U-Boot: `i2c dev <n>; i2c probe`. ch3 is reachable via `i2c mw 0x71 0 8 1` then
  `i2c md 0x2e 3d 1` (adt7475 = 0x75). Do **not** select ch0 (`i2c mw 0x71 0 1 1`) + read —
  it wedges the bus and needs an SP805 reset. (Our U-Boot LED blink is now solid-on so a
  wedge no longer floods — see defconfig PREBOOT.)
