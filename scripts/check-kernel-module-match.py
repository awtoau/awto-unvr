#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Boot-time check: does the running kernel actually match the module tree
it's running with? Runs on woomera itself (shipped into the rootfs and
enabled as a systemd oneshot -- see check-kernel-module-match.service).

Why this exists (#162, follow-up to #161): a kernel/module version mismatch
sat silently on this box for 9 days, producing a recurring-but-non-fatal
`resolve_symbol`/`ref_module` Oops on every single boot, with nothing
anywhere to flag it. The mismatch came from a manual/ad-hoc deploy that
bypassed both sanctioned deploy scripts, leaving no trail. Per-build-OUT-
directory separation (#131) only keeps the *official build scripts* from
colliding with each other -- it does nothing to catch a manual copy, and
nothing checked kernel/module consistency at all. This closes that gap:
whatever put the modules there, a mismatch is now loud and immediate on
the very next boot instead of silently recurring for days.

Mechanism: `sync_modules()` (scripts/_fedora_deploy.py) writes
`/lib/modules/<kver>/.deployed-from` at deploy time, recording the exact
kernel build banner (`Linux version ...` string, extracted from the
build-out's raw `Image` binary via `strings` -- this differs per actual
build even when the KVER string itself doesn't, e.g. "7.1.8-dirty" for
every build regardless of date) that SHOULD be running once this module
tree is paired with its matching kernel. This script compares that
recorded banner against the ACTUALLY-running kernel's own banner
(`/proc/version`) at boot. A match proves kernel and modules came from
the same build; a mismatch means someone/something deployed one without
the other.

Deliberately fails OPEN, never blocks boot: an out-of-band manual copy
is still *possible* (this can't prevent that) -- the goal is only to
make it immediately visible instead of silently recurring. A missing
marker (e.g. a kernel that predates this check, or a module tree from
before #162) logs a one-line note, not an error -- it means "no
provenance recorded to check", not "mismatch confirmed".
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path


def kernel_banner() -> str:
    return Path("/proc/version").read_text().strip()


def read_marker(kver: str) -> str | None:
    marker = Path(f"/lib/modules/{kver}/.deployed-from")
    if not marker.is_file():
        return None
    for line in marker.read_text().splitlines():
        if line.startswith("banner="):
            return line[len("banner=") :]
    return None


def log_kmsg(level: str, msg: str) -> None:
    """Write straight to the kernel log buffer so this is visible even on
    the serial console (dmesg/journal both read from here) regardless of
    whether anything is watching the systemd journal specifically."""
    try:
        with open("/dev/kmsg", "w") as f:
            f.write(f"<{level}>check-kernel-module-match: {msg}\n")
    except OSError:
        pass
    print(f"check-kernel-module-match: {msg}", file=sys.stderr)


def main() -> int:
    kver = subprocess.run(
        ["uname", "-r"], capture_output=True, text=True, check=True
    ).stdout.strip()
    running = kernel_banner()
    expected = read_marker(kver)

    if expected is None:
        log_kmsg("6", f"no .deployed-from marker for {kver} - nothing to check (pre-#162 deploy?)")
        return 0

    if expected == running:
        log_kmsg("6", f"kernel/module match confirmed for {kver}")
        return 0

    # kern.crit (2) - loud on purpose, this is exactly the class of bug
    # that produced a recurring boot-time Oops for 9 days undetected.
    log_kmsg("2", "KERNEL/MODULE MISMATCH DETECTED")
    log_kmsg("2", f"running:  {running}")
    log_kmsg("2", f"deployed: {expected}")
    log_kmsg("2", "modules were synced from a different kernel build than the one "
                  "currently running - see docs/kernel-module-mismatch-recurring "
                  "memory / GitHub #161 #162")
    return 0


if __name__ == "__main__":
    sys.exit(main())
