#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Read-only decode of the Alpine V2 SB / NB / CPU PLLs (runs on woomera, root).

  python3 read-pll.py            # all three PLLs

- PLL ring base 0xfd860000, PLL n at +0xb00 + n*0x100 (SB 0, NB 1, CPU 2); +0x40 mirrors +0x00.
- Decode: VCO = 100 MHz * (setup_0[7:0] + 1) / (setup_0[29:24] + 1); out = VCO / ch0_cfg[10:0].
- Decode fits three independent readings: NB 933.33 MHz (docs/overclock-and-caps.md), SB 500 MHz (sp804 timer
  in dmesg), CPU 1.69 GHz (timed add chain vs cntvct). The doc's setup_0 {ref,pre,post,out} layout does not.
- Reads /dev/mem only, never writes. Maps one page that al_thermal (fd860a00) also ioremaps.
"""

import mmap
import struct
import sys

RING = 0xFD860000
REF_MHZ = 100
PLLS = (("SB", 0xB00), ("NB", 0xC00), ("CPU", 0xD00))


def out_mhz(setup0: int, ch0: int) -> float:
    vco = REF_MHZ * ((setup0 & 0xFF) + 1) / (((setup0 >> 24) & 0x3F) + 1)
    return vco / (ch0 & 0x7FF) if ch0 & 0x7FF else 0.0


def main() -> int:
    try:
        with open("/dev/mem", "rb", 0) as fh:
            m = mmap.mmap(fh.fileno(), 0x1000, offset=RING, prot=mmap.PROT_READ)
            page = m[:]
    except OSError as e:
        print(f"/dev/mem read failed ({e}); run as root", file=sys.stderr)
        return 2
    for name, off in PLLS:
        w = struct.unpack_from("<32I", page, off)
        print(
            f"{name} @0x{RING + off:08x}: setup_0=0x{w[0]:08x} ch0=0x{w[8]:08x} "
            f"setup_6=0x{w[6]:08x} status_7=0x{w[7]:08x} -> {out_mhz(w[0], w[8]):.2f} MHz"
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
