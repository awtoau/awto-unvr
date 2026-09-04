# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""The box's LAN identity is stored once. This test is the thing that keeps it so.

Why it exists: the repo rule is "resolve by MAC, never a hardcoded IP", yet
by 2026-09-05 eight scripts carried a literal 192.168.25.x (a stale lease as
a constant, the U-Boot static copied instead of imported, a regressed
dev-host IP, three copies of the subnet, stale docstring examples) and four
carried the MACs - one of them the bare OUI, the exact bug ssh-woomera.py
had already fixed once. Each was added under pressure and each was a
reasonable local decision. Only a failing test stops the next one.

Homes:
  IPs  (192.168.25.x)     scripts/_net.py   LAN_SUBNET, UNVR_IPADDR
  MACs (74:ac:b9...)      scripts/_box.py   MAC_1G, MAC_10G, WOOMERA_MACS
Scanned: dev.py, scripts/*.py, scripts/*.tcl (tcl gets $IPADDR/$SERVERIP
injected by dev.py console-tcl). tests/ are not scanned: fixtures need
literals.

Allowlist:
  scripts/decode-identity.py  an MTD dump FILENAME carries the MAC
                              (images/mtd/UNVR-74acb941a811-...). Not a MAC
                              constant; a glob or --image arg would be
                              better, not changed in this pass.

Run: pytest tests/test_lan_identity_single_source.py -q
"""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

import _box  # noqa: E402
import _net  # noqa: E402

IP_RE = re.compile(r"192\.168\.25\.\d+")
MAC_RE = re.compile(r"74:?ac:?b9", re.IGNORECASE)

IP_HOME = {"scripts/_net.py"}
MAC_HOME = {"scripts/_box.py"}
MAC_ALLOW = {"scripts/decode-identity.py"}  # dump filename, see docstring


def _scanned() -> list[Path]:
    scripts = REPO / "scripts"
    return [
        REPO / "dev.py",
        *sorted(scripts.glob("*.py")),
        *sorted(scripts.glob("*.tcl")),
    ]


def _hits(pattern: re.Pattern, skip: set[str]) -> list[str]:
    out = []
    for p in _scanned():
        rel = p.relative_to(REPO).as_posix()
        if rel in skip:
            continue
        for n, line in enumerate(p.read_text(errors="replace").splitlines(), 1):
            if pattern.search(line):
                out.append(f"{rel}:{n}: {line.strip()[:80]}")
    return out


def test_no_lan_ip_literal_outside_net():
    hits = _hits(IP_RE, IP_HOME)
    assert not hits, "LAN IP literal outside scripts/_net.py:\n  " + "\n  ".join(hits)


def test_no_box_mac_literal_outside_box():
    hits = _hits(MAC_RE, MAC_HOME | MAC_ALLOW)
    assert not hits, "box MAC/OUI literal outside scripts/_box.py:\n  " + "\n  ".join(
        hits
    )


def test_the_homes_still_hold_them():
    # The guard must not be satisfiable by deleting the constants.
    assert IP_RE.fullmatch(_net.UNVR_IPADDR)
    assert _net.LAN_SUBNET.startswith("192.168.25.")
    assert MAC_RE.match(_box.MAC_1G) and MAC_RE.match(_box.MAC_10G)
    assert _box.WOOMERA_MACS == {_box.MAC_1G, _box.MAC_10G}


def test_dev_describe_reports_the_1g_mac_from_box():
    r = subprocess.run(
        [sys.executable, str(REPO / "dev.py"), "describe"],
        capture_output=True,
        text=True,
        cwd=REPO,
        check=True,
    )
    assert json.loads(r.stdout)["board"]["mac"] == _box.MAC_1G
