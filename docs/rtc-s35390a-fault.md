# RTC s35390a — ch0 i2c wedge (root cause + why it's not U-Boot-fixable)

Datasheet: [sources/S35390A_E.pdf](../sources/S35390A_E.pdf) (ABLIC Rev.4.2). All tests 2026-08-18.

## One-line
The **s35390a RTC on PCA9546 mux ch0 holds the i2c line at the pin level** — it's in the
datasheet's **"indefinite status"** after power-loss/low-battery and never got the mandated
**RESET**. Everything else on i2c is fine. It cannot be cleared from U-Boot on this SoC.

## Datasheet mechanism (this is by design, not a bug)
- **STATUS1** (1 byte): `B0 POC` (power-on, set at power-up, read-clears), `B1 BLD`
  (battery-low, latches when Vdd ≤ V_DET), `B7 RESET` (write 1 to initialize).
- **After POC=1 or BLD=1 you MUST write RESET.** If you don't: *"the internal circuit may be
  in the indefinite status"* (datasheet, Power-on Detection + Low-Voltage sections). Also
  indefinite if the rail doesn't ramp 0→1.3 V within 10 ms at power-on.
- Indefinite status ⇒ unpredictable behaviour, incl. **holding SDA/SCL** — which is what we see.
- Voltages (Tables 4/6/7): **communicate ≥ 1.3 V**; keep time down to ~0.85 V; **V_DET
  (battery-low threshold) = 1.0 V typ (0.65–1.35 V)**, ~0.15 V hysteresis.
- Stock Linux 5.1 read the RTC because its rtc-s35390a **writes RESET** at probe and the chip
  powered up clean (charged cell → retained state → POC handled). Our mainline-7.1 probe
  fails `-5` *before* completing that, so the chip stays indefinite.

## How to detect a bad backup battery
- **Software:** read STATUS1 → **BLD (bit 1) = 1** means the cell dropped to ≤ ~1 V (V_DET).
  `POC (bit 0) = 1` means power was lost. (Needs a working bus — which ch0 currently isn't.)
- **Physical:** measure the **SII MS621** cell. Healthy charged ≈ 2.5–3.3 V; **< ~1 V = flat**
  (below V_DET, BLD will latch). Note it's *rechargeable* — if it won't hold/charge, that's a
  dead cell or a broken charge path.

## What we tested
- **Behind-mux WORKS**: ch3 adt7475 reads `0x3d=0x75` at 100 kHz; nonexistent addr on ch3
  NAKs clean, bus survives. Mux/DW/timing/NAK-handling all fine.
- **ch0 wedges on contact**: selecting ch0 + *any* access (even a nonexistent address, even a
  RESET **write**) → `Timed out waiting for bus` / -121, and the whole bus (incl. main-bus
  0x21) hangs until an SP805 reset. So the line is held **before a transfer can even start**.
- **No in-controller recovery**: AL-324 DW i2c core is **v1.20** (IC_COMP_VERSION `0x3132302a`).
  No hardware SDA-stuck-recovery (IC_ENABLE[3] doesn't stick — that's a v2.00a+ feature).
  `i2c reset` doesn't recover. IC_STATUS idle / TX_ABRT_SOURCE 0, yet transfers refused.
- **No GPIO deblock route**: the pld i2c (fd880000) isn't in the MUIO GPIO map, so we can't
  bit-bang SCL to clock the chip free.

## Conclusion
- **Not fixable from U-Boot / mainline i2c on this board.** We can neither deliver the RESET
  (bus held first) nor recover the held bus (no HW recovery, no GPIO pins).
- The chip only comes good when it **powers up clean** — which needs a **charged MS621** (so it
  retains state and POC is handled) and a proper power ramp. If the cell truly won't charge,
  that's the fault to fix (cell or charge path); no amount of software clears a held bus.
- Optional hardening once the RTC is reachable: add `i2c-mux-idle-disconnect` so a future ch0
  wedge can't poison the main bus / other channels.

## Meanwhile — al_eth console flood
al_eth polls the SFP EEPROM (0x50, also ch0) every ~2 s → hits the wedged ch0 → endless
`i2c_designware fd880000.i2c: controller timed out`. Mitigate by unbinding the SFP port:
`echo 0000:00:02.0 > /sys/bus/pci/drivers/al_eth/unbind` (RJ45/SSH unaffected). See
[i2c-map.md](i2c-map.md).
