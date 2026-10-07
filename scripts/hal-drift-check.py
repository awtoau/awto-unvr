#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Detect NEW divergence between the five vendored copies of the Annapurna HAL.

Why: the `al_*` HAL is vendored five times at three different vintages, with
nothing keeping them in step (#218). Two examples found by hand:

  - UDMA AXI timeout is 5M in modules/al_ssm and uboot-port, 1M in
    modules/al_eth and modules/al_dma. The 5M value arrived WITH the vendor
    import (c919d82) - it is the vendor's own newer HAL sitting beside two
    older copies.
  - al_udma_m2s_pref_get() tested the wrong bit in modules/al_eth until
    6341528, while uboot-port had been correct all along.

So a fix landed in one tree is not a fix everywhere, and the answer differs per
file. Equality is the WRONG check - these files are legitimately different
vintages, so a plain diff would fail on every run and be ignored within a day.

Equality is the WRONG check for a second reason: these files differ pairwise
almost everywhere (comments, includes, vendor vintage), so "which trees agree"
is nearly always "none" and never changes - an agreement-only check reports OK
through the very edit it exists to catch. Verified: it did.

So the baseline records the CONTENT HASH PER TREE. Any edit to any shared file
changes its hash and is reported, naming the file and the trees that did NOT
change - which is exactly the "you fixed al_eth, check the other three" prompt.
The cost is that intended edits need --update; that is the point, since it
forces the sibling question to be answered explicitly.

Also checks hal/pcie-al-alpine-regs.h - the one header that is genuinely shared
(a single file #include'd by U-Boot and EDK2) rather than vendored per tree. The
baseline model above cannot see it, so it is compared by #define value against
the Linux fork's canonical copy instead. See check_shared_header().

Scope after #256 phase 3. The al_eth HAL is STAGED from modules/al_eth/ at
build time (scripts/stage_hal.py); both the U-Boot copy (phase 2) and the EDK2
one (phase 3, 61 files) are GONE. Four vendored copies remain, all Linux
modules - the fourth is modules/al_nand (#208), a disjoint file set with one
consumer, so registered here rather than staged.

The U-Boot tree still appears below for al_serdes and al_ssm, which carry HAL
files of their own that neither phase touched.

Two checks cover the other direction - a copy coming BACK:
  check_no_staged_copy()   - a staged tree that gets checked in
  check_no_edk2_hal_copy() - a HAL file re-copied into the EDK2 package

Usage:
    ./scripts/hal-drift-check.py            # check against the baseline
    ./scripts/hal-drift-check.py --update   # re-record after an INTENDED change
    ./scripts/hal-drift-check.py --show <basename>   # who has it, and do they match
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
BASELINE = REPO / "scripts" / "hal-drift-baseline.json"

# Each tree is (label, [roots]). Roots differ because uboot-port nests the HAL.
TREES: dict[str, list[str]] = {
    "al_eth": ["modules/al_eth"],
    "al_dma": ["modules/al_dma"],
    "al_ssm": ["modules/al_ssm"],
    # #208. NOT staged from modules/al_eth: the NAND HAL is a disjoint file set
    # with one consumer, so staging it would mean putting NAND sources under
    # al_eth. It is registered here instead, which is what keeps its copies of
    # al_hal_plat_services.h and friends in the drift baseline.
    "al_nand": ["modules/al_nand"],
    "uboot": [
        "uboot-port/drivers/phy/al_serdes",
        "uboot-port/drivers/crypto/al_ssm",
    ],
}

# EDK2 kept a fifth copy (61 files, df19434) until #256 phase 3. It is gone:
# AlpineHalLib now compiles the STAGED shared HAL. A checked-in HAL file
# reappearing under this directory means someone re-copied it, so it fails -
# checked by path, so it fires on names the baseline does not know.
# The two files that legitimately live here are the host shim, verified against
# hal/al_hal_plat_contract.h at compile time, not vendored HAL.
EDK2_HAL_DIR = "Platform/Ubiquiti/UNVR/Library/AlpineHalLib"
EDK2_SHIM_FILES = {"al_hal_plat_services.h", "al_hal_plat_types.h"}

# hal/pcie-al-alpine-regs.h is the one header genuinely SHARED (one file,
# #include'd) rather than vendored per tree, so the basename check above
# cannot see it - it never appears in 2+ trees. Its risk is the other
# direction: drifting from the Linux fork's canonical copy, which lives
# outside this repo. Compared by #define VALUES, not bytes: the two copies
# legitimately carry different file-header prose, and a byte-hash check
# would fire on that forever and be ignored (this script's own docstring).
SHARED_HEADER = "hal/pcie-al-alpine-regs.h"
SHARED_HEADER_CANONICAL = Path(
    "/mnt/2tb/unvr-port-refs/linux-v7.3-fresh/drivers/pci/controller/pcie-al-alpine-regs.h"
)
# Consumers that #include it directly - listed so a dropped include is caught.
SHARED_HEADER_CONSUMERS = [
    "uboot-port/board/annapurna/alpine/alpine.c",
    "Platform/Ubiquiti/UNVR/Drivers/AlPcieSnoopFixDxe/AlPcieSnoopFixDxe.c",
]

# Where scripts/stage_hal.py stages the shared Linux HAL inside a build tree
# (#256). It is a BUILD ARTIFACT: if a copy of it ever appears under version
# control, someone has checked in a derived tree and the drift this script
# exists to catch is back, by a new route. Checked by path, so it fires even
# on files whose basenames the baseline does not already know.
STAGED_DIR_NAME = "al_hal_shared"

DEFINE_RE = re.compile(
    r"^\s*#\s*define\s+(\w+)(?:\([^)]*\))?\s+(.*?)\s*(?:/\*.*)?$", re.M
)


def check_no_staged_copy() -> list[str]:
    """Fail if a staged (derived) HAL tree has been checked in.

    stage_hal.py writes it read-only into the build tree and rewrites it every
    build; a tracked copy means that was bypassed."""
    tracked = subprocess.run(
        ["git", "ls-files"],
        cwd=REPO,
        capture_output=True,
        text=True,
        check=True,
        timeout=60,
    ).stdout.split()
    offenders = [f for f in tracked if f"/{STAGED_DIR_NAME}/" in f]
    if not offenders:
        return []
    return [
        f"  CHECKED-IN STAGED HAL: {f}\n"
        f"      {STAGED_DIR_NAME}/ is a build artifact - stage it, do not commit it"
        for f in sorted(offenders)
    ]


def check_no_edk2_hal_copy() -> list[str]:
    """Fail if a checked-in HAL file reappears in the EDK2 package (#256 p3)."""
    tracked = subprocess.run(
        ["git", "ls-files", EDK2_HAL_DIR],
        cwd=REPO,
        capture_output=True,
        text=True,
        check=True,
        timeout=60,
    ).stdout.split()
    offenders = [
        f
        for f in tracked
        if Path(f).name.startswith(("al_hal_", "al_init_", "al_serdes"))
        and Path(f).name not in EDK2_SHIM_FILES
    ]
    return [
        f"  CHECKED-IN EDK2 HAL: {f}\n"
        "      AlpineHalLib builds the STAGED shared HAL"
        " - stage it, do not commit it"
        for f in sorted(offenders)
    ]


def defines(text: str) -> dict[str, str]:
    """name -> value, whitespace-normalised. Comments and prose ignored."""
    return {
        m.group(1): " ".join(m.group(2).split())
        for m in DEFINE_RE.finditer(text)
        if m.group(2).strip()
    }


def check_shared_header() -> list[str]:
    """Compare the shared header's constants against the canonical Linux copy."""
    problems = []
    local = REPO / SHARED_HEADER

    if not local.is_file():
        return [f"  MISSING {SHARED_HEADER} - both U-Boot and EDK2 #include it"]

    for consumer in SHARED_HEADER_CONSUMERS:
        p = REPO / consumer
        if not p.is_file():
            problems.append(f"  MISSING consumer {consumer}")
        elif "pcie-al-alpine-regs.h" not in p.read_text():
            problems.append(
                f"  {consumer} no longer #includes pcie-al-alpine-regs.h"
                " - it has gone back to hand-typed constants"
            )

    if not SHARED_HEADER_CANONICAL.is_file():
        # A missing reference tree is not a failure - it is not in this repo.
        print(
            f"note: canonical copy absent ({SHARED_HEADER_CANONICAL}), value check skipped"
        )
        return problems

    ours = defines(local.read_text())
    theirs = defines(SHARED_HEADER_CANONICAL.read_text())
    for name in sorted(set(ours) | set(theirs)):
        o, t = ours.get(name), theirs.get(name)
        if o == t:
            continue
        problems.append(
            f"  VALUE DIFFERS  {name}: {SHARED_HEADER}={o!r} vs Linux fork={t!r}"
        )
    return problems


def sha(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()[:16]


def collect() -> dict[str, dict[str, str]]:
    """basename -> {tree: hash}, for basenames present in 2+ trees."""
    seen: dict[str, dict[str, str]] = {}
    for tree, roots in TREES.items():
        for root in roots:
            base = REPO / root
            if not base.is_dir():
                continue
            for p in base.rglob("*"):
                if p.suffix not in (".c", ".h") or not p.is_file():
                    continue
                seen.setdefault(p.name, {})[tree] = sha(p)
    return {k: v for k, v in seen.items() if len(v) > 1}


def agreement(hashes: dict[str, str]) -> list[list[str]]:
    """Which trees agree with which - reported for context, not compared."""
    by_hash: dict[str, list[str]] = {}
    for tree, h in hashes.items():
        by_hash.setdefault(h, []).append(tree)
    return sorted([sorted(v) for v in by_hash.values()])


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--update",
        action="store_true",
        help="re-record the baseline after an intended change",
    )
    ap.add_argument(
        "--show",
        metavar="BASENAME",
        help="report which trees carry this file and whether they agree",
    )
    args = ap.parse_args()

    cur = collect()

    if args.show:
        h = cur.get(args.show)
        if not h:
            print(f"{args.show}: not shared by 2+ trees")
            return 1
        for grp in agreement(h):
            print(f"  {'  ==  '.join(grp)}")
        return 0

    if args.update:
        BASELINE.write_text(
            json.dumps(
                {k: dict(sorted(v.items())) for k, v in sorted(cur.items())}, indent=1
            )
            + "\n"
        )
        print(f"baseline updated: {len(cur)} shared files -> {BASELINE.name}")
        return 0

    if not BASELINE.exists():
        print(f"FATAL: {BASELINE} missing - run --update once to record it")
        return 1

    old = json.loads(BASELINE.read_text())

    problems = []
    for name in sorted(set(old) | set(cur)):
        o, n = old.get(name), cur.get(name)
        if o == n:
            continue
        if o is None:
            problems.append(f"  NEW SHARED FILE  {name}: {sorted(n)}")
            continue
        if n is None:
            problems.append(f"  NO LONGER SHARED {name}")
            continue
        changed = sorted(t_ for t_ in n if o.get(t_) != n[t_])
        untouched = sorted(t_ for t_ in n if t_ not in changed)
        problems.append(
            f"  CHANGED in {', '.join(changed)}: {name}\n"
            f"      NOT changed in: {', '.join(untouched) or '(none)'}"
            f" - does the same fix apply there?"
        )

    shared = check_shared_header()
    if shared:
        print(f"SHARED-HEADER DRIFT: {len(shared)} problem(s) in {SHARED_HEADER}\n")
        print("\n".join(shared))
        print()

    staged = check_no_staged_copy() + check_no_edk2_hal_copy()
    if staged:
        print(f"STAGED HAL COMMITTED: {len(staged)} file(s)\n")
        print("\n".join(staged))
        print()

    if problems:
        print(f"HAL DRIFT: {len(problems)} change(s) vs the baseline (#218)\n")
        print("\n".join(problems))
        print(
            "\nAnswer the sibling question for each, then record it:\n"
            "  ./scripts/hal-drift-check.py --update"
        )
        return 1

    if shared or staged:
        return 1

    print(
        f"HAL drift: OK ({len(cur)} vendored files unchanged,"
        f" {SHARED_HEADER} matches the Linux fork,"
        f" no checked-in {STAGED_DIR_NAME}/)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
