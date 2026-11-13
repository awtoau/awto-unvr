#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Receive woomera's kernel log over netconsole (UDP) - the console when no serial is attached.

- Box side: `netconsole=6665@<box ip>/eth0,6666@<host ip>/<host mac>` (truenas-install maint profile).
- Opens UDP 6666 in firewalld (runtime + permanent, same helper as tftp), appends every
  datagram to tmp/logs/netconsole.log with a host timestamp. Runs until killed.

  ./dev.py netconsole-listen [--port 6666]
"""

from __future__ import annotations

import argparse
import os
import socket
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _firewall import ensure_tftp_open  # noqa: E402
from _repo import LOGS, make_log  # noqa: E402

log = make_log("netconsole-listen")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--port", type=int, default=6666)
    a = ap.parse_args()
    os.environ.setdefault("AWTO_FIREWALL_APPLY", "1")
    ensure_tftp_open(port=a.port, apply=True, log=log)
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("0.0.0.0", a.port))
    out = LOGS / "netconsole.log"
    log(f"listening on UDP {a.port} -> {out}")
    with open(out, "a", buffering=1) as f:
        while True:
            data, src = s.recvfrom(65535)
            ts = time.strftime("%Y-%m-%dT%H:%M:%S%z")
            for line in data.decode(errors="replace").splitlines():
                f.write(f"{ts} {src[0]} {line}\n")


if __name__ == "__main__":
    sys.exit(main())
