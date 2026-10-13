#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Enforce the cross-tree quirk registry (#342).

hal-drift-check.py answers "are the shared files actually shared?". It cannot
answer "is this fix present everywhere it belongs?", because a quirk that was
never ported anywhere leaves every file unchanged and every hash matching.

That is the gap this closes. scripts/hal-quirks.json lists, per quirk, the
files that MUST carry it and a marker regex proving they do, plus the trees
that are explicitly exempt and why.

Checks (all fail the gate):
  1. MISSING QUIRK    a file in applies_to whose marker is absent - the fix was
                      ported to one generation and not another.
  2. MISSING FILE     a file in applies_to that no longer exists (renamed or
                      retired without updating the registry).
  3. NO EXEMPT REASON an exempt entry with an empty reason.

Usage:
    ./scripts/hal-quirk-check.py            # check
    ./scripts/hal-quirk-check.py --list     # what is registered, and where it must live
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
REGISTRY = REPO / "scripts" / "hal-quirks.json"


def quirks() -> dict[str, dict]:
    reg = json.loads(REGISTRY.read_text())
    return {k: v for k, v in reg.items() if not k.startswith("_")}


def check() -> list[str]:
    errs: list[str] = []

    for name, q in quirks().items():
        issue = q.get("issue", "?")

        for rel, spec in q.get("applies_to", {}).items():
            path = REPO / rel
            if not path.exists():
                errs.append(
                    f"MISSING FILE   {name} ({issue}): {rel} does not exist - "
                    f"renamed or retired? update scripts/hal-quirks.json"
                )
                continue

            marker = spec.get("marker")
            if not marker:
                errs.append(
                    f"NO MARKER      {name} ({issue}): {rel} has no marker regex"
                )
                continue

            if not re.search(marker, path.read_text()):
                errs.append(
                    f"MISSING QUIRK  {name} ({issue}): {rel} does not match /{marker}/\n"
                    f"                 symptom when absent: {q.get('symptom', '?')}\n"
                    f"                 {spec.get('note', '')}"
                )

        for tree, reason in q.get("exempt", {}).items():
            if not reason.strip():
                errs.append(
                    f"NO EXEMPT REASON {name} ({issue}): {tree} is exempt with no reason"
                )

    return errs


def show() -> None:
    for name, q in quirks().items():
        hw = (
            "verified on hardware"
            if q.get("verified_on_hardware")
            else "NOT yet verified on hardware"
        )
        print(f"{name}  [{q.get('issue', '?')}]  ({hw})")
        print(f"  symptom: {q.get('symptom', '?')}")
        for rel, spec in q.get("applies_to", {}).items():
            print(f"  must carry it: {rel}")
            if spec.get("note"):
                print(f"                 {spec['note']}")
        for tree, reason in q.get("exempt", {}).items():
            print(f"  exempt: {tree} - {reason}")
        print()


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--list", action="store_true", help="print the registry")
    args = ap.parse_args()

    if args.list:
        show()
        return 0

    errs = check()
    if errs:
        print("HAL quirk drift:")
        for e in errs:
            print(f"  {e}")
        return 1

    n = len(quirks())
    files = sum(len(q.get("applies_to", {})) for q in quirks().values())
    print(
        f"HAL quirks: OK ({n} registered, {files} carrier file(s), all markers present)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
