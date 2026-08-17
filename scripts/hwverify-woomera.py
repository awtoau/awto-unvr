#!/usr/bin/env python3
"""Userspace hardware inventory for woomera — read-only, via the serial console.

Reports RAM + every bound device through its proper Linux subsystem (no register
pokes): meminfo/iomem, loaded drivers, hwmon/rtc/leds/watchdog/nvmem sysfs, the
i2c bus map + per-bus scan, tty list, and the live DT model. Run it before and
after deploying the new DTB to see the new devices (eeprom@57 via at24, i2c_gen
bus, ttyS2) appear. Output -> tmp/logs/hwverify-woomera.log.
"""
from __future__ import annotations
import sys
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _console as con  # noqa: E402
from _repo import LOGS  # noqa: E402

LOG = LOGS / "hwverify-woomera.log"


def log(m):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    LOG.open("a").write(line + "\n")


# label -> shell command. All read-only.
CHECKS = [
    ("kernel",        "uname -r; cat /proc/device-tree/model 2>/dev/null; echo"),
    ("RAM total",     "grep MemTotal /proc/meminfo"),
    ("RAM banks",     "grep -i 'System RAM' /proc/iomem"),
    ("DDR dmesg",     "dmesg | grep -iE 'ddr|memory:|Machine model' | head -20"),
    ("drivers",       "lsmod | grep -iE 'at24|ina2xx|pmbus|adt7475|pca953x|s35390|i2c_desig|sp805|nvmem' || echo '(none of the target modules loaded)'"),
    ("i2c buses",     "i2cdetect -l 2>/dev/null || echo 'i2c-tools missing'"),
    ("i2c scan",      "for n in $(i2cdetect -l 2>/dev/null | sed -n 's/^i2c-\\([0-9]*\\).*/\\1/p'); do echo \"== i2c-$n ==\"; i2cdetect -y -r $n 2>/dev/null; done"),
    ("hwmon",         "for h in /sys/class/hwmon/hwmon*; do echo -n \"$h: \"; cat $h/name 2>/dev/null; done"),
    ("rtc",           "ls /sys/class/rtc 2>/dev/null; hwclock -r 2>/dev/null || echo '(no rtc read)'"),
    ("leds",          "ls /sys/class/leds 2>/dev/null"),
    ("watchdog",      "ls /sys/class/watchdog 2>/dev/null; cat /sys/class/watchdog/watchdog0/identity 2>/dev/null"),
    ("nvmem",         "ls -l /sys/bus/nvmem/devices 2>/dev/null; ls /sys/bus/i2c/devices/*/eeprom 2>/dev/null || echo '(no at24 eeprom sysfs — DTB lacks eeprom@57 or at24 not loaded)'"),
    ("gpio",          "ls /sys/bus/i2c/drivers/pca953x 2>/dev/null; gpioinfo 2>/dev/null | grep -iE 'chip|pca' | head"),
    ("ttyS",          "ls /dev/ttyS* 2>/dev/null"),
    ("i2c devices",   "ls /sys/bus/i2c/devices 2>/dev/null"),
]


def main():
    try:
        s = con.connect()
    except FileNotFoundError as e:
        sys.exit(str(e))
    con.login(s)
    log("shell ready — running read-only hardware inventory")
    for label, cmd in CHECKS:
        rc, out = con.sh(s, cmd, timeout=60)
        log(f"\n===== {label} (rc={rc}) =====\n{out}")
    log("\nDONE — full inventory in tmp/logs/hwverify-woomera.log")
    s.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
