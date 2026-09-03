#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Detect NEW divergence between the four vendored copies of the Annapurna HAL.

Why: the `al_*` HAL is vendored four times at three different vintages, with
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

Usage:
    ./scripts/hal-drift-check.py            # check against the baseline
    ./scripts/hal-drift-check.py --update   # re-record after an INTENDED change
    ./scripts/hal-drift-check.py --show <basename>   # who has it, and do they match
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
BASELINE = REPO / "scripts" / "hal-drift-baseline.json"

# Each tree is (label, [roots]). Roots differ because uboot-port nests the HAL.
TREES: dict[str, list[str]] = {
    "al_eth": ["modules/al_eth"],
    "al_dma": ["modules/al_dma"],
    "al_ssm": ["modules/al_ssm"],
    "uboot": [
        "uboot-port/drivers/net/al_eth/hal",
        "uboot-port/drivers/phy/al_serdes",
        "uboot-port/drivers/crypto/al_ssm",
    ],
}


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

    if problems:
        print(f"HAL DRIFT: {len(problems)} change(s) vs the baseline (#218)\n")
        print("\n".join(problems))
        print(
            "\nAnswer the sibling question for each, then record it:\n"
            "  ./scripts/hal-drift-check.py --update"
        )
        return 1

    print(f"HAL drift: OK ({len(cur)} shared files unchanged)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
