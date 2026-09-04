#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Send commands to a live EDK2 UEFI Shell prompt over the console socket.

Assumes the box is ALREADY sitting at the "Shell>" prompt - typically
left there by `./dev.py uefi-chainload-probe`, which stops as soon as it
sees "UEFI Interactive Shell". This script does NOT boot, power-cycle or
chainload anything; it only types at a prompt that already exists.

Used for docs/uefi.md P2's acceptance test: `map -r` must list a
filesystem and `ls fs0:` must list files off the SSD's ESP.

Note the shell shares ttyS0 with EDK2's DEBUG-build trace, so command
output arrives interleaved with driver prints. Everything received is
dumped verbatim; read it as a transcript, not as clean stdout.

Usage:
    ./dev.py uefi-shell-cmd "map -r" "ls fs0:"
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
import _console  # noqa: E402

# Per-command quiet window. The shell echoes and answers a local command
# (map/ls hit already-enumerated in-memory state, no device I/O) in well
# under a second; the DEBUG trace is what actually keeps the line busy.
# 2.0s of silence therefore means "this command is done talking", and
# 1.25x-style tightness is wrong here - the metric is idle time, not
# expected duration. On expiry we simply move to the next command.
QUIET_S = 2.0

# Ceiling per command so a wedged shell cannot hang the run forever.
# ~8x the observed answer time for map/ls; on expiry we log and continue.
CMD_LIMIT_S = 16.0


def send_cmd(s, cmd: str) -> str:
    s.sendall(cmd.encode() + b"\r")
    buf = b""
    start = time.monotonic()
    last_rx = start
    while True:
        now = time.monotonic()
        if now - start > CMD_LIMIT_S:
            print(f"  [limit] {CMD_LIMIT_S}s elapsed for {cmd!r}, moving on")
            break
        if buf and now - last_rx > QUIET_S:
            break
        try:
            d = s.recv(4096)
        except TimeoutError:
            continue
        if d:
            buf += d
            last_rx = time.monotonic()
    return buf.decode(errors="replace")


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("cmds", nargs="+", help="UEFI Shell commands to run in order")
    args = ap.parse_args()

    s = _console.connect()
    # Wake the prompt and flush any trailing trace before the first command.
    send_cmd(s, "")
    for cmd in args.cmds:
        print(f"\n=== {cmd}\n")
        print(send_cmd(s, cmd))
    return 0


if __name__ == "__main__":
    sys.exit(main())
