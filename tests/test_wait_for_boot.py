"""No-console ssh fallback for wait-for-boot / verify-versions (#352).

Run: pytest tests/test_wait_for_boot.py -q
"""

from __future__ import annotations

import importlib.util
import os
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")


def _load(name: str, file: str):
    spec = importlib.util.spec_from_file_location(name, REPO / "scripts" / file)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


wfb = _load("wait_for_boot", "wait-for-boot.py")
vv = _load("verify_versions", "verify-versions.py")


def _fake_box(monkeypatch, mod, states):
    """Each boot_state() call returns (or raises) the next item of `states`."""
    it = iter(states)

    def boot_state(_ip):
        s = next(it)
        if isinstance(s, Exception):
            raise s
        return s

    monkeypatch.setattr(mod._box, "locate", lambda: "192.0.2.1")
    monkeypatch.setattr(mod._box, "boot_state", boot_state)


def test_ssh_waits_past_old_boot(monkeypatch):
    monkeypatch.setattr(wfb.time, "sleep", lambda _s: None)
    _fake_box(
        monkeypatch,
        wfb,
        [
            ("7.4.0-rc2-aaa+", "old"),
            RuntimeError("No route to host"),
            ("7.4.0-rc2-bbb+", "new"),
        ],
    )
    assert wfb.wait_ssh("7.4.0-rc2-bbb+", "old") == 0


def test_ssh_wrong_kernel(monkeypatch):
    _fake_box(monkeypatch, wfb, [("7.4.0-rc2-aaa+", "new")])
    assert wfb.wait_ssh("7.4.0-rc2-bbb+", "old") == 4


def test_ssh_timeout_logs_last_error(monkeypatch, capsys):
    monkeypatch.setattr(wfb, "SSH_TIMEOUT_S", -1.0)
    assert wfb.wait_ssh("x", None) == 3
    assert "TIMEOUT" in capsys.readouterr().out


def test_verify_kernel_rev_from_uname(monkeypatch):
    _fake_box(monkeypatch, vv, [("7.4.0-rc2-bc1fd32-dirty+", "id")])
    rev, banner, err = vv.kernel_over_ssh()
    assert (rev, err) == ("bc1fd32-dirty", None)
    assert banner == "uname -r: 7.4.0-rc2-bc1fd32-dirty+"
