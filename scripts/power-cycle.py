#!/usr/bin/env python3
"""Power-cycle (or just on/off) the UNVR via the Sonoff TH smart outlet,
verifying the resulting state.

CLI entry point for the shared logic in scripts/_power.py - the standalone
counterpart of what scripts/ram-boot-deploy.py already does before every
RAM-boot attempt. Exists so there is never a reason to reach for an inline
`python3 -c "... aioesphomeapi ..."` snippet again: that bypassed this
project's own house rule (scripts live in ./scripts/<name>.py) three times in
one session before this command existed - and, tonight, once more even
after it existed, for the plain on/off case this command didn't yet cover.

Run: ./dev.py power-cycle [--on | --off]
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _power import power_cycle_verified, power_set_verified
from _repo import make_log

log = make_log("power-cycle", stamped=False)


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    g = ap.add_mutually_exclusive_group()
    g.add_argument("--on", action="store_true", help="turn on only, no cycle")
    g.add_argument(
        "--off",
        action="store_true",
        help="turn off only, no cycle "
        "(e.g. to hold the box off while reseating a cable)",
    )
    args = ap.parse_args()
    try:
        if args.on:
            power_set_verified(True, log=log)
        elif args.off:
            power_set_verified(False, log=log)
        else:
            power_cycle_verified(log=log)
    except RuntimeError as e:
        log(f"FATAL: {e}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
