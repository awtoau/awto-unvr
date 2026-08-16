#!/usr/bin/env python3
"""Resolve ARM32 literal-pool xrefs to target addresses in a flat binary.

objdump -b binary does not annotate `ldr rX,[pc,#imm]` pools. This finds each
pool word equal to a target VA, then the `ldr` that loads it, and prints a
disasm window. Base VA and target list on the command line.
"""
from __future__ import annotations

import argparse
import re
import struct
import subprocess
import sys
from pathlib import Path

LDR_PC = re.compile(
    r"^\s*([0-9a-f]+):\s+[0-9a-f]{8}\s+ldr\s+(\w+),\s*\[pc,\s*#(-?\d+)\]")
MOVW = re.compile(r"^\s*([0-9a-f]+):\s+[0-9a-f]{8}\s+movw\s+(\w+),\s*#(\d+)")
MOVT = re.compile(r"^\s*([0-9a-f]+):\s+[0-9a-f]{8}\s+movt\s+(\w+),\s*#(\d+)")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("binary", type=Path)
    ap.add_argument("targets", nargs="+", help="hex VAs to find xrefs to")
    ap.add_argument("--base", type=lambda s: int(s, 0), default=0x01000000)
    ap.add_argument("--window", type=int, default=12)
    a = ap.parse_args()

    b = a.binary.read_bytes()
    base = a.base
    dis = subprocess.run(
        ["arm-linux-gnu-objdump", "-D", "-b", "binary", "-m", "arm", "-EL",
         f"--adjust-vma=0x{base:x}", str(a.binary)],
        capture_output=True, text=True, check=True).stdout.splitlines()

    tgts = {int(t, 0) for t in a.targets}
    low = {t & 0xFFFF: t for t in tgts}
    for i, ln in enumerate(dis):
        # literal pool
        m = LDR_PC.match(ln)
        if m:
            pc = int(m.group(1), 16)
            pool = pc + 8 + int(m.group(3))
            fo = pool - base
            if 0 <= fo + 4 <= len(b):
                val = struct.unpack_from("<I", b, fo)[0]
                if val in tgts:
                    _emit(dis, i, a.window, f"ldr <-0x{val:08x} @0x{pc:08x}")
            continue
        # movw low half then find matching movt within next few insns
        mw = MOVW.match(ln)
        if mw:
            reg, lo16 = mw.group(2), int(mw.group(3))
            if lo16 in low:
                for j in range(i + 1, min(i + 6, len(dis))):
                    mt = MOVT.match(dis[j])
                    if mt and mt.group(2) == reg:
                        val = (int(mt.group(3)) << 16) | lo16
                        if val in tgts:
                            _emit(dis, i, a.window,
                                  f"movw/movt {reg}<-0x{val:08x}")
                        break


def _emit(dis, i, window, label):
    print(f"\n=== {label} ===")
    for l in dis[max(0, i - window):i + window]:
        print(l)
    return 0


if __name__ == "__main__":
    sys.exit(main())
