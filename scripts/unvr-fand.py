#!/usr/bin/env python3
"""UNVR (woomera) fan daemon: ADT7475 PWM driven by the HOTTEST of drives / SoC / board.

- Never 0 PWM: floor FLOOR_PWM (stock curve floor, scripts/unvr-fancontrol.py, ~35 %).
- Per-source linear ramp floor -> 255; fan = max over sources (curve()).
- Ramp up at once, ramp down slowly with a deadband (next_pwm()).
- Fail SAFE = 255 on any unreadable source / write failure / SIGTERM.
- Tach is only logged (fan4 is not populated); control never depends on it.
- Chip fallback: pwm_use_point2_pwm_at_crit=1 forces full speed at temp*_crit with no daemon.
Decisions: #361 (drives), #44 (SoC), #373 (fans-off regression).
"""

import glob
import json
import signal
import subprocess
import sys
import time
from pathlib import Path

FLOOR_PWM, MAX_PWM = 90, 255

# source -> (temp at floor, temp at 255) in C
CURVES = {
    # WD Purple limit 65 C (datasheet); full speed 10 C below it
    "drive": (40, 55),
    # die idles ~60 C under load; Alpine V2 has no published Tj below 100 C
    "soc": (60, 85),
    # ADT7475 diodes ~45 C with fans on; stock curve used temp3 30..55 C
    "board": (40, 60),
}

# 10 s: HDD thermal mass is minutes; 1 smartctl per disk per poll is cheap
POLL_S = 10
# smartctl normally returns in <1 s; 8 s under heavy I/O, then fail safe
SMART_TIMEOUT_S = 8
DOWN_STEP, DOWN_DEADBAND = 8, 8  # counts/poll; ~3 %: no hunting around a threshold
TACH_GRACE_POLLS = 3  # 30 s for spin-up before a 0 RPM fan is reported
TACH_FANS = (1, 2, 3)  # fan4 is not populated on this board


def curve(src, temp_c):
    lo, hi = CURVES[src]
    if temp_c <= lo:
        return FLOOR_PWM
    if temp_c >= hi:
        return MAX_PWM
    return round(FLOOR_PWM + (MAX_PWM - FLOOR_PWM) * (temp_c - lo) / (hi - lo))


def target_pwm(readings):
    """readings: {src: [temp_c | None, ...]}; None (unreadable) -> MAX_PWM."""
    out = FLOOR_PWM
    for src, temps in readings.items():
        for t in temps:
            out = max(out, MAX_PWM if t is None else curve(src, t))
    return out


def next_pwm(cur, target):
    if cur is None or target >= cur:
        return target
    if cur - target < DOWN_DEADBAND:
        return cur
    return max(target, cur - DOWN_STEP)


def log(msg):
    print(msg, flush=True)


def read_int(p):
    try:
        return int(Path(p).read_text())
    except (OSError, ValueError):
        return None


def hwmon(name):
    for h in glob.glob("/sys/class/hwmon/hwmon*"):
        try:
            if Path(h, "name").read_text().strip() == name:
                return Path(h)
        except OSError:
            pass
    return None


def soc_temp():
    for z in glob.glob("/sys/class/thermal/thermal_zone*"):
        try:
            if Path(z, "type").read_text().strip() == "cpu-thermal":
                v = read_int(Path(z, "temp"))
                return None if v is None else v / 1000
        except OSError:
            pass
    return None


def board_temps(h):
    out = []
    for n in (1, 2, 3):
        v = read_int(h / f"temp{n}_input")
        out.append(None if v is None else v / 1000)
    return out


def sata_disks():
    """Internal (non-USB, non-removable) sdX; USB sticks are not heat sources."""
    out = []
    for b in sorted(glob.glob("/sys/block/sd*")):
        if "/usb" in str(Path(b).resolve()) or read_int(f"{b}/removable"):
            continue
        out.append("/dev/" + Path(b).name)
    return out


def drive_temp(dev):
    """C, or None on failure. A drive in standby is cold: floor (-inf -> 0)."""
    try:
        r = subprocess.run(
            ["smartctl", "-n", "standby", "-j", "-A", dev],
            capture_output=True,
            text=True,
            timeout=SMART_TIMEOUT_S,
        )
    except subprocess.TimeoutExpired:
        log(f"ERROR smartctl {dev} exceeded {SMART_TIMEOUT_S}s: fail safe")
        return None
    try:
        j = json.loads(r.stdout)
    except ValueError:
        log(f"ERROR smartctl {dev}: no JSON (rc={r.returncode}): fail safe")
        return None
    if j.get("power_mode") == "standby" or j["smartctl"]["exit_status"] == 2:
        return 0
    cur = j.get("temperature", {}).get("current")
    if cur is None:
        log(f"ERROR smartctl {dev}: no temperature: fail safe")
    return cur


def set_pwm(h, val):
    ok = True
    for p in (1, 2, 3):
        try:
            (h / f"pwm{p}_enable").write_text("1")
            (h / f"pwm{p}").write_text(str(val))
        except OSError as e:
            log(f"ERROR pwm{p} write {val} failed: {e}")
            ok = False
    return ok


def main():
    h = hwmon("adt7475")
    if h is None:
        log("ERROR adt7475 hwmon not found")
        return 1
    try:  # chip forces full speed at temp*_crit on its own
        (h / "pwm_use_point2_pwm_at_crit").write_text("1")
    except OSError as e:
        log(f"WARN pwm_use_point2_pwm_at_crit: {e}")

    def bye(*_):
        set_pwm(h, MAX_PWM)  # unmanaged fans run full, never off
        sys.exit(0)

    signal.signal(signal.SIGTERM, bye)
    signal.signal(signal.SIGINT, bye)
    set_pwm(h, MAX_PWM)  # until the first complete reading
    cur, stalled = MAX_PWM, {}
    while True:
        disks = sata_disks()
        readings = {
            "drive": [drive_temp(d) for d in disks] or [None],
            "soc": [soc_temp()],
            "board": board_temps(h),
        }
        tgt = target_pwm(readings)
        new = next_pwm(cur, tgt)
        if not set_pwm(h, new):
            new = MAX_PWM
            set_pwm(h, new)
        for n in TACH_FANS:
            rpm = read_int(h / f"fan{n}_input")
            stalled[n] = stalled.get(n, 0) + 1 if not rpm else 0
            if new >= FLOOR_PWM and stalled[n] == TACH_GRACE_POLLS:
                log(f"ERROR fan{n} reads {rpm} RPM at pwm {new}: stalled/unplugged?")
        rpms = [read_int(h / f"fan{n}_input") for n in (1, 2, 3, 4)]
        log(f"pwm={new} target={tgt} {readings} rpm={rpms}")
        cur = new
        time.sleep(POLL_S)


if __name__ == "__main__":
    sys.exit(main())
