#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Detect divergence in the board facts shared by the Linux and U-Boot DTs.

Why: there are four device trees and nothing keeps them in step (#221).

  dts/alpine-v2-ubnt-unvr-ea16.dts       -> Linux, built to /boot/unvr.dtb (SSD)
  uboot-port/arch/dts/awto-...-uboot.dts -> awto-uboot, COMPILED INTO u-boot.bin
  Platform/Ubiquiti/UNVR/DeviceTree/     -> UEFI, referenced by nothing (dead)
  dts/reference/, docs/hw-reference/     -> vendor record, never built

Unlike the HAL copies (scripts/hal-drift-check.py) these are NOT copies of each
other - each describes what its own stage needs. So a content hash is the wrong
check. Only a handful of BOARD FACTS must agree, because both stages program
the same silicon from them: get one wrong and the port comes up misconfigured
with no error.

Property names differ between the trees (Linux `mdio-freq-khz` vs U-Boot
`mdc-mdio-freq`), so this compares VALUES per fact, not text.

No baseline file: the facts are hardware, so any disagreement is a bug now, not
drift to re-record. That is why it needs no --update.

Usage:
    ./scripts/dt-drift-check.py          # compare, non-zero on disagreement
    ./scripts/dt-drift-check.py -v       # also print the agreeing facts
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
LINUX_DTS = REPO / "dts/alpine-v2-ubnt-unvr-ea16.dts"
UBOOT_DTS = REPO / "uboot-port/arch/dts/awto-alpine-v2-unvr-uboot.dts"

# fact -> (linux property, uboot property). Both stages configure the SerDes and
# MDIO from these; a mismatch misconfigures the port silently.
FACTS: dict[str, tuple[str, str]] = {
    "10G serdes group": ("serdes-grp", "serdes-grp"),
    "10G serdes lane": ("serdes-lane", "serdes-lane"),
    "10G dac length": ("dac-length", "dac-length"),
    "10G auto-neg": ("auto-neg", "auto-neg"),
    "10G link-training": ("link-training", "link-training"),
    "10G fec": ("fec", "fec"),
    "10G force-1000base-x": ("force-1000base-x", "force-1000base-x"),
    "1G phy address": ("phy-addr", "phy-addr"),
    "1G auto-neg mode": ("auto-neg-mode", "auto-neg-mode"),
}


def values_of(text: str, prop: str) -> list[str]:
    """Every value assigned to `prop`, normalised.

    <0x03> and <3> are the same number; "disabled" keeps its quotes stripped.
    Returns a list because a property can legitimately appear on both ports.
    """
    out = []
    for m in re.finditer(rf"^\s*{re.escape(prop)}\s*=\s*([^;]+);", text, re.M):
        raw = m.group(1).strip()
        if raw.startswith("<"):
            nums = re.findall(r"0x[0-9a-fA-F]+|\d+", raw)
            out.append(" ".join(str(int(n, 0)) for n in nums))
        else:
            out.append(raw.strip('"'))
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("-v", "--verbose", action="store_true")
    a = ap.parse_args()

    for p in (LINUX_DTS, UBOOT_DTS):
        if not p.exists():
            print(f"FATAL: missing {p.relative_to(REPO)}", file=sys.stderr)
            return 2

    linux = LINUX_DTS.read_text()
    uboot = UBOOT_DTS.read_text()

    bad, missing = [], []
    for fact, (lprop, uprop) in FACTS.items():
        lv, uv = values_of(linux, lprop), values_of(uboot, uprop)
        if not lv or not uv:
            missing.append((fact, lprop, uprop, lv, uv))
        elif set(lv) != set(uv):
            bad.append((fact, lv, uv))
        elif a.verbose:
            print(f"  ok   {fact}: {lv[0]}")

    for fact, lprop, uprop, lv, uv in missing:
        where = "linux" if not lv else "uboot"
        prop = lprop if not lv else uprop
        print(f"  MISSING  {fact}: not found in {where} DT (`{prop}`)")
    for fact, lv, uv in bad:
        print(f"  DIFFERS  {fact}: linux={lv} uboot={uv}")

    if bad or missing:
        n = len(bad) + len(missing)
        print(
            f"\nDT DRIFT: {n} board fact(s) disagree between the Linux and "
            f"U-Boot device trees (#221).\nBoth stages program the same "
            f"silicon from these - fix the one that is wrong, do not "
            f"re-record.\n  {LINUX_DTS.relative_to(REPO)}\n  "
            f"{UBOOT_DTS.relative_to(REPO)}"
        )
        return 1

    print(f"DT drift: OK ({len(FACTS)} shared board facts agree)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
