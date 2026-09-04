#!/usr/bin/env python3
"""Continuously watch 1G (enp0s1) + 10G (enp0s2) link state + SFP DDM on woomera.

One persistent SSH connection runs a remote poll loop (passive sysfs + i2c
reads, does NOT cycle the interface) and streams a line per port per tick
back here, plus a line of SFP+ DDM telemetry (TX bias/power, RX power,
status bits - see #111). Mirrors the console-log pattern (dev.py
console-status/-peek): tees to tmp/logs/eth-watch.log so you can `tail -f`
it same as the serial console log, while this process also prints live to
stdout. Meant to run continuously during #111 SerDes-EQ bring-up so a
module dropout (TX_FAULT/RX_LOS) is caught with a timestamp, not inferred
after the fact.

  ./scripts/eth-watch.py            # runs until Ctrl-C
  tail -f tmp/logs/eth-watch.log    # from another terminal, to just watch
"""

from __future__ import annotations

import argparse
import datetime
import subprocess
import sys
from pathlib import Path

import _box

DEFAULT_ROOT_PASSWORD = "unvr"  # documented default, see docs/fedora-on-ssd.md
LOG = Path("tmp/logs/eth-watch.log")

# 2s: fast enough to see a link transition without much delay, slow enough not
# to spam the log/SSH session. Passive sysfs reads only - no interface cycling,
# so any interval here is cheap; this is just a human-watchable refresh rate.
POLL_INTERVAL_S = 2

# SFP+ DDM: bus 2 / mux ch1, addr 0x51 (docs/i2c-map.md). TX bias=100-101,
# TX power=102-103, RX power=104-105, status=110 (SFF-8472 sec 9.11).
REMOTE_LOOP = f"""
read_be16() {{
  hi=$(i2cget -y 2 0x51 "$1" b 2>/dev/null)
  lo=$(i2cget -y 2 0x51 "$(( $1 + 1 ))" b 2>/dev/null)
  if [ -z "$hi" ] || [ -z "$lo" ]; then echo ""; else echo "$(( hi * 256 + lo ))"; fi
}}
while true; do
  for IF in enp0s1 enp0s2; do
    SPEED=$(cat /sys/class/net/$IF/speed 2>/dev/null || echo "?")
    DUPLEX=$(cat /sys/class/net/$IF/duplex 2>/dev/null || echo "?")
    CARRIER=$(cat /sys/class/net/$IF/carrier 2>/dev/null || echo "0")
    OPER=$(cat /sys/class/net/$IF/operstate 2>/dev/null || echo "?")
    echo "$IF speed=${{SPEED}}Mb duplex=$DUPLEX carrier=$CARRIER oper=$OPER"
  done
  BIAS=$(read_be16 100)
  TXP=$(read_be16 102)
  RXP=$(read_be16 104)
  STATUS=$(i2cget -y 2 0x51 110 b 2>/dev/null)
  if [ -n "$BIAS" ] && [ -n "$TXP" ] && [ -n "$RXP" ]; then
    awk -v bias="$BIAS" -v txp="$TXP" -v rxp="$RXP" -v st="$STATUS" 'BEGIN {{
      txmw = txp * 0.1 / 1000; rxmw = rxp * 0.1 / 1000
      txdbm = (txmw <= 0) ? -40 : 10 * log(txmw) / log(10)
      rxdbm = (rxmw <= 0) ? -40 : 10 * log(rxmw) / log(10)
      printf "sfp ddm: bias=%.2fmA tx=%.2fdBm rx=%.2fdBm status=%s\\n", bias * 2 / 1000, txdbm, rxdbm, st
    }}'
  else
    echo "sfp ddm: read failed (module absent / i2c-dev not loaded / bus busy)"
  fi
  sleep {POLL_INTERVAL_S}
done
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
    print(
        f"# woomera at {host} - polling every {POLL_INTERVAL_S}s, Ctrl-C to stop",
        file=sys.stderr,
    )

    LOG.parent.mkdir(parents=True, exist_ok=True)
    proc = subprocess.Popen(
        [
            *_box.sshpass_argv(args.password, connect_timeout=8),
            f"root@{host}",
            REMOTE_LOOP,
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )

    try:
        with LOG.open("a") as f:
            for line in proc.stdout:
                stamp = (
                    datetime.datetime.now().astimezone().isoformat(timespec="seconds")
                )
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
