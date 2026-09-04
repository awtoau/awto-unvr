#!/usr/bin/env python3
"""Open a shell on woomera over the 1G port - a second terminal beside the
serial console.

The serial console is a single shared resource: tio owns the port, the log is
the record of the boot, and typing into it competes with whatever else is
watching. SSH gives an independent session that does not disturb it, and it is
far faster than 115200 for anything that produces output.

The box's address is NOT fixed. DHCP has handed out .136, .140 and .129 to the
1G port across this project, so the IP is discovered from the 1G MAC rather
than assumed - the MAC is stable (NOR EEPROM 0x1f0000), the lease is not.

Run: ./dev.py ssh                  # interactive shell
     ./dev.py ssh -- uname -a      # run one command and exit
"""

from __future__ import annotations

import os
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

# 1G port (enp0s1), base MAC from the NOR identity blob. The 10G port is
# base+2; we use the 1G one because it is the always-connected RJ45.
MAC_1G = "74:ac:b9:41:a8:11"
SUBNET = "192.168.25"
USER = "root"

SSH_OPTS = [
    "-o",
    "StrictHostKeyChecking=no",
    "-o",
    "UserKnownHostsFile=/dev/null",
    "-o",
    "LogLevel=ERROR",
    # 5s: a LAN box either answers in well under a second or is not there.
    # Long enough to absorb a retransmit, short enough that a down box reports
    # rather than hangs.
    "-o",
    "ConnectTimeout=5",
]


def ip_from_arp() -> str | None:
    """Find the box's IP from the neighbour table by its stable 1G MAC."""
    out = subprocess.run(
        ["ip", "neigh", "show"], capture_output=True, text=True, check=False
    ).stdout
    for line in out.splitlines():
        if MAC_1G in line.lower():
            m = re.match(rf"({re.escape(SUBNET)}\.\d+)\s", line)
            if m:
                return m.group(1)
    return None


def ip_from_sweep() -> str | None:
    """Populate the neighbour table, then look again.

    `ip neigh` only knows hosts this machine has talked to recently. One ping
    per address primes it. -c1 -W1: a LAN host replies in ~1 ms, so 1 s is
    already 1000x headroom; the sweep is 254 hosts in parallel.
    """
    procs = [
        subprocess.Popen(
            ["ping", "-c1", "-W1", f"{SUBNET}.{i}"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        for i in range(1, 255)
    ]
    for p in procs:
        p.wait()
    return ip_from_arp()


def main() -> int:
    argv = sys.argv[1:]
    ip = ip_from_arp() or ip_from_sweep()
    if not ip:
        print(
            f"woomera not found on {SUBNET}.0/24 (looking for 1G MAC {MAC_1G}).\n"
            "Box powered off, or on a different subnet - check the serial console.",
            file=sys.stderr,
        )
        return 2
    print(f"woomera: {ip} (1G, {MAC_1G})", file=sys.stderr)
    return subprocess.run(
        ["ssh", *SSH_OPTS, f"{USER}@{ip}", *argv], check=False
    ).returncode


if __name__ == "__main__":
    raise SystemExit(main())
