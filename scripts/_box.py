#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Where is woomera on the LAN? ONE resolver, imported - never shelled out to.

- Resolve by MAC, never by IP: the DHCP lease moves (.149, .106, .129, .136,
  .140 so far) and a stale address fails "refused", not "timeout".
- The MACs live here and only here (WOOMERA_MACS below); a guard test
  fails on any other copy.
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

from _net import LAN_SUBNET

# The box's four NICs. The two al_eth MACs come from the NOR identity blob at 0x1f0000
# (docs/identity-partitions.md). Stock's `mac: [base] + [2]` is a COUNT of two
# allocated MACs (#222/#223): base+0 = 1G RJ45 (eth0), base+1 = 10G SFP+.
# EXACT MACs, never the OUI: other UBNT gear shares 74:AC:B9 and an OUI match
# once picked a neighbouring device. BOTH ports: all four host NICs answer ARP
# for any local IP and both box ports sit on one subnet (#170), so `ip neigh`
# routinely names the 10G MAC for an IP the 1G port also answers.
MAC_1G = "74:ac:b9:41:a8:11"
MAC_10G = "74:ac:b9:41:a8:12"
# The two r8152 USB NICs (Realtek OUIs, read off the box with `ip -o link`).
# The fallback when both al_eth ports are down - CLAUDE.md's old ".100".
MAC_USB1 = "60:7d:09:4c:39:7b"  # enP1p1s0u1u1
MAC_USB2 = "80:6d:97:13:a5:6b"  # enP1p1s0u1u3
WOOMERA_MACS = frozenset({MAC_1G, MAC_10G, MAC_USB1, MAC_USB2})

# ICMP on a LAN answers in well under a millisecond; 1 s is ~3 orders of
# magnitude over that. On expiry: host treated as down and skipped.
PING_TIMEOUT_S = 1

# Regenerable: last address that answered. Repo-relative, not cwd-relative.
CACHE = Path(__file__).resolve().parent.parent / "tmp" / "woomera-addr"

# ssh option sets - exactly three, each with a reason. Scripts build their
# command lines from these; a guard test fails on any other "-o Strict..."
# literal.
#
# Interactive/long-lived session (./dev.py ssh). Keepalives because the box
# has two NICs on one subnet and picks its egress per reply (#170): 15 s x 8 =
# 2 min before a quiet session is called dead, outlasting a reboot's link
# flap. ConnectTimeout=8: fail fast on stale ARP.
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
# One-shot command or scp from a script, key auth: never prompt (BatchMode),
# fail fast, and no host-key bookkeeping - the box's key changes on every
# rootfs rebuild, and a stale known_hosts row must not block a deploy.
SSH_OPTS_BATCH = [
    "-o",
    "BatchMode=yes",
    "-o",
    "ConnectTimeout=8",
    "-o",
    "StrictHostKeyChecking=no",
    "-o",
    "UserKnownHostsFile=/dev/null",
    "-o",
    "LogLevel=ERROR",
]
# Password auth through sshpass - the box's documented lab root password
# (docs/fedora-on-ssd.md), for hosts with no key installed yet.
SSH_OPTS_PASSWORD = [
    "-o",
    "StrictHostKeyChecking=accept-new",
    "-o",
    "PreferredAuthentications=password",
    "-o",
    "PubkeyAuthentication=no",
]
# rsync -e / string form of the same.
SSH_PASSWORD_E = " ".join(["ssh", *SSH_OPTS_PASSWORD])


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


def scan(subnet: str = LAN_SUBNET) -> str | None:
    hosts = [str(h) for h in ipaddress.ip_network(subnet).hosts()]
    with ThreadPoolExecutor(max_workers=64) as pool:
        for ip, hit in zip(hosts, pool.map(is_woomera, hosts)):
            if hit:
                return ip
    return None


def locate(subnet: str = LAN_SUBNET) -> str | None:
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


def require(subnet: str = LAN_SUBNET, *, hint: str = "is it up?") -> str:
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


def ssh_argv(
    ip: str, user: str = "root", cmd: list[str] | tuple = (), *, batch: bool = False
) -> list[str]:
    """ssh command line to the box as `user`, optionally running `cmd`.
    batch=True: SSH_OPTS_BATCH (scripted one-shot); default: the keepalive
    session ssh-woomera.py execs."""
    opts = SSH_OPTS_BATCH if batch else SSH_OPTS_INTERACTIVE
    return ["ssh", *opts, f"{user}@{ip}", *cmd]


def sshpass_argv(
    password: str, *, prog: str = "ssh", connect_timeout: int | None = None
) -> list[str]:
    """`sshpass -p <pw> <prog> [-o ConnectTimeout=N] <SSH_OPTS_PASSWORD>` -
    the caller appends the target (root@host, or root@host:path for scp)."""
    ct = ["-o", f"ConnectTimeout={connect_timeout}"] if connect_timeout else []
    return ["sshpass", "-p", password, prog, *ct, *SSH_OPTS_PASSWORD]
