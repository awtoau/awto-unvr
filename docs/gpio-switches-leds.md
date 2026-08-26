# GPIO / switches / LEDs / RPS map

Reverse-engineered from the Ubiquiti GPL U-Boot + kernel (4.1.37), the live stock
DTB (`hw-reference/20260816-104601/`), the extracted firmware userland
(`UNVR-5.1.25.bin` rootfs), and board photos (`photos/`). 2026-08-17.

GPIO bases (from `live.dts` baseidx): gpio0=0, gpio1=8, gpio2=16, gpio3=24,
gpio4=32, gpio5=40, sgpo=48 (64 lines), pca9575@0x29=464, @0x21=480, @0x20=496.

## SW1 / SW2 — the mystery tactile switches (dormant)

- **Physical:** two momentary tactiles by the RTC (`photos/20260816_231647.jpg`),
  silk `SW2` (left) / `SW1` (right), next to the S-35390A (U5050) + coin cell
  (`RTC BATT1`, MS621), between C135/C136. No function silkscreen.
- **GPIO: UNKNOWN — correction.** gpio 33/34 were earlier guessed to be SW1/SW2 (the
  two orphan `sysfs`-owned inputs in gpio.txt), but the `rpsd` binary reverse proves
  **gpio 33 = `rps_prnt` (RPS present)** and **gpio 34 = `12v_lp` (12 V load sense)** —
  those are the RPS sense inputs (see [rps-subsystem.md](rps-subsystem.md)), exported by
  rpsd, not the buttons.
- **2026-08-27, owner-confirmed against the physical board — mystery resolved:**
  - **SW1 = wired in parallel with the external front-panel reset button** —
    same net as `gpio-keys`'s `KEY_RESTART` on gpio4.6 (pin 38;
    [dts/alpine-v2-ubnt-unvr-ea16.dts:849-858](../dts/alpine-v2-ubnt-unvr-ea16.dts#L849-L858),
    the only input that line can produce). This is the one exercised live:
    `gpio-top.py`'s own capture showed pin 38 toggle ACTIVE then release, and
    the *very next* console line was `stage2_loader v2.22.3` — boot ROM
    restarting with **no OS shutdown sequence at all** in between. So the
    gpio4.6 net isn't only a software `KEY_RESTART`/logind reboot — it drives
    a real hardware POR, which is why disabling `HandleRebootKey=` in logind
    (see reset-button section below) did NOT stop a reset via this switch:
    nothing for logind to react to, Linux never got a chance to run any
    shutdown code.
  - **SW2 = a separate, direct hardware reset** — distinct from the gpio4.6
    net (not just a duplicate access point for the same signal). Not yet
    probed for exactly what it drives (SoC reset pin vs. a supervisor IC);
    treat as an unconditional hard reset with no software visibility at all
    until proven otherwise.
- **Function:** no boot-mode/strap read in U-Boot or preboot. Both switches are
  reset paths, not factory-test/debug inputs as previously guessed. NOT
  boot-select/recovery. **Pressing either will reset the box — no software
  mitigation exists for either one.**

## Reset button — press-duration semantics

- **HW:** front button = `gpio-keys` → **GPIO 38 (gpio4.6)**, active-low, IRQ,
  `KEY_RESTART` (408).
- **Handler:** `sbin/infctld` (→ `ubnt-tools`), reads `/dev/input/event*`:
  - **short press → reboot** (only if a config flag is set)
  - **hold ~5 s → 119 s → factory reset** (`(tv_sec−5) ≤ 114` → `/sbin/reset2defaults`;
    119 s upper bound guards a stuck button). `reset2defaults` writes
    `/boot/reset2defaults` + `/persistent/system/reset_reason`, runs reset hooks.
  - re-entry guarded ("RESET in progress, ignoring..").
- The **~10 s-at-power-on recovery is a separate U-Boot function**.
- Disable: `ubnt-systool` / `infctld -n`. (Vendor stock firmware only — our
  Fedora build has no `infctld`; short-press reboot on our build was instead
  `systemd-logind`'s own default `HandleRebootKey=`, now disabled via
  `/etc/systemd/logind.conf.d/99-awto-no-reboot-key.conf`, see
  `scripts/build-fedora-rootfs.py`.)
- **2026-08-27: the button/net is a genuine hardware POR, not only a software
  key event** — disabling logind's handling did not stop a reboot triggered
  via this net (see SW1/SW2 finding above: boot ROM restarted with zero OS
  shutdown sequence in between). No software-only "unwiring" is possible.

## RPS connector (PSE) — summary

Populated **PSE** power port on this 4-bay board (owner-confirmed, **not** Pro-only):
redundant **54 V + 12 V** ORed with the main supply. On ea16, `rpsd` uses only
**`/dev/ttyS2`** (RS-232 via MAX3221 `U122`) + two SoC-GPIO sense inputs — **gpio 33 =
`rps_prnt`**, **gpio 34 = `12v_lp`** (these are the two `sysfs` inputs once mis-assigned
to SW1/SW2). The rear `RPS POWER ON` LED is a hardwired rail indicator, not GPIO.

**Full mechanism, protocol, pin map, connector, and remote-reset/poweroff repurposing
→ [rps-subsystem.md](rps-subsystem.md).**

## Full GPIO / LED / expander map

**SoC PL061 (all 48 lines, mux registers, spare-pin candidates):** full per-ball
table → [gpio-map.md](gpio-map.md). Functional notes for the lines this doc covers:
pin 0 = SFP+ speed LED; pin 31 = `ulogo_blue`; pin 33/34 = RPS sense (`rps_prnt`/
`12v_lp`, NOT SW1/SW2 — see above); pin 37 = `ulogo_white`; pin 38 = reset button
(`KEY_RESTART`, IRQ — semantics below); pin 42 = HDD `force-power-on-wa`.

Reset button short-press is already fully wired end-to-end with **no custom code**:
`gpio-keys` DTS node → `KEY_RESTART` evdev → `systemd-logind`'s own built-in
`HandleRebootKey=` default handling triggers a real reboot. Confirmed live via
`journalctl`: `Reboot key pressed short. / Rebooting... / System is rebooting.`
No userspace daemon (ours or vendor's `infctld`) is involved.

**SGPO** (base 48 → external 74VHC595 `UB20`): lines 16–23 (global 64–71) = SATA
activity/presence LEDs, 8 ports (host0/host1 × 0–3); ea16 maps the 4 populated
(sgpo 16/18/20/22).

**PCA9575 expanders (I2C `i2c-pld` @fd880000):**
| Chip | Base | Function |
|---|---|---|
| @0x20 | 496 | pin2 (498)=SFP+ **1G link LED** (`sfp_1g`); inputs pins 3,7,9,10,12,14,15 (mask 0xd688); userland exports 12/14/15 (508/510/511)=board straps/status/present-sense |
| @0x21 | 480 | pins0–3 (480–483)=**bay1–4 pwr-enable** (hogged high); 4–7 (484–487)=**bay present** (active-low); 12–15 (492–495)=**bay fault LEDs** |
| @0x29 | 464 | Pro **bays5–8**: pwren 464–467, present 468–471, fault 476–479 — **unpopulated on the 4-bay** (probe EREMOTEIO) |

**I2C mux `pca9546@0x71`:** ch0 = RTC **S-35390A@0x30** (U5050); ch3 = **ADT7475@0x2e**.
**Fan:** ADT7475 PWM over I2C (not GPIO); userland `ustd` PID (`fan_ctrl`); U-Boot
`slowfan` env sets a slow default.

**Corrections to `hardware.md`:** `gpio 37` = `ulogo_white` (was "unknown"); the
`pwren 464 / present 468 / fault 476` quoted for HDD control are the **@0x29 (Pro)**
expander — the **populated** bay control is **@0x21 (480/484/492)**.
