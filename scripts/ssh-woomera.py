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

# Ubiquiti Networks OUIs (subset covering current gear; extend as needed).
UBNT_OUIS = {
    "00:15:6d",
    "00:27:22",
    "04:18:d6",
    "18:e8:29",
    "24:5a:4c",
    "44:d9:e7",
    "68:72:51",
    "74:83:c2",
    "74:ac:b9",
    "78:8a:20",
    "80:2a:a8",
    "b4:fb:e4",
    "dc:9f:db",
    "e0:63:da",
    "f0:9f:c2",
    "fc:ec:da",
}

# ICMP on a LAN answers in well under a millisecond; 1 s is ~3 orders of magnitude
# over that. On expiry: host treated as down and skipped (full scan then runs).
PING_TIMEOUT_S = 1

CACHE = Path("tmp/woomera-addr")  # regenerable: last address that answered


def mac_of(ip: str) -> str | None:
    subprocess.run(
        ["ping", "-c", "1", "-W", str(PING_TIMEOUT_S), ip], capture_output=True
    )
    out = subprocess.run(
        ["ip", "neigh", "show", ip], capture_output=True, text=True
    ).stdout
    m = re.search(r"lladdr ([0-9a-f:]{17})", out)
    return m.group(1).lower() if m else None


def is_woomera(ip: str) -> bool:
    mac = mac_of(ip)
    return bool(mac) and mac[:8] in UBNT_OUIS


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
