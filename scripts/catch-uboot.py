#!/usr/bin/env python3
"""Stream ESC to the console socket until the U-Boot prompt appears.

The autoboot window is `bootdelay=2` and easy to miss by hand. This holds one
connection to tio's socket open and sends ESC continuously, so the device lands
at the U-Boot prompt on its own - start it BEFORE power-cycling.

Exits as soon as the prompt is seen, so it cannot keep typing into U-Boot.

Run: ./scripts/catch-uboot.py            # then power-cycle the unit
     ./scripts/catch-uboot.py --seconds 180
"""

from __future__ import annotations

import argparse
import os
import socket
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import LOGS  # noqa: E402

SOCK = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "tio-unvr.sock"

# The device prints this during the countdown; seeing it means we are in.
PROMPT_HINTS = (b"ALPINE_UBNT_NAS", b"=>", b"Autobooting", b"press \"<Esc><Esc>\"")
# Confirmation that autoboot was actually stopped rather than merely announced.
STOPPED_HINTS = (b"ALPINE_UBNT_NAS_ALL>", b"ALPINE_UBNT_NAS>")

# 20 ESC/s. U-Boot polls its console during the countdown; at bootdelay=2 that
# is ~40 chances to land inside the window.
ESC_INTERVAL = 0.05


def log(msg):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {msg}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    with (LOGS / "catch-uboot.log").open("a") as fh:
        fh.write(line + "\n")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    # Ceiling only - the loop exits the moment the prompt appears. Long enough
    # for a human to walk to the unit and pull power.
    ap.add_argument("--seconds", type=float, default=240.0)
    a = ap.parse_args()

    if not SOCK.exists():
        sys.exit(f"console socket absent: {SOCK}\nStart it with ./dev.py console")

    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(0.05)
    s.connect(str(SOCK))
    log(f"streaming ESC to {SOCK} - POWER-CYCLE THE UNVR NOW")

    buf = b""
    last = 0.0
    end = time.monotonic() + a.seconds
    announced = False
    try:
        while time.monotonic() < end:
            now = time.monotonic()
            if now - last >= ESC_INTERVAL:
                try:
                    s.sendall(b"\x1b")
                except OSError as e:
                    log(f"send failed: {e}")
                    return 1
                last = now
            try:
                chunk = s.recv(4096)
            except socket.timeout:
                continue
            if not chunk:
                break
            buf = (buf + chunk)[-16384:]
            if not announced and any(h in buf for h in PROMPT_HINTS):
                announced = True
                log("device is booting - countdown seen")
            if any(h in buf for h in STOPPED_HINTS):
                log("U-BOOT PROMPT REACHED - autoboot stopped")
                return 0
    finally:
        s.close()
    log("timed out without reaching the prompt")
    return 1


if __name__ == "__main__":
    sys.exit(main())
