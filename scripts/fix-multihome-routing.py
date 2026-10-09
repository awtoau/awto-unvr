#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Make woomera reply on the port a packet arrived on, not whatever the route picks (#170).

All 4 NICs sit on one /24 with DHCP leases. Linux's default weak-host
behaviour means an address is answered by whichever interface the route table
prefers - so enp0s1's address gets answered from the 10G MAC, iperf3
`-B <addr>` measures a different port than intended, and `tcpdump` on a MAC
sees nothing. Three wrong conclusions have come from this.

Two changes, both needed:

1. **Strict ARP** - each port only answers ARP for addresses it owns, and
   always sources ARP from the address on the outgoing interface:
     arp_ignore=1, arp_announce=2
   Without this the switch's MAC table maps all four addresses to whichever
   port replied first.

2. **Per-interface source routing** - one table per NIC plus an `ip rule`
   matching that NIC's source address, so a reply from .133 leaves enp0s1.
   The single `main` table cannot express this: it has one preferred route
   per destination regardless of source.

`rp_filter` is deliberately left at 0: with per-interface tables in place,
strict reverse-path would drop legitimate asymmetric traffic during a link
flap, and the ARP changes already stop the address confusion.

Idempotent - re-running replaces the rules and tables it owns. Nothing else
is touched; DHCP keeps managing the addresses.

    ./scripts/fix-multihome-routing.py            # show what would change
    ./scripts/fix-multihome-routing.py --apply    # apply now (not persistent)
    ./scripts/fix-multihome-routing.py --persist  # + NetworkManager dispatcher
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

from _box import require, ssh_argv  # noqa: E402
from _net import LAN_SUBNET  # noqa: E402
from _repo import make_log  # noqa: E402

log = make_log("fix-multihome-routing")

# Subnet from the one home (#260). Gateway is read off the box rather than
# hardcoded: it is a property of the LAN, not of this repo.
SUBNET = LAN_SUBNET

# Table ids per NIC. 100+ to stay clear of the reserved local/main/default.
TABLES = {
    "enp0s1": 101,
    "enp0s2": 102,
    "enP1p1s0u1u1": 103,
    "enP1p1s0u1u3": 104,
}

# Written by --persist. NetworkManager runs dispatcher scripts on every
# interface up/down, which is what re-applies this after a DHCP renew or a
# link flap - a sysctl.d file alone would not restore the rules/tables.
DISPATCHER = "/etc/NetworkManager/dispatcher.d/90-awto-multihome"


def box_sh(host: str, cmd: str, timeout: int = 60) -> str:
    """Run cmd on the box. Timeout: these are ip/sysctl calls that return in
    milliseconds; 60s is ~1000x and only bounds a hung ssh. On expiry the
    caller sees empty output and logs it."""
    try:
        r = subprocess.run(
            ssh_argv(host, cmd=[cmd]),
            capture_output=True,
            text=True,
            check=False,
            timeout=timeout,
        )
        return r.stdout.strip()
    except subprocess.TimeoutExpired:
        log(f"TIMEOUT after {timeout}s: {cmd[:60]}")
        return ""


def live_addrs(host: str) -> dict[str, str]:
    """iface -> its IPv4, for the NICs in TABLES that are up with an address."""
    out = {}
    raw = box_sh(host, "ip -4 -br addr show")
    for line in raw.splitlines():
        f = line.split()
        if len(f) >= 3 and f[0] in TABLES and "UP" in f[1]:
            out[f[0]] = f[2].split("/")[0]
    return out


def plan(addrs: dict[str, str], gateway: str) -> list[str]:
    """The commands that make replies leave the ingress port."""
    cmds = [
        # Strict ARP, system-wide. all.* is the floor; per-iface values are
        # max(all, iface) for these two, so setting all is sufficient.
        "sysctl -qw net.ipv4.conf.all.arp_ignore=1",
        "sysctl -qw net.ipv4.conf.all.arp_announce=2",
    ]
    for iface, ip in sorted(addrs.items()):
        t = TABLES[iface]
        cmds += [
            f"ip route flush table {t} 2>/dev/null || true",
            f"ip route add {SUBNET} dev {iface} src {ip} table {t}",
            f"ip route add default via {gateway} dev {iface} table {t}",
            f"ip rule del from {ip} table {t} 2>/dev/null || true",
            f"ip rule add from {ip} table {t}",
        ]
    return cmds


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--apply", action="store_true", help="apply now (not persistent)")
    ap.add_argument("--persist", action="store_true", help="apply + survive reboot")
    a = ap.parse_args()

    host = require(hint="is woomera up?")
    addrs = live_addrs(host)
    if not addrs:
        log("no NICs from the table are up with an address - nothing to do")
        return 1
    log(f"box {host}; NICs: " + ", ".join(f"{k}={v}" for k, v in sorted(addrs.items())))

    gw = box_sh(host, "ip route show default | head -1 | awk '{print $3}'")
    if not gw:
        log("no default gateway on the box - cannot build per-iface tables")
        return 1
    log(f"gateway {gw}")
    cmds = plan(addrs, gw)
    if not (a.apply or a.persist):
        log("DRY RUN - would run on the box:")
        for c in cmds:
            log(f"  {c}")
        log("pass --apply (now) or --persist (survives reboot)")
        return 0

    box_sh(host, " && ".join(cmds), timeout=120)
    log("applied")

    if a.persist:
        body = "\n".join(
            [
                "#!/bin/sh",
                "# awto-unvr #170: reply on the ingress port. Re-applied on every",
                "# interface up, so a DHCP renew or link flap cannot undo it.",
                '[ "$2" = "up" ] || [ "$2" = "dhcp4-change" ] || exit 0',
                *cmds,
            ]
        )
        # Heredoc via ssh: the dispatcher must be 0755 and root-owned or
        # NetworkManager silently skips it.
        box_sh(
            host,
            f"cat > {DISPATCHER} <<'EOF'\n{body}\nEOF\n"
            f"chmod 0755 {DISPATCHER} && chown root:root {DISPATCHER} && "
            f"echo ok",
            timeout=120,
        )
        log(f"persisted via {DISPATCHER}")

    log("--- verify:")
    log(box_sh(host, "ip rule show | head -8"))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
