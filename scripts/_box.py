#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Where is woomera on the LAN? ONE resolver, imported - never shelled out to.

- Resolve by MAC, never by IP: the DHCP lease moves (.149, .106, .129, .136,
  .140 so far) and a stale address fails "refused", not "timeout".
- EXACT MACs, never the OUI: other UBNT gear shares 74:AC:B9, and an OUI
  match once picked a neighbouring device. docs/identity-partitions.md.
- BOTH MACs: all four host NICs answer ARP for any local IP (arp_ignore=0)
  and both box ports sit on one subnet (#170), so `ip neigh` routinely
  names the 10G port's MAC for an IP the 1G port also answers.
- Does NOT import _repo on purpose: ssh-woomera.py is run by tools outside
  dev.py and must not trip the direct-invocation guard.

    from _box import locate, require, ssh_argv
"""

from __future__ import annotations

import ipaddress
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

# The two al_eth MACs from the NOR identity blob at 0x1f0000: base+1 is the
# 1G RJ45 (eth0), base+2 the 10G SFP+ (eth1).
MAC_1G = "74:ac:b9:41:a8:11"
MAC_10G = "74:ac:b9:41:a8:12"
WOOMERA_MACS = frozenset({MAC_1G, MAC_10G})

DEFAULT_SUBNET = "192.168.25.0/24"

# ICMP on a LAN answers in well under a millisecond; 1 s is ~3 orders of
# magnitude over that. On expiry: host treated as down and skipped.
PING_TIMEOUT_S = 1

# Regenerable: last address that answered. Repo-relative, not cwd-relative.
CACHE = Path(__file__).resolve().parent.parent / "tmp" / "woomera-addr"

# ssh options for an interactive/long-lived session to the box. Keepalives
# because the box has two NICs on one subnet and picks its egress per reply
# (#170): 15 s x 8 = 2 min before a quiet session is called dead, which
# outlasts a reboot's link flap. ConnectTimeout=8: fail fast on stale ARP.
SSH_OPTS_INTERACTIVE = [
    "-o",
    "StrictHostKeyChecking=accept-new",
    "-o",
    "ServerAliveInterval=15",
    "-o",
    "ServerAliveCountMax=8",
    "-o",
    "ConnectTimeout=8",
    "-o",
    "TCPKeepAlive=yes",
]


def macs_of(ip: str) -> list[str]:
    """Every neighbour-table MAC for `ip`, lowercase, after one ping to
    populate it. All rows, not the first: a stale row can name a different
    port of the same box and must not hide a good one."""
    subprocess.run(
        ["ping", "-c", "1", "-W", str(PING_TIMEOUT_S), ip],
        capture_output=True,
        check=False,
    )
    out = subprocess.run(
        ["ip", "neigh", "show", ip], capture_output=True, text=True, check=False
    ).stdout
    return [m.lower() for m in re.findall(r"lladdr ([0-9a-f:]{17})", out)]


def is_woomera(ip: str) -> bool:
    return bool(WOOMERA_MACS.intersection(macs_of(ip)))


def scan(subnet: str = DEFAULT_SUBNET) -> str | None:
    hosts = [str(h) for h in ipaddress.ip_network(subnet).hosts()]
    with ThreadPoolExecutor(max_workers=64) as pool:
        for ip, hit in zip(hosts, pool.map(is_woomera, hosts)):
            if hit:
                return ip
    return None


def locate(subnet: str = DEFAULT_SUBNET) -> str | None:
    """The box's current address, or None. Cached last-known address is
    re-verified by MAC before use; a full scan runs only when it moved."""
    if CACHE.exists():
        cached = CACHE.read_text().strip()
        if cached and is_woomera(cached):
            return cached
        print(f"# {cached} is no longer woomera, rescanning {subnet}", file=sys.stderr)
    found = scan(subnet)
    if found:
        CACHE.parent.mkdir(parents=True, exist_ok=True)
        CACHE.write_text(found + "\n")
    return found


def require(subnet: str = DEFAULT_SUBNET, *, hint: str = "is it up?") -> str:
    """locate(), or exit 1 with a one-line reason."""
    ip = locate(subnet)
    if not ip:
        sys.exit(f"woomera not found on the LAN - {hint}")
    return ip


def flush_failed_neighbours(ip: str) -> None:
    """Drop FAILED/INCOMPLETE neighbour rows for `ip` so ssh re-ARPs. A bad
    row on the NIC the route picks gives "No route to host" while the box
    is up and other NICs resolved it fine (#170)."""
    for line in subprocess.run(
        ["ip", "neigh", "show", ip], capture_output=True, text=True, check=False
    ).stdout.splitlines():
        if "FAILED" in line or "INCOMPLETE" in line:
            dev = line.split(" dev ", 1)[1].split()[0] if " dev " in line else None
            if dev:
                subprocess.run(
                    ["ip", "neigh", "del", ip, "dev", dev],
                    capture_output=True,
                    check=False,
                )


def ssh_argv(ip: str, user: str = "root", cmd: list[str] | tuple = ()) -> list[str]:
    """The ssh command line ssh-woomera.py runs: keepalive session as `user`,
    optionally running `cmd` instead of a shell."""
    return ["ssh", *SSH_OPTS_INTERACTIVE, f"{user}@{ip}", *cmd]
