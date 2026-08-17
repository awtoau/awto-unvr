#!/usr/bin/env python3
"""Shared serial-console driver for woomera (the ea16 UNVR running Fedora).

Talks to the `tio` unix-domain socket that bridges the serial console, logs in
(root/unvr — owner's LAN device, self-set weak default, see improvements-audit.md),
and runs shell commands with a return-code fence. Used by read-ddr-spd.py,
hwverify-woomera.py, etc. so the console contract lives in ONE place.
"""
from __future__ import annotations
import os, re, socket, time
from pathlib import Path

SOCK = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "tio-unvr.sock"
PROMPT = "@@P@@"
USER, PASSWD = "root", "unvr"


def connect():
    """Connect to the console socket. Raises if tio isn't running."""
    if not SOCK.exists():
        raise FileNotFoundError(f"console socket absent: {SOCK} (start tio)")
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(0.2)
    s.connect(str(SOCK))
    return s


def _ru(s, needle, limit, extra=()):
    """Read until `needle` (or any of `extra`) appears, or `limit` seconds pass."""
    buf = b""
    end = time.monotonic() + limit
    while time.monotonic() < end:
        try:
            c = s.recv(4096)
        except socket.timeout:
            continue
        if not c:
            break
        buf += c
        if needle.encode() in buf:
            return buf, needle
        for n in extra:
            if n.encode() in buf:
                return buf, n
    return buf, None


def login(s):
    """Log in if at a login prompt and install a stable PS1."""
    s.sendall(b"\r")
    _, hit = _ru(s, "]#", 6, extra=("login:", PROMPT))
    if hit == "login:":
        s.sendall(USER.encode() + b"\r"); _ru(s, "Password:", 6)
        s.sendall(PASSWD.encode() + b"\r"); _ru(s, "]#", 12, extra=("incorrect",))
    # Quiet the shell so output parses cleanly: systemd 256+ sets PROMPT_COMMAND
    # to emit OSC-3008 "context" markers (machineid/cwd/... ) that interleave with
    # command output. Unset it, force TERM=dumb so nothing re-enables terminal
    # integration, then a bare marker prompt.
    s.sendall(b"unalias -a 2>/dev/null; unset PROMPT_COMMAND; export TERM=dumb; true\r")
    _ru(s, "]#", 4, extra=(PROMPT,))
    s.sendall(f"export PS1='{PROMPT}'\r".encode()); _ru(s, PROMPT, 6); _ru(s, PROMPT, 3)


def sh(s, cmd, timeout=30):
    """Run `cmd`, return (rc, cleaned_output). RC fence survives ANSI noise."""
    s.sendall(cmd.encode() + b"; echo @@RC=$?@@\r")
    buf = b""
    end = time.monotonic() + timeout
    rc = None
    while time.monotonic() < end:
        try:
            c = s.recv(4096)
        except socket.timeout:
            continue
        if not c:
            break
        buf += c
        m = re.search(rb"@@RC=(\d+)@@", buf)
        if m:
            rc = int(m.group(1)); break
    _ru(s, PROMPT, 4)
    txt = buf.decode(errors="replace")
    txt = re.sub(r"\x1b\[[0-9;?]*[a-zA-Z]", "", txt)              # CSI
    txt = re.sub(r"\x1b\][^\x07\x1b]*(?:\x07|\x1b\\)?", "", txt)  # OSC (complete or truncated)
    txt = re.sub(r"\]3008;[^\r\n]*", "", txt)                     # leaked OSC-3008 body remnant
    lines = [l for l in txt.splitlines() if "@@RC=" not in l and "; echo @@RC" not in l]
    return rc, "\n".join(lines).strip()
