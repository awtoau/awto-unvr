#!/usr/bin/env python3
"""Connect to woomera by MAC, not by address - the DHCP lease moves.

Seen at .149 (2026-08-16) then .106 (2026-08-18); .149 was reassigned to another
host, so a stale address fails "refused" rather than "timeout". The Ubiquiti OUI
is the only stable handle. See docs/unvr-access-research.md.

  ssh-woomera.py              # interactive shell
  ssh-woomera.py --print      # just print the address
  ssh-woomera.py -- uname -a  # run a command
"""

from __future__ import annotations

import argparse
import ipaddress
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

# ICMP on a LAN answers in well under a millisecond; 1 s is ~3 orders of magnitude
# over that. On expiry: host treated as down and skipped (full scan then runs).
PING_TIMEOUT_S = 1

CACHE = Path("tmp/woomera-addr")  # regenerable: last address that answered


def macs_of(ip: str) -> list[str]:
    subprocess.run(
        ["ping", "-c", "1", "-W", str(PING_TIMEOUT_S), ip],
        capture_output=True,
        check=False,
    )
    out = subprocess.run(
        ["ip", "neigh", "show", ip], capture_output=True, text=True, check=False
    ).stdout
    # One entry PER HOST NIC on a shared subnet, and a stale one can name a
    # different port of the same box. Match if ANY entry is the 1G MAC rather
    # than taking the first - one stale row must not hide a good one.
    return [m.lower() for m in re.findall(r"lladdr ([0-9a-f:]{17})", out)]


# The 1G RJ45 (enp0s1), from the NOR identity blob at 0x1f0000. Match this
# EXACTLY, not just the OUI: the 10G port is base+1 and other UBNT gear shares
# the OUI, so an OUI match picked the 10G port (.127) - unroutable from here -
# while the box answered on the 1G port (.136).
WOOMERA_MAC_1G = "74:ac:b9:41:a8:11"


def is_woomera(ip: str) -> bool:
    return WOOMERA_MAC_1G in macs_of(ip)


def scan(subnet: str) -> str | None:
    hosts = [str(h) for h in ipaddress.ip_network(subnet).hosts()]
    with ThreadPoolExecutor(max_workers=64) as pool:
        for ip, hit in zip(hosts, pool.map(is_woomera, hosts)):
            if hit:
                return ip
    return None


def locate(subnet: str) -> str | None:
    if CACHE.exists():  # fast path: lease usually has not moved
        cached = CACHE.read_text().strip()
        if cached and is_woomera(cached):
            return cached
        print(f"# {cached} is no longer woomera, rescanning {subnet}", file=sys.stderr)
    found = scan(subnet)
    if found:
        CACHE.parent.mkdir(parents=True, exist_ok=True)
        CACHE.write_text(found + "\n")
    return found


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--subnet", default="192.168.25.0/24")
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

    print(f"# woomera at {ip}", file=sys.stderr)
    os.execvp(
        "ssh",
        [
            "ssh",
            "-o",
            "StrictHostKeyChecking=accept-new",
            f"{args.user}@{ip}",
            *args.cmd,
        ],
    )


if __name__ == "__main__":
    sys.exit(main())
