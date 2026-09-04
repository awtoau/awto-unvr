#!/usr/bin/env python3
"""Scan all I2C buses + list SPI/MTD + hwmon/RTC on woomera, over the serial console.

Enumerates every /dev/i2c-N adapter (native controllers + pca9546 mux channels),
probes each with i2cdetect (installs i2c-tools if missing and network is up), lists
the DT-bound i2c devices with their addresses, the SPI devices + NOR MTD map, and
the hwmon sensors (adt7475) + RTC (s35390a). Read-only. Output -> tmp/logs.

Assumes the console is at a Fedora shell (root) or login prompt.
"""

from __future__ import annotations

import sys
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _console
from _repo import LOGS


def log(m):
    line = (
        f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    )
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    (LOGS / "i2c-spi-scan.log").open("a").write(line + "\n")


def sh(s, cmd, timeout=30):
    return _console.sh(s, cmd, timeout=timeout)


def section(s, title, cmd, timeout=30):
    rc, out = sh(s, cmd, timeout)
    log(f"\n===== {title} (rc={rc}) =====\n{out}")
    return out


def main():
    if not _console.SOCK.exists():
        sys.exit(f"console socket absent: {_console.SOCK}")
    s = _console.connect()
    _console.login(s)
    log("shell ready")

    section(
        s,
        "I2C adapters",
        "for b in /sys/class/i2c-dev/i2c-*; do n=$(basename $b); "
        'echo "$n: $(cat $b/name)"; done',
    )

    have = sh(s, "command -v i2cdetect >/dev/null && echo yes || echo no")[1]
    if "yes" not in have:
        log("i2cdetect missing — trying to install i2c-tools")
        section(
            s, "install i2c-tools", "dnf -y -q install i2c-tools 2>&1 | tail -5", 180
        )
        have = sh(s, "command -v i2cdetect >/dev/null && echo yes || echo no")[1]

    if "yes" in have:
        # one on-device loop over every adapter — no fragile bus-list capture
        section(
            s,
            "i2cdetect probe (all buses)",
            'for b in /sys/class/i2c-dev/i2c-*; do n=$(basename $b | sed "s/i2c-//"); '
            'echo "--- bus $n ($(cat $b/name)) ---"; i2cdetect -y -r "$n"; done',
            60,
        )
    else:
        log(
            "i2cdetect unavailable and no install — falling back to bound-device list only"
        )

    section(
        s,
        "DT-bound I2C devices (addr = bus-XXXX)",
        'for d in /sys/bus/i2c/devices/*-*; do echo "$(basename $d): '
        '$(cat $d/name 2>/dev/null)"; done',
    )

    section(
        s,
        "SPI devices",
        'for d in /sys/bus/spi/devices/*; do [ -e "$d" ] && '
        'echo "$(basename $d): $(cat $d/modalias 2>/dev/null)"; done 2>/dev/null; '
        'echo "-- mtd --"; cat /proc/mtd 2>/dev/null',
    )

    section(
        s,
        "hwmon sensors",
        'for h in /sys/class/hwmon/hwmon*; do echo "$(basename $h): '
        '$(cat $h/name 2>/dev/null)"; done; echo "-- sensors --"; '
        'command -v sensors >/dev/null && sensors 2>/dev/null || echo "(no sensors bin)"',
    )

    section(s, "RTC", "ls /sys/class/rtc/ 2>/dev/null; hwclock -r 2>&1 | head -2")

    log("\nDONE — full scan in tmp/logs/i2c-spi-scan.log")
    s.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
