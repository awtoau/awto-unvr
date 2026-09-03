#!/usr/bin/env python3
"""dev.py — canonical entry point for awto-unvr (awto dev.py convention).

AI agents: run `./dev.py describe` for machine-readable JSON help.
Humans: run `./dev.py --help`.

All output is timestamped in LOCAL time and mirrored to ./tmp/logs/dev.log.

Repurposing a Ubiquiti UNVR (Annapurna Labs Alpine V2, aarch64). Board sysid
0xea16 - "UNVR without eMMC": kernel/rootfs in NAND, USERDEV on an internal USB
stick. Boot chain in docs/boot-flow.md, prior art in docs/sources.md.

Which build do I want? docs/build.md - one table for every kernel/U-Boot/
rootfs variant (which script, which dev.py command, what it's for).

Console model: ONE tio owns the serial port and exposes a unix socket + a plain
log; whoever does NOT own it works through those. Two ways to own it:
  - `console`      - agent runs tio backgrounded (headless; agent drives).
  - `console-own`  - YOU run a real interactive tio in the foreground (you drive,
                     Ctrl-t q to quit); the agent reads the same socket + log.
Either way there is one owner and no /dev/ttyUSB* fight, no tmux/pane lifecycle.
tio does the logging (-L --log-strip -t).
"""

from __future__ import annotations

import json
import os
import re
import shutil
import socket
import subprocess
import sys
import time
from collections import deque
from datetime import datetime
from pathlib import Path

PROJECT = "awto-unvr"

REPO = Path(__file__).resolve().parent
TMP = REPO / "tmp"
LOGS = TMP / "logs"
LOG = LOGS / "dev.log"

# --------------------------------------------------------------------------
# Hardware / console constants - the only device-specific configuration.
# --------------------------------------------------------------------------
# 115200 8N1 is not a guess: U-Boot's own `loadbootargs` sets
# console=ttyS0,115200. Confirmed by baud sweep against the live unit.
CONSOLE_BAUD = int(
    os.environ.get("UNVR_CONSOLE_BAUD", "115200")
)  # override for baud tests (e.g. 1000000)
# The UNVR console is the CP2102 (Silicon Labs, 3.3 V logic — what the UNVR UART
# wants). Match it ONLY by its stable by-id path.
# Do NOT fall back to bare /dev/ttyUSB<N>: those numbers are assigned in USB
# enumeration order and silently point at whatever adapter happens to be there -
# on this host that was an unrelated PL2303 (often 5 V, wrong for the UNVR UART).
# If the CP2102 is absent, _console_port() returns None and the caller errors
# clearly ("CP2102 not connected") instead of driving the wrong device.
# Override the path with UNVR_CONSOLE_PORT for a one-off different adapter.
CONSOLE_PORTS = [
    os.environ.get(
        "UNVR_CONSOLE_PORT",
        "/dev/serial/by-id/usb-Silicon_Labs_CP2102_USB_to_UART_Bridge_Controller_0001-if00-port0",
    ),
]
CONSOLE_SOCK = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "tio-unvr.sock"
CONSOLE_LOG = LOGS / "unvr-console.log"
CONSOLE_TAIL = LOGS / "unvr-console-tail.log"
CONSOLE_PID = TMP / "tio-unvr.pid"
# tio runs with --log-append and never truncates, so the log grows across every
# session. Roll at 20 MB - the size an editor tokenising the whole file on open
# starts choking on, and ~250k lines, deeper than any grep here reaches back.
CONSOLE_ROLL_BYTES = 20 * 1024 * 1024
# Keep 3 generations (.1-.3): a few sessions of history, 60 MB worst case in
# tmp/, which `clean` wipes anyway.
CONSOLE_ROLL_KEEP = 3
# console-peek default. 500 lines is a few screens - a boot sequence or the tail
# of a failing command - and small enough that the slice opens instantly.
CONSOLE_PEEK_LINES = 500

BOARD_SYSID = "0xea16"
BOARD_MAC = "74:ac:b9:41:a8:11"

# --------------------------------------------------------------------------
# STEPS - the only project-specific build table. This repo has no compiled
# artefact; the work is analysis and hardware bring-up, so build/run stay
# unconfigured and SKIP by design.
# --------------------------------------------------------------------------
STEPS: dict[str, tuple[str, list[str], bool]] = {
    "build": ("compile the project (n/a - analysis repo)", [], True),
    "test": ("run the test suite", [sys.executable, "-m", "pytest", "-q"], True),
    "lint": ("static analysis", ["ruff", "check", "."], True),
    "fmt-check": (
        "formatting check (no writes)",
        ["ruff", "format", "--check", "."],
        False,
    ),
    "fmt": ("reformat in place", ["ruff", "format", "."], False),
    "run": ("build + execute (n/a)", [], True),
}

GATE = ["fmt-check", "lint", "test"]
CI = ["fmt-check", "lint", "test"]

# --------------------------------------------------------------------------
# Logging - Tier A (dev tooling): local time, stderr + ./tmp/logs/dev.log,
# colour on TTY only, file copy always plain.
# --------------------------------------------------------------------------
_COLOR = {
    "FATAL": "\033[1;91m",
    "ERROR": "\033[31m",
    "WARN": "\033[33m",
    "INFO": "\033[97m",
    "DEBUG": "\033[94m",
    "ALERT": "\033[92m",
}
_RESET = "\033[0m"
_USE_COLOR = (
    sys.stderr.isatty()
    and not os.environ.get("NO_COLOR")
    and os.environ.get("CLICOLOR") != "0"
)


def _stamp() -> str:
    # Local zone incl. DST (AEST +1000 / AEDT +1100). Never hardcode the offset
    # and never use UTC here: a human reads these live.
    return datetime.now().astimezone().strftime("%H:%M:%S.%f%z")


def log(msg: str, level: str = "INFO") -> None:
    line = f"{_stamp()}  {level:<5} [dev.py] {msg}"
    if _USE_COLOR:
        sys.stderr.write(f"{_COLOR.get(level, '')}{line}{_RESET}\n")
    else:
        sys.stderr.write(line + "\n")
    sys.stderr.flush()
    try:
        LOGS.mkdir(parents=True, exist_ok=True)
        with LOG.open("a", encoding="utf-8") as fh:
            fh.write(line + "\n")  # file copy is ALWAYS plain
    except OSError:
        pass  # never let logging kill the run


SKIPPED = 125  # distinct from any real tool's exit code


def run_step(name: str, extra: list[str] | None = None) -> int:
    """Run one STEPS entry. Returns its exit code, or SKIPPED."""
    _summary, argv, takes_extra = STEPS[name]
    if not argv:
        log(f"{name}: not configured for this project - skipped", "WARN")
        return SKIPPED
    if not shutil.which(argv[0]) and argv[0] != sys.executable:
        log(f"{name}: {argv[0]} not on PATH - skipped", "WARN")
        return SKIPPED
    cmd = argv + (list(extra) if (extra and takes_extra) else [])
    log("run: " + " ".join(cmd))
    rc = subprocess.call(cmd, cwd=REPO)
    log(f"rc={rc}: " + " ".join(cmd), "INFO" if rc == 0 else "ERROR")
    return rc


# --------------------------------------------------------------------------
# Command registry - ONE source of truth. The decorator populates the table
# that drives dispatch, --help AND describe, so they cannot drift apart.
# --------------------------------------------------------------------------
COMMANDS: dict[str, dict] = {}


def command(summary: str, *, args: str = "", kind: str = "action"):
    def deco(fn):
        COMMANDS[fn.__name__.replace("cmd_", "").replace("_", "-")] = {
            "summary": summary,
            "args": args,
            "kind": kind,
            "fn": fn,
        }
        return fn

    return deco


# --------------------------------------------------------------------------
# Console lifecycle
# --------------------------------------------------------------------------
def _console_port() -> str | None:
    for cand in CONSOLE_PORTS:
        if Path(cand).exists():
            return cand
    return None


def _console_pid() -> int | None:
    """PID of the live tio owning OUR port, or None. Clears a stale pidfile.

    Match only the tio bound to OUR socket. `pgrep -x tio` alone matches ANY tio
    on the machine - e.g. an unrelated tio on another project's serial device -
    and would falsely report our port as owned. Filter by our socket name in the
    process cmdline."""
    p = subprocess.run(
        ["pgrep", "-x", "tio"], capture_output=True, text=True, check=False
    )
    if p.returncode == 0 and p.stdout.strip():
        for pid in p.stdout.split():
            try:
                cmdline = Path(f"/proc/{pid}/cmdline").read_bytes()
            except OSError:
                continue
            if CONSOLE_SOCK.name.encode() in cmdline:
                return int(pid)
    CONSOLE_PID.unlink(missing_ok=True)
    return None


def _roll_console_log() -> Path | None:
    """Roll CONSOLE_LOG to .1 (shifting .1->.2 ...) if it is over the cap.

    Returns the rolled-to path, else None (absent, or under cap). Only safe
    while nothing holds the file open, so callers roll BEFORE starting tio -
    renaming underneath a live tio loses every line it writes after."""
    try:
        size = CONSOLE_LOG.stat().st_size
    except FileNotFoundError:
        return None
    if size <= CONSOLE_ROLL_BYTES:
        return None

    def gen(n: int) -> Path:
        return CONSOLE_LOG.parent / f"{CONSOLE_LOG.name}.{n}"

    gen(CONSOLE_ROLL_KEEP).unlink(missing_ok=True)  # past the keep window
    for n in range(CONSOLE_ROLL_KEEP - 1, 0, -1):
        if gen(n).exists():
            gen(n).rename(gen(n + 1))
    CONSOLE_LOG.rename(gen(1))
    log(
        f"rolled {CONSOLE_LOG.relative_to(REPO)} ({size / (1024 * 1024):.1f} MB)"
        f" -> {gen(1).relative_to(REPO)}",
        "WARN",
    )
    return gen(1)


@command("start tio owning the serial port, exposing a socket + log", kind="action")
def cmd_console(_extra: list[str]) -> int:
    if not shutil.which("tio"):
        log("tio not on PATH", "ERROR")
        return 1
    pid = _console_pid()
    if pid:
        log(f"already running (pid {pid}) on {CONSOLE_SOCK}", "ALERT")
        return 0
    port = _console_port()
    if not port:
        log("no serial adapter found. Tried:", "ERROR")
        for c in CONSOLE_PORTS:
            log(f"  {c}", "ERROR")
        return 1
    CONSOLE_SOCK.unlink(missing_ok=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    TMP.mkdir(parents=True, exist_ok=True)
    _roll_console_log()  # startup is the one moment no tio holds the log
    tio_cmd = " ".join(
        [
            "tio",
            "-b",
            str(CONSOLE_BAUD),
            "--socket",
            f"unix:{CONSOLE_SOCK}",
            "-L",
            "--log-file",
            str(CONSOLE_LOG),
            "--log-append",
            # --log-strip keeps the on-disk copy plain: ANSI in a log file ruins
            # every later grep, which is the Tier A rule.
            "--log-strip",
            "-t",
            port,
        ]
    )
    # tio MUST have a tty on stdin. With a pipe or /dev/null it takes the
    # documented "echo cmd | tio" pipe mode instead: sends stdin to the device,
    # sees EOF, exits 0 immediately, and never creates the socket. `script`
    # allocates the pty; setsid detaches it so it outlives this process.
    cmd = ["setsid", "script", "-qec", tio_cmd, "/dev/null"]
    log("run: " + " ".join(cmd))
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        stdin=subprocess.DEVNULL,
        start_new_session=True,
        cwd=REPO,
    )
    CONSOLE_PID.write_text(str(proc.pid))
    log(f"tio pid {proc.pid} on {port} @ {CONSOLE_BAUD}", "ALERT")
    log(f"socket : {CONSOLE_SOCK}")
    log(f"log    : {CONSOLE_LOG.relative_to(REPO)}")
    log("attach : ./dev.py console-attach   (or: nc -U " + str(CONSOLE_SOCK) + ")")
    return 0


@command(
    "run tio in the FOREGROUND (you own the port); socket+log stay live for the agent",
    kind="action",
)
def cmd_console_own(_extra: list[str]) -> int:
    """You get a real interactive tio in your own terminal, owning the serial
    port. --socket + -L still expose the SAME socket + log the agent's tools use,
    so console-send / log-reading keep working against your session. Ctrl-t q to
    quit (which ends the agent's access until a console is restarted)."""
    if not shutil.which("tio"):
        log("tio not on PATH", "ERROR")
        return 1
    if _console_pid():
        log(
            "a tio already owns the port - stop it first (./dev.py console-stop)",
            "ERROR",
        )
        return 1
    port = _console_port()
    if not port:
        log("no serial adapter found", "ERROR")
        return 1
    CONSOLE_SOCK.unlink(missing_ok=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    TMP.mkdir(parents=True, exist_ok=True)
    _roll_console_log()
    cmd = [
        "tio",
        "-b",
        str(CONSOLE_BAUD),
        "--socket",
        f"unix:{CONSOLE_SOCK}",
        "-L",
        "--log-file",
        str(CONSOLE_LOG),
        "--log-append",
        "--log-strip",
        "-t",
        port,
    ]
    log(
        f"foreground tio on {port} @ {CONSOLE_BAUD} — you drive. "
        f"Agent reads {CONSOLE_SOCK} + the log. Ctrl-t q to quit."
    )
    proc = subprocess.Popen(cmd, cwd=REPO)  # inherits your tty -> real interactive tio
    CONSOLE_PID.write_text(str(proc.pid))
    try:
        return proc.wait()
    finally:
        CONSOLE_PID.unlink(missing_ok=True)


@command("report console state (pid, socket, port, log)", kind="action")
def cmd_console_status(_extra: list[str]) -> int:
    pid = _console_pid()
    port = _console_port()
    log(f"port   : {port or 'NONE FOUND'}", "INFO" if port else "WARN")
    log(f"pid    : {pid or 'not running'}", "INFO" if pid else "WARN")
    log(
        f"socket : {CONSOLE_SOCK} {'(present)' if CONSOLE_SOCK.exists() else '(absent)'}"
    )
    if CONSOLE_LOG.exists():
        log(
            f"log    : {CONSOLE_LOG.relative_to(REPO)} ({CONSOLE_LOG.stat().st_size} B)"
        )
    else:
        log(f"log    : {CONSOLE_LOG.relative_to(REPO)} (absent)", "WARN")
    return 0 if pid else 1


@command("stop the tio that owns the serial port", kind="action")
def cmd_console_stop(_extra: list[str]) -> int:
    pid = _console_pid()
    if not pid:
        log("not running", "WARN")
        return 0
    os.kill(pid, 15)
    CONSOLE_PID.unlink(missing_ok=True)
    CONSOLE_SOCK.unlink(missing_ok=True)
    log(f"stopped pid {pid}", "ALERT")
    return 0


@command("attach an interactive client to the console socket", kind="action")
def cmd_console_attach(_extra: list[str]) -> int:
    if not _console_pid():
        log("console not running - start it with ./dev.py console", "ERROR")
        return 1
    # socat puts the LOCAL terminal in raw mode (echo=0) so keystrokes and escape
    # sequences pass through untouched - a real interactive console. `nc -U` does
    # NOT (line-buffered, cooked tty) which leaks cursor-report garbage. escape
    # 0x1d = Ctrl-] detaches, leaving the shared tio (and its socket) running so
    # the agent keeps programmatic access to the same session.
    if shutil.which("socat"):
        log(
            f"attaching to {CONSOLE_SOCK} via socat - Ctrl-] to detach (tio keeps running)"
        )
        return subprocess.call(
            ["socat", "-,raw,echo=0,escape=0x1d", f"UNIX-CONNECT:{CONSOLE_SOCK}"]
        )
    if shutil.which("nc"):
        log(
            f"socat missing; falling back to raw nc on {CONSOLE_SOCK} "
            "(cooked tty - escape sequences will leak) - Ctrl-C to detach",
            "ALERT",
        )
        return subprocess.call(["nc", "-U", str(CONSOLE_SOCK)])
    log("neither socat nor nc on PATH", "ERROR")
    return 1


# Keyword tokens for --raw sends (control bytes you can't type as an arg).
_RAW_TOKENS = {
    "CR": b"\r",
    "LF": b"\n",
    "CRLF": b"\r\n",
    "ENTER": b"\r",
    "ESC": b"\x1b",
    "TAB": b"\t",
    "SPACE": b" ",
    "NUL": b"\x00",
    "CTRL-C": b"\x03",
    "CTRL-D": b"\x04",
    "CTRL-M": b"\r",
}


def _raw_bytes(tokens: list[str]) -> bytes:
    """Expand --raw args: keyword tokens (CR/LF/ESC/...) or \\xNN / \\r / \\n /
    \\t escapes. Anything else is sent literally (UTF-8)."""
    out = b""
    for t in tokens:
        if t in _RAW_TOKENS:
            out += _RAW_TOKENS[t]
        else:
            out += t.encode().decode("unicode_escape").encode("latin-1")
    return out


@command(
    "send to the console; optionally wait for a pattern before returning",
    args="[--raw] [--expect NEEDLE [--timeout S]] <text|CR|LF|ESC ...>",
    kind="action",
)
def cmd_console_send(extra: list[str]) -> int:
    """The one console write primitive - use this instead of writing a new script.

      ./dev.py console-send printenv bootcmd            # send a line (+CR)
      ./dev.py console-send --raw ESC                   # send a bare ESC byte
      ./dev.py console-send --expect 'ALPINE_UBNT_NAS_ALL>' setenv x 1
      ./dev.py console-send --raw --expect Bytes CRLF   # send CRLF, wait for it
      ./dev.py console-send --raw --expect 'awto-nas#|ALPINE_UBNT_NAS_ALL>' CR

    --expect takes `|`-separated needles and returns on the FIRST to appear
    (one call to probe our-prompt vs stock vs login, not N). It prints
    "<<MATCHED: needle>>" so you know which fired.

    Without --expect it sends and reads a 1.5 s window (a shell echoes in ms, so
    ~1000x latency; empty output on no reply, never a hang). With --expect it
    reads UNTIL the needle appears - a condition, not a delay - and returns
    immediately when it does. --timeout is a FAILSAFE only (default 10 s: covers
    a U-Boot/shell command; raise it for tftp/nand writes). On expiry it prints
    what it captured and logs a loud FAIL naming the needle + elapsed, exit 3.
    --raw expands CR/LF/CRLF/ESC/TAB/CTRL-C/... and \\xNN escapes and does NOT
    append a CR (you control the line ending).

    --expect never matches inside the echo of what you just sent (its byte
    length is known exactly, so those bytes are skipped before searching) -
    a needle that's also literally in the command text, e.g.
    `--expect DONE ... "echo DONE"`, will NOT false-match on the echo, only
    on the command's real post-execution output."""
    if not _console_pid():
        log("console not running - start it with ./dev.py console", "ERROR")
        return 1
    raw = False
    expect: str | None = None
    timeout = 10.0
    words: list[str] = []
    it = iter(extra)
    for a in it:
        if a == "--raw":
            raw = True
        elif a == "--expect":
            expect = next(it, None)
            if expect is None:
                log("--expect needs a NEEDLE", "ERROR")
                return 2
        elif a == "--timeout":
            v = next(it, None)
            if v is None or not v.replace(".", "", 1).isdigit():
                log("--timeout needs seconds", "ERROR")
                return 2
            timeout = float(v)
        else:
            words.append(a)
    payload = _raw_bytes(words) if raw else (" ".join(words).encode() + b"\r")

    try:
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.settimeout(0.2)
        s.connect(str(CONSOLE_SOCK))
    except OSError as e:
        log(f"cannot connect to {CONSOLE_SOCK}: {e}", "ERROR")
        return 1
    s.sendall(payload)

    got = b""
    if expect is None:  # legacy: fixed read window
        end = time.monotonic() + 1.5
        while time.monotonic() < end:
            try:
                chunk = s.recv(4096)
            except TimeoutError:
                continue
            if not chunk:
                break
            got += chunk
        s.close()
        sys.stdout.buffer.write(got)
        sys.stdout.buffer.flush()
        return 0

    # `--expect` matches ANY of `|`-separated needles (one call instead of N
    # probes for awto-nas#/stock/login). On match, prints "<<MATCHED: needle>>" so
    # the caller knows which fired without a second round-trip.
    #
    # The console echoes typed input character-for-character before any real
    # output appears. A needle baked into the SAME command line (e.g.
    # `--expect DONE ... "echo DONE"`) used to match on that echo instantly,
    # before the command even ran - hit repeatedly during #131 debugging
    # despite being a known pitfall. Fix: the echo of our own send is exactly
    # len(payload) bytes, so never search those bytes - only what arrives
    # after can be genuine command output.
    needles = [n.encode() for n in expect.split("|") if n]
    echo_len = len(payload)
    t0 = time.monotonic()
    end = t0 + timeout
    while time.monotonic() < end:
        try:
            chunk = s.recv(4096)
        except TimeoutError:
            continue
        if not chunk:
            break
        got += chunk
        searchable = got[echo_len:] if len(got) > echo_len else b""
        hit = next((n for n in needles if n in searchable), None)
        if hit is not None:
            s.close()
            sys.stdout.buffer.write(got)
            sys.stdout.buffer.flush()
            print(f"\n<<MATCHED: {hit.decode(errors='replace')}>>")
            return 0
    s.close()
    sys.stdout.buffer.write(got)
    sys.stdout.buffer.flush()
    log(
        f"FAIL: none of {expect!r} seen in {time.monotonic() - t0:.1f}s "
        f"(failsafe {timeout:.0f}s)",
        "ERROR",
    )
    return 3


@command(
    "what's actually at the console right now - send CR, print whatever answers",
    kind="action",
)
def cmd_check(_extra: list[str]) -> int:
    """Ground truth for 'is it hung or just quiet'. A U-Boot/shell prompt often
    sits unread in the tio buffer with no trailing newline, so line-counting or
    grepping the console log (tmp/logs/unvr-console.log) can show no new lines
    even though something IS live and would answer instantly. This sends a bare
    CR and prints exactly what comes back in 1.5s - empty means genuinely no
    live prompt (or something still executing), non-empty means the box is
    responsive right now, whatever the log tail looks like."""
    return cmd_console_send([])


# Home of the pure-Python mini Jim-Tcl interpreter (written for espjtag). Reused
# here so console automation is Tcl scripts, same as the espjtag Tcl harness.
ESPJTAG_TCL = Path("/mnt/2tb/git/espjtag/scripts")


def _load_minijimtcl():
    import importlib

    if str(ESPJTAG_TCL) not in sys.path:
        sys.path.insert(0, str(ESPJTAG_TCL))
    try:
        return importlib.import_module("mini_jimtcl")
    except ModuleNotFoundError as e:
        raise RuntimeError(f"mini_jimtcl.py not found under {ESPJTAG_TCL} ({e})")


class _ConsoleTcl:
    """MiniJimTcl with send/send_raw/expect wired to the tio console socket. ONE
    persistent socket for the whole script, so expect sees output from a prior
    send (per-call connect would lose it). expect waits for a CONDITION (the
    needle) with a failsafe timeout that raises TclError (so Tcl `catch` works)."""

    def __init__(self, mod):
        self.s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.s.settimeout(0.2)
        self.s.connect(str(CONSOLE_SOCK))
        self.buf = b""
        self.TclError = mod.TclError
        self.tcl = mod.MiniJimTcl()
        self.tcl.register("send", self._send)
        self.tcl.register("send_raw", self._send_raw)
        self.tcl.register("expect", self._expect)

    def _send(self, a: list[str]) -> str:
        self.s.sendall(" ".join(a).encode() + b"\r")
        return ""

    def _send_raw(self, a: list[str]) -> str:
        self.s.sendall(_raw_bytes(a))
        return ""

    def _expect(self, a: list[str]) -> str:
        if not a:
            raise self.TclError("expect: needs a NEEDLE")
        needle = a[0].encode()
        timeout = (
            float(a[1]) if len(a) > 1 else 10.0
        )  # failsafe; expect waits on the needle
        end = time.monotonic() + timeout
        while True:
            i = self.buf.find(needle)
            if i >= 0:
                cut = i + len(needle)
                out, self.buf = self.buf[:cut], self.buf[cut:]
                return out.decode(errors="replace")
            if time.monotonic() >= end:
                raise self.TclError(f"expect: {a[0]!r} not seen in {timeout:.0f}s")
            try:
                chunk = self.s.recv(4096)
            except TimeoutError:
                continue
            if not chunk:
                raise self.TclError("expect: console socket closed")
            self.buf += chunk

    def close(self):
        try:
            self.s.close()
        except OSError:
            pass


@command(
    "drive the console with a Tcl script (send/expect via mini_jimtcl)",
    args="<script.tcl> | -e '<tcl>'",
    kind="action",
)
def cmd_console_tcl(extra: list[str]) -> int:
    """Reuses espjtag's mini_jimtcl - full Tcl (set/if/while/proc/catch/...) plus
    three console leaf commands:
      send <words...>       line + CR
      send_raw <CR|ESC|..>  raw bytes, no CR (keywords + \\xNN, as console-send)
      expect <needle> [s]   read UNTIL needle (a condition); TclError after [s]=10 failsafe
    e.g.  ./dev.py console-tcl reflash.tcl
          ./dev.py console-tcl -e 'send_raw CR; expect "login:"'
    A reflash.tcl is just: expect PROMPT; send "tftpboot ..."; expect "Bytes transferred"; ..."""
    if not _console_pid():
        log("console not running - start it with ./dev.py console", "ERROR")
        return 1
    if not extra:
        log("usage: ./dev.py console-tcl <script.tcl> | -e '<tcl>'", "ERROR")
        return 2
    if extra[0] == "-e":
        script = " ".join(extra[1:])
    else:
        p = Path(extra[0])
        if not p.exists():
            log(f"no such script: {p}", "ERROR")
            return 2
        script = p.read_text()
    try:
        mod = _load_minijimtcl()
    except RuntimeError as e:
        log(str(e), "ERROR")
        return 1
    ct = _ConsoleTcl(mod)
    try:
        out = ct.tcl.eval(script)
        if out:
            print(out)
        return 0
    except ct.TclError as e:
        log(f"tcl: {e}", "ERROR")
        return 3
    except Exception as e:  # interpreter/runtime error
        log(f"tcl error: {e}", "ERROR")
        return 3
    finally:
        ct.close()


@command(
    "write a bounded tail of the console log somewhere safe to open",
    args="[-n LINES]",
    kind="action",
)
def cmd_console_peek(extra: list[str]) -> int:
    """NEVER open the live console log in an editor. tio appends to it for a
    whole session and it only rolls at 20 MB, well past where an editor that
    tokenises the file on open becomes unusable. This writes the last N lines
    (default 500) to a separate file and prints that path. It is a snapshot, not
    a live view - re-run it to refresh, or `tail -f` the console log itself."""
    lines = CONSOLE_PEEK_LINES
    if extra:
        if (
            extra[0] not in ("-n", "--lines")
            or len(extra) != 2
            or not extra[1].isdigit()
        ):
            log("usage: ./dev.py console-peek [-n LINES]", "ERROR")
            return 2
        lines = int(extra[1])
    if not CONSOLE_LOG.exists():
        log(
            f"no console log at {CONSOLE_LOG.relative_to(REPO)} - "
            "start it with ./dev.py console",
            "ERROR",
        )
        return 1
    with CONSOLE_LOG.open("r", errors="replace") as fh:
        # deque(maxlen=) streams; read_text() on a 20 MB log is the same mistake
        # as opening it in an editor.
        tail = list(deque(fh, maxlen=lines))
    LOGS.mkdir(parents=True, exist_ok=True)
    CONSOLE_TAIL.write_text("".join(tail))
    log(
        f"{CONSOLE_LOG.relative_to(REPO)} "
        f"({CONSOLE_LOG.stat().st_size / (1024 * 1024):.1f} MB) -> "
        f"{CONSOLE_TAIL.relative_to(REPO)} ({len(tail)} lines, "
        f"{CONSOLE_TAIL.stat().st_size / 1024:.0f} KB)",
        "ALERT",
    )
    print(CONSOLE_TAIL)
    return 0


# --------------------------------------------------------------------------
# Meta / aggregate commands
# --------------------------------------------------------------------------
@command("machine-readable command list (JSON, for AI agents)", kind="meta")
def cmd_describe(_extra: list[str]) -> int:
    print(
        json.dumps(
            {
                "project": PROJECT,
                "schema": 1,
                "entrypoint": "./dev.py",
                "log": str(LOG.relative_to(REPO)),
                "board": {"sysid": BOARD_SYSID, "mac": BOARD_MAC},
                "console": {
                    "baud": CONSOLE_BAUD,
                    "socket": str(CONSOLE_SOCK),
                    "log": str(CONSOLE_LOG.relative_to(REPO)),
                },
                "exit_codes": {
                    "0": "success",
                    "2": "usage error",
                    "125": "step skipped (tool absent)",
                    "other": "failure",
                },
                "commands": {
                    name: {k: v for k, v in meta.items() if k != "fn"}
                    for name, meta in COMMANDS.items()
                },
            },
            indent=2,
        )
    )
    return 0


@command("check the toolchain is present; run this first when stuck", kind="meta")
def cmd_doctor(_extra: list[str]) -> int:
    log(f"project    : {PROJECT}")
    log(f"python     : {sys.version.split()[0]} ({sys.executable})")
    log(
        "gil        : "
        + ("disabled (free-threaded)" if not sys._is_gil_enabled() else "enabled")
        if hasattr(sys, "_is_gil_enabled")
        else "gil        : n/a (<3.13)"
    )
    missing = []
    for tool in ("tio", "nc", "ddrescue", "unsquashfs", "gh", "nmap"):
        if shutil.which(tool):
            log(f"{tool:10s} : ok")
        else:
            log(f"{tool:10s} : MISSING", "ERROR")
            missing.append(tool)
    port = _console_port()
    log(
        f"serial     : {port}" if port else "serial     : no adapter found",
        "INFO" if port else "WARN",
    )
    for name, (_s, argv, _e) in STEPS.items():
        if not argv:
            log(f"{name:10s} : not configured", "WARN")
        elif argv[0] == sys.executable or shutil.which(argv[0]):
            log(f"{name:10s} : {argv[0]} ok")
        else:
            log(f"{name:10s} : {argv[0]} MISSING", "ERROR")
            missing.append(argv[0])
    if missing:
        log(f"missing tools: {', '.join(sorted(set(missing)))}", "ERROR")
        return 1
    log("doctor ok", "ALERT")
    return 0


@command("remove build output and logs", kind="action")
def cmd_clean(_extra: list[str]) -> int:
    if _console_pid():
        log("console is running - stop it first (./dev.py console-stop)", "ERROR")
        return 1
    for path in (TMP,):
        if path.exists():
            shutil.rmtree(path)
            log(f"removed {path.relative_to(REPO)}")
    return 0


# --------------------------------------------------------------------------
# Build + box: wrap the U-Boot / Fedora builds and the chainload flow so a
# session runs one command instead of re-deriving the tftp + catch dance.
# --------------------------------------------------------------------------
TFTP_ROOT = TMP / "tftp"
CHAINLOAD_BIN = TFTP_ROOT / "u-boot-chainload.bin"
UBOOT_BIN = TMP / "uboot-build" / "u-boot.bin"


def _run_script(rel: str, extra: list[str]) -> int:
    p = REPO / rel
    if not p.exists():
        log(f"{rel} not found", "ERROR")
        return 1
    cmd = [sys.executable, str(p), *extra]
    log("run: " + " ".join(cmd))
    env = {**os.environ, "AWTO_VIA_DEVPY": "1"}
    return subprocess.call(cmd, cwd=REPO, env=env)


@command(
    "build our U-Boot chainload image (scripts/uboot-build.py)",
    args="[--clean]",
    kind="action",
)
def cmd_build_uboot(extra: list[str]) -> int:
    return _run_script("scripts/uboot-build.py", extra)


@command(
    "build the Fedora kernel + DTB + al_* modules, version set by "
    "AWTO_KERNEL_SRC/AWTO_KERNEL_VER env vars (scripts/build-linux-fedora.py)",
    kind="action",
)
def cmd_build_fedora(extra: list[str]) -> int:
    return _run_script("scripts/build-linux-fedora.py", extra)


@command(
    "build the ea16 netboot/initramfs image, version set by AWTO_KERNEL_SRC/ "
    "AWTO_KERNEL_VER env vars (scripts/build-linux-ea16.py)",
    kind="action",
)
def cmd_build_ea16(extra: list[str]) -> int:
    return _run_script("scripts/build-linux-ea16.py", extra)


@command(
    "build the Fedora rootfs tarball via dnf --installroot "
    "(scripts/build-fedora-rootfs.py)",
    args="[--keep]",
    kind="action",
)
def cmd_build_fedora_rootfs(extra: list[str]) -> int:
    return _run_script("scripts/build-fedora-rootfs.py", extra)


@command(
    "boot a kernel+DTB (+ optional module tree) via RAM through stock "
    "U-Boot, no flash/SSH needed to get there (scripts/ram-boot-deploy.py)",
    args="--kernel PATH --dtb PATH [--modules-tar PATH --kver KVER] [--skip-power-cycle]",
    kind="action",
)
def cmd_ram_boot_deploy(extra: list[str]) -> int:
    # No sudo needed: the interpreter (the real binary behind sys.executable's
    # symlink chain) carries cap_net_bind_service, granted once via
    # `sudo setcap 'cap_net_bind_service=+ep' <real python3 binary>` - it can
    # bind UDP port 69 itself (embedded tftpd, in-process - see
    # ram-boot-deploy.py) as the normal user, no privilege escalation and no
    # XDG_RUNTIME_DIR-under-root console-socket mismatch to work around.
    return _run_script("scripts/ram-boot-deploy.py", extra)


@command(
    "power-cycle the UNVR via the Sonoff TH outlet, verifying it lands ON "
    "(scripts/power-cycle.py)",
    kind="action",
)
def cmd_power_cycle(extra: list[str]) -> int:
    # The one power-control primitive - use this instead of an inline
    # `python3 -c "... aioesphomeapi ..."` snippet (three of those in one
    # session, each a duplicate of scripts/ram-boot-deploy.py's own
    # power_cycle_verified(), which now imports the same scripts/_power.py
    # this runs).
    return _run_script("scripts/power-cycle.py", extra)


@command(
    "sweep the 10G SerDes TX equalisation knobs (amp / pre+post cursor taps / "
    "slew) and measure TX throughput at each - #121 signal-integrity probe "
    "(scripts/serdes-tx-sweep.py)",
    args="[--param amp|c_plus_1|c_minus_1|...] [--values 1,2,3] [--duration N]",
    kind="action",
)
def cmd_serdes_tx_sweep(extra: list[str]) -> int:
    return _run_script("scripts/serdes-tx-sweep.py", extra)


@command(
    "A/B regression benchmark suite: iperf3 on every ethernet port (1G, 10G, "
    "and any USB-attached), crypto, SATA disks, USB disk - JSON snapshot, "
    "--compare diffs two (scripts/bench-all.py)",
    args="[--skip-eth|--skip-crypto|--skip-disk|--skip-usb] [--compare A B]",
    kind="action",
)
def cmd_bench_all(extra: list[str]) -> int:
    return _run_script("scripts/bench-all.py", extra)


@command(
    "power-cycle, catch the U-Boot prompt, run read-only bench diagnostics "
    "(scripts/uboot-bench-check.py) - no writes, no chainload jump",
    kind="action",
)
def cmd_uboot_bench_check(extra: list[str]) -> int:
    return _run_script("scripts/uboot-bench-check.py", extra)


@command(
    "build the UNVR P0 EDK2 firmware volume (scripts/build-uefi-p0.py, "
    "docs/uefi.md) - software only, no hardware contact",
    kind="action",
)
def cmd_build_uefi_p0(extra: list[str]) -> int:
    return _run_script("scripts/build-uefi-p0.py", extra)


@command(
    "docs/uefi.md §5's dry chainload probe: tftp+crc32-verify+`go` the P0 "
    "UNVR.fd, watch for the UEFI Shell prompt (scripts/uefi-chainload-probe.py)",
    kind="action",
)
def cmd_uefi_chainload_probe(extra: list[str]) -> int:
    return _run_script("scripts/uefi-chainload-probe.py", extra)


@command(
    "send a serial BREAK (+ optional Magic SysRq letter) to the UNVR's "
    "console, e.g. to grab a live blocked-task dump during a hang that "
    "console-send can't reach - tio's socket mode doesn't relay ctrl-t "
    "sequences (scripts/serial-break.py)",
    args="[--sysrq LETTER]",
    kind="action",
)
def cmd_serial_break(extra: list[str]) -> int:
    return _run_script("scripts/serial-break.py", extra)


def _tftpd_bound(port: int) -> bool:
    r = subprocess.run(
        ["ss", "-lun", f"sport = :{port}"],
        capture_output=True,
        text=True,
        check=False,
    )
    return "UNCONN" in r.stdout


def _kill_stale_tftpd(port: int = 69) -> None:
    """Kill whatever's bound to `port`, ours or foreign. #119: a tftpd left
    running from 3 days earlier (wrong --root, images/tftp not tmp/tftp) was
    silently reused by the old bound()-only check, serving a 4-day-stale
    U-Boot binary that crash-reset the box. Never reuse - always kill first,
    so a fresh, correctly-rooted server starts every time."""
    if not _tftpd_bound(port):
        return
    log(f"killing stale process on port {port} (#119 - never reuse a tftpd)", "WARN")
    subprocess.run(
        ["sudo", "fuser", "-k", f"{port}/udp"],
        capture_output=True,
        check=False,
    )
    for _ in range(20):
        if not _tftpd_bound(port):
            return
        time.sleep(0.05)
    log(f"tftpd on port {port} still bound after 1s - proceeding anyway", "WARN")


def _ensure_tftpd(port: int = 69, root: str = "tmp/tftp") -> None:
    """Kill any existing process on `port` (#119), then start a fresh
    scripts/tftpd.py serving `root`. Waits for the bind: expected <100 ms,
    bounded 40x50ms=2s, then warn+proceed (the chainload catch loop is long
    enough to tolerate a late bind).

    `root` is NOT optional-by-convention: publish-fedora stages NAND-flash
    artifacts to images/tftp (a longer-lived, semi-persistent location -
    flash can happen a while after publish, in a separate dev.py
    invocation), while uboot-test/chainload use tmp/tftp (throwaway,
    started+served fresh each time). #119 root cause was exactly this
    distinction going unmanaged: images/tftp's server was started once,
    manually, outside any script's ownership, and nothing tracked or
    refreshed it - it silently went stale over days until a later #119 fix
    (correctly) killed it as a foreign stale process, which also removed
    the NAND-flash workflow's only server out from under it. Fix: whichever
    command STAGES a file for a root now also explicitly (re)starts that
    root's server, so it's never implicit/manual again."""
    _kill_stale_tftpd(port)
    root_path = REPO / root
    root_path.mkdir(parents=True, exist_ok=True)
    log(f"starting tftpd (root {root})")
    # Port 69 is <1024 - tftpd.py itself refuses to bind it without root
    # (checked directly: exits 1, "needs root for <1024"). This was silently
    # swallowed before (stdout/stderr -> DEVNULL) - the process just died
    # immediately and every caller "continued anyway" against nothing.
    cmd = [
        sys.executable,
        str(REPO / "scripts" / "tftpd.py"),
        "--root",
        root,
        "--port",
        str(port),
    ]
    # sudo sanitizes the environment by default, so AWTO_VIA_DEVPY (needed
    # past the _repo.py script-invocation guard) wouldn't survive a plain
    # `sudo -n` prefix - route it through `env` explicitly instead of
    # relying on env_keep being configured in sudoers.
    if port < 1024:
        cmd = ["sudo", "-n", "env", "AWTO_VIA_DEVPY=1", *cmd]
    subprocess.Popen(
        cmd,
        cwd=REPO,
        env={**os.environ, "AWTO_VIA_DEVPY": "1"},
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        start_new_session=True,
    )
    for _ in range(40):
        if _tftpd_bound(port):
            return
        time.sleep(0.05)
    log("tftpd not bound after 2s - is passwordless sudo set up for tftpd.py?", "ERROR")


def _verify_uboot_banner(expected_bin: Path) -> bool:
    """Post-boot check (#119): confirm the console actually printed the
    banner embedded in `expected_bin`, not a stale binary from an earlier
    build. U-Boot's own banner ("U-Boot 2026.07-dirty (Mon DD YYYY -
    HH:MM:SS +ZZZZ)") is a literal string baked in at build time - extract it
    from the just-built binary and from the live console log, and compare
    the full line: any mismatch means the box booted something else."""
    banner_re = re.compile(
        rb"U-Boot 20\d\d\.\d\d-\w+ \([A-Za-z]+ \d+ \d{4} - [\d:]+ [+-]\d{4}\)"
    )
    try:
        blob = expected_bin.read_bytes()
    except OSError as e:
        log(f"banner check: can't read {expected_bin}: {e}", "WARN")
        return False
    m = banner_re.search(blob)
    if not m:
        log("banner check: no U-Boot banner string found in the built binary", "WARN")
        return False
    expected = m.group(0).decode()

    log_path = REPO / "tmp" / "logs" / "unvr-console.log"
    try:
        tail = log_path.read_bytes()[-200_000:]
    except OSError as e:
        log(f"banner check: can't read console log: {e}", "WARN")
        return False
    seen = [mm.group(0).decode() for mm in banner_re.finditer(tail)]
    if not seen:
        log("banner check: no U-Boot banner seen on the console yet", "WARN")
        return False
    actual = seen[-1]
    if actual == expected:
        log(f"banner check OK: {actual}")
        return True
    log(
        f"BANNER MISMATCH: box printed {actual!r}, built binary is {expected!r}",
        "ERROR",
    )
    log(
        "this means the box booted a DIFFERENT build than the one just staged - stop",
        "ERROR",
    )
    return False


@command(
    "stage the built U-Boot + chainload it onto the box "
    "(tftpd + catch stock + tftpboot + go)",
    args="[chainload.tcl]",
    kind="action",
)
def cmd_chainload(extra: list[str]) -> int:
    if not UBOOT_BIN.exists():
        log(
            f"no {UBOOT_BIN.relative_to(REPO)} - run ./dev.py build-uboot first",
            "ERROR",
        )
        return 1
    TFTP_ROOT.mkdir(parents=True, exist_ok=True)
    shutil.copy2(UBOOT_BIN, CHAINLOAD_BIN)
    log(f"staged {CHAINLOAD_BIN.relative_to(REPO)} ({UBOOT_BIN.stat().st_size} bytes)")
    _ensure_tftpd()
    scripts_dir = str(REPO / "scripts")
    if scripts_dir not in sys.path:
        sys.path.insert(0, scripts_dir)
    from _net import detect_server_ip

    server_ip = detect_server_ip()
    log(f"tftp server IP (auto-detected): {server_ip}")
    tcl = extra[0] if extra else "scripts/chainload-and-test.tcl"
    log(f"chainloading via {tcl} (reset the box now to hit stock U-Boot)")
    script = f"set SERVERIP {server_ip}\n" + Path(tcl).read_text()
    return cmd_console_tcl(["-e", script])


def _probe_awto_nas(timeout_s: float = 4.0) -> bool:
    """Quick, non-disruptive check: is the console already sitting at our
    own awto-nas# prompt right now? Used so uboot-test can pick warm vs
    cold automatically instead of the caller having to know/remember the
    box's current state (the actual point of "one command that works
    every time" - see dev.py's uboot-test docstring)."""
    scripts_dir = str(REPO / "scripts")
    if scripts_dir not in sys.path:
        sys.path.insert(0, scripts_dir)
    import _console

    try:
        s = _console.connect()
    except FileNotFoundError:
        return False
    try:
        s.sendall(b"\r")
        buf = b""
        end = time.monotonic() + timeout_s
        while time.monotonic() < end:
            try:
                d = s.recv(4096)
                if d:
                    buf += d
            except TimeoutError:
                continue
            if b"awto-nas#" in buf:
                return True
        return False
    finally:
        s.close()


@command(
    "put the FRESH build on the box + stop at awto-nas# for hands-on testing, "
    "from ANY starting box state: SP805-reset -> catch stock -> tftp -> go "
    "(scripts/uboot-test.tcl). Auto-detects warm (already at awto-nas#) vs "
    "cold (power-cycle first) - pass --cold/--warm to force one explicitly.",
    kind="action",
)
def cmd_uboot_test(_extra: list[str]) -> int:
    if not UBOOT_BIN.exists():
        log(
            f"no {UBOOT_BIN.relative_to(REPO)} - run ./dev.py build-uboot first",
            "ERROR",
        )
        return 1
    if "--cold" in _extra:
        cold = True
    elif "--warm" in _extra:
        cold = False
    else:
        at_prompt = _probe_awto_nas()
        cold = not at_prompt
        log(f"uboot-test: auto-detected {'COLD (not at awto-nas#)' if cold else 'WARM (already at awto-nas#)'}")
    TFTP_ROOT.mkdir(parents=True, exist_ok=True)
    shutil.copy2(UBOOT_BIN, CHAINLOAD_BIN)
    log(f"staged {CHAINLOAD_BIN.relative_to(REPO)} ({UBOOT_BIN.stat().st_size} bytes)")
    _ensure_tftpd()
    scripts_dir = str(REPO / "scripts")
    if scripts_dir not in sys.path:
        sys.path.insert(0, scripts_dir)
    from _net import detect_server_ip

    server_ip = detect_server_ip()
    log(f"tftp server IP (auto-detected): {server_ip}")
    power_proc = None
    if cold:
        # Non-blocking: power_cycle_verified()'s own cut+restore+verify cycle
        # takes real wall-clock seconds on its own, on top of the SoC's own
        # ~9s ROM/S2/preboot chain before stock's autoboot window even opens
        # - blocking on it first (as an earlier version of this did) starts
        # the ESC-catch loop below too late, missing the window entirely
        # (confirmed live 2026-09-03: 15+ min of nothing but console bell
        # chars, no boot prompt ever caught). The proven pattern used
        # elsewhere this session (uefi-chainload-probe.py) starts catching
        # BEFORE triggering the reset - this starts the power-cycle in the
        # background and immediately proceeds to the catch loop instead,
        # so both are in flight concurrently rather than strictly sequenced.
        log("uboot-test --cold: starting power-cycle in background, catch loop starts now")
        power_proc = subprocess.Popen(
            [sys.executable, "dev.py", "power-cycle"], cwd=REPO
        )
    log("uboot-test: reset+catch+tftp+go, then STOP at awto-nas# (no auto-tests)")
    cold_line = "set COLD 1\n" if cold else ""
    script = f"set SERVERIP {server_ip}\n{cold_line}" + Path("scripts/uboot-test.tcl").read_text()
    rc = cmd_console_tcl(["-e", script])
    if power_proc is not None:
        power_rc = power_proc.wait(timeout=30)
        if power_rc != 0:
            log(f"power-cycle.py exited {power_rc} - verify box power state", "ERROR")
    return rc


@command(
    "redeploy a completely fresh Fedora rootfs onto the SSD: SP805-reset "
    "(over SSH) -> catch stock -> netboot installer -> reformat sda2 -> "
    "stream fresh rootfs+modules over HTTP. Leaves box at installer shell; "
    "run reboot-to-uboot.tcl + ./dev.py flash + power-cycle after to boot "
    "it (scripts/deploy-fedora-rootfs.tcl)",
    kind="action",
)
def cmd_deploy_fedora_rootfs(_extra: list[str]) -> int:
    scripts_dir = str(REPO / "scripts")
    if scripts_dir not in sys.path:
        sys.path.insert(0, scripts_dir)
    from _net import detect_server_ip
    from _repo import ea16_build_out, kernel_build_ver
    import _fedora_deploy as fd

    ea16_out = ea16_build_out()
    ea16_ver = kernel_build_ver()
    ea16_uimage = ea16_out / f"uImage-unvr-ea16-{ea16_ver}"
    ea16_dtb = ea16_out / f"alpine-v2-ubnt-unvr-ea16-{ea16_ver}.dtb"
    rootfs_tar = REPO / "tmp" / "fedora-rootfs-ea16.tar"
    for p in (ea16_uimage, ea16_dtb, rootfs_tar):
        if not p.is_file():
            log(f"ABORT: missing {p} - build it first", "ERROR")
            return 1
    if not fd.MODROOT.is_dir():
        log(f"ABORT: no module tree at {fd.MODROOT} - build the fedora kernel first", "ERROR")
        return 1

    TFTP_ROOT.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ea16_uimage, TFTP_ROOT / ea16_uimage.name)
    shutil.copy2(ea16_dtb, TFTP_ROOT / ea16_dtb.name)
    log(f"staged installer {ea16_uimage.name} + {ea16_dtb.name} into {TFTP_ROOT.relative_to(REPO)}")
    _ensure_tftpd()

    serve_dir = REPO / "tmp" / "rootfs-deploy"
    serve_dir.mkdir(parents=True, exist_ok=True)
    link = serve_dir / "fedora-rootfs-ea16.tar"
    if link.is_symlink() or link.exists():
        link.unlink()
    link.symlink_to(rootfs_tar)
    modtar = serve_dir / f"modules-{fd.KVER}.tar"
    subprocess.run(
        ["tar", "-C", str(fd.MODROOT.parent), "-cf", str(modtar), fd.KVER], check=True
    )
    log(f"staged rootfs tar (symlink) + fresh module tar ({modtar.name}) in {serve_dir.relative_to(REPO)}")

    import http.server
    import socketserver
    import threading
    from functools import partial

    http_port = 8100
    handler = partial(http.server.SimpleHTTPRequestHandler, directory=str(serve_dir))
    httpd = socketserver.TCPServer(("0.0.0.0", http_port), handler)
    threading.Thread(target=httpd.serve_forever, daemon=True).start()
    server_ip = detect_server_ip()
    log(f"serving {serve_dir.relative_to(REPO)} on {server_ip}:{http_port}")

    host = subprocess.run(
        [sys.executable, "scripts/ssh-woomera.py", "--print"],
        cwd=REPO, capture_output=True, text=True, timeout=15, check=False,
    ).stdout.strip()
    if not host:
        log("ABORT: woomera not reachable over SSH - can't arm the SP805 reset", "ERROR")
        httpd.shutdown()
        return 1
    log(f"arming SP805 watchdog over SSH ({host}) - box resets to stock U-Boot in ~2s")
    subprocess.run(
        [
            "sshpass", "-p", fd.ROOT_PASSWORD, "ssh",
            "-o", "StrictHostKeyChecking=accept-new",
            "-o", "PreferredAuthentications=password",
            "-o", "PubkeyAuthentication=no",
            "-o", "ConnectTimeout=5",
            f"root@{host}",
            "python3 -c \"import fcntl,struct; f=open('/dev/watchdog','r+b',buffering=0); "
            "fcntl.ioctl(f,0xC0045706,struct.pack('I',1)); exec('while True: pass')\"",
        ],
        timeout=8, check=False,
    )

    script = (
        f"set SERVERIP {server_ip}\nset HTTPPORT {http_port}\n"
        + Path("scripts/deploy-fedora-rootfs.tcl").read_text()
    )
    rc = cmd_console_tcl(["-e", script])
    httpd.shutdown()
    return rc


@command(
    "sync a freshly-built Fedora rootfs onto woomera's LIVE running SSD "
    "over SSH - rsync --delete, no reboot/reformat/console needed "
    "(scripts/sync-fedora-rootfs.py) [--dry-run] [--yes]",
    args="[--dry-run] [--yes]",
    kind="action",
)
def cmd_sync_fedora_rootfs(extra: list[str]) -> int:
    return _run_script("scripts/sync-fedora-rootfs.py", extra)


@command(
    "poll the console log for boot progress (trouble markers / login prompt) "
    "instead of one long blocking wait (scripts/wait-for-boot.py)",
    kind="action",
)
def cmd_wait_for_boot(extra: list[str]) -> int:
    return _run_script("scripts/wait-for-boot.py", extra)


@command(
    "phase 1/2 of a Fedora deploy: regenerate tftp uImage+DTB from build-out "
    "+ sync module tree onto woomera. Run while Fedora is up (scripts/publish-fedora.py)",
    kind="action",
)
def cmd_publish_fedora(extra: list[str]) -> int:
    # Ensure a fresh, correctly-rooted server BEFORE staging (#119): flash
    # may happen much later in a separate invocation, so this can't be a
    # start/stop-around-the-call pattern - it must be left running for
    # whenever ./dev.py flash eventually runs.
    _ensure_tftpd(root="images/tftp")
    return _run_script("scripts/publish-fedora.py", extra)


@command(
    "phase 2/2 of a Fedora deploy: flash the published kernel+DTB into NAND + "
    "set U-Boot to boot it. Run ./dev.py publish-fedora first, then reset to "
    "U-Boot (scripts/flash-nand.py)",
    kind="action",
)
def cmd_flash(extra: list[str]) -> int:
    # Safety net (#119): if the images/tftp server died or was never
    # started (e.g. flash run without a fresh publish-fedora just before),
    # don't let the box sit mid-tftpboot against nothing. Only starts one
    # if genuinely absent - doesn't disturb a live, correctly-rooted server.
    if not _tftpd_bound(69):
        log("no tftpd bound on 69 - starting one before flash (#119)", "WARN")
        _ensure_tftpd(root="images/tftp")
    return _run_script("scripts/flash-nand.py", extra)


def _box_has_module_tree(kver: str) -> bool:
    """Does the box already have /lib/modules/<kver>? Checked over the serial
    console, so it works even when the box has no network (which is exactly
    when it matters - see deploy-fedora's bootstrap path)."""
    scripts_dir = str(REPO / "scripts")
    if scripts_dir not in sys.path:
        sys.path.insert(0, scripts_dir)
    import _console

    s = None
    try:
        s = _console.connect()
        _console.login(s)
        _, out = _console.sh(
            s, f"test -d /lib/modules/{kver} && echo MODS_YES || echo MODS_NO", timeout=15
        )
    except Exception:
        return False
    finally:
        if s is not None:
            s.close()
    return "MODS_YES" in (out or "")


def _reset_to_uboot() -> int:
    """Get the box to a U-Boot prompt, whatever state it's in.

    Prefers the SP805 watchdog reset (fast, no power-cycle) but that needs
    /dev/watchdog, which needs the watchdog MODULE - and a box booted on a
    kernel whose module tree is missing has no /dev/watchdog at all (hit
    live 2026-09-03: NAND held 7.1.8-dirty while /lib/modules had only
    7.2/7.3 trees, so nothing loaded - no watchdog AND no network, #165).
    Checks for the device first and falls back to power-cycle + catch-stock
    rather than spending ~400s spamming ESC after a watchdog that was never
    going to fire."""
    scripts_dir = str(REPO / "scripts")
    if scripts_dir not in sys.path:
        sys.path.insert(0, scripts_dir)
    import _console

    has_watchdog = False
    s = None
    try:
        s = _console.connect()
        _console.login(s)
        _, out = _console.sh(s, "test -c /dev/watchdog && echo WD_YES || echo WD_NO", timeout=10)
        has_watchdog = "WD_YES" in (out or "")
    except Exception:
        pass
    finally:
        if s is not None:
            s.close()

    if has_watchdog:
        log("reset-to-uboot: SP805 watchdog reset (no power-cycle)")
        rc = cmd_console_tcl(["scripts/reboot-to-uboot.tcl"])
        if rc == 0:
            return 0
        log("reset-to-uboot: watchdog path failed - falling back to power-cycle", "WARN")
    else:
        log("reset-to-uboot: no /dev/watchdog (unhealthy or non-Linux state) - "
            "using power-cycle + catch-stock")

    # Same catch-before-reset ordering as uboot-test --cold / the EDK2 probe:
    # the ESC stream must already be running when power comes back.
    catch = subprocess.Popen(
        [sys.executable, "scripts/catch-uboot.py", "--seconds", "90"],
        cwd=REPO,
        env={**os.environ, "AWTO_VIA_DEVPY": "1"},
    )
    rc = cmd_power_cycle([])
    if rc != 0:
        catch.kill()
        log("reset-to-uboot: power-cycle failed", "ERROR")
        return rc
    try:
        return catch.wait(timeout=120)
    except subprocess.TimeoutExpired:
        catch.kill()
        log("reset-to-uboot: never caught the stock U-Boot prompt", "ERROR")
        return 1


def _probe_fedora_shell(timeout_s: float = 8.0) -> bool:
    """Is the box currently at a usable Fedora shell (login prompt or an
    already-logged-in root shell)? Used by deploy-fedora to decide whether
    it needs to boot the box first - same reasoning as _probe_awto_nas()."""
    scripts_dir = str(REPO / "scripts")
    if scripts_dir not in sys.path:
        sys.path.insert(0, scripts_dir)
    import _console

    # `uname -s`, not `echo <marker>`: U-Boot has its own `echo` builtin, so a
    # marker-echo probe returns a false positive at the awto-nas# prompt and
    # deploy-fedora then skips booting the box and fails confusingly later
    # (hit live 2026-09-03). U-Boot has no `uname`, so requiring "Linux" in
    # the output distinguishes a real Linux shell from a bootloader prompt.
    s = None
    try:
        s = _console.connect()
        _console.login(s)
        rc, out = _console.sh(s, "uname -s", timeout=timeout_s)
    except Exception:
        return False
    finally:
        if s is not None:
            s.close()
    return "Linux" in (out or "")


@command(
    "ONE-SHOT Fedora deploy, any starting box state: publish (tftp artifacts + "
    "module sync) -> reset to U-Boot -> flash NAND -> boot -> VERIFY the box "
    "actually came up running the kernel just flashed. Fails loudly rather "
    "than leaving a half-deployed box (#165)",
    kind="action",
)
def cmd_deploy_fedora(extra: list[str]) -> int:
    # #105/#161/#165 are all the same shape: a two-phase deploy where one
    # phase ran and the other silently didn't, leaving NAND's kernel and the
    # deployed module tree from different builds. publish-fedora and flash
    # stay available separately, but the default path is this - one command,
    # ending in a real check that the box boots what was just built, which
    # is the step none of those incidents had.
    scripts_dir = str(REPO / "scripts")
    if scripts_dir not in sys.path:
        sys.path.insert(0, scripts_dir)
    # _fedora_deploy imports _repo, whose _enforce_via_devpy() guard fires on
    # import. dev.py only sets AWTO_VIA_DEVPY for the children it spawns, not
    # for its own process - but we ARE dev.py, which is exactly what that
    # guard exists to permit (it's there to stop bare `python3 scripts/x.py`).
    os.environ.setdefault("AWTO_VIA_DEVPY", "1")
    import _console
    from _fedora_deploy import KVER

    log(f"deploy-fedora: target kernel {KVER}")

    # publish-fedora needs a live Fedora (module rsync over SSH). The box
    # could be anywhere right now - U-Boot, EDK2, powered down, mid-boot -
    # so get it to Fedora first rather than failing and making the caller
    # do it, which is the whole point of this being one command.
    if not _probe_fedora_shell():
        log("deploy-fedora: box is not at a Fedora shell - power-cycling to boot it")
        rc = cmd_power_cycle([])
        if rc != 0:
            log("deploy-fedora: power-cycle failed", "ERROR")
            return rc
        # wait-for-boot returns non-zero if it saw ANY trouble line during
        # boot (e.g. a benign lm_sensors.service failure) even when it also
        # reached the login prompt - too strict to abort a deploy on. The
        # authoritative test is whether the box is actually usable now, which
        # _probe_fedora_shell() answers directly, so that decides.
        rc = cmd_wait_for_boot([])
        if rc != 0:
            log("deploy-fedora: wait-for-boot reported trouble lines - "
                "probing the shell directly to decide", "WARN")
        if not _probe_fedora_shell():
            log("deploy-fedora: no usable Fedora shell after boot", "ERROR")
            return 1

    rc = cmd_publish_fedora(extra)
    if rc != 0:
        # Bootstrap case (#165): a box booted on a kernel whose module tree is
        # missing has no network, so the module rsync can't run - but flashing
        # the new kernel is exactly what fixes that, and publish_artifacts()
        # has already regenerated the tftp images by the time the rsync fails.
        # Only proceed if the target kernel's module tree is ALREADY on the box
        # (checkable over the console, no network needed) - that keeps the real
        # invariant intact: never boot a kernel without its matching modules.
        if _box_has_module_tree(KVER):
            log(f"deploy-fedora: publish's module sync failed (box unreachable), "
                f"but /lib/modules/{KVER} is already present on the box - "
                f"continuing to flash to bootstrap it back online. Re-run this "
                f"command afterwards to sync the current modules.", "WARN")
        else:
            log(f"deploy-fedora: publish failed and the box has no "
                f"/lib/modules/{KVER} - NOT flashing (booting it would give a "
                f"kernel with no modules, which is how this box got wedged in "
                f"the first place, #165)", "ERROR")
            return rc

    rc = _reset_to_uboot()
    if rc != 0:
        log("deploy-fedora: could not reach the U-Boot prompt - NOT flashing", "ERROR")
        return rc

    rc = cmd_flash(extra)
    if rc != 0:
        log("deploy-fedora: FLASH FAILED - NAND may hold the previous kernel "
            "while the module tree on disk is now the new one. Re-run this "
            "command; do not leave the box in this state (#165)", "ERROR")
        return rc

    log("deploy-fedora: booting the freshly-flashed kernel")
    rc = cmd_boot_verify([])
    if rc != 0:
        log("deploy-fedora: box did not reach a Fedora login after flashing", "ERROR")
        return rc

    # The check none of #105/#161/#165 had: confirm the RUNNING kernel is
    # the one just flashed, not whatever NAND happened to still hold.
    s = _console.connect()
    try:
        _console.login(s)
        _, running = _console.sh(s, "uname -r", timeout=15)
    finally:
        s.close()
    running = running.strip().splitlines()[-1].strip() if running.strip() else ""
    if running != KVER:
        log(f"deploy-fedora: MISMATCH - flashed {KVER} but box is running "
            f"{running!r}. NAND did not take, or it booted something else.", "ERROR")
        return 1
    log(f"deploy-fedora: verified - box is running {running}, the kernel just flashed")
    return 0


@command(
    "netboot: catch U-Boot, tftp kernel+DTB, bootm in one session (scripts/netboot.py)",
    args="--tag <7.1|6.18> [opts]",
    kind="action",
)
def cmd_netboot(extra: list[str]) -> int:
    return _run_script("scripts/netboot.py", extra)


@command(
    "boot the flashed kernel + wait for Fedora login (scripts/boot-verify.tcl)",
    kind="action",
)
def cmd_boot_verify(_extra: list[str]) -> int:
    return cmd_console_tcl(["scripts/boot-verify.tcl"])


@command("fail-fast pre-commit gate: " + " + ".join(GATE), kind="aggregate")
def cmd_gate(_extra: list[str]) -> int:
    for name in GATE:
        rc = run_step(name)
        if rc not in (0, SKIPPED):
            log(f"GATE FAILED at {name} (rc={rc})", "FATAL")
            return rc
    log("GATE PASSED", "ALERT")
    return 0


@command("run every leg, collect all results, one GO/NO-GO", kind="aggregate")
def cmd_ci(_extra: list[str]) -> int:
    results: dict[str, int] = {}
    for name in CI:
        results[name] = run_step(name)  # no early exit - that is the point
    failed = {k: v for k, v in results.items() if v not in (0, SKIPPED)}
    skipped = [k for k, v in results.items() if v == SKIPPED]
    log("-" * 52)
    for name, rc in results.items():
        state = "SKIP" if rc == SKIPPED else ("ok" if rc == 0 else f"FAIL rc={rc}")
        log(f"  {name:12s} {state}", "INFO" if rc in (0, SKIPPED) else "ERROR")
    if skipped:
        log(f"{len(skipped)} step(s) skipped: {', '.join(skipped)}", "WARN")
    if failed:
        log(
            f"NO-GO - {len(failed)} of {len(results)} failed: {', '.join(failed)}",
            "FATAL",
        )
        return 1
    log("GO", "ALERT")
    return 0


@command("verify dev.py's own contract (help/describe/exit codes)", kind="meta")
def cmd_selftest(_extra: list[str]) -> int:
    failures = []

    def check(label: str, cond: bool) -> None:
        log(f"  {label}: {'ok' if cond else 'FAIL'}", "INFO" if cond else "ERROR")
        if not cond:
            failures.append(label)

    out = subprocess.run(
        [sys.executable, __file__, "describe"],
        capture_output=True,
        text=True,
        cwd=REPO,
        check=False,
    )
    check("describe exits 0", out.returncode == 0)
    try:
        doc = json.loads(out.stdout)
        check("describe emits valid JSON", True)
        check("describe has project+commands", {"project", "commands"} <= doc.keys())
        check(
            "every command documented",
            all(m.get("summary") for m in doc.get("commands", {}).values()),
        )
    except json.JSONDecodeError:
        check("describe emits valid JSON", False)

    h = subprocess.run(
        [sys.executable, __file__, "--help"],
        capture_output=True,
        text=True,
        cwd=REPO,
        check=False,
    )
    check("--help exits 2 (usage)", h.returncode == 2)
    check("--help lists commands", all(c in h.stdout for c in COMMANDS))

    u = subprocess.run(
        [sys.executable, __file__, "no-such-command"],
        capture_output=True,
        text=True,
        cwd=REPO,
        check=False,
    )
    check("unknown command exits 2", u.returncode == 2)

    check(
        "registry has no duplicate handlers",
        len({m["fn"] for m in COMMANDS.values()}) == len(COMMANDS),
    )

    if failures:
        log(f"selftest FAILED: {', '.join(failures)}", "FATAL")
        return 1
    log("selftest ok", "ALERT")
    return 0


def _register_steps() -> None:
    """Expose each STEPS entry as a first-class command."""
    for name, (summary, _argv, takes_extra) in STEPS.items():
        if name in COMMANDS:
            continue
        COMMANDS[name] = {
            "summary": summary,
            "args": "[extra args passed through]" if takes_extra else "",
            "kind": "step",
            "fn": lambda extra, n=name: run_step(n, extra),
        }


_register_steps()


def usage() -> int:
    print(__doc__.split("Console model:")[0].rstrip())
    print("\nUsage: ./dev.py <command> [args]\n")
    order = ["aggregate", "step", "action", "meta"]
    kinds = sorted(
        {m["kind"] for m in COMMANDS.values()},
        key=lambda k: (order.index(k) if k in order else len(order), k),
    )
    for kind in kinds:
        group = {n: m for n, m in COMMANDS.items() if m["kind"] == kind}
        if not group:
            continue
        print(f"  {kind}:")
        for name, meta in sorted(group.items()):
            arg = f" {meta['args']}" if meta["args"] else ""
            print(f"    {name:<16}{meta['summary']}{arg}")
        print()
    print(f"Log: {LOG.relative_to(REPO)}   JSON help: ./dev.py describe")
    return 2


def main(argv: list[str] | None = None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    if not argv or argv[0] in ("-h", "--help", "help"):
        return usage()
    name, *extra = argv
    meta = COMMANDS.get(name)
    if meta is None:
        print(f"unknown command: {name!r} - try ./dev.py describe", file=sys.stderr)
        return 2
    rc = meta["fn"](extra)
    return 0 if rc == SKIPPED else rc


if __name__ == "__main__":
    sys.exit(main())
