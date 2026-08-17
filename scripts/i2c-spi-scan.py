#!/usr/bin/env python3
"""Scan all I2C buses + list SPI/MTD + hwmon/RTC on woomera, over the serial console.

Enumerates every /dev/i2c-N adapter (native controllers + pca9546 mux channels),
probes each with i2cdetect (installs i2c-tools if missing and network is up), lists
the DT-bound i2c devices with their addresses, the SPI devices + NOR MTD map, and
the hwmon sensors (adt7475) + RTC (s35390a). Read-only. Output -> tmp/logs.

Assumes the console is at a Fedora shell (root) or login prompt.
"""
from __future__ import annotations
import os, re, socket, sys, time
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import LOGS  # noqa: E402

SOCK = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "tio-unvr.sock"
PROMPT = "@@P@@"
USER, PASSWD = "root", "unvr"


def log(m):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    (LOGS / "i2c-spi-scan.log").open("a").write(line + "\n")


def _ru(s, needle, limit, extra=()):
    buf = b""; end = time.monotonic() + limit
    while time.monotonic() < end:
        try: c = s.recv(4096)
        except socket.timeout: continue
        if not c: break
        buf += c
        if needle.encode() in buf: return buf, needle
        for n in extra:
            if n.encode() in buf: return buf, n
    return buf, None


def login(s):
    s.sendall(b"\r")
    _, hit = _ru(s, "]#", 6, extra=("login:", PROMPT))
    if hit == "login:":
        s.sendall(USER.encode() + b"\r"); _ru(s, "Password:", 6)
        s.sendall(PASSWD.encode() + b"\r"); _ru(s, "]#", 12, extra=("incorrect",))
    s.sendall(b"unalias -a 2>/dev/null; true\r"); _ru(s, "]#", 4, extra=(PROMPT,))
    s.sendall(f"export PS1='{PROMPT}'\r".encode()); _ru(s, PROMPT, 6); _ru(s, PROMPT, 3)
    log("shell ready")


def sh(s, cmd, timeout=30):
    s.sendall(cmd.encode() + b"; echo @@RC=$?@@\r")
    buf = b""; end = time.monotonic() + timeout; rc = None
    while time.monotonic() < end:
        try: c = s.recv(4096)
        except socket.timeout: continue
        if not c: break
        buf += c
        m = re.search(rb"@@RC=(\d+)@@", buf)
        if m: rc = int(m.group(1)); break
    _ru(s, PROMPT, 4)
    txt = re.sub(r"\x1b\[[0-9;?]*[a-zA-Z]", "", buf.decode(errors="replace"))
    txt = re.sub(r"\x1b\][0-9;].*?(\x07|\x1b\\)", "", txt)  # OSC sequences
    # keep the lines between the echoed cmd and the RC marker
    lines = [l for l in txt.splitlines() if "@@RC=" not in l and "; echo @@RC" not in l]
    return rc, "\n".join(lines).strip()


def section(s, title, cmd, timeout=30):
    rc, out = sh(s, cmd, timeout)
    log(f"\n===== {title} (rc={rc}) =====\n{out}")
    return out


def main():
    if not SOCK.exists():
        sys.exit(f"console socket absent: {SOCK}")
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(0.2); s.connect(str(SOCK))
    login(s)

    section(s, "I2C adapters", 'for b in /sys/class/i2c-dev/i2c-*; do n=$(basename $b); '
            'echo "$n: $(cat $b/name)"; done')

    have = sh(s, "command -v i2cdetect >/dev/null && echo yes || echo no")[1]
    if "yes" not in have:
        log("i2cdetect missing — trying to install i2c-tools")
        section(s, "install i2c-tools", "dnf -y -q install i2c-tools 2>&1 | tail -5", 180)
        have = sh(s, "command -v i2cdetect >/dev/null && echo yes || echo no")[1]

    if "yes" in have:
        # one on-device loop over every adapter — no fragile bus-list capture
        section(s, "i2cdetect probe (all buses)",
                'for b in /sys/class/i2c-dev/i2c-*; do n=$(basename $b | sed "s/i2c-//"); '
                'echo "--- bus $n ($(cat $b/name)) ---"; i2cdetect -y -r "$n"; done', 60)
    else:
        log("i2cdetect unavailable and no install — falling back to bound-device list only")

    section(s, "DT-bound I2C devices (addr = bus-XXXX)",
            'for d in /sys/bus/i2c/devices/*-*; do echo "$(basename $d): '
            '$(cat $d/name 2>/dev/null)"; done')

    section(s, "SPI devices", 'for d in /sys/bus/spi/devices/*; do [ -e "$d" ] && '
            'echo "$(basename $d): $(cat $d/modalias 2>/dev/null)"; done 2>/dev/null; '
            'echo "-- mtd --"; cat /proc/mtd 2>/dev/null')

    section(s, "hwmon sensors", 'for h in /sys/class/hwmon/hwmon*; do echo "$(basename $h): '
            '$(cat $h/name 2>/dev/null)"; done; echo "-- sensors --"; '
            'command -v sensors >/dev/null && sensors 2>/dev/null || echo "(no sensors bin)"')

    section(s, "RTC", 'ls /sys/class/rtc/ 2>/dev/null; hwclock -r 2>&1 | head -2')

    log("\nDONE — full scan in tmp/logs/i2c-spi-scan.log")
    s.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
