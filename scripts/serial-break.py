#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Send a serial BREAK (+ optional follow-up Magic SysRq letter) directly to
the UNVR's tty device via TIOCSBRK/TIOCCBRK ioctls -- bypassing tio's own
socket protocol.

Why this exists: tio's own interactive `Ctrl-T B` keybind sends BREAK, which
is exactly what's needed to trigger Magic SysRq over serial (`docs` memory:
kernel.sysrq=1 was enabled specifically so a stuck box is recoverable via
serial BREAK+key without a physical power-cycle). But tio's `-S`/`--socket`
mode -- what `dev.py console-send` drives -- is documented (tio's own man
page, "Redirect I/O to socket") as relaying input "as if entered at the
terminal ... except that ctrl-t sequences are not recognized". So there is
no way to trigger a BREAK through the scripted console-send path at all.

BREAK is a UART line condition, not a data byte, so it can't be worked
around by sending some special byte sequence through the socket either --
it has to go through the actual serial line. This script opens a SECOND fd
on the same tty device tio already has open and drives TIOCSBRK/TIOCCBRK
directly. This works at the kernel level regardless of which process's fd
requests the ioctl (multiple opens of a serial device are allowed by the
driver -- there's no O_EXCL lock here), so it works even while tio is
actively using the device for the normal console session.

Deliberately does NOT touch termios (baud/parity/etc) on the device --
only the break ioctls and, for --sysrq, one raw byte write for the
follow-up letter -- specifically so it can't disturb tio's own line
configuration for the primary session.

Kernel side: mainline serial drivers arm a 5s window (SYSRQ_TIMEOUT) after
detecting a BREAK during which the next received byte is treated as the
Magic SysRq command letter -- so timing between the break and the
follow-up write is not tight; no need to race it.

BREAK hold duration: 250ms. Not a kernel-driver requirement (a break
condition of even a few ms is enough for hardware BI detection at any
sane baud rate) -- generous specifically to account for the CP2102
USB-serial bridge's own USB-transaction latency/buffering, so the break
condition is unambiguously present on the wire for long enough regardless
of bridge-side jitter.

Usage:
    scripts/serial-break.py                  # BREAK only, no follow-up letter
    scripts/serial-break.py --sysrq h         # BREAK + 'h' (SAFE: prints the
                                               # sysrq help list, no side effects)
    scripts/serial-break.py --sysrq w         # BREAK + 'w' (blocked-task dump)

Logs to tmp/logs/serial-break.log.
"""

from __future__ import annotations

import argparse
import fcntl
import os
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import make_log

DEV = "/dev/serial/by-id/usb-Silicon_Labs_CP2102_USB_to_UART_Bridge_Controller_0001-if00-port0"
BREAK_HOLD_S = 0.25  # see module docstring for reasoning

# Python's termios module doesn't expose these (only TCSBRK, a different,
# blocking libc-level break primitive). These are the standard Linux
# generic ioctl numbers from <asm-generic/ioctls.h> - stable across every
# mainstream architecture (x86/aarch64/arm/etc; only a handful of odd
# architectures like sparc/powerpc/mips use a different numbering scheme,
# irrelevant here since this always runs on the x86_64 dev host).
TIOCSBRK = 0x5427
TIOCCBRK = 0x5428

log = make_log("serial-break")


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--sysrq",
        metavar="LETTER",
        help="single sysrq command letter to send right after the break "
        "(e.g. 'h' for the safe help-list self-test, 'w' for a blocked-task "
        "dump). Omit to send BREAK alone.",
    )
    a = ap.parse_args()
    if a.sysrq and len(a.sysrq) != 1:
        log(f"FATAL: --sysrq must be a single letter, got {a.sysrq!r}")
        return 2

    dev = Path(DEV)
    if not dev.exists():
        log(f"FATAL: {DEV} not found -- console adapter unplugged?")
        return 1

    fd = os.open(str(dev.resolve()), os.O_RDWR | os.O_NOCTTY)
    try:
        log(
            f"opened {dev} (real path {dev.resolve()}) as a second fd -- termios untouched"
        )
        fcntl.ioctl(fd, TIOCSBRK)
        log(f"BREAK asserted, holding {BREAK_HOLD_S}s")
        time.sleep(BREAK_HOLD_S)
        fcntl.ioctl(fd, TIOCCBRK)
        log("BREAK cleared")
        if a.sysrq:
            os.write(fd, a.sysrq.encode())
            log(f"sent sysrq letter {a.sysrq!r}")
    finally:
        os.close(fd)
    return 0


if __name__ == "__main__":
    sys.exit(main())
