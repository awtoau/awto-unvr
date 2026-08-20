#!/usr/bin/env python3
"""Sweep an extracted UNVR rootfs for userland hardware access.

Scans every ELF and script under bin/sbin/usr/lib for device-access
patterns (serial, i2c, gpio, mmio, watchdog, mtd, hwmon, LED, input,
ethtool ioctls, raw /dev nodes) and prints matches per file.

Usage: fw-hw-sweep.py <rootfs-dir>
Output: stdout (redirect to tmp/logs/fw-hw-sweep.raw).
"""

import os
import re
import subprocess
import sys

# pattern label -> regex over `strings` output of each file
PATTERNS = {
    "serial": r"/dev/tty(S|RPS|AMA|HS|USB)|hciattach|ttyS%d|B115200|termios",
    "i2c": r"/dev/i2c-|i2c_smbus|I2C_SLAVE|/sys/bus/i2c|i2c-%d|smbus",
    "gpio": r"/sys/class/gpio|gpiochip|/dev/gpiochip|gpio%d|GPIO",
    "mmio": r"/dev/mem|devmem|mmap.*0xf[cd]|iopl|/dev/kmem",
    "watchdog": r"/dev/watchdog|watchdog_ioctl|WDIOC",
    "mtd": r"/dev/mtd|MEMERASE|mtd%d|MEMGETINFO",
    "hwmon": r"/sys/class/hwmon|/sys/devices/platform/.*hwmon|pwm\d|fan\d_input|temp\d_input",
    "led": r"/sys/class/leds|brightness|led%d|/sys/devices/platform/.*led",
    "input": r"/dev/input/event|EVIOC|KEY_",
    "ethtool_ioctl": r"ETHTOOL_G|ETHTOOL_S|SIOCETHTOOL|GMODULEEEPROM|GLINKSETTINGS",
    "sysfs_platform": r"/sys/devices/platform/[a-z0-9-]+",
    "raw_dev": r"/dev/[a-z][a-z0-9_-]{2,}",
}

SKIP_DIRS = {"share", "doc", "locale", "python3", "perl", "terminfo"}


def is_elf(path):
    try:
        with open(path, "rb") as f:
            return f.read(4) == b"\x7fELF"
    except OSError:
        return False


def is_script(path):
    try:
        with open(path, "rb") as f:
            head = f.read(2)
        return head == b"#!"
    except OSError:
        return False


def scan(path):
    try:
        out = subprocess.run(
            ["strings", "-n", "4", path],
            capture_output=True,
            text=True,
            timeout=30,
        ).stdout
    except Exception:
        return {}
    hits = {}
    for label, pat in PATTERNS.items():
        found = sorted(
            set(re.findall(pat, out))
            if "(" in pat
            else set(m.group(0) for m in re.finditer(pat, out))
        )
        # collect full matching lines for context
        lines = [ln for ln in out.splitlines() if re.search(pat, ln)]
        if lines:
            hits[label] = sorted(set(lines))[:40]
    return hits


def main():
    root = sys.argv[1]
    targets = []
    for base in ("bin", "sbin", "usr/bin", "usr/sbin", "usr/lib", "lib"):
        d = os.path.join(root, base)
        for dp, dns, fns in os.walk(d):
            dns[:] = [x for x in dns if x not in SKIP_DIRS]
            for fn in fns:
                p = os.path.join(dp, fn)
                if os.path.islink(p):
                    continue
                if is_elf(p) or is_script(p):
                    targets.append(p)
    for p in sorted(set(targets)):
        hits = scan(p)
        if not hits:
            continue
        rel = os.path.relpath(p, root)
        print(f"\n########## {rel}")
        for label, lines in hits.items():
            print(f"  [{label}]")
            for ln in lines:
                print(f"    {ln}")


if __name__ == "__main__":
    main()
