#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""From an awto-nas# prompt: optionally reset the saved env to compiled defaults,
clear the stay-in-U-Boot canary, and run bootcmd (SSD boot, #216).

Why --reset-env exists: awto-uboot persists its env to NOR 0x1E0000 and a SAVED
env OVERRIDES CONFIG_BOOTARGS / CONFIG_BOOTCOMMAND. After any defconfig change
the old values silently win until `env default -a; saveenv` is run (#158,
#216). Run this with --reset-env after every awto-uboot rebuild.

Expects the box already at awto-nas# (e.g. after `./dev.py uboot-test`).

Usage:
    ./scripts/awto-uboot-boot-ssd.py [--reset-env] [--no-boot]
"""

from __future__ import annotations

import argparse
import os
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
import _console

PROMPT = _console.AWTO_PROMPT.encode()
# bootm -> "Starting kernel" was ~1s after the 148ms ext4load; 40s covers a
# full scsi scan (4 disks, ~10s) plus load with wide margin. Past that, the
# bootcmd fell through and we are back at the prompt - report it.
BOOT_WAIT_S = 40
CMD_WAIT_S = 8


def send_expect(s, cmd: bytes, wait: float, until: bytes | None = None) -> str:
    s.sendall(cmd + b"\r")
    buf = b""
    end = time.monotonic() + wait
    while time.monotonic() < end:
        try:
            d = s.recv(4096)
            if d:
                buf += d
        except TimeoutError:
            continue
        if until and until in buf:
            break
    return buf.decode(errors="replace")


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--reset-env",
        action="store_true",
        help="env default -a; saveenv before booting",
    )
    ap.add_argument(
        "--no-boot",
        action="store_true",
        help="stop after showing the effective bootcmd/bootargs",
    )
    args = ap.parse_args()

    s = _console.connect()
    try:
        out = send_expect(s, b"", 3, PROMPT)
        if PROMPT not in out.encode():
            print("FATAL: not at awto-nas# - run ./dev.py uboot-test first")
            return 1

        if args.reset_env:
            print(send_expect(s, b"env default -a", CMD_WAIT_S, PROMPT).strip()[-200:])
            print(send_expect(s, b"saveenv", 15, PROMPT).strip()[-300:])

        for var in (b"bootcmd", b"bootargs"):
            o = send_expect(s, b"printenv " + var, CMD_WAIT_S, PROMPT)
            line = next(
                (ln for ln in o.splitlines() if ln.startswith(var.decode() + "=")), ""
            )
            print(f"{var.decode()}: {line[len(var) + 1 :][:300]}")

        if args.no_boot:
            return 0

        send_expect(s, b"mw.l 0x10000000 0", CMD_WAIT_S, PROMPT)
        o = send_expect(s, b"run bootcmd", BOOT_WAIT_S, b"Starting kernel")
        for ln in o.splitlines():
            if any(
                k in ln
                for k in (
                    "Device ",
                    "booting from",
                    "bytes read",
                    "Verifying",
                    "Starting kernel",
                    "no /boot",
                    "rror",
                )
            ):
                print("  " + ln.strip()[:110])
        if "Starting kernel" not in o:
            print(
                f"FAIL: no 'Starting kernel' within {BOOT_WAIT_S}s - bootcmd fell through"
            )
            return 2
        print("OK: kernel started - now run ./dev.py wait-for-boot")
        return 0
    finally:
        s.close()


if __name__ == "__main__":
    sys.exit(main())
