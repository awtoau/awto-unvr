#!/usr/bin/env python3
"""Poll the console log for boot progress instead of one long blocking expect.

A single `expect "login:" 90` call is silent for the full 90s even when the
real answer (trouble, or already-done) is available in seconds - nothing to
watch, no way to tell "still booting" from "hung" until the whole window
expires. This polls tmp/logs/unvr-console.log every POLL_S, printing a
progress line so it's visible as it happens, and returns the moment it sees
either the login prompt or a real trouble marker (shared regex with
boot-watch.py) - whichever comes first.

Timeout: TIMEOUT_S below. A clean boot reaches login in ~15-20s (observed,
this session); the slowest OBSERVED boot on this box was ~200s, when a prior
kernel Oops left dbus-broker/NetworkManager retrying during startup (2026-08-20,
see issue #106). 300s = ~1.5x that worst case, not a round number.
"""

from __future__ import annotations

import re
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import LOGS

CONSOLE_LOG = LOGS / "unvr-console.log"
POLL_S = 2.0
TIMEOUT_S = 300.0

TROUBLE = re.compile(
    r"Kernel panic|Internal error|\bOops:|BUG:|WARNING: CPU:|hung_task|"
    r"rcu_sched self-detected|Call trace:|watchdog: BUG|"
    r"segfault|\[FAILED\]|Failed to start|Failed to mount|"
    r"NO-STOCK-UBOOT|Unable to mount root|VFS: Unable to",
    re.IGNORECASE,
)
SUCCESS = re.compile(r"login:")


def main() -> int:
    if not CONSOLE_LOG.is_file():
        sys.exit(f"no console log at {CONSOLE_LOG} - is the console running?")
    start = time.monotonic()
    pos = CONSOLE_LOG.stat().st_size  # only new bytes from here on
    trouble_seen: list[str] = []
    last_report = 0.0
    print(
        f"watching {CONSOLE_LOG} from byte {pos}, timeout {TIMEOUT_S:.0f}s", flush=True
    )
    while True:
        elapsed = time.monotonic() - start
        if elapsed > TIMEOUT_S:
            print(f"TIMEOUT after {elapsed:.0f}s - no login prompt seen", flush=True)
            if trouble_seen:
                print("trouble lines seen along the way:", flush=True)
                for line in trouble_seen[-10:]:
                    print(f"  {line}", flush=True)
            return 3

        size = CONSOLE_LOG.stat().st_size
        if size > pos:
            with CONSOLE_LOG.open("rb") as f:
                f.seek(pos)
                chunk = f.read(size - pos).decode(errors="replace")
            pos = size
            for line in chunk.splitlines():
                if TROUBLE.search(line):
                    trouble_seen.append(line.strip())
                    print(f"[{elapsed:5.1f}s] TROUBLE: {line.strip()}", flush=True)
                if SUCCESS.search(line):
                    print(f"[{elapsed:5.1f}s] LOGIN PROMPT REACHED", flush=True)
                    if trouble_seen:
                        print(
                            f"  ({len(trouble_seen)} trouble line(s) seen during boot - review above)",
                            flush=True,
                        )
                        return 1
                    return 0
        elif elapsed - last_report > 10:
            last_report = elapsed
            print(
                f"[{elapsed:5.1f}s] still waiting ({len(trouble_seen)} trouble lines so far)",
                flush=True,
            )

        time.sleep(POLL_S)


if __name__ == "__main__":
    sys.exit(main())
