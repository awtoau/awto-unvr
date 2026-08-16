#!/usr/bin/env python3
"""Configure the UNVR's ADT7475 for hardware automatic fan control.

Runs ON the device (woomera) at boot via a systemd oneshot. The ADT7475 fan
controller powers up in manual/fixed mode; this puts it in hardware AUTOMATIC
mode with a temp-curve so fans ramp with temperature and the chip enforces it
even if userspace dies.

Interim curve: driven by temp3 (the board sensor the vendor curve targets).
temp3 is ambient-ish, so this is not SoC-load-aware - the proper fix is the
al_thermal SoC-die sensor (issue #44). Chosen to keep fans ALWAYS ON (Tmin below
the ~36 C resting temp, so they never cut out) and ramp to full by 55 C; the
chip's crit (100 C) is the hard backstop.

  Tmin=30 C -> pwm floor 90 (~35%) ; Tmax=55 C -> pwm 255 (full)
"""
import glob
import subprocess
import sys
import time
from pathlib import Path

TMIN_MC = 30000     # below this, fans hold the floor (never off)
TMAX_MC = 55000     # at/above this, fans full
FLOOR_PWM = 90      # ~35% duty floor
TEMP_CH = 4         # bitmask: bit2 = temp3 drives the fan


def find_adt7475(retries=20):
    subprocess.run(["modprobe", "adt7475"], check=False)
    for _ in range(retries):
        for h in glob.glob("/sys/class/hwmon/hwmon*"):
            try:
                if Path(h, "name").read_text().strip() == "adt7475":
                    return Path(h)
            except OSError:
                pass
        time.sleep(0.5)  # bounded wait for the i2c/DT probe to register hwmon
    return None


def w(p: Path, v):
    try:
        p.write_text(str(v))
    except OSError as e:
        print(f"  write {p.name} failed: {e}", file=sys.stderr)


def main() -> int:
    h = find_adt7475()
    if h is None:
        print("adt7475 hwmon not found", file=sys.stderr)
        return 1
    w(h / "temp3_auto_point1_temp", TMIN_MC)
    w(h / "temp3_auto_point2_temp", TMAX_MC)
    for p in (1, 2, 3):
        w(h / f"pwm{p}_auto_point1_pwm", FLOOR_PWM)
        w(h / f"pwm{p}_auto_channels_temp", TEMP_CH)
        w(h / f"pwm{p}_enable", 2)          # 2 = hardware automatic
    print(f"adt7475 ({h.name}): auto mode, temp3 curve {TMIN_MC//1000}-{TMAX_MC//1000}C, "
          f"floor pwm {FLOOR_PWM}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
