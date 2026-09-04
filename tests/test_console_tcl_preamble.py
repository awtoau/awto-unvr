# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""dev.py console-tcl injects $IPADDR into every script it runs (#252).

Nine .tcl files each carried their own "192.168.25.140". They now say
$IPADDR and dev.py prepends `set IPADDR <_net.UNVR_IPADDR>` so the literal
has one home. These tests pin that contract without a console: the Tcl
interpreter is real, the send/expect leaf commands are fakes.

Run: pytest tests/test_console_tcl_preamble.py -q
"""

from __future__ import annotations

import importlib.util
import os
import sys
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

from _net import UNVR_IPADDR  # noqa: E402


def _dev():
    spec = importlib.util.spec_from_file_location("dev_module", REPO / "dev.py")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def test_preamble_defines_ipaddr_from_net():
    assert _dev()._tcl_preamble() == f"set IPADDR {UNVR_IPADDR}\n"


def test_console_tcl_substitutes_ipaddr_into_send(monkeypatch, tmp_path):
    dev = _dev()
    try:
        dev._load_minijimtcl()  # cmd_console_tcl passes the module to the fake
    except RuntimeError as e:
        pytest.skip(str(e))
    sent: list[str] = []

    class FakeConsoleTcl:
        def __init__(self, mod):
            self.TclError = mod.TclError
            self.tcl = mod.MiniJimTcl()
            self.tcl.register("send", lambda a: sent.append(" ".join(a)) or "")
            self.tcl.register("send_raw", lambda a: "")
            self.tcl.register("expect", lambda a: "")

        def close(self):
            pass

    monkeypatch.setattr(dev, "_console_pid", lambda: 1)
    monkeypatch.setattr(dev, "_ConsoleTcl", FakeConsoleTcl)
    script = tmp_path / "t.tcl"
    # Same shapes the real scripts use: quoted send, and a quoted send with
    # ';' and quotes inside (linux-to-autochain.tcl).
    script.write_text(
        'send "setenv ipaddr $IPADDR"\n'
        "send \"setenv bootchain 'setenv ipaddr $IPADDR; go 0x1100000'\"\n"
    )
    assert dev.cmd_console_tcl([str(script)]) == 0
    assert sent == [
        f"setenv ipaddr {UNVR_IPADDR}",
        f"setenv bootchain 'setenv ipaddr {UNVR_IPADDR}; go 0x1100000'",
    ]


def test_no_console_tcl_carries_the_literal():
    bad = sorted(
        p.name for p in (REPO / "scripts").glob("*.tcl") if UNVR_IPADDR in p.read_text()
    )
    assert not bad, f"use $IPADDR (injected by dev.py console-tcl): {bad}"
