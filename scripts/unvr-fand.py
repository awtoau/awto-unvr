#!/usr/bin/env python3
"""UNVR (woomera) fan daemon: ADT7475 fans OFF unless the SoC is hot.

- Policy (Dan, 2026-11-17): all fans off; full speed at SoC >= SOC_ON_C, off again at <= SOC_OFF_C.
- Drive temps: kernel drivetemp hwmon, smartctl fallback (#373). Control: SoC >= 80 C or any drive >= 60 C (Dan 2026-11-18); board temps are logged only.
- Fail SAFE = 255 on an unreadable SoC temp / write failure / SIGTERM.
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

OFF_PWM, MAX_PWM = 0, 255
SOC_ON_C, SOC_OFF_C = 80, 50  # stock-era thresholds (7674ee4); hysteresis stops hunting
DRIVE_ON_C, DRIVE_OFF_C = (
    60,
    60,
)  # Dan 2026-11-18: any drive >= 60 C on, < 60 C off (no hysteresis)

# 10 s: SoC thermal mass is seconds-minutes; drive reads are cheap
POLL_S = 10
# smartctl fallback (no drivetemp hwmon) normally returns in <1 s; 8 s under heavy I/O (log only, not control)
SMART_TIMEOUT_S = 8
TACH_GRACE_POLLS = 3  # 30 s for spin-up before a 0 RPM fan is reported
TACH_FANS = (1, 2, 3)  # fan4 is not populated on this board


def target_pwm(soc_c, on, drives=()):
    """-> (pwm, on). soc_c None (unreadable) -> full speed.

    Also on if any drive >= DRIVE_ON_C; off again below DRIVE_OFF_C (None/failed reads ignored).
    """
    hot = [t for t in drives if t is not None]
    drive_hot = bool(hot) and max(hot) >= DRIVE_ON_C
    drive_warm = bool(hot) and max(hot) >= DRIVE_OFF_C
    if soc_c is None or soc_c >= SOC_ON_C or drive_hot:
        return MAX_PWM, True
    if on and (soc_c > SOC_OFF_C or drive_warm):
        return MAX_PWM, True
    return OFF_PWM, False


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


def drivetemp_temps(root="/sys/class/hwmon"):
    """C per internal drive from kernel drivetemp hwmon; [] if none (-> smartctl)."""
    out = []
    for h in sorted(glob.glob(f"{root}/hwmon*")):
        try:
            if Path(h, "name").read_text().strip() != "drivetemp":
                continue
            if "/usb" in str(Path(h, "device").resolve()):
                continue
        except OSError:
            continue
        v = read_int(Path(h, "temp1_input"))
        out.append(None if v is None else v / 1000)
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
    on, stalled = True, {}
    while True:
        disks = sata_disks()
        readings = {
            "drive": drivetemp_temps() or [drive_temp(d) for d in disks] or [None],
            "soc": [soc_temp()],
            "board": board_temps(h),
        }
        new, on = target_pwm(readings["soc"][0], on, readings["drive"])
        tgt = new
        if not set_pwm(h, new):
            new = MAX_PWM
            set_pwm(h, new)
        for n in TACH_FANS:
            rpm = read_int(h / f"fan{n}_input")
            stalled[n] = stalled.get(n, 0) + 1 if not rpm else 0
            if new > OFF_PWM and stalled[n] == TACH_GRACE_POLLS:
                log(f"ERROR fan{n} reads {rpm} RPM at pwm {new}: stalled/unplugged?")
        rpms = [read_int(h / f"fan{n}_input") for n in (1, 2, 3, 4)]
        log(f"pwm={new} target={tgt} {readings} rpm={rpms}")
        time.sleep(POLL_S)


if __name__ == "__main__":
    sys.exit(main())
