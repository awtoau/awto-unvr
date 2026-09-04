#!/usr/bin/env python3
"""Connect to woomera by MAC, not by address - the DHCP lease moves.

The resolver itself lives in scripts/_box.py (import it; do not shell out to
this script and parse its stdout). See docs/unvr-access-research.md.

  ssh-woomera.py              # interactive shell
  ssh-woomera.py --print      # just print the address
  ssh-woomera.py -- uname -a  # run a command
"""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _box import flush_failed_neighbours, locate, ssh_argv
from _net import LAN_SUBNET


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--subnet", default=LAN_SUBNET)
    ap.add_argument("--user", default="root")
    ap.add_argument("--print", dest="print_only", action="store_true")
    ap.add_argument("cmd", nargs="*", help="command to run remotely (default: shell)")
    args = ap.parse_args()

    ip = locate(args.subnet)
    if not ip:
        print(
            f"woomera not found on {args.subnet} - box off, or on another subnet",
            file=sys.stderr,
        )
        return 1

    if args.print_only:
        print(ip)
        return 0

    flush_failed_neighbours(ip)
    print(f"# woomera at {ip}", file=sys.stderr)
    os.execvp("ssh", ssh_argv(ip, args.user, args.cmd))


if __name__ == "__main__":
    sys.exit(main())
