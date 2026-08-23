#!/usr/bin/env python3
"""Classify a preboot blob's coverage gaps as code vs data, from a CoverageReport run.

Reads Ghidra's coverage.txt (undefined runs) + the raw blob, and buckets each gap:
  CODE           4-aligned, decodes fully, carries control-flow (push/pop/bl/bx/ldm/stm)
  zero-sled      >50% 0x00000000 words -> padding / .bss-style
  no-controlflow decodes but no call/return structure -> data table
  partial-decode capstone hit an invalid instr -> data
  small/unaligned len<8 or not 4-aligned -> inter-function padding / literal
Prints per-bucket totals and the CODE list (addresses to force-disassemble).

Usage: measure-preboot-coverage.py <coverage.txt> <blob.bin> <base_hex>
Log: tmp/logs/measure-preboot-coverage.log
"""

from __future__ import annotations

import logging
import re
import sys
from pathlib import Path

import capstone

REPO = Path(__file__).resolve().parent.parent
LOG = REPO / "tmp" / "logs" / "measure-preboot-coverage.log"
CF = {"push", "pop", "bl", "blx", "bx", "stmfd", "ldmfd", "stm", "ldm", "svc"}


def classify(md, b, base, a, l):
    if a % 4 or l < 8:
        return "small/unaligned", 0
    d = b[a - base : a - base + l]
    words = [d[i : i + 4] for i in range(0, len(d) - 3, 4)]
    zero = sum(1 for w in words if w == b"\0\0\0\0")
    if zero > len(words) * 0.5:
        return "zero-sled", zero
    ins = list(md.disasm(d, 0))
    if sum(i.size for i in ins) < l * 0.98:
        return "partial-decode", 0
    cf = sum(1 for i in ins if i.mnemonic.split(".")[0] in CF)
    return ("CODE", cf) if cf >= 1 else ("no-controlflow", 0)


def main() -> int:
    cov, blob, base = Path(sys.argv[1]), Path(sys.argv[2]), int(sys.argv[3], 0)
    LOG.parent.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(
        level=logging.INFO,
        format="%(message)s",
        handlers=[logging.FileHandler(LOG), logging.StreamHandler(sys.stdout)],
    )
    b = blob.read_bytes()
    thumb = "thumb" in sys.argv[4:] if len(sys.argv) > 4 else False
    mode = capstone.CS_MODE_THUMB if thumb else capstone.CS_MODE_ARM
    md = capstone.Cs(capstone.CS_ARCH_ARM, mode)
    gaps = [
        (int(m.group(1), 16), int(m.group(2)))
        for m in re.finditer(r"0x0*([0-9a-f]+)  len=(\d+)", cov.read_text())
    ]
    buckets: dict[str, list] = {}
    for a, l in gaps:
        c, s = classify(md, b, base, a, l)
        buckets.setdefault(c, []).append((a, l, s))
    logging.info(
        "== %s (%d gaps, %d bytes undefined) ==",
        blob.name,
        len(gaps),
        sum(l for _, l in gaps),
    )
    for c in sorted(buckets, key=lambda k: -sum(x[1] for x in buckets[k])):
        lst = buckets[c]
        logging.info("%-16s %4d gaps  %7d B", c, len(lst), sum(x[1] for x in lst))
    code = sorted(buckets.get("CODE", []))
    logging.info(
        "\nCODE gaps (%d, %d B) -- force-disassemble candidates:",
        len(code),
        sum(x[1] for x in code),
    )
    for a, l, s in code:
        logging.info("  0x%08x len=%-5d cf=%d", a, l, s)
    # emit a bare address list for a disassemble script
    (REPO / "tmp" / "code-gaps.txt").write_text(
        "\n".join("0x{:08x}".format(a) for a, _, _ in code) + "\n"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
