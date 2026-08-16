#!/usr/bin/env python3
"""Hysteresis fan daemon for the UNVR (woomera). Quiet by default, MAX when hot.

Driven by the **CPU/SoC die temp** — the al_thermal `cpu-thermal` zone
(`/sys/class/thermal`), falling back to /dev/mem only if the zone is absent.
Disk temps are deliberately ignored — the WD drives
sit at ~50 C, which is normal and well within rating, not a fan trigger. The
ADT7475 is driven in MANUAL mode with bang-bang hysteresis on SoC temp:

    soc >= HIGH  -> fans MAX  (255)
    soc <= LOW   -> fans FLOOR (quiet)
    in between   -> hold (hysteresis, no oscillation)

Defaults: HIGH=60 C, LOW=50 C, FLOOR=80/255 (~31%). Tune via the constants.
"""
import glob, mmap, struct, sys, time
from pathlib import Path

HIGH_C, LOW_C = 60, 50
FLOOR_PWM, MAX_PWM = 80, 255   # 80/255 ~31% quiet floor (drives rest happy here)
POLL_S = 15

# SoC die sensor (Alpine V2 thermal unit) via /dev/mem
SOC_BASE, SOC_STATUS_OFF = 0xfd860000, 0x0a0c
SOC_OFFSET, SOC_MULT = 1090, 3520


def soc_temp():
    # Preferred: the al_thermal cpu-thermal zone (standard kernel interface).
    for z in glob.glob("/sys/class/thermal/thermal_zone*"):
        try:
            if Path(z, "type").read_text().strip() == "cpu-thermal":
                return int(Path(z, "temp").read_text()) // 1000
        except OSError:
            pass
    # Fallback: raw /dev/mem read (if al_thermal isn't loaded).
    try:
        with open("/dev/mem", "rb", 0) as f:
            m = mmap.mmap(f.fileno(), 4096, offset=SOC_BASE, prot=mmap.PROT_READ)
            s = struct.unpack("<I", m[SOC_STATUS_OFF:SOC_STATUS_OFF + 4])[0]
        raw = s & 0xfff
        return ((raw * SOC_MULT) // 4096 - SOC_OFFSET) // 10
    except OSError:
        return None


def adt7475():
    for h in glob.glob("/sys/class/hwmon/hwmon*"):
        try:
            if Path(h, "name").read_text().strip() == "adt7475":
                return Path(h)
        except OSError:
            pass
    return None


def set_pwm(h, val):
    for p in (1, 2, 3):
        try:
            (h / f"pwm{p}_enable").write_text("1")   # manual
            (h / f"pwm{p}").write_text(str(val))
        except OSError as e:
            print(f"pwm{p} write failed: {e}", file=sys.stderr)


def main():
    h = adt7475()
    if h is None:
        print("adt7475 not found", file=sys.stderr)
        return 1
    state = "FLOOR"        # start quiet; first loop escalates if already hot
    set_pwm(h, FLOOR_PWM)
    while True:
        soc = soc_temp()
        # If the SoC read ever fails, fail SAFE to MAX rather than risk no cooling.
        if soc is None:
            state = "MAX"
        elif soc >= HIGH_C:
            state = "MAX"
        elif soc <= LOW_C:
            state = "FLOOR"
        # else: hold previous state (hysteresis)
        set_pwm(h, MAX_PWM if state == "MAX" else FLOOR_PWM)
        print(f"soc={soc}C -> {state}", flush=True)
        time.sleep(POLL_S)


if __name__ == "__main__":
    sys.exit(main())
