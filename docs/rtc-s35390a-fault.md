# RTC s35390a / ch0 i2c wedge — a CODE fault in how we drive ch0, NOT a hardware fault

Datasheet: [sources/chips/S-35390A.pdf](../sources/chips/S-35390A.pdf) (ABLIC Rev.4.2), chip doc
[docs/chips/s-35390a.md](chips/s-35390a.md). Single source for the ch0 wedge — other docs link here.

## Root cause: OUR software, not the chip and not the cell
- **The chip is good and the coin cell is good.** Stock **Linux 5.1 talked to this exact RTC on
  this exact board successfully.** A part that answers under stock firmware is not faulty, and a
  cell that keeps it alive is not flat. Do **not** conclude "dead chip", "flat MS621", or "pull the
  cell / power-cycle the RTC" — that was wrong and is now struck from this doc.
- The wedge is produced **fresh, by our code, every time we touch ch0** — it is not a stuck
  hardware state we inherit and must "recover". Fix the code that drives ch0 and the wedge goes
  away. This is the project's own rule: our U-Boot does all init itself; when it misbehaves, assume
  our code, not the silicon.
- Working reference = **stock Linux 5.1's i2c/RTC path.** The bug is the delta between that and
  ours (U-Boot + Linux 7.1). Close the delta.

## What happens (symptom)
- Selecting PCA9546/TCA9546 **ch0** + touching *any* address (even a nonexistent one) makes the
  s35390a hold **SDA low** at the pin and wedges the whole pld bus; `i2c reset` doesn't clear it.
- ch0 devices: **s35390a RTC @0x30** + **SFP EEPROM @0x50**. SCL is never held (S-35390A SCL is
  input-only); only SDA is driven low.
- ch3 (**adt7475** fan ctrl) reads cleanly, NAKs a nonexistent address cleanly, bus survives — so
  the mux, the DW controller, and NAK handling are fine in general. **Only our ch0 access wedges.**
- Datasheet Fig 46 ("Reset After Communication Interruption"): if a transfer to the s35390a is
  aborted mid-frame, the chip holds SDA low until clocked through. So **our access is aborting the
  s35390a mid-transfer** — the question is which part of our code does that.

## Suspected code bug (to pin — this is the open work)
Stock Linux 5.1 worked; ours wedges. Candidates, in order:
1. **i2c SCL timing not matching stock's proven values.** Stock's DTB carries explicit raw counts
   `i2c-{ss,fs,hs}-scl-{h,l}cnt-raw` AND `i2c-sda-hold-time-ns` (live.dts:249-255). We restored ONLY
   `sda-hold-time-ns=300` and left the DW driver to *compute* the SCL h/l counts from
   clock-frequency + the 500 MHz input clock. If our computed SCL edges differ from stock's raw
   counts, the timing-fussy s35390a mis-samples a phantom START/STOP and aborts the transfer.
   **Fix to try: set the raw hcnt/lcnt props to stock's exact values on `i2c_pld` in both DTS.**
   (100 kHz alone did NOT fix it — so speed is not the whole story; the exact edge counts are.)
2. **Mux select/deselect sequence.** How we select ch0 and whether we deselect correctly around the
   RTC access — a malformed select/transfer/deselect can leave the s35390a mid-frame.
3. **s35390a access sequence.** The mainline driver's probe reads STATUS1 first; the chip's odd
   addressing (device addrs 0x30-0x36 = registers, bit-reversed data) means a malformed first
   access wedges it. Compare our driver/access path to stock Linux 5.1's.

## Not the cause (ruled out / struck)
- **Bit-bang bus recovery** — the pld i2c SCL/SDA are dedicated pads, not MUIO-muxable to GPIO
  (RESOLVED below), so `scl-gpios`/`sda-gpios` recovery is impossible. But recovery is the wrong
  frame anyway: **fix the code so we never wedge it**, don't build machinery to un-wedge it.
- **RTC power-cycle / coin-cell pull / MS621 health** — struck. The chip and cell are proven good.
- **DW core v1.20 "no HW recovery"** — true but irrelevant once we stop wedging the chip.

## pld SCL/SDA GPIO routability — RESOLVED: NO (2026-08-19)
- AL-324 MUIO pin-mux (`delroth-alpine_hal/drivers/pbs/al_hal_muio_mux_map.h`, enum
  `include/pbs/al_hal_muio_mux.h`) has exactly one i2c entry: `AL_MUIO_MUX_IF_I2C_GEN` = I2C_B
  (i2c_gen 0xfd894000) on pins 30/31. **No I2C_A/pld entry** anywhere; pld SCL/SDA (0xfd880000) are
  dedicated pads. Cross-checked vs the live 48-ball mux read (docs/gpio-map.md). This only closes
  the (wrong-frame) bit-bang path; it says nothing about the chip being faulty.

## Current mitigation (until the code bug is fixed)
- `i2c-sda-hold-time-ns = <300>` restored on both `i2c_pld` nodes (helped, not sufficient).
- Idle mux deselect (U-Boot mux uclass writes 0x00 after each access; Linux
  `i2c-mux-idle-disconnect`) — reduces main-bus exposure.
- Linux `rtc@30 status=disabled`, and the al_eth SFP-EEPROM poll (0x50 on ch0, every ~2 s) is the
  last ch0 toucher / console-flood source — configure eth2 fixed 10gbase-r with no EEPROM probe.
- These keep the bus usable; they do NOT fix the underlying code bug.

## Open questions
- Which of our code paths (SCL timing / mux sequence / RTC access) aborts the s35390a mid-transfer?
- Does setting stock's exact raw SCL h/l counts on `i2c_pld` stop the wedge? (First thing to test.)
- Diff our i2c/RTC path against stock Linux 5.1's working path end-to-end.
</content>
