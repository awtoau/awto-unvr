# RTC s35390a / ch0 i2c wedge — likely cause + the datasheet recovery

Datasheet: [sources/chips/S-35390A.pdf](../sources/chips/S-35390A.pdf) (ABLIC Rev.4.2), chip doc [docs/chips/s-35390a.md](chips/s-35390a.md). Tests 2026-08-18.

## One-line
Something on PCA9546 mux **ch0 holds SDA low** and wedges the i2c bus. ch0's only device
(SFP pulled) is the **S-35390A RTC**, whose datasheet documents exactly this stuck-SDA state
and a specific recovery: **START → 63 SCL clocks (master SDA released) → STOP**. The current
U-Boot/DesignWare path has no way to emit that raw clocking, so it can't recover it *yet*.

## The datasheet recovery — "Reset After Communication Interruption" (Fig 46, p.36)
This is the load-bearing mechanism:
- If communication is interrupted (e.g. master reset / aborted transfer mid-byte), the
  S-35390A keeps its prior internal state and, if it was driving SDA (ACK or read output),
  **holds SDA low** and **will not accept a STOP**.
- Recovery: master issues a **START**, then **63 SCL clock pulses with its SDA released
  (High-Z)** — 7 bytes' worth — then a **STOP**. This clocks the RTC through its pending
  transfer, releases SDA, and resets the internal circuit.
- ABLIC: *"recommended to be executed at initialization of the system after the master's
  power supply voltage is raised."* Caution: if it lands on a write-ACK, a stray write may occur.
- Note: **SCL is input-only** on the S-35390A; only **SDA** is bidirectional/open-drain. So the
  chip can hold **SDA** low — never SCL.

## Second, separate mechanism (don't conflate)
- STATUS1: `B0 POC` (power-on, read-clears), `B1 BLD` (battery-low, latches ≤ V_DET),
  `B7 RESET` (write 1 to init). After POC=1 or BLD=1, or if the rail doesn't ramp 0→1.3 V
  within 10 ms, the internal circuit may be in the datasheet's **"indefinite status"** and
  needs the RESET init.
- This is a *different* condition from the SDA-hold above. Our tests show the SDA hold; they do
  **not** prove which abnormal state caused it. Present both, attribute to neither exclusively.

## Voltages (Tables 4/6/7)
- Communicate: **V_DD ≥ 1.3 V** (specified). Time-keeping min: **V_DET − 0.15 V** (so ~0.85 V is
  a derived typical, not a fixed guaranteed threshold).
- **V_DET (battery-low threshold) = 1.0 V typ, 0.65–1.35 V**, ~0.15 V hysteresis.
- Backup cell (SII MS621): **3.0 V nominal, 2.8–3.3 V charge, 3.1 V standard charge.** Below
  V_DET the BLD flag latches.
- **NB:** while the box is powered the RTC runs on the main ~3.3 V rail, not the cell — so the
  cell is irrelevant to whether it *communicates* during operation. The chip is powered and
  should respond; the wedge is a bus/protocol condition, i.e. a **code** problem to recover, not
  a dead-chip problem.

## How to detect a bad backup cell
- **Software:** read STATUS1 → **BLD (bit 1)=1** = cell dropped ≤ ~1 V; POC (bit 0)=1 = power lost.
- **Physical:** measure the MS621 — charged ≈ 3.0 V (2.8–3.3 V); a flat/failed-charge cell reads
  low. If suspect, measure **RTC V_DD during power-up** to check the datasheet 0→1.3 V/10 ms ramp.

## What we tested
- **Behind-mux WORKS**: ch3 adt7475 reads `0x3d=0x75` at 100 kHz; nonexistent addr on ch3 NAKs
  clean, bus survives. Mux/DW/timing/NAK-handling all fine.
- **ch0 holds SDA on contact**: selecting ch0 + *any* access (even a nonexistent address, even a
  RESET **write**) → `Timed out waiting for bus` / -121, and the whole bus (incl. main-bus 0x21)
  hangs until an SP805 reset. SFP was **physically removed**, so ch0's remaining device is the
  S-35390A — it is the source, not merely implicated.
- **No in-controller recovery on this core**: DW i2c is **v1.20** (IC_COMP_VERSION `0x3132302a`);
  no HW SDA-stuck-recovery (IC_ENABLE[3] doesn't stick — v2.00a+ only). `i2c reset` and a manual
  IC_ENABLE toggle do not recover it (mux stays on ch0; can't send the deselect while SDA held).
- The Linux "error resetting chip" probe message is misleading: rtc-s35390a reads STATUS1 first;
  that read can fail *before* any RESET write. Upstream (incl. v5.1) does NOT issue RESET before
  its first STATUS1 access, so earlier stock success doesn't prove RESET-at-probe fixed it.

## Where the fix lives (under investigation)
The documented recovery is **raw SCL clocking** (START + 63 clocks + STOP), not a register write —
so it needs a way to generate clocks on the pld bus:
- **U-Boot board init**: run the 63-clock recovery on fd880000 at boot, before Linux.
- **Linux**: either the same via a driver hook, or provide `scl-gpios`/`sda-gpios` on the i2c node
  so `i2c_generic_scl_recovery` can bit-bang it — IF the pld SCL/SDA can be muxed to GPIO.
- Open question (being researched): can the DW v1.20 core emit the 63 clocks (dummy transfer), or
  must we bit-bang GPIO? And are the pld i2c pins GPIO-muxable at all?

## Containment (partial, not a fix)
- `i2c-mux-idle-disconnect` on the PCA9546 **reduces** main-bus exposure by leaving ch0
  disconnected when idle — but does **not** guarantee containment: once ch0 is selected and its
  target drags SDA low, software may be unable to send the deselect command. A hardware PCA9546
  RESET line (if the board has one) would be the stronger recovery/containment path.

## Meanwhile — al_eth console flood
al_eth polls the SFP EEPROM (0x50, ch0) every ~2 s → hits the held ch0 → endless
`i2c_designware fd880000.i2c: controller timed out`. An interrupted poll is itself a plausible
*cause* of the comm-interruption SDA hold. Mitigate by unbinding the SFP port:
`echo 0000:00:02.0 > /sys/bus/pci/drivers/al_eth/unbind` (RJ45/SSH unaffected). See [i2c-map.md](i2c-map.md).
