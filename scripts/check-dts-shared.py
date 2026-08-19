#!/usr/bin/env python3
"""CI guard: shared i2c timing facts must match across the Linux + U-Boot DTS.

Both device trees describe the SAME pld i2c bus (i2c@fd880000). Its clock speed
and SDA hold time are timing-critical and divergence-prone:
  - dropping i2c-sda-hold-time-ns wedged the s35390a RTC (docs/rtc-s35390a-fault.md)
  - the bus runs at its rated-max 400 kHz; a silent drop to 100 kHz is a regression
Until the trees are unified (#75) these live in two files and can drift. This
check fails the build on any drift from the canonical values below.

Standalone:  python3 scripts/check-dts-shared.py
Wired into:  build-linux-71-fedora.py, uboot-build.py (run before compile).
"""
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

NODE = "i2c@fd880000"                     # the shared pld bus, both trees
EXPECT = {
    "clock-frequency":      100000,       # Standard-mode: stock's speed, needed so the
                                          # s35390a RTC on ch0 does not wedge (rtc-s35390a-fault.md)
    "i2c-sda-hold-time-ns": 300,          # stock value; dropping it wedges the RTC
}
FILES = [
    "dts/alpine-v2-ubnt-unvr-ea16.dts",
    "uboot-port/arch/dts/awto-alpine-v2-unvr-uboot.dts",
]


def node_block(text, node):
    """Return the text inside `[label:] node { ... }` (brace-balanced), or None."""
    m = re.search(r"(?:[\w-]+:\s*)?" + re.escape(node) + r"\s*\{", text)
    if not m:
        return None
    i, depth = m.end(), 1
    while i < len(text) and depth:
        depth += (text[i] == "{") - (text[i] == "}")
        i += 1
    return text[m.end():i]


def prop_val(block, prop):
    """First `prop = <N>;` in the block as an int (dec or 0x), or None."""
    m = re.search(re.escape(prop) + r"\s*=\s*<\s*(0x[0-9a-fA-F]+|\d+)\s*>", block)
    return int(m.group(1), 0) if m else None


def check():
    errs = []
    for rel in FILES:
        path = os.path.join(REPO, rel)
        block = node_block(open(path).read(), NODE)
        if block is None:
            errs.append(f"{rel}: node {NODE} not found")
            continue
        for prop, want in EXPECT.items():
            got = prop_val(block, prop)
            if got is None:
                errs.append(f"{rel}: {NODE} missing '{prop}' (want {want})")
            elif got != want:
                errs.append(f"{rel}: {NODE} {prop}={got}, want {want}")
    return errs


def main():
    errs = check()
    if errs:
        print("DTS shared-fact check FAILED:", file=sys.stderr)
        for e in errs:
            print("  -", e, file=sys.stderr)
        print("Fix both trees to the canonical values, or update EXPECT in "
              "scripts/check-dts-shared.py if the intended value changed.",
              file=sys.stderr)
        return 1
    facts = ", ".join(f"{k}={v}" for k, v in EXPECT.items())
    print(f"DTS shared-fact check OK: {NODE} [{facts}] match across "
          f"{len(FILES)} trees")
    return 0


if __name__ == "__main__":
    sys.exit(main())
