#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Stage the Linux al_eth HAL into a host build tree as a build artifact (#256).

modules/al_eth/al_hal_* is the SINGLE SOURCE. It carries 18 commits of real
fixes (__must_check on every HAL function, the MDIO BUSY race, the eye-size MSB
`=+`, the sch_mode register field, the V3-only 40G path on rev-2 silicon, an
uninit phydev deref, clear-on-read counters) that a second checked-in copy would
have to re-earn. So no host keeps its own copy: each stages from here.

The staged tree is a BUILD ARTIFACT:
  - never checked in (the destination is gitignored),
  - deleted and rewritten every build, so it cannot drift or hold a local edit,
  - written read-only, so an edit to the staged copy fails loudly at the point
    of the edit rather than silently vanishing at the next build.

Staged files are byte-identical to modules/al_eth/. If a file will not build on
a host, the fix is that host's shim or the platform contract - never a patch to
the staged copy, which is what "single source" means.

Usage (normally called by scripts/uboot-build.py, not by hand):
    scripts/stage_hal.py --dest <dir>     # stage, print a manifest
    scripts/stage_hal.py --dest <dir> --clean
"""

from __future__ import annotations

import argparse
import os
import shutil
import stat
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
SOURCE = REPO / "modules" / "al_eth"

# The shared HAL: vendor al_hal_*/al_init_* sources only. The Linux DRIVER
# alongside them (al_eth_main.c, al_eth_phylink.c, kcompat.*, Makefile) is
# host-specific and is NOT shared - a host writes its own glue.
GLOB_PATTERNS = ("al_hal_*.c", "al_hal_*.h", "al_init_*.c", "al_init_*.h")

# Linux-only HAL files: real HAL by name, but they depend on Linux subsystems a
# bare-metal host has no answer for. Excluded with the reason, so a host hitting
# a missing symbol finds it stated rather than rediscovering it.
EXCLUDE: dict[str, str] = {
    "al_hal_plat_services.h": (
        "the per-host platform shim itself - each host supplies its own, "
        "checked against hal/al_hal_plat_contract.h"
    ),
}


def hal_sources() -> list[Path]:
    """Every shared HAL file, sorted, exclusions applied."""
    seen: dict[str, Path] = {}
    for pat in GLOB_PATTERNS:
        for p in SOURCE.glob(pat):
            if p.name in EXCLUDE:
                continue
            seen[p.name] = p
    return [seen[n] for n in sorted(seen)]


def clean(dest: Path) -> int:
    """Remove a previously staged tree. Staged files are read-only, so chmod
    before unlink or the rmtree fails on the first file."""
    if not dest.is_dir():
        return 0
    n = 0
    for p in dest.rglob("*"):
        if p.is_file():
            p.chmod(stat.S_IWUSR | stat.S_IRUSR)
            n += 1
    shutil.rmtree(dest)
    return n


def stage(dest: Path) -> list[str]:
    """Delete any previous staging and copy the HAL in fresh.

    Read-only (0444) on purpose: the staged copy is derived, and an edit to it
    is a mistake that must fail at the edit, not be silently reverted next
    build. Returns the staged basenames.
    """
    clean(dest)
    dest.mkdir(parents=True, exist_ok=True)
    names = []
    for src in hal_sources():
        dst = dest / src.name
        shutil.copyfile(src, dst)
        dst.chmod(stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)
        names.append(src.name)
    return names


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--dest", required=True, help="directory to stage into")
    ap.add_argument("--clean", action="store_true", help="remove staged tree")
    args = ap.parse_args()

    dest = Path(args.dest)
    if args.clean:
        print(f"stage_hal: removed {clean(dest)} staged files from {dest}")
        return 0

    if not SOURCE.is_dir():
        print(f"stage_hal: source HAL missing: {SOURCE}", file=sys.stderr)
        return 1

    names = stage(dest)
    print(f"stage_hal: staged {len(names)} HAL files from {SOURCE} -> {dest}")
    return 0


if __name__ == "__main__":
    os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
    sys.exit(main())
