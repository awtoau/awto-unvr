#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Install/manage always-on iperf3 servers on the dev host AND woomera (#241).

A persistent server removes the "is the server even up" failure from every
throughput measurement - ad-hoc `iperf3 -s -D` died with its shell and left
runs reporting a connect timeout that looked like a box-side fault.

Both ends run one, so a test can be driven from either side:
  * dev host - systemd --user unit, needs `loginctl enable-linger`
  * woomera  - system unit (root, no login session to linger)

Both listen on ALL interfaces on port 5701. Which path is measured is a
CLIENT-side choice:

    ./dev.py ssh -- "iperf3 -c <dev-host> -p 5701 --bind-dev enp0s1"   # 1G
    ./dev.py ssh -- "iperf3 -c <dev-host> -p 5701 --bind-dev enp0s2 -P 4"   # 10G
    iperf3 -c <box-ip> -p 5701 -P 4                       # dev host -> box

`--bind-dev`, not `-B <addr>`: `-B` selects a source address, not an egress
port. Since #170's fix (strict ARP + per-NIC route tables) `-B` does follow the
right port on woomera, but --bind-dev constrains it directly and does not
depend on those rules being present. One A57 core cannot saturate 10GbE, so
10G needs `-P 4` (#241).

    ./scripts/iperf-server.py                  # status, both ends
    ./scripts/iperf-server.py --install        # dev host
    ./scripts/iperf-server.py --install-box    # woomera
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


BOX_UNIT_PATH = f"/etc/systemd/system/{UNIT}"

# Box unit: a SYSTEM unit, not --user. There is no login session on the box to
# linger, and the iperf3 binary lives in /usr/bin there too.
BOX_UNIT_TEXT = UNIT_TEXT.replace(
    "WantedBy=default.target", "WantedBy=multi-user.target"
)


def sc(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run(
        ["systemctl", "--user", *args], capture_output=True, text=True, check=False
    )


def box_sh(host: str, cmd: str, timeout: int = 120) -> tuple[int, str]:
    """Run cmd on the box. 120s bounds a systemctl call that returns in ms;
    on expiry the step is logged with its limit and install reports failure."""
    try:
        r = subprocess.run(
            ssh_argv(host, cmd=[cmd]),
            capture_output=True,
            text=True,
            check=False,
            timeout=timeout,
        )
        return r.returncode, (r.stdout + r.stderr).strip()
    except subprocess.TimeoutExpired:
        log(f"TIMEOUT after {timeout}s: {cmd[:60]}")
        return 124, ""


def install_box() -> int:
    host = require(hint="is woomera up?")
    rc, o = box_sh(host, "command -v iperf3 || echo MISSING")
    if "MISSING" in o:
        log("iperf3 is not installed on the box - dnf install iperf3 first")
        return 1
    # Heredoc then enable --now: one ssh round trip, idempotent.
    rc, o = box_sh(
        host,
        f"cat > {BOX_UNIT_PATH} <<'EOF'\n{BOX_UNIT_TEXT}\nEOF\n"
        f"systemctl daemon-reload && systemctl enable --now {UNIT} && echo installed",
    )
    log(f"box install rc={rc}: {o[-200:]}")
    rc, o = box_sh(
        host,
        f"systemctl is-active {UNIT}; systemctl is-enabled {UNIT}; "
        f"ss -lntu 2>/dev/null | grep {PORT} || echo 'NOT LISTENING'",
    )
    log(f"box {UNIT}:\n{o}")
    return 0 if "NOT LISTENING" not in o else 1


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--install", action="store_true", help="write unit, enable, start")
    ap.add_argument(
        "--install-box", action="store_true", help="same, as a system unit on woomera"
    )
    a = ap.parse_args()

    if a.install_box:
        return install_box()

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
