# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""scripts/_box.py - the ONE woomera resolver.

Everything here is fake-network: macs_of()/is_woomera() are monkeypatched and
subprocess.run is stubbed, so the suite never pings or ARPs the real box.

Run: pytest tests/test_box.py -q
"""

from __future__ import annotations

import importlib.util
import os
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

import _box  # noqa: E402

IP = "192.168.25.140"


# --- MAC matching under ARP flux -----------------------------------------
#
# All four host NICs answer ARP for any local IP (arp_ignore=0) and both of
# the box's ports sit on one subnet (#170), so `ip neigh` routinely names the
# 10G port's MAC for an IP the 1G port also answers. Requiring the 1G MAC
# specifically reported the box as absent while it was up.


def test_matches_the_1g_mac(monkeypatch):
    monkeypatch.setattr(_box, "macs_of", lambda ip: [_box.MAC_1G])
    assert _box.is_woomera(IP)


def test_matches_the_10g_mac(monkeypatch):
    # The regression: this used to return False and the box read as absent.
    monkeypatch.setattr(_box, "macs_of", lambda ip: [_box.MAC_10G])
    assert _box.is_woomera(IP)


def test_matches_when_a_stale_row_comes_first(monkeypatch):
    monkeypatch.setattr(_box, "macs_of", lambda ip: ["00:11:22:33:44:55", _box.MAC_1G])
    assert _box.is_woomera(IP)


def test_other_ubnt_gear_sharing_the_oui_is_not_woomera(monkeypatch):
    # Still exact MACs, never the OUI - an OUI match once picked a neighbour.
    monkeypatch.setattr(_box, "macs_of", lambda ip: ["74:ac:b9:99:99:99"])
    assert not _box.is_woomera(IP)


def test_no_neighbour_entry_is_not_woomera(monkeypatch):
    monkeypatch.setattr(_box, "macs_of", lambda ip: [])
    assert not _box.is_woomera(IP)


def test_macs_of_returns_every_row(monkeypatch):
    calls = []

    def fake_run(argv, **kw):
        calls.append(argv)
        out = ""
        if argv[:3] == ["ip", "neigh", "show"]:
            # Real `ip neigh` output shape: one row per host NIC (#170).
            out = (
                f"{IP} dev enp5s0 lladdr 74:ac:b9:41:a8:12 STALE\n"
                f"{IP} dev enp7s0 lladdr 74:ac:b9:41:a8:11 REACHABLE\n"
            )
        return subprocess.CompletedProcess(argv, 0, stdout=out, stderr="")

    monkeypatch.setattr(_box.subprocess, "run", fake_run)
    assert _box.macs_of(IP) == [_box.MAC_10G, _box.MAC_1G]
    assert calls[0][:2] == ["ping", "-c"]  # neighbour table populated first


# --- locate(): cache fast path, rescan on a moved lease -------------------


def test_locate_uses_a_verified_cache_without_scanning(monkeypatch, tmp_path):
    cache = tmp_path / "woomera-addr"
    cache.write_text(IP + "\n")
    monkeypatch.setattr(_box, "CACHE", cache)
    monkeypatch.setattr(_box, "is_woomera", lambda ip: ip == IP)
    monkeypatch.setattr(_box, "scan", lambda subnet: pytest_fail("scan ran"))
    assert _box.locate() == IP


def test_locate_rescans_and_rewrites_a_stale_cache(monkeypatch, tmp_path):
    cache = tmp_path / "woomera-addr"
    cache.write_text("192.168.25.149\n")
    monkeypatch.setattr(_box, "CACHE", cache)
    monkeypatch.setattr(_box, "is_woomera", lambda ip: ip == IP)
    monkeypatch.setattr(_box, "scan", lambda subnet: IP)
    assert _box.locate() == IP
    assert cache.read_text().strip() == IP


def test_locate_returns_none_when_absent(monkeypatch, tmp_path):
    monkeypatch.setattr(_box, "CACHE", tmp_path / "absent")
    monkeypatch.setattr(_box, "scan", lambda subnet: None)
    assert _box.locate() is None


def test_require_exits_with_the_hint(monkeypatch, tmp_path):
    monkeypatch.setattr(_box, "CACHE", tmp_path / "absent")
    monkeypatch.setattr(_box, "scan", lambda subnet: None)
    try:
        _box.require(hint="is it up and in Linux?")
    except SystemExit as e:
        assert "is it up and in Linux?" in str(e.code)
    else:
        raise AssertionError("require() must exit when the box is absent")


def pytest_fail(msg):
    raise AssertionError(msg)


# --- FAILED neighbour flush (#170) ----------------------------------------


def test_flush_deletes_only_failed_rows(monkeypatch):
    dels = []

    def fake_run(argv, **kw):
        if argv[:3] == ["ip", "neigh", "show"]:
            out = (
                f"{IP} dev enp5s0 FAILED\n"
                f"{IP} dev enp7s0 lladdr 74:ac:b9:41:a8:11 REACHABLE\n"
                f"{IP} dev enp9s0 INCOMPLETE\n"
            )
            return subprocess.CompletedProcess(argv, 0, stdout=out, stderr="")
        if argv[:3] == ["ip", "neigh", "del"]:
            dels.append(argv)
        return subprocess.CompletedProcess(argv, 0, stdout="", stderr="")

    monkeypatch.setattr(_box.subprocess, "run", fake_run)
    _box.flush_failed_neighbours(IP)
    assert dels == [
        ["ip", "neigh", "del", IP, "dev", "enp5s0"],
        ["ip", "neigh", "del", IP, "dev", "enp9s0"],
    ]


# --- ssh argv is what ssh-woomera.py exec'd ------------------------------


def test_ssh_argv_keepalives_and_command():
    argv = _box.ssh_argv(IP, cmd=["uname", "-a"])
    assert argv[0] == "ssh"
    assert "ServerAliveInterval=15" in argv
    assert "ServerAliveCountMax=8" in argv
    assert "ConnectTimeout=8" in argv
    assert argv[-3:] == [f"root@{IP}", "uname", "-a"]


def test_ssh_woomera_cli_is_a_thin_wrapper():
    spec = importlib.util.spec_from_file_location(
        "_ssh_woomera", REPO / "scripts/ssh-woomera.py"
    )
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    assert mod.locate is _box.locate
    assert mod.ssh_argv is _box.ssh_argv
