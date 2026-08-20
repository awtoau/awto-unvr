#!/usr/bin/env python3
"""Find boot-time delays >THRESH in the woomera console log (kernel timestamps).

Parses the LAST boot (from the final "Booting Linux") in tmp/logs/unvr-console.log,
extracts each dmesg kernel timestamp [   NN.nnnnnn], computes the gap to the next
line, and reports every gap over THRESH seconds with the lines on each side (what
finished vs what came next). That localises where boot spends its time —
SATA/link timeouts, deferred-probe retries, udev stalls, fsck, etc.

Usage: boot-delay-analyze.py [THRESH_SECONDS]   (default 0.2)
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import LOGS

LOG = LOGS / "unvr-console.log"
THRESH = float(sys.argv[1]) if len(sys.argv) > 1 else 0.2
ANSI = re.compile(r"\x1b\[[0-9;?]*[a-zA-Z]|\x1b\][0-9;][^\x07]*\x07")
KTS = re.compile(r"\[\s*(\d+\.\d+)\]\s*(.*)$")


def clean(s: str) -> str:
    return ANSI.sub("", s).rstrip()


def main():
    if not LOG.exists():
        sys.exit(f"no console log at {LOG}")
    raw = LOG.read_text(errors="replace").splitlines()

    # find the last boot
    start = 0
    for i, ln in enumerate(raw):
        if "Booting Linux on physical CPU" in ln:
            start = i
    print(f"analysing boot starting at log line {start + 1}, threshold {THRESH}s\n")

    # collect (ktime, text) in boot order
    events = []
    for ln in raw[start:]:
        c = clean(ln)
        m = KTS.search(c)
        if m:
            events.append((float(m.group(1)), c[m.end(1) :].lstrip(" ]")))
        if "login:" in c:
            break

    if not events:
        sys.exit("no kernel timestamps found after the boot marker")

    total = events[-1][0] - events[0][0]
    gaps = []
    for (t0, txt0), (t1, txt1) in zip(events, events[1:]):
        d = t1 - t0
        if d >= THRESH:
            gaps.append((d, t0, txt0, t1, txt1))

    gaps.sort(reverse=True)
    print(
        f"boot window {events[0][0]:.2f}s .. {events[-1][0]:.2f}s "
        f"({total:.1f}s of dmesg), {len(gaps)} gap(s) >= {THRESH}s:\n"
    )
    for d, t0, txt0, t1, txt1 in gaps:
        print(f"  ┌ +{d:6.3f}s gap  (from {t0:.3f}s to {t1:.3f}s)")
        print(f"  │  after : {txt0[:110]}")
        print(f"  └  next  : {txt1[:110]}\n")
    if not gaps:
        print("  (no gaps over threshold — boot is dominated by many small steps)")


if __name__ == "__main__":
    main()
