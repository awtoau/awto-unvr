# RTC s35390a / ch0 i2c wedge — a CODE fault (FIXED for targeted access), NOT hardware

**FIXED (2026-08-19, on-box):** pinning stock's raw SCL hcnt/lcnt counts on the pld i2c
node makes `i2c md 0x30 0 1` read the s35390a STATUS1 cleanly (0x00) where it previously
returned -121 / wedged the bus. Commit "i2c: honor stock's raw SCL hcnt/lcnt". The fix is
CODE (our DW i2c driver computed too-tight SCL edges); the chip and cell were always good.
Remaining: a full `i2c probe` (all addresses) still wedges — targeted RTC access works.

> ## ⚠️ RECOVERY IS A COLD POWER CYCLE. A REBOOT DOES NOT WORK.
>
> Measured 2026-09-04: after `systemctl reboot` from a wedged state the box came back
> with **123 `controller timed out` inside 2 minutes of uptime** — still wedged on a
> fresh boot. After a power cycle: 0 timeouts. A warm reset never removes power from
> the s35390a, so it comes back still holding SDA, and nothing in the SoC reset path
> clocks it through.
>
> **It is not cosmetic — it drops the 10G link.** The SFP driver's i2c reads fail long
> enough that it concludes the module was pulled:
> ```
> sfp sfp: failed to read SFP soft status: -ETIMEDOUT
> sfp sfp: module removed
> al_eth_10g 0000:00:02.0 enp0s2: Link is Down
> ```
>
> **What wedges it:**
> - `i2cdetect` / any full bus scan that probes empty addresses on ch0
> - an SMBus-style access to 0x30. `i2cget -y <bus> 0x30` issues a READ_BYTE and is
>   NOT what the driver does — `rtc-s35390a.c:87` builds a raw `i2c_msg` with
>   `I2C_M_RD`. Mainline itself excludes 0x30-0x37 from SMBus Quick probing
>   (`i2c-core-base.c:2462`) because chips there do not tolerate it.
>   The driver-shaped test is `i2ctransfer -y <bus> r1@0x30`.
>
> **`i2cdetect` exiting 0 proves nothing** — it only means the scan finished. The wedge
> surfaces on the NEXT access. Any probe must re-read the bus afterwards to know.

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
- ch0 device: **s35390a RTC @0x30-0x37** (its own multi-address quirk) — SCL is never held
  (S-35390A SCL is input-only); only SDA is driven low. The SFP EEPROM @0x50 is on ch1, a
  separate bus, uninvolved in this wedge.
- ch3 (**adt7475** fan ctrl) reads cleanly, NAKs a nonexistent address cleanly, bus survives — so
  the mux, the DW controller, and NAK handling are fine in general. **Only our ch0 access wedges.**
- Datasheet Fig 46 ("Reset After Communication Interruption"): if a transfer to the s35390a is
  aborted mid-frame, the chip holds SDA low until clocked through. So **our access is aborting the
  s35390a mid-transfer** — the question is which part of our code does that.

## Suspected code bug (to pin — this is the open work)
Stock Linux 5.1 worked; ours wedges. Candidates, in order:
1. **i2c SCL timing not matching stock's proven values (leading candidate).** Stock's DTB carries
   explicit raw counts AND sda-hold (live.dts:249-255):
   `ss hcnt=0x855 lcnt=0xb0b, fs hcnt=0x19d lcnt=0x320, hs hcnt=0xf3 lcnt=0x198, sda-hold=0x12c`.
   We restored ONLY `sda-hold-time-ns=300` and let the DW driver *compute* the SCL h/l counts.
   - Verified in the mainline driver (drivers/i2c/designware_i2c.c): the OF-bound
     `snps,designware-i2c` path ALWAYS computes via `dw_i2c_calc_timing()` — `priv->scl_sda_cfg`
     (the explicit-count override at set_bus_speed:254-264) is wired ONLY for the Intel Baytrail
     PCI variant (designware_i2c_pci.c:70 byt_config). It does NOT read `i2c-*-scl-*cnt-raw` from DT.
   - `dw_i2c_calc_timing()` back-solves for the MINIMUM legal SCL high/low; stock's raw counts are
     LARGER (ss: 0x855/0xb0b ~= 4.27/5.65 us at the 500 MHz ic_clk vs the ~4.0/4.7 us minimum). So
     stock ran deliberately more conservative SCL timing than mainline computes — plausibly what the
     fussy s35390a needs. 100 kHz alone did NOT fix it, consistent with edge-COUNTS (not speed).
   - **Fix path (needs a small driver change, then DTS):** patch designware_i2c.c to read
     `i2c-{ss,fs,hs}-scl-{h,l}cnt-raw` + `i2c-sda-hold-time-ns` into a `dw_scl_sda_cfg` and set
     `priv->scl_sda_cfg` when present (mirror byt_config); then add stock's raw counts to `i2c_pld`
     in both DTS.
   - **Mainline Linux's i2c-designware (#98, not U-Boot's) — same gap, different fix.** Mainline's
     `drivers/i2c/busses/i2c-designware-common.c` ignores the raw-count properties entirely (they're
     non-standard vendor props, ACPI-only in upstream); it always computes hcnt/lcnt itself from
     `i2c_dw_scl_hcnt()`/`i2c_dw_scl_lcnt()`, which only take the standard `i2c-sda-fall-time-ns` (feeds
     hcnt) / `i2c-scl-falling-time-ns` (feeds lcnt) DT properties — patching the driver isn't needed,
     tuning these is enough. Derived from the actual formulas (not guessed):
     `hcnt = round(ic_clk_kHz × (tHIGH + sda_fall_ns) / 1e6) − 3`,
     `lcnt = round(ic_clk_kHz × (tLOW + scl_fall_ns) / 1e6) − 1`, with `ic_clk` = 500000 (kHz, this
     board's sbclk), `tHIGH` = 4000ns, `tLOW` = 4700ns (I2C spec minimums, hardcoded in the driver, not
     DT-tunable). With the driver's own 300ns default for both: hcnt = 2147 (already **above** stock's
     0x855=2133 — fine as-is, `i2c-sda-fall-time-ns` left untouched). lcnt = 2499, **short** of stock's
     0xb0b=2827 by 328 — this was the actual gap. Setting `i2c-scl-falling-time-ns = <1000>` (up from
     the 300ns default) computes lcnt = 2849, comfortably above stock's value. Applied to the Linux
     DTS only (`dts/alpine-v2-ubnt-unvr-ea16.dts`) — U-Boot's patched driver keeps using its own raw
     counts, unaffected by this property.
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
- Linux `rtc@30 status=disabled` is the last ch0 toucher / console-flood source. The al_eth
  SFP-EEPROM poll (0x50) is on ch1, uninvolved in this wedge.
- These keep the bus usable; they do NOT fix the underlying code bug.

## Open questions
- Which of our code paths (SCL timing / mux sequence / RTC access) aborts the s35390a mid-transfer?
- Does setting stock's exact raw SCL h/l counts on `i2c_pld` stop the wedge? (First thing to test.)
- Diff our i2c/RTC path against stock Linux 5.1's working path end-to-end.
</content>
