# RTC s35390a / ch0 i2c wedge: likely cause + why the current U-Boot path can't recover it

Datasheet: [sources/chips/S-35390A.pdf](../sources/chips/S-35390A.pdf) (ABLIC Rev.4.2), chip doc
[docs/chips/s-35390a.md](chips/s-35390a.md). Single source for the ch0 wedge — other docs link here.

## What happens
- Selecting PCA9546 **ch0** + touching *any* address (even a nonexistent one) **holds SDA low
  at the pin** and wedges the whole pld bus. `i2c reset` can't clear it — only a SoC reset.
- ch0 devices: **s35390a RTC @0x30** + **SFP EEPROM @0x50**.
- Reproduces in stock U-Boot, our U-Boot, and Linux 7.1.
- Behind-mux is otherwise fine: ch3 **adt7475 reads `0x3d = 0x75` at 100 kHz**, a nonexistent
  ch3 address NAKs cleanly, bus survives. Mux / DW / timing / NAK handling all OK — **only ch0**.
- **SCL is not held** — the S-35390A's SCL is input-only. Only **SDA** is bidirectional
  open-drain, so the chip can pull SDA low, never SCL.

## Likely cause — a dropped DT timing property, NOT a dead chip
- Mainline's ea16 DTB lost **`i2c-sda-hold-time-ns`** (stock had 0x12c = **300 ns**; see
  `docs/hw-reference/.../live.dts`).
- Without it the DW driver reuses the leftover `DW_IC_SDA_HOLD` value — too short — so the SDA
  data edge drifts into the SCL-high window and the timing-fussy s35390a mis-reads a phantom
  START/STOP. That is exactly the **communication-interruption** condition the datasheet
  documents (Fig 46, "Reset After Communication Interruption"): the transfer aborts mid-frame
  and the chip **holds SDA low** until it is clocked through its pending transfer.
- The tolerant chips (adt7475 / EEPROM / PCA9575) don't care; only the RTC does.
- **The chip is a healthy validated production part.** This is a software/DT fault.

### Abnormal-state caveat (which RTC state, exactly)
- POC/BLD or an invalid power ramp can leave the internal circuit in the datasheet's
  **"indefinite status"**, which requires initialization. Separately, the datasheet explicitly
  documents a **communication-interruption** state in which the S-35390A holds SDA low until
  clocked through its pending transfer.
- The observed ch0 wedge is **consistent with one of these abnormal RTC states, but the tests do
  not distinguish which one.** The dropped `i2c-sda-hold-time-ns` is the concrete mechanism that
  triggers the communication-interruption path.

## Fix (committed)
- Restore **`i2c-sda-hold-time-ns = <300>`** on both `i2c_pld` nodes (Fedora + U-Boot dts).
  Dramatically reduced the arbitration-loss flood (to ~1/boot). **Verified.**
- Idle mux containment: keep the PCA9546 deselected when idle. U-Boot's mux uclass deselects
  (writes 0x00) after each access; Linux needs `i2c-mux-idle-disconnect`. This **reduces the
  exposure of the main bus** by leaving ch0 disconnected when idle — it does **not** guarantee
  containment of a hard SDA-low wedge: once ch0 is selected and its target drags SDA low,
  software may be unable to send the deselect. A hardware **PCA9546 RESET line** (if the board
  has one) would be the stronger containment/recovery path.

## Clearing an already-latched chip — the 63-clock procedure
- If a chip latched into the SDA-hold state on a *prior* boot, the timing fix alone won't
  release it. Datasheet recovery (Fig 46, load-bearing):
  **START → 63 SCL clocks with master SDA released (High-Z) → STOP.** This clocks the chip
  through its interrupted transfer and releases SDA. Caution: if it lands on a write-ACK, a
  stray write may occur.
- **No mechanism to run it on this U-Boot / DesignWare path.** The "not recoverable here"
  conclusion rests on two facts:
  1. the datasheet's recovery needs **raw SCL clocking** (63 pulses, master SDA released), and
  2. this path has **no way to generate that** — AL-324 DW core is **v1.20** (IC_COMP_VERSION
     `0x3132302a`), no HW SDA-stuck-recovery (IC_ENABLE[3] is a v2.00a+ feature, bit doesn't
     stick); needs U-Boot board-init raw clocking or an `scl-gpios`/`sda-gpios` bit-bang — **iff**
     the pld SCL/SDA can be driven as GPIO. They aren't in the MUIO GPIO map → route uncertain
     (open item).
- Not resting on "POC/BLD causes the hold" — the recovery gap holds regardless of which abnormal
  state latched the chip.

## RTC electrical / recovery notes
- **VDD ≥ 1.3 V** for specified communication (below this the 2-wire interface is out of spec).
- **VDET = 0.65–1.35 V (1.0 V typ).** Timekeeping minimum = **VDET − 0.15 V**, so ~0.85 V is a
  *derived typical*, NOT a guaranteed fixed threshold.
- Backup cell **SII MS621**: **3.0 V nominal, 2.8–3.3 V charge voltage, 3.1 V standard charge**.
- **Recovery, not a power-up prerequisite:** a charged cell is not a datasheet prerequisite for
  operation. With no usable bus-recovery path, recovery requires the RTC to return to a valid
  electrical state — e.g. a true RTC power cycle with the datasheet-compliant VDD ramp: **from
  0 V, RTC VDD must reach 1.3 V within 10 ms** (ABLIC). A healthy charged MS621 should normally
  prevent RTC VDD from collapsing into the low-voltage region; **if the cell is flat, measure
  RTC VDD during power-up** to confirm the 0→1.3 V / 10 ms requirement.

## Attribution — ch0 wedged (proved) vs RTC the culprit (strongly implicated)
- The address-independent failure proves a **downstream-ch0 problem** directly. The RTC is
  **strongly implicated** but not proved sole culprit: the **SFP EEPROM @0x50 also sits on ch0**,
  and the current tests did not physically remove/isolate it.
- Stock **Linux 5.1 previously communicated with the RTC successfully.** The upstream driver
  contains the RESET init path (`s35390a_init()` writes RESET|24H) but does **not** issue RESET
  before its first STATUS1 access — probe reads STATUS1 and continues; init runs from
  `set_time()` / `RTC_VL_CLR`. Current upstream is the same. So earlier success does **not** prove
  RESET-at-probe recovered the device.
- The driver's **"error resetting chip"** probe message is misleading — that failure can occur on
  the initial STATUS1 read, *before* any RESET write.

## Console flood (the visible symptom)
- al_eth polls the SFP EEPROM (0x50, ch0) every ~2 s (`al_eth_module_detect` →
  `al_eth_i2c_byte_read`); each poll hits a wedged ch0 → endless
  `i2c_designware fd880000.i2c: controller timed out`. Confirmed by ftrace stack.
- Mitigation: unbind al_eth from the SFP port —
  `echo 0000:00:02.0 > /sys/bus/pci/drivers/al_eth/unbind` (or keep `enp0s2` down). RJ45/SSH
  unaffected. Not persistent.

## Open questions
- Which abnormal RTC state actually latches ch0 (communication-interruption vs indefinite status)?
- Can pld SCL/SDA be driven as GPIO for a bit-bang 63-clock recovery (MUIO map says no)?
- Does the board expose a PCA9546 RESET line for hardware containment?
- Is the SFP EEPROM fully ruled out? (needs an isolate-and-retest on ch0).
- MS621 health: measure RTC VDD ramp on power-up (0→1.3 V / 10 ms).
