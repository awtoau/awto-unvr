#!/usr/bin/env python3
"""Power-cycle the UNVR via the Sonoff TH smart outlet, verifying it lands ON.

CLI entry point for the shared logic in scripts/_power.py - the standalone
counterpart of what scripts/ram-boot-deploy.py already does before every
RAM-boot attempt. Exists so there is never a reason to reach for an inline
`python3 -c "... aioesphomeapi ..."` snippet again: that bypassed this
project's own house rule (scripts live in ./scripts/<name>.py) three times in
one session before this command existed.

Run: ./dev.py power-cycle
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _power import power_cycle_verified
from _repo import log_path

LOG = log_path("power-cycle")


def log(msg: str) -> None:
    print(msg, flush=True)
    with LOG.open("a") as fh:
        fh.write(msg + "\n")


def main() -> int:
    argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    ).parse_args()
    try:
        power_cycle_verified(log=log)
    except RuntimeError as e:
        log(f"FATAL: {e}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
