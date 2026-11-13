#!/usr/bin/env python3
"""Poll for boot progress instead of one long blocking expect.

Console path (tio live): polls tmp/logs/unvr-console.log every POLL_S,
prints progress, returns on the login prompt or a trouble marker (shared
regex with boot-watch.py), whichever comes first.

SSH path (no live console, #352): polls `uname -r` + boot_id over ssh.
- Success = a new boot (boot_id != --old-boot-id) running --kver, when given.
- Cannot see console trouble lines; only answers "is it back, on what kernel".

Exit: 0 booted clean, 1 booted with trouble lines (console only),
      3 timeout, 4 booted a different kernel than --kver (ssh only).

Console timeout: TIMEOUT_S. A clean boot reaches login in ~15-20s; the slowest
OBSERVED boot was ~200s, when a prior kernel Oops left dbus-broker /
NetworkManager retrying during startup (2026-08-20, #106). 300s = ~1.5x that.
SSH timeout: SSH_TIMEOUT_S below, derivation in #352.

Run: ./dev.py wait-for-boot [--kver <uname -r>] [--old-boot-id <id>]
"""

from __future__ import annotations

import argparse
import re
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _box  # noqa: E402
import _console  # noqa: E402
from _repo import LOGS, make_log  # noqa: E402

CONSOLE_LOG = LOGS / "unvr-console.log"
POLL_S = 2.0
TIMEOUT_S = 300.0

# Waits for: reset -> sshd answering on the new kernel. Expected ~70 s, worst
# ~80 s (console log reset->login + sshd start, #352). 1.5x -> 120 s.
# Expiry: log limit, elapsed and last ssh error; exit 3.
SSH_TIMEOUT_S = 120.0
SSH_POLL_S = 3.0

TROUBLE = re.compile(
    r"Kernel panic|Internal error|\bOops:|BUG:|WARNING: CPU:|hung_task|"
    r"rcu_sched self-detected|Call trace:|watchdog: BUG|"
    r"segfault|\[FAILED\]|Failed to start|Failed to mount|"
    r"NO-STOCK-UBOOT|Unable to mount root|VFS: Unable to",
    re.IGNORECASE,
)
SUCCESS = re.compile(r"login:")

log = make_log("wait-for-boot", stamped=False)


def wait_console() -> int:
    start = time.monotonic()
    pos = CONSOLE_LOG.stat().st_size  # only new bytes from here on
    trouble_seen: list[str] = []
    last_report = 0.0
    log(f"watching {CONSOLE_LOG} from byte {pos}, timeout {TIMEOUT_S:.0f}s")
    while True:
        elapsed = time.monotonic() - start
        if elapsed > TIMEOUT_S:
            log(f"TIMEOUT after {elapsed:.0f}s - no login prompt seen")
            if trouble_seen:
                log("trouble lines seen along the way:")
                for line in trouble_seen[-10:]:
                    log(f"  {line}")
            return 3

        size = CONSOLE_LOG.stat().st_size
        if size > pos:
            with CONSOLE_LOG.open("rb") as f:
                f.seek(pos)
                chunk = f.read(size - pos).decode(errors="replace")
            pos = size
            for line in chunk.splitlines():
                if TROUBLE.search(line):
                    trouble_seen.append(line.strip())
                    log(f"[{elapsed:5.1f}s] TROUBLE: {line.strip()}")
                if SUCCESS.search(line):
                    log(f"[{elapsed:5.1f}s] LOGIN PROMPT REACHED")
                    if trouble_seen:
                        log(
                            f"  ({len(trouble_seen)} trouble line(s) seen during "
                            "boot - review above)"
                        )
                        return 1
                    return 0
        elif elapsed - last_report > 10:
            last_report = elapsed
            log(
                f"[{elapsed:5.1f}s] still waiting ({len(trouble_seen)} trouble lines so far)"
            )

        time.sleep(POLL_S)


def wait_ssh(kver: str | None, old_boot_id: str | None) -> int:
    want = f"kernel {kver}" if kver else "any kernel"
    log(
        f"ssh: polling every {SSH_POLL_S:.0f}s for {want}, timeout {SSH_TIMEOUT_S:.0f}s"
    )
    start = time.monotonic()
    last_err = "no attempt yet"
    while True:
        elapsed = time.monotonic() - start
        if elapsed > SSH_TIMEOUT_S:
            log(
                f"TIMEOUT: ssh wait limit {SSH_TIMEOUT_S:.0f}s, elapsed "
                f"{elapsed:.0f}s, last: {last_err}"
            )
            return 3
        ip = _box.locate()
        if ip is None:
            last_err = "not on the LAN (MAC scan)"
        else:
            try:
                running, boot_id = _box.boot_state(ip)
            except RuntimeError as e:
                last_err = f"{ip}: {e}"
            else:
                if old_boot_id and boot_id == old_boot_id:
                    last_err = f"{ip}: still the pre-reboot boot ({running})"
                elif kver and running != kver:
                    log(
                        f"[{elapsed:5.1f}s] WRONG KERNEL: booted {running}, expected {kver}"
                    )
                    return 4
                else:
                    log(f"[{elapsed:5.1f}s] ssh up on {ip}, running {running}")
                    return 0
        log(f"[{elapsed:5.1f}s] waiting - {last_err}")
        time.sleep(SSH_POLL_S)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--kver", help="ssh path: required `uname -r` after boot")
    ap.add_argument("--old-boot-id", help="ssh path: boot_id before the reboot")
    a = ap.parse_args()
    why = _console.live_reason()
    if why is None and CONSOLE_LOG.is_file():
        return wait_console()
    log(f"no live console ({why or 'no console log'}) - falling back to ssh")
    return wait_ssh(a.kver, a.old_boot_id)


if __name__ == "__main__":
    sys.exit(main())
