#!/usr/bin/env python3
"""Report 1G (enp0s1) and 10G SFP+ (enp0s2) link status on woomera over SSH.

For the 10G port, briefly enables al_eth dynamic debug and cycles the
interface to capture the link-manage/SFP-read trace (see #98), then turns
dynamic debug back off. Read-only otherwise. Run it yourself to watch live:
  ./scripts/eth-link-status.py
"""

from __future__ import annotations

import argparse
import datetime
import subprocess
import sys
from pathlib import Path

import _box

# Documented default (docs/fedora-on-ssd.md) - not a secret, this box's own lab
# root password. Override with --password if it's been changed.
DEFAULT_ROOT_PASSWORD = "unvr"

LOG = Path("tmp/logs/eth-link-status.log")

REMOTE_SCRIPT = r"""
set -e
echo "=== ip -br link ==="
ip -br link
echo
echo "=== ip -br addr ==="
ip -br addr
echo
for IF in enp0s1 enp0s2; do
  echo "=== $IF sysfs ==="
  for F in speed duplex carrier operstate; do
    V=$(cat /sys/class/net/$IF/$F 2>/dev/null || echo "n/a")
    echo "  $F: $V"
  done
done
echo
echo "=== enp0s2 (10G) link-manage trace ==="
echo module al_eth +p > /sys/kernel/debug/dynamic_debug/control 2>/dev/null || true
dmesg -c >/dev/null
ip link set enp0s2 down
ip link set enp0s2 up
dmesg | grep -iE "enp0s2|al_eth_group_lm|al_eth_lm|sfp|serdes" || echo "  (no driver trace output - see #98 for why)"
echo module al_eth -p > /sys/kernel/debug/dynamic_debug/control 2>/dev/null || true
"""


def locate_woomera() -> str:
    return _box.require()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--host", help="woomera's address (default: auto-locate by MAC OUI)"
    )
    ap.add_argument("--password", default=DEFAULT_ROOT_PASSWORD)
    args = ap.parse_args()

    host = args.host or locate_woomera()
    print(f"# woomera at {host}", file=sys.stderr)

    result = subprocess.run(
        [
            *_box.sshpass_argv(args.password, connect_timeout=8),
            f"root@{host}",
            REMOTE_SCRIPT,
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    output = result.stdout + result.stderr

    LOG.parent.mkdir(parents=True, exist_ok=True)
    stamp = datetime.datetime.now().astimezone().isoformat(timespec="seconds")
    with LOG.open("a") as f:
        f.write(f"\n=== {stamp} ===\n{output}\n")

    print(output)
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
