# RTC s35390a — ch0 i2c wedge: root cause, fix, recovery

Datasheet: [sources/chips/S-35390A.pdf](../sources/chips/S-35390A.pdf) (ABLIC Rev.4.2), chip doc
[docs/chips/s-35390a.md](chips/s-35390a.md). Single source for the ch0 wedge — other docs link here.

## What happens
- Selecting PCA9546 **ch0** + touching *any* address (even a nonexistent one) **holds SDA low
  at the pin** and wedges the whole pld bus. `i2c reset` can't clear it — only a SoC reset.
- ch0 devices: **s35390a RTC @0x30** (the culprit) + **SFP EEPROM @0x50** (collateral).
- Reproduces in stock U-Boot, our U-Boot, and Linux 7.1.
- Behind-mux is otherwise fine: ch3 **adt7475 reads `0x3d = 0x75` at 100 kHz**, a nonexistent
  ch3 address NAKs cleanly, bus survives. Mux / DW / timing / NAK handling all OK — **only ch0**.

## Root cause — a dropped DT timing property, NOT a dead chip
- Mainline's ea16 DTB lost **`i2c-sda-hold-time-ns`** (stock had 0x12c = **300 ns**).
- Without it the DW driver reuses the leftover `DW_IC_SDA_HOLD` value — too short — so the SDA
  data edge drifts into the SCL-high window and the timing-fussy s35390a mis-reads a phantom
  START/STOP, aborts the transfer and **holds SDA** (datasheet Fig 46, "Reset After
  Communication Interruption").
- The tolerant chips (adt7475 / EEPROM / PCA9575) don't care; only the RTC does.
- **The chip is a healthy validated production part.** This is a software/DT fault.

## Fix (committed)
- Restore **`i2c-sda-hold-time-ns = <300>`** on both `i2c_pld` nodes (Fedora + U-Boot dts).
- Keep the PCA9546 deselected when idle so a wedged ch0 can't poison the parent bus — U-Boot's
  mux uclass deselects (writes 0x00) after each access; Linux needs `i2c-mux-idle-disconnect`.

## Clearing an already-latched chip
- If a chip latched into the SDA-hold state on a *prior* boot, the timing fix alone won't
  release it. Datasheet recovery: **START → 63 SCL clocks (master SDA released, High-Z) →
  STOP** (Fig 46). Caution: if it lands on a write-ACK, a stray write may occur.
- **No in-controller recovery on this SoC:** AL-324 DW core is **v1.20** (IC_COMP_VERSION
  `0x3132302a`) — no HW SDA-stuck-recovery (IC_ENABLE[3], a v2.00a+ feature — bit doesn't
  stick). Needs raw clocking: U-Boot board init or an `scl-gpios`/`sda-gpios` bit-bang — **iff**
  the pld SCL/SDA can be driven as GPIO. They aren't in the MUIO GPIO map, so that route is
  uncertain (open item).

## Console flood (the visible symptom)
- al_eth polls the SFP EEPROM (0x50, ch0) every ~2 s (`al_eth_module_detect` →
  `al_eth_i2c_byte_read`); each poll hits a wedged ch0 → endless
  `i2c_designware fd880000.i2c: controller timed out`. Confirmed by ftrace stack.
- Mitigation: unbind al_eth from the SFP port —
  `echo 0000:00:02.0 > /sys/bus/pci/drivers/al_eth/unbind` (or keep `enp0s2` down). RJ45/SSH
  unaffected. Not persistent.
