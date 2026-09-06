#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Compile each staged Linux HAL source under U-Boot, alone (#256).

Answers the phase-1 question directly: which shared HAL files build against a
host's shim, and which need something the platform contract does not yet give
them. Per-file, so a failure names the file and the symbol instead of being
buried in a link error at the end of a full build.

The source is never patched to make it compile - a failure here is a finding
about the shim or the contract.

Requires a configured build tree (scripts/uboot-build.py has run) for
generated/autoconf.h. Reads only; writes nothing into TREE.

Usage:
    scripts/hal-host-portability.py            # summary, log to tmp/logs
    scripts/hal-host-portability.py --verbose  # full diagnostics per failure
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

from _repo import make_log  # noqa: E402

TREE = Path(os.environ.get("UBOOT_TREE", "/mnt/2tb/unvr-port-refs/u-boot-v2026.07"))
BUILDDIR = REPO / "tmp" / "uboot-build"
STAGE = "drivers/net/al_hal_shared"
SHIM = "drivers/net/al_hal_shim"
CROSS = "aarch64-linux-gnu-"

log = make_log("hal-host-portability")

# 1.25x a cold cross-cc1 syntax pass on this host (~1.5 s worst case observed);
# a hang means the toolchain wedged, and the file it was on is named.
CC_TIMEOUT_S = 60


def cc_argv(src: Path) -> list[str]:
    autoconf = BUILDDIR / "include" / "generated" / "autoconf.h"
    return [
        f"{CROSS}gcc",
        "-fsyntax-only",
        "-D__KERNEL__",
        "-DCONFIG_AL_ETH",
        f"-I{BUILDDIR / 'include'}",
        f"-I{TREE / 'include'}",
        f"-I{TREE / 'arch/arm/include'}",
        f"-I{TREE / SHIM}",
        f"-I{TREE / STAGE}",
        "-include",
        str(autoconf),
        "-include",
        str(TREE / "include/linux/kconfig.h"),
        str(src),
    ]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--verbose", action="store_true", help="print full diagnostics")
    args = ap.parse_args()

    staged = TREE / STAGE
    if not staged.is_dir():
        log(f"ABORT: {staged} missing - run scripts/uboot-build.py first")
        return 1
    if not (BUILDDIR / "include" / "generated" / "autoconf.h").exists():
        log(f"ABORT: {BUILDDIR} not configured - run scripts/uboot-build.py first")
        return 1

    sources = sorted(staged.glob("*.c"))
    ok, bad = [], []
    for src in sources:
        r = subprocess.run(
            cc_argv(src), capture_output=True, text=True, timeout=CC_TIMEOUT_S
        )
        errs = [ln for ln in r.stderr.splitlines() if "error:" in ln]
        if errs:
            bad.append((src.name, errs))
            log(f"FAIL {src.name} ({len(errs)} errors)")
            for ln in errs if args.verbose else errs[:3]:
                log(f"       {ln.strip()}")
        else:
            ok.append(src.name)
            log(f"ok   {src.name}")

    log(f"{len(ok)}/{len(sources)} staged HAL sources compile under U-Boot")
    if bad:
        log("files needing a shim/contract answer (never a patch to the source):")
        for name, errs in bad:
            log(f"  {name}: {len(errs)} errors")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
