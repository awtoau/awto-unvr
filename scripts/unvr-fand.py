#!/usr/bin/env python3
"""Hysteresis fan daemon for the UNVR (woomera). Quiet by default, MAX when hot.

The hot components are the spinning drives (~50 C), not the board (~36 C), so the
ADT7475's own sensors are the wrong input. This polls the real temps — SoC die
(via /dev/mem, until al_thermal is wired, #44) + each drive (SMART) — and drives
the ADT7475 in MANUAL mode with bang-bang hysteresis:

    hottest >= HIGH  -> fans MAX  (255)
    hottest <= LOW   -> fans FLOOR (quiet)
    in between       -> hold (hysteresis, no oscillation)

Defaults: HIGH=60 C, LOW=50 C, FLOOR=60/255 (~24%). Tune via the constants.
"""
import glob, mmap, struct, subprocess, sys, time
from pathlib import Path

HIGH_C, LOW_C = 60, 50
FLOOR_PWM, MAX_PWM = 80, 255   # 80/255 ~31% quiet floor (drives rest happy here)
POLL_S = 15

# SoC die sensor (Alpine V2 thermal unit) via /dev/mem
SOC_BASE, SOC_STATUS_OFF = 0xfd860000, 0x0a0c
SOC_OFFSET, SOC_MULT = 1090, 3520


def soc_temp():
    try:
        with open("/dev/mem", "rb", 0) as f:
            m = mmap.mmap(f.fileno(), 4096, offset=SOC_BASE, prot=mmap.PROT_READ)
            s = struct.unpack("<I", m[SOC_STATUS_OFF:SOC_STATUS_OFF + 4])[0]
        raw = s & 0xfff
        return ((raw * SOC_MULT) // 4096 - SOC_OFFSET) // 10
    except OSError:
        return None


def disk_temps():
    temps = []
    for dev in sorted(glob.glob("/dev/sd?")):
        r = subprocess.run(["smartctl", "-A", dev], capture_output=True, text=True)
        for line in r.stdout.splitlines():
            if "Temperature_Cel" in line or "Airflow_Temperature" in line:
                try:
                    temps.append((dev, int(line.split()[9])))
                except (IndexError, ValueError):
                    pass
                break
    return temps


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
        disks = disk_temps()
        hottest = max([t for t in ([soc] if soc else []) + [d[1] for d in disks]] or [0])
        if hottest >= HIGH_C:
            state = "MAX"
        elif hottest <= LOW_C:
            state = "FLOOR"
        # else: hold previous state (hysteresis)
        set_pwm(h, MAX_PWM if state == "MAX" else FLOOR_PWM)
        print(f"hottest={hottest}C soc={soc} disks={disks} -> {state}", flush=True)
        time.sleep(POLL_S)


if __name__ == "__main__":
    sys.exit(main())
