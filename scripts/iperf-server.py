#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Install/manage the always-on iperf3 server on the DEV HOST for woomera tests.

A persistent server removes the "is the server even up" failure from every
throughput measurement - ad-hoc `iperf3 -s -D` died with its shell and left
runs reporting a connect timeout that looked like a box-side fault.

Listens on ALL interfaces on port 5701. The dev host has 1G enp5s0 and 10G
enp7s0 on one /24, and woomera has 4 NICs on the same subnet - which path is
measured is chosen CLIENT-side:

    ./dev.py ssh -- "iperf3 -c <dev-host> -p 5701 --bind-dev enp0s1"   # 1G
    ./dev.py ssh -- "iperf3 -c <dev-host> -p 5701 --bind-dev enp0s2 -P 4"   # 10G

`--bind-dev`, never `-B <addr>`: weak-host routing means a source address does
not select an egress port, and `-B` has produced wrong numbers twice (#170).
One A57 core cannot saturate 10GbE, so 10G needs `-P 4` (#241).

    ./scripts/iperf-server.py            # status
    ./scripts/iperf-server.py --install  # write the unit, enable, start
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

from _repo import make_log  # noqa: E402

log = make_log("iperf-server")

UNIT = "iperf3-awto.service"
UNIT_PATH = Path.home() / ".config/systemd/user" / UNIT
PORT = 5701

UNIT_TEXT = f"""[Unit]
Description=iperf3 server for awto-unvr throughput tests (woomera)
Documentation=https://github.com/awtoau/awto-unvr/issues/241
After=network-online.target

[Service]
# All interfaces: which path is measured is a client-side --bind-dev choice,
# and binding one address would not constrain it anyway (#170).
ExecStart=/usr/bin/iperf3 --server --port {PORT}
Restart=always
RestartSec=5

[Install]
WantedBy=default.target
"""


def sc(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run(
        ["systemctl", "--user", *args], capture_output=True, text=True, check=False
    )


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--install", action="store_true", help="write unit, enable, start")
    a = ap.parse_args()

    if a.install:
        UNIT_PATH.parent.mkdir(parents=True, exist_ok=True)
        UNIT_PATH.write_text(UNIT_TEXT)
        log(f"wrote {UNIT_PATH}")
        sc("daemon-reload")
        sc("enable", UNIT)
        sc("start", UNIT)
        # Linger: without it the unit stops at logout and is absent after a
        # reboot, which is exactly the "server not up" failure this removes.
        r = subprocess.run(
            ["loginctl", "show-user", os.environ.get("USER", "dan"), "-p", "Linger"],
            capture_output=True,
            text=True,
            check=False,
        )
        if "yes" not in r.stdout:
            log("NOTE: linger is off - run: sudo loginctl enable-linger $USER")

    active = sc("is-active", UNIT).stdout.strip()
    enabled = sc("is-enabled", UNIT).stdout.strip()
    log(f"{UNIT}: {active}, {enabled}, port {PORT}")
    if active != "active":
        log("not running - pass --install")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
