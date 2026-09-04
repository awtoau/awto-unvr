# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""_console.login() against a scripted fake socket - no tio, no box.

Also pins that the two scripts which used to carry their own login()/sh()
copies (i2c-spi-scan.py: pre-OSC-3008-fix; deploy-modules-woomera.py: its
own variant) now go through _console, so the console contract has one home.

Run: pytest tests/test_console_login.py -q
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

import _console  # noqa: E402


class FakeSock:
    """Answers each sendall() with the next scripted reply; recv() drains it
    in one chunk, then raises TimeoutError like a real non-blocking read."""

    def __init__(self, replies: list[bytes]):
        self.replies = list(replies)
        self.pending = b""
        self.sent: list[bytes] = []

    def sendall(self, data: bytes) -> None:
        self.sent.append(data)
        if self.replies:
            self.pending += self.replies.pop(0)

    def recv(self, n: int) -> bytes:
        if not self.pending:
            raise TimeoutError
        out, self.pending = self.pending, b""
        return out


def test_login_rejected_password_raises(monkeypatch):
    # Reads run to their limit on silence; keep that instant here.
    monkeypatch.setattr(_console, "_ru", _fast_ru)
    s = FakeSock([b"\r\nfedora login: ", b"Password: ", b"\r\nLogin incorrect\r\n"])
    with pytest.raises(RuntimeError, match="login incorrect"):
        _console.login(s)


def test_login_from_prompt_installs_marker_prompt(monkeypatch):
    monkeypatch.setattr(_console, "_ru", _fast_ru)
    s = FakeSock(
        [
            b"\r\nfedora login: ",
            b"Password: ",
            b"\r\n[root@woomera ~]# ",
            b"[root@woomera ~]# ",
            _console.PROMPT.encode(),
            _console.PROMPT.encode(),
        ]
    )
    _console.login(s)
    joined = b"".join(s.sent)
    assert b"root\r" in joined and b"unvr\r" in joined
    assert b"unset PROMPT_COMMAND" in joined  # the OSC-3008 fix, once, here
    assert f"export PS1='{_console.PROMPT}'".encode() in joined


def _fast_ru(s, needle, limit, extra=()):
    """_console._ru without the wall-clock wait: one drain, then decide."""
    buf = b""
    try:
        buf = s.recv(4096)
    except TimeoutError:
        pass
    if needle.encode() in buf:
        return buf, needle
    for n in extra:
        if n.encode() in buf:
            return buf, n
    return buf, None


@pytest.mark.parametrize(
    "script", ["scripts/i2c-spi-scan.py", "scripts/deploy-modules-woomera.py"]
)
def test_scripts_no_longer_carry_their_own_login(script):
    src = (REPO / script).read_text()
    assert "def login(" not in src and "def _ru(" not in src
    assert "def _read_until(" not in src
    assert "_console.login(" in src and "_console.sh(" in src
