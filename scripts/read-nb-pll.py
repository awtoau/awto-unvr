#!/usr/bin/env python3
"""Dump the Alpine V2 NB (north-bridge) PLL registers @0xfd860c00 (runs on woomera, root).

Why: the DDR clock comes from this PLL. Two sources disagree about its rate --
  - PBS boot_strap NB_PLL field = 7 -> al_bootstrap_ddr_pll_freq_get() = 800 MHz
  - vendor live.dts `nbclk` = 933.33 MHz, and the live arch timer runs at 58.33 MHz
    (= 933.33/16), which no integer divisor of 800 MHz produces
Reading the PLL itself settles which is the running rate.

The strap is only a RESET-TIME default; firmware may reprogram the PLL afterwards, in
which case the strap no longer describes the running clock. That is the hypothesis this
script tests.

0xfd860c00 is NOT claimed in /proc/iomem (no driver ioremaps it), so a /dev/mem mapping
here does not create a conflicting mapping. Read-only -- never writes.

Root only (raw /dev/mem MMIO read).
Log -> tmp/logs/read-nb-pll.log

Copyright (c) 2026 Awto / Daniel Tyrrell
SPDX-License-Identifier: GPL-2.0-or-later
"""

import mmap
import struct
import sys

NB_PLL_BASE = 0xFD860C00
PAGE = 0x1000
PAGE_BASE = NB_PLL_BASE & ~(PAGE - 1)
PAGE_OFF = NB_PLL_BASE - PAGE_BASE
NREGS = 0x20  # 0x00..0x7c -- covers +0x00/+0x04/+0x20/+0x40 named in the preload script


def main() -> int:
    try:
        with open("/dev/mem", "rb", 0) as f:
            m = mmap.mmap(f.fileno(), PAGE, offset=PAGE_BASE, prot=mmap.PROT_READ)
            raw = m[PAGE_OFF : PAGE_OFF + NREGS * 4]
    except (PermissionError, OSError) as e:
        print(f"/dev/mem read failed ({e}) -- run as root", file=sys.stderr)
        return 2

    words = struct.unpack(f"<{NREGS}I", raw)
    print(f"NB PLL @0x{NB_PLL_BASE:08x}")
    for i, w in enumerate(words):
        print(f"  +0x{i * 4:02x}  0x{w:08x}")

    # The EEPROM preload script writes these; print them together for comparison.
    print("\nvalues the 0x57 preload script writes (docs/ddr-eeprom-0x57.md §3):")
    for off, want in ((0x00, 0x8000001B), (0x04, 0x1D4CC00D), (0x20, 0x000C7003)):
        got = words[off // 4]
        print(
            f"  +0x{off:02x}  eeprom=0x{want:08x}  live=0x{got:08x}  {'same' if got == want else 'DIFFERENT -> reprogrammed after reset'}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
