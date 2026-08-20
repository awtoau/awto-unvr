#!/usr/bin/env python3
"""Tail the serial console log live and flag real trouble as it appears.

Should be running for every deploy/boot-verify from now on - a hung/degraded
boot (like the #104 module-mismatch Oops) can sit for minutes past a plain
"wait for login:" timeout without anyone noticing WHY. This prints only
flagged trouble lines (the console log itself is enormous and mostly noise),
so it's safe to leave running in the background during a boot attempt.

  ./scripts/boot-watch.py            # from the current end of the log, live
  ./scripts/boot-watch.py --from-start   # rescan the whole log first
"""

from __future__ import annotations

import argparse
import re
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import LOGS

CONSOLE_LOG = LOGS / "unvr-console.log"

# Genuine trouble, not routine boot noise. Ordered roughly by severity for
# the reader's eye, not that it affects matching.
TROUBLE = re.compile(
    r"Kernel panic|Internal error|Oops:|BUG:|WARNING: CPU:|hung_task|"
    r"rcu_sched self-detected|Call trace:|watchdog: BUG|"
    r"segfault|\[FAILED\]|Failed to start|Failed to mount|"
    r"NO-STOCK-UBOOT|Unable to mount root|VFS: Unable to",
    re.IGNORECASE,
)

# 1s: the console log is appended to in small bursts as bytes arrive over
# the 115200-baud serial link; polling faster just burns CPU for no benefit,
# polling much slower would delay flagging trouble by a noticeable amount
# during an active deploy.
POLL_INTERVAL_S = 1


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--from-start",
        action="store_true",
        help="scan the whole log first, not just new lines from now",
    )
    args = ap.parse_args()

    if not CONSOLE_LOG.exists():
        sys.exit(
            f"no console log at {CONSOLE_LOG} - is the console running? (dev.py console)"
        )

    with CONSOLE_LOG.open("r", errors="replace") as f:
        if not args.from_start:
            f.seek(0, 2)  # end - only new lines from here
        print(f"# watching {CONSOLE_LOG} for trouble - Ctrl-C to stop", file=sys.stderr)
        found = 0
        try:
            while True:
                line = f.readline()
                if not line:
                    time.sleep(POLL_INTERVAL_S)
                    continue
                if TROUBLE.search(line):
                    found += 1
                    print(line.rstrip(), flush=True)
        except KeyboardInterrupt:
            pass
    print(f"# {found} trouble line(s) flagged", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
