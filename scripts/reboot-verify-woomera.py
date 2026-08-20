#!/usr/bin/env python3
"""Reboot woomera into the freshly-deployed kernel+DTB and verify the unlock.

Careful: watches the console through the reboot for the login prompt (success),
or a panic/abort (new kernel bad), or a timeout (hang). On success runs the
verification battery — open /dev/mem, eeprom@57 auto-bind, i2c_gen bus, ttyS2,
temps. On failure it STOPS and prints the exact U-Boot fallback (load the .bak),
which is driven separately (reboot-to-uboot + ext4load *.bak).

Boot-watch cap = 240 s: a full reboot here is ~60-120 s (U-Boot bootdelay 2 s +
kernel + systemd); 2x margin. On expiry => boot hang => manual fallback.
"""

from __future__ import annotations

import sys
import time
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _console as con
from _repo import LOGS

LOG = LOGS / "reboot-verify-woomera.log"
BOOT_CAP = 240
BAD = ("Kernel panic", "Synchronous Abort", "Unable to handle", "Internal error")


def log(m):
    line = (
        f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    )
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    LOG.open("a").write(line + "\n")


def watch_boot(s):
    """Read the console until the login prompt (ok), a crash string, or the cap."""
    buf = b""
    end = time.monotonic() + BOOT_CAP
    while time.monotonic() < end:
        try:
            c = s.recv(4096)
        except Exception:
            continue
        if not c:
            continue
        buf += c
        tail = buf[-4096:].decode(errors="replace")
        if "login:" in tail:
            return "ok"
        for b in BAD:
            if b in tail:
                return b
    return "timeout"


def verify(s):
    con.login(s)
    log("re-logged in after reboot")
    checks = [
        ("kernel", "uname -r"),
        (
            "/dev/mem MMIO (DDR MSTR @0xf0080000)",
            "python3 -c \"import mmap,os,struct; f=os.open('/dev/mem',os.O_RDONLY); "
            "m=mmap.mmap(f,4096,mmap.MAP_SHARED,mmap.PROT_READ,offset=0xf0080000); "
            "print('MSTR=0x%08x'%struct.unpack('<I',m[0:4])[0])\" 2>&1",
        ),
        (
            "eeprom@57 auto-bound (new DTB)",
            "ls -l /sys/bus/i2c/devices/0-0057/eeprom 2>&1 | tail -1",
        ),
        ("i2c buses (i2c_gen should add one)", "i2cdetect -l 2>/dev/null | sort"),
        ("ttyS2", "ls /dev/ttyS2"),
        (
            "temps",
            'for h in /sys/class/hwmon/hwmon*; do echo -n "$(cat $h/name) "; '
            "cat $h/temp1_input 2>/dev/null; done",
        ),
    ]
    for label, cmd in checks:
        rc, out = con.sh(s, cmd, 30)
        log(f"\n=== {label} (rc={rc}) ===\n{out}")


def main():
    s = con.connect()
    con.login(s)
    rc, base = con.sh(s, "uname -r; grep -c . /proc/uptime")
    log(f"baseline (old kernel): {base}")
    log("sending reboot — watching console (cap %ds)..." % BOOT_CAP)
    con.sh(s, "sync")
    s.sendall(b"reboot\r")

    result = watch_boot(s)
    if result == "ok":
        log("BOOT OK — login prompt seen")
        verify(s)
        log("DONE — verification battery complete (see log)")
        s.close()
        return 0
    log(f"BOOT FAILED: {result}")
    log(
        "RECOVER: catch stock U-Boot (reboot-to-uboot) then:\n"
        "  ext4load scsi 0:2 0x02000000 /boot/uImage-unvr-ea16-7.1-fedora-gz.bak\n"
        "  ext4load scsi 0:2 0x04078000 /boot/alpine-v2-ubnt-unvr-ea16-7.1-fedora.dtb.bak\n"
        "  bootm 0x02000000 - 0x04078000"
    )
    s.close()
    return 2


if __name__ == "__main__":
    sys.exit(main())
