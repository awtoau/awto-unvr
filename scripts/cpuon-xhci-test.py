#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""#140: does bringing up a cluster-1 core change the xHCI Enable-Slot timeout?

Hypothesis under test: AL-324 is a 2x2 A57 part. Linux brings up both clusters
and its xHCI driver works; U-Boot and EDK2 run single-core and both fail
Enable Slot. If CCU slave4 (cluster1) snoop routing is gated on cluster1 being
powered, powering it is the one variable that explains all three.

Sequence, from an awto-nas# prompt:
  1. baseline `usb start` (expect the known Enable-Slot failure)
  2. `cpuon`            - PSCI version + AFFINITY_INFO, no core brought up
  3. `cpuon <mpidr>`    - CPU_ON a cluster-1 core, prove it executed
  4. `usb reset` / `usb start` again - did anything change?

Requires a real USB device plugged into the external port (the failure only
appears with a device attached).

Usage:
    ./scripts/cpuon-xhci-test.py [--mpidr 0x100]
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
LOG = REPO / "tmp" / "logs" / "cpuon-xhci-test.log"

# `usb start` on this box: the xHCI Enable-Slot path retries internally and the
# whole command has been observed taking ~15 s when it fails (issue #140's
# console captures). 25 s is ~1.6x that - deliberately above 1.25x because the
# failing path's retry count is the thing under test and may change. On expiry:
# log what was captured and carry on to the next step, never hang.
USB_WAIT_S = 25
# cpuon: one SMC plus a bounded 5 ms in-command poll; the command itself cannot
# take more than a few ms. 6 s covers console round-trip with huge margin.
# On expiry: report no prompt seen and continue.
CMD_WAIT_S = 6


def logline(msg: str) -> None:
    line = f"[{time.strftime('%Y-%m-%dT%H:%M:%S%z')}] {msg}"
    print(line, flush=True)
    with open(LOG, "a") as f:
        f.write(line + "\n")


def send_expect(s, cmd: bytes, wait: float, until: bytes = PROMPT) -> str:
    s.sendall(cmd + b"\r")
    buf = b""
    end = time.monotonic() + wait
    # Ignore the prompt echoed back with our own command line before output.
    seen_echo = False
    while time.monotonic() < end:
        try:
            d = s.recv(4096)
            if d:
                buf += d
        except TimeoutError:
            continue
        if not seen_echo:
            if cmd and cmd in buf:
                seen_echo = True
            continue
        if until in buf:
            break
    return buf.decode(errors="replace")


def step(s, title: str, cmd: bytes, wait: float) -> str:
    logline(f"=== {title}: `{cmd.decode()}` ===")
    out = send_expect(s, cmd, wait)
    for ln in out.splitlines():
        logline("  | " + ln.rstrip()[:150])
    return out


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--mpidr",
        default="0x100",
        help="MPIDR of the cluster-1 core to CPU_ON (default 0x100)",
    )
    ap.add_argument(
        "--skip-baseline",
        action="store_true",
        help="skip the pre-cpuon `usb start` (if already captured)",
    )
    args = ap.parse_args()

    LOG.parent.mkdir(parents=True, exist_ok=True)
    logline(f"cpuon-xhci-test start (target mpidr {args.mpidr})")

    s = _console.connect()
    try:
        out = send_expect(s, b"", 3)
        if PROMPT not in out.encode():
            logline("FATAL: not at awto-nas# - run ./dev.py uboot-test --cold first")
            return 1

        step(s, "CCU/PCIe state before", b"aldiag", CMD_WAIT_S)

        if not args.skip_baseline:
            step(s, "BASELINE usb start (single-core)", b"usb start", USB_WAIT_S)
            step(s, "BASELINE usb tree", b"usb tree", CMD_WAIT_S)

        step(s, "PSCI probe (no core brought up)", b"cpuon", CMD_WAIT_S)
        on = step(
            s,
            f"PSCI CPU_ON mpidr {args.mpidr}",
            b"cpuon " + args.mpidr.encode(),
            CMD_WAIT_S,
        )
        ran = "SECONDARY RAN" in on
        logline(f"RESULT: secondary executed = {ran}")

        step(s, "AFTER-cpuon usb reset", b"usb reset", USB_WAIT_S)
        after = step(s, "AFTER-cpuon usb start", b"usb start", USB_WAIT_S)
        step(s, "AFTER-cpuon usb tree", b"usb tree", CMD_WAIT_S)

        failed = "Enable Slot" in after or "unable to get device descriptor" in after
        logline(f"RESULT: cluster1 up = {ran}; xHCI still failing = {failed}")
        logline(f"full log: {LOG}")
        return 0
    finally:
        s.close()


if __name__ == "__main__":
    sys.exit(main())
