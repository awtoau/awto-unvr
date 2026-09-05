#!/usr/bin/env python3
"""Shared serial-console driver for woomera (the ea16 UNVR running Fedora).

Talks to the `tio` unix-domain socket that bridges the serial console, logs in
(root/unvr — owner's LAN device, self-set weak default, see improvements-audit.md),
and runs shell commands with a return-code fence. Used by read-ddr-spd.py,
hwverify-woomera.py, etc. so the console contract lives in ONE place.
"""

from __future__ import annotations

import os
import re
import socket
import time
from pathlib import Path

SOCK = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "tio-unvr.sock"
PROMPT = "@@P@@"
USER, PASSWD = "root", "unvr"

# The two bootloader prompts on this box's console - one home each.
AWTO_PROMPT = "awto-nas#"  # our U-Boot (NAND 0x1300000, #216)
STOCK_PROMPT = "ALPINE_UBNT_NAS_ALL>"  # UBNT's 2015.07 stock U-Boot
# EDK2: only Shell.efi itself prints this; BDS's boot-option dump also says
# "UEFI Shell", which is a false positive. Prompt is "<FSn>:\>" after `fsN:`.
UEFI_SHELL_MARK = "UEFI Interactive Shell"
UEFI_SHELL_PROMPT_SUFFIX = ":\\>"
UEFI_CRASH_MARKS = ("Synchronous Exception", "Data Abort", "Instruction Abort")


def connect(timeout: float = 0.2):
    """Connect to the console socket. Raises if tio isn't running.
    `timeout` is the recv() poll interval each caller's read loop turns on."""
    if not SOCK.exists():
        raise FileNotFoundError(f"console socket absent: {SOCK} (start tio)")
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(timeout)
    s.connect(str(SOCK))
    return s


def _ru(s, needle, limit, extra=()):
    """Read until `needle` (or any of `extra`) appears, or `limit` seconds pass."""
    buf = b""
    end = time.monotonic() + limit
    while time.monotonic() < end:
        try:
            c = s.recv(4096)
        except TimeoutError:
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
        s.sendall(USER.encode() + b"\r")
        _ru(s, "Password:", 6)
        s.sendall(PASSWD.encode() + b"\r")
        _, hit = _ru(s, "]#", 12, extra=("incorrect",))
        if hit == "incorrect":
            raise RuntimeError(f"login incorrect - {USER}/{PASSWD} rejected")
    # Quiet the shell so output parses cleanly: systemd 256+ sets PROMPT_COMMAND
    # to emit OSC-3008 "context" markers (machineid/cwd/... ) that interleave with
    # command output. Unset it, force TERM=dumb so nothing re-enables terminal
    # integration, then a bare marker prompt.
    s.sendall(b"unalias -a 2>/dev/null; unset PROMPT_COMMAND; export TERM=dumb; true\r")
    _ru(s, "]#", 4, extra=(PROMPT,))
    s.sendall(f"export PS1='{PROMPT}'\r".encode())
    _ru(s, PROMPT, 6)
    _ru(s, PROMPT, 3)


def sh(s, cmd, timeout=30):
    """Run `cmd`, return (rc, cleaned_output). RC fence survives ANSI noise."""
    s.sendall(cmd.encode() + b"; echo @@RC=$?@@\r")
    buf = b""
    end = time.monotonic() + timeout
    rc = None
    while time.monotonic() < end:
        try:
            c = s.recv(4096)
        except TimeoutError:
            continue
        if not c:
            break
        buf += c
        m = re.search(rb"@@RC=(\d+)@@", buf)
        if m:
            rc = int(m.group(1))
            break
    _ru(s, PROMPT, 4)
    txt = buf.decode(errors="replace")
    txt = re.sub(r"\x1b\[[0-9;?]*[a-zA-Z]", "", txt)  # CSI
    txt = re.sub(
        r"\x1b\][^\x07\x1b]*(?:\x07|\x1b\\)?", "", txt
    )  # OSC (complete or truncated)
    txt = re.sub(r"\]3008;[^\r\n]*", "", txt)  # leaked OSC-3008 body remnant
    lines = [l for l in txt.splitlines() if "@@RC=" not in l and "; echo @@RC" not in l]
    return rc, "\n".join(lines).strip()


# --- stage-handoff failure detection -------------------------------------
#
# The failure this catches: we sent a jump command meant for stage B (EDK2,
# a kernel) and stage A (U-Boot) never left. Every subsequent keystroke -
# the BdsWait 's' hotkey spam - is then typed at stage A's live prompt and
# echoes straight back:
#
#     awto-nas# sssssssssssssssssssssssssss
#
# That echo is PROOF, available in well under a second, that the handoff
# failed. Waiting out the stage-B marker timeout after it adds nothing.

# Banner each jump command prints before transferring control. Both are a
# printf immediately preceding the branch, not work - if neither appears the
# jump was never attempted (bad address, command not built in, typo).
JUMP_BANNERS = ("Starting application at", "bootedk2:")


def jump_failed(text: str, sent: str, stage_a_prompt: str, tail_chars: int = 400):
    """Proof, from the transcript, that a stage-A -> stage-B jump did not take.

    Returns a one-line reason, or None if there is no proof yet. Deliberately
    conservative: "no evidence of success" is NOT proof and never returns a
    reason here - only positive evidence that stage A is still running.

    text          everything received since the jump command was sent
    sent          the extra bytes we typed after the jump (e.g. "s" hotkeys);
                  "" if none, which disables the echo check
    stage_a_prompt  the prompt that must NOT reappear, e.g. "awto-nas#"
    """
    tail = text[-tail_chars:]
    # Stage A re-prompting after the jump command means control came back to
    # it: `go` rejected the address, or the command does not exist.
    if stage_a_prompt in tail:
        return f"{stage_a_prompt} re-prompted after the jump - it did not take"
    # Three of our own keystrokes in a row, un-consumed, at a live prompt.
    # Three not one: a single character can legitimately appear in stage-B
    # trace output, a run of the exact byte we are spamming cannot.
    if sent and sent * 3 in tail:
        return (
            f"our own {sent!r} keystrokes are echoing - stage A still has the console"
        )
    return None


def jump_banner_missing(text: str) -> bool:
    """True if no jump command has announced itself yet.

    Only meaningful after the banner's own grace window has elapsed - the
    banner is a printf on the line before the branch, so its absence past
    a second means the command errored out rather than jumped.
    """
    return not any(b in text for b in JUMP_BANNERS)
