#!/usr/bin/env python3
"""Survey the LAN for woomera without the serial console.

Discriminator is the box's exact MACs (_box.WOOMERA_MACS), not the SSH
banner: Fedora does not tag its banner, so `SSH-2.0-OpenSSH_10.2` is
indistinguishable from any other modern distro. Banner is still reported as
a hint. Every live host is listed, so this doubles as a LAN survey.

Found the box at .149 when the docs said .140 - see
docs/unvr-access-research.md.
"""

from __future__ import annotations

import argparse
import ipaddress
import logging
import re
import socket
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from _box import WOOMERA_MACS
from _net import LAN_SUBNET

# TCP connect on a LAN is sub-millisecond and sshd sends its banner immediately on
# accept; ICMP likewise. 2.0 s is ~3 orders of magnitude over that - generous for a
# host that is up but loaded. On expiry: host reported as down/no-banner, not retried.
PROBE_TIMEOUT_S = 2.0

log = logging.getLogger("find-woomera")


def ping(ip: str) -> bool:
    """Populate the ARP cache so the MAC lookup below has something to read."""
    return (
        subprocess.run(
            ["ping", "-c", "1", "-W", str(int(PROBE_TIMEOUT_S)), ip],
            capture_output=True,
            check=False,
        ).returncode
        == 0
    )


def mac_of(ip: str) -> str | None:
    out = subprocess.run(
        ["ip", "neigh", "show", ip], capture_output=True, text=True, check=False
    ).stdout
    m = re.search(r"lladdr ([0-9a-f:]{17})", out)
    return m.group(1).lower() if m else None


def banner(ip: str, port: int = 22) -> str | None:
    try:
        with socket.create_connection((ip, port), PROBE_TIMEOUT_S) as s:
            s.settimeout(PROBE_TIMEOUT_S)
            return s.recv(256).decode("utf-8", "replace").split("\n")[0].strip()
    except OSError:
        return None


def survey(ip: str) -> tuple[str, str | None, str | None] | None:
    if not ping(ip):
        return None
    return ip, mac_of(ip), banner(ip)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--subnet", default=LAN_SUBNET)
    ap.add_argument("--workers", type=int, default=64)
    args = ap.parse_args()

    logdir = Path("tmp/logs")
    logdir.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(message)s",
        handlers=[
            logging.FileHandler(logdir / "find-woomera-ssh.log"),
            logging.StreamHandler(sys.stdout),
        ],
    )

    hosts = [str(h) for h in ipaddress.ip_network(args.subnet).hosts()]
    log.info("probing %d hosts in %s", len(hosts), args.subnet)

    with ThreadPoolExecutor(max_workers=args.workers) as pool:
        live = [r for r in pool.map(survey, hosts) if r]

    found = []
    for ip, mac, ban in sorted(live, key=lambda t: ipaddress.ip_address(t[0])):
        is_box = mac in WOOMERA_MACS
        if is_box:
            found.append(ip)
        log.info(
            "  %-15s %-17s %-28s%s",
            ip,
            mac or "-",
            ban or "(no ssh)",
            "  <-- woomera" if is_box else "",
        )

    log.info("%d host(s) up", len(live))
    if found:
        log.info("RESULT: woomera at %s", found)
        log.info("  try: ssh root@%s", found[0])
        return 0
    log.info("RESULT: neither woomera MAC on this subnet - box is off or elsewhere")
    return 1


if __name__ == "__main__":
    sys.exit(main())
