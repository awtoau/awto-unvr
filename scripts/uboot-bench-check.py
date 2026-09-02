#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Reach the U-Boot prompt and run a fixed set of READ-ONLY diagnostic
commands - no writes, no chainload jump.

Written for docs/uefi.md §5's "exact next bench step": confirm the
running U-Boot's command set and bdinfo/env before trusting any EDK2
chainload plan. Reuses ram-boot-deploy.py's proven catch-uboot.py-race +
power_cycle_verified() pattern (single deploy concept) rather than a new
hand-rolled reimplementation.

Usage:
    ./dev.py uboot-bench-check
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
from _power import power_cycle_verified  # noqa: E402

_rbd_spec = importlib.util.spec_from_file_location(
    "_ram_boot_deploy", REPO / "scripts/ram-boot-deploy.py")
_rbd = importlib.util.module_from_spec(_rbd_spec)
_rbd_spec.loader.exec_module(_rbd)

# docs/uefi.md §5 - deliberately READ-ONLY: no setenv/saveenv, no tftpboot,
# no go/bootm. Confirms command availability and bench values only.
COMMANDS = ["version", "help", "help bootm", "printenv", "bdinfo", "help go"]


def main() -> int:
    print("starting catch-uboot.py to win the autoboot race")
    catch = subprocess.Popen(
        [sys.executable, "scripts/catch-uboot.py", "--seconds", "60"], cwd=REPO)
    power_cycle_verified(log=print)
    try:
        rc = catch.wait(timeout=70)
    except subprocess.TimeoutExpired:
        catch.kill()
        print("FATAL: catch-uboot.py hung waiting for the U-Boot prompt")
        return 1
    if rc != 0:
        print("FATAL: catch-uboot.py did not reach the U-Boot prompt (autoboot won)")
        return 1
    print("U-Boot prompt reached, autoboot stopped\n")

    for cmd in COMMANDS:
        print(f"=== {cmd} ===")
        out = _rbd.run_devpy("--expect", "ALPINE_UBNT_NAS_ALL>", "--timeout", "10", cmd)
        print(out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
