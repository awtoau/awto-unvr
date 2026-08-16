# [Patch 11] SGPO LED controller + HDD bay LEDs
labels: porting, leds, gpio, patch
---

Small Alpine SGPO (serial GPIO) driver exposed via gpiochip + LED subsystem; drives
the 8 SATA bay activity/presence LEDs.
Roadmap: [patch order](../porting-roadmap.md#patch-order).

## Tasks
- [ ] SGPO gpio-controller driver (`annapurna-labs,alpine-sgpo` @ 0xfd8b4000, 64
  lines) — RE'd out-of-tree already exists as a base.
- [ ] Map 8 bay LEDs (host0/1 ports 0–3) to gpio-leds / ledtrig-disk.
- [ ] HDD power control (`ubnt,hdd-pwrctl`: pwren/present/fault-led GPIOs).
- [ ] u-logo blue/white LEDs via PL061 (gpio-leds).

## Live evidence
- `al-sgpo ... probed`, `gpiochip8` 64 lines, 8 `sata led gpio`
  ([gpio.txt](../hw-reference/20260816-104601/gpio.txt)); `sata_sw_leds` +
  `hdd_pwrctl1/2` mapping in [live.dts](../hw-reference/20260816-104601/live.dts).

## Acceptance criteria
- [ ] Bay LEDs reflect disk activity/presence; u-logo controllable.

Depends: #12.
