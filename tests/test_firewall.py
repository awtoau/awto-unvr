# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""scripts/_firewall.py parsing + verdict logic (#136). No firewalld needed."""

from __future__ import annotations

import os
import sys
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

import _firewall as fw  # noqa: E402

IP = "10.1.2.3"
IP_OUT = f"2: eth9    inet {IP}/24 brd 10.1.2.255 scope global eth9\n"
ACTIVE = """Work (default)
  interfaces: eth9
trusted
  sources: 10.1.2.0/24
docker
  interfaces: docker0
"""


def test_iface_of_ip():
    assert fw.iface_of_ip(IP_OUT, IP) == "eth9"
    assert fw.iface_of_ip(IP_OUT, "10.9.9.9") == ""


def test_zone_source_beats_interface():
    assert fw.zone_of(ACTIVE, "eth9", IP) == "trusted"


def test_zone_by_interface():
    assert fw.zone_of(ACTIVE, "eth9", "10.7.7.7") == "Work"
    assert fw.zone_of(ACTIVE, "nope", "10.7.7.7") == ""


@pytest.mark.parametrize(
    ("ports", "port", "want"),
    [
        ("1025-65535/udp", 6969, True),
        ("1025-65535/udp", 69, False),
        ("69/udp", 69, True),
        ("69/tcp", 69, False),
        ("", 69, False),
    ],
)
def test_port_in_list(ports, port, want):
    assert fw.port_in_list(ports, port) is want


def test_zone_permits():
    assert fw.zone_permits("ssh tftp", "", "default", 69)
    assert not fw.zone_permits("ssh", "1025-65535/udp", "default", 69)
    assert fw.zone_permits("ssh", "1025-65535/udp", "default", 6969)
    assert fw.zone_permits("", "", "ACCEPT", 69)
    assert not fw.zone_permits("tftp", "", "default", 6969)


def test_target_parse():
    assert fw._target_of("Work (active)\n  target: ACCEPT\n") == "ACCEPT"
    assert fw._target_of("") == ""


def test_fix_commands():
    assert fw.fix_commands("Z", 69)[1] == [
        "sudo",
        "firewall-cmd",
        "--permanent",
        "--zone=Z",
        "--add-service=tftp",
    ]
    assert fw.fix_commands("Z", 6969)[0][-1] == "--add-port=6969/udp"


def test_blocked_exits_with_command_unless_applied(monkeypatch):
    v = fw.Verdict("blocked", "x", "Z", "eth9", fw.fix_commands("Z", 69))
    monkeypatch.setattr(fw, "check_tftp", lambda ip, port=69: v)
    monkeypatch.delenv(fw.APPLY_ENV, raising=False)
    with pytest.raises(SystemExit) as e:
        fw.ensure_tftp_open(69, server_ip=IP, log=lambda m: None)
    assert "--add-service=tftp" in str(e.value)


def test_skipped_when_firewalld_off(monkeypatch):
    v = fw.Verdict("skipped", "firewalld not running")
    monkeypatch.setattr(fw, "check_tftp", lambda ip, port=69: v)
    assert fw.ensure_tftp_open(69, server_ip=IP, log=lambda m: None) is v
