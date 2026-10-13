#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Copy a file onto the box through its Linux serial console (no network).

The console is a root shell (serial-getty autologin). The file goes over as
base64 into `cat > tmp`, terminated with Ctrl-D, then decoded in place and
sha256-verified against the local copy. Paced by the tty echo: each chunk is
sent only after the previous one has echoed back, so nothing outruns the UART.

For when both network paths are gone at once (#253: U-Boot cannot TX; a bad
al_eth module panics Linux at link-up). The alternative pipes were slower or
absent: xmodem into U-Boot had a handshake problem, and the kernel has no
pl2303 driver for the USB-serial adaptor.

Usage:
    ./scripts/console-put.py tmp/koput/al_eth_1g.ko \\
        /lib/modules/7.3.0-rc6-26b89e9-dirty+/extra/al_eth_1g.ko

Expects a Linux root prompt on the console. Log: tmp/logs/console-put.log
"""

from __future__ import annotations

import argparse
import base64
import hashlib
import os
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
import _console
from _repo import make_log

log = make_log("console-put", stamped=False)

PROMPT = "]# "  # bash root prompt tail; works with the OSC-decorated prompt
CHUNK = 64 * 77  # 64 base64 lines of 76 chars + \n
# Echo pacing makes the link half-duplex: measured 3.2 KB/s with 8-line
# chunks, so budget 3 KB/s; bound = 1.25x that plus 20 s of shell work.
SERIAL_BPS = 3_000


def rd(s, until: str, limit: float) -> str:
    buf, end = b"", time.monotonic() + limit
    while time.monotonic() < end:
        try:
            d = s.recv(4096)
        except TimeoutError:
            continue
        if not d:
            break
        buf += d
        if until.encode() in buf:
            break
    return buf.decode("utf-8", "replace")


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("src")
    ap.add_argument("dst")
    a = ap.parse_args()

    data = Path(a.src).read_bytes()
    sha = hashlib.sha256(data).hexdigest()
    b64 = base64.encodebytes(data)  # 76-col lines, newline terminated
    bound = len(b64) / SERIAL_BPS * 1.25 + 20
    log(
        f"{a.src}: {len(data)} bytes, {len(b64)} base64, sha256 {sha[:16]}..; bound {bound:.0f}s"
    )

    s = _console.connect()
    try:
        # Ctrl-C first: a previous aborted run leaves the shell inside `cat`.
        s.sendall(b"\x03\r")
        if PROMPT not in rd(s, PROMPT, 5):
            log("FATAL: no root prompt on the console")
            return 1
        tmp = "/tmp/console-put.b64"
        s.sendall(f"cat > {tmp}\r".encode())
        rd(s, "\n", 2)  # swallow the command echo

        t0 = time.monotonic()
        sent = 0
        while sent < len(b64):
            chunk = b64[sent : sent + CHUNK]
            s.sendall(chunk)
            # Wait for the echo of this chunk before sending the next.
            got, end = 0, time.monotonic() + 5
            while got < len(chunk) and time.monotonic() < end:
                try:
                    got += len(s.recv(4096))
                except TimeoutError:
                    continue
            if got < len(chunk):
                log(f"FATAL: echo stalled at {sent + got}/{len(b64)} bytes")
                return 1
            sent += len(chunk)
            if time.monotonic() - t0 > bound:
                log(f"FATAL: transfer exceeded {bound:.0f}s at {sent}/{len(b64)}")
                return 1
        s.sendall(b"\x04")  # EOF to cat
        if PROMPT not in rd(s, PROMPT, 10):
            log("FATAL: no prompt after EOF")
            return 1
        log(f"sent {len(b64)} bytes in {time.monotonic() - t0:.0f}s")

        s.sendall(
            f"base64 -d {tmp} > {a.dst} && /bin/rm -f {tmp} && sha256sum {a.dst}\r".encode()
        )
        out = rd(s, PROMPT, 30)
        if sha in out:
            log(f"OK: {a.dst} sha256 matches")
            return 0
        log(f"FATAL: sha256 mismatch or decode failed: {out.strip()[-200:]!r}")
        return 1
    finally:
        s.close()


if __name__ == "__main__":
    sys.exit(main())
