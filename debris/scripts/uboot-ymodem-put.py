#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Put a file onto the SSD from the awto-nas# prompt over the serial console.

Recovery path when the box has no network in either U-Boot or Linux (#253:
U-Boot cannot TX on either port today; a bad al_eth module panics Linux at
link-up, so Linux has no network either). The console is the only pipe left.

Sequence at the prompt: `loady <addr>` + ymodem send through tio's socket
(`sz --ymodem` with its stdio on the socket), then `ext4write` to the path.
Flash is never touched; the SSD file is replaced in place.

Usage:
    ./scripts/uboot-ymodem-put.py tmp/koput/al_eth_1g.ko \\
        /lib/modules/7.3.0-rc6-26b89e9-dirty+/extra/al_eth_1g.ko
    ./scripts/uboot-ymodem-put.py SRC DST --scsi 1:2 --addr 0x02000000

Expects the box already held at awto-nas# (bootdelay -1) with `scsi scan`
done. Log: tmp/logs/uboot-ymodem-put.log
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
import _console
from _repo import make_log

log = make_log("uboot-ymodem-put", stamped=False)

PROMPT = _console.AWTO_PROMPT
SOCK = str(_console.SOCK)
# xmodem-1k at 115200 8N1: ~10 KB/s. loady/sb and sz --ymodem both opened
# with a ZMODEM ZRQINIT header that U-Boot rejected; xmodem has no header.
YMODEM_BPS = 7_000
CMD_WAIT_S = 8


def send_expect(s, cmd: str, wait: float, until: str) -> str:
    s.sendall(cmd.encode() + b"\r")
    buf, end = b"", time.monotonic() + wait
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
    ap.add_argument("dst", help="absolute path inside the partition")
    ap.add_argument("--scsi", default="1:2", help="U-Boot scsi dev:part (default 1:2)")
    ap.add_argument("--addr", default="0x02000000", help="RAM load address")
    a = ap.parse_args()

    src = Path(a.src)
    size = src.stat().st_size
    # Bound: bytes at YMODEM_BPS, x1.25, plus 10 s for the handshake. On
    # expiry the sz process is killed and the step fails loudly.
    xfer_s = int(size / YMODEM_BPS * 1.25) + 10
    log(f"{src} ({size} bytes) -> scsi {a.scsi} {a.dst}; ymodem bound {xfer_s}s")

    s = _console.connect()
    try:
        if PROMPT not in send_expect(s, "", 3, PROMPT):
            log("FATAL: not at awto-nas#")
            return 1
        # Wait for U-Boot to confirm receive mode BEFORE handing the socket to
        # sx. Closing right after sendall truncated the command: U-Boot printed
        # loadx usage, never entered xmodem, and sx waited for a NAK forever.
        out = send_expect(s, f"loadx {a.addr}", 5, "Ready for binary")
        if "Ready for binary" not in out:
            log(f"FATAL: loadx did not start: {out[-200:]}")
            return 1
        log("U-Boot in xmodem receive")
        s.close()

        log("sending (sx -k = xmodem-1k, over the console socket)")
        r = subprocess.run(
            ["ncat", "-U", SOCK, "--exec", f"/usr/bin/sx -k -b {src}"],
            timeout=xfer_s,
            capture_output=True,
        )
        if r.returncode:
            log(
                f"FATAL: sz exited {r.returncode}: {r.stderr.decode(errors='replace')[-200:]}"
            )
            return 1

        s = _console.connect()
        out = send_expect(s, "", 10, PROMPT)
        m = re.search(r"Total Size = 0x([0-9a-fA-F]+)", out)
        got = int(m.group(1), 16) if m else -1
        log(
            f"U-Boot received 0x{got:x} = {got} bytes"
            if got >= 0
            else f"no size line: {out[-300:]}"
        )
        if got != size:
            log(f"FATAL: size mismatch (sent {size})")
            return 1

        out = send_expect(
            s, f"ext4write scsi {a.scsi} {a.addr} {a.dst} 0x{size:x}", 60, PROMPT
        )
        if "bytes written" not in out:
            log(f"FATAL: ext4write did not confirm: {out[-300:]}")
            return 1
        log(
            "written: "
            + next(ln for ln in out.splitlines() if "bytes written" in ln).strip()
        )

        out = send_expect(s, f"ext4size scsi {a.scsi} {a.dst}", CMD_WAIT_S, PROMPT)
        log("ext4size: " + " ".join(out.split()[-6:]))
        return 0
    finally:
        try:
            s.close()
        except Exception:
            pass


if __name__ == "__main__":
    sys.exit(main())
