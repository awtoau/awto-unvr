#!/usr/bin/env python3
"""Continuously watch 1G (enp0s1) + 10G (enp0s2) link state on woomera.

One persistent SSH connection runs a remote poll loop (passive sysfs reads,
does NOT cycle the interface) and streams a line per port per tick back here.
Mirrors the console-log pattern (dev.py console-status/-peek): tees to
tmp/logs/eth-watch.log so you can `tail -f` it same as the serial console log,
while this process also prints live to stdout.

  ./scripts/eth-watch.py            # runs until Ctrl-C
  tail -f tmp/logs/eth-watch.log    # from another terminal, to just watch
"""
from __future__ import annotations

import argparse
import datetime
import subprocess
import sys
from pathlib import Path

DEFAULT_ROOT_PASSWORD = "unvr"  # documented default, see docs/fedora-on-ssd.md
LOG = Path("tmp/logs/eth-watch.log")

# 2s: fast enough to see a link transition without much delay, slow enough not
# to spam the log/SSH session. Passive sysfs reads only - no interface cycling,
# so any interval here is cheap; this is just a human-watchable refresh rate.
POLL_INTERVAL_S = 2

REMOTE_LOOP = f"""
while true; do
  for IF in enp0s1 enp0s2; do
    SPEED=$(cat /sys/class/net/$IF/speed 2>/dev/null || echo "?")
    DUPLEX=$(cat /sys/class/net/$IF/duplex 2>/dev/null || echo "?")
    CARRIER=$(cat /sys/class/net/$IF/carrier 2>/dev/null || echo "0")
    OPER=$(cat /sys/class/net/$IF/operstate 2>/dev/null || echo "?")
    echo "$IF speed=${{SPEED}}Mb duplex=$DUPLEX carrier=$CARRIER oper=$OPER"
  done
  sleep {POLL_INTERVAL_S}
done
"""


def locate_woomera() -> str:
    out = subprocess.run(
        [sys.executable, "scripts/ssh-woomera.py", "--print"],
        capture_output=True, text=True, check=True,
    )
    return out.stdout.strip()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--host", help="woomera's address (default: auto-locate by MAC OUI)")
    ap.add_argument("--password", default=DEFAULT_ROOT_PASSWORD)
    args = ap.parse_args()

    host = args.host or locate_woomera()
    print(f"# woomera at {host} - polling every {POLL_INTERVAL_S}s, Ctrl-C to stop", file=sys.stderr)

    LOG.parent.mkdir(parents=True, exist_ok=True)
    proc = subprocess.Popen(
        ["sshpass", "-p", args.password, "ssh",
         "-o", "ConnectTimeout=8", "-o", "StrictHostKeyChecking=accept-new",
         "-o", "PreferredAuthentications=password", "-o", "PubkeyAuthentication=no",
         f"root@{host}", REMOTE_LOOP],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1,
    )

    try:
        with LOG.open("a") as f:
            for line in proc.stdout:
                stamp = datetime.datetime.now().astimezone().isoformat(timespec="seconds")
                out = f"{stamp} {line.rstrip()}"
                print(out, flush=True)
                f.write(out + "\n")
                f.flush()
    except KeyboardInterrupt:
        pass
    finally:
        proc.terminate()
    return 0


if __name__ == "__main__":
    sys.exit(main())
