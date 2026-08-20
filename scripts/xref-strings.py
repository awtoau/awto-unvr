#!/usr/bin/env python3
"""Resolve aarch64 adrp/add xrefs to data addresses in a stripped ELF.

Disassembles with aarch64-linux-gnu-objdump, tracks adrp page bases per
register, and reports code addresses that materialise a target address.
Optionally dumps the surrounding disassembly window.
"""

from __future__ import annotations

import argparse
import logging
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
LOG = REPO / "tmp" / "logs" / "xref-strings.log"
INSN = re.compile(r"^\s*([0-9a-f]+):\s+([0-9a-f]{8})\s+(\S+)\s*([^/]*)")


def disasm(path: Path) -> list[str]:
    r = subprocess.run(
        ["aarch64-linux-gnu-objdump", "-d", str(path)],
        capture_output=True,
        text=True,
        check=True,
    )
    return r.stdout.splitlines()


def disasm_raw(path: Path) -> list[str]:
    r = subprocess.run(
        ["aarch64-linux-gnu-objdump", "-d", str(path)],
        capture_output=True,
        text=True,
        check=True,
    )
    return r.stdout.splitlines()


def build_xrefs(lines: list[str]) -> dict[int, list[int]]:
    """target data addr -> [code addrs]"""
    page: dict[str, int] = {}
    xr: dict[int, list[int]] = {}
    for ln in lines:
        m = INSN.match(ln)
        if not m:
            continue
        pc, op, args = int(m.group(1), 16), m.group(3), m.group(4)
        if op == "adrp":
            a = args.split(",")
            if len(a) >= 2:
                try:
                    page[a[0].strip()] = int(a[1].split("<")[0].strip(), 16)
                except ValueError:
                    pass
        elif op in ("add", "ldr", "ldrb", "ldrh") and "#" in args:
            a = [x.strip() for x in args.split(",")]
            src = None
            imm = None
            if op == "add" and len(a) >= 3:
                src, imm = a[1], a[2]
            elif len(a) >= 2 and a[1].startswith("["):
                src = a[1].lstrip("[").rstrip("]")
                imm = a[2].rstrip("]") if len(a) > 2 else "#0"
            if src in page and imm and imm.startswith("#"):
                try:
                    tgt = page[src] + int(imm[1:].split("]")[0], 0)
                except ValueError:
                    continue
                xr.setdefault(tgt, []).append(pc)
        elif op == "ret":
            page.clear()
    return xr


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("elf", type=Path)
    ap.add_argument("targets", nargs="+", help="hex data addresses")
    ap.add_argument(
        "--window", type=int, default=0, help="lines of disasm around each hit"
    )
    a = ap.parse_args()
    LOG.parent.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(
        level=logging.INFO,
        format="%(message)s",
        handlers=[logging.FileHandler(LOG), logging.StreamHandler(sys.stdout)],
    )

    lines = disasm(a.elf)
    xr = build_xrefs(lines)
    raw = disasm_raw(a.elf)
    idx = {}
    for i, ln in enumerate(raw):
        m = re.match(r"^\s*([0-9a-f]+):", ln)
        if m:
            idx[int(m.group(1), 16)] = i

    for t in a.targets:
        tv = int(t, 16)
        hits = xr.get(tv, [])
        logging.info("== 0x%x: %d xrefs %s", tv, len(hits), [hex(h) for h in hits])
        if a.window:
            for h in hits:
                i = idx.get(h)
                if i is None:
                    continue
                logging.info("--- around 0x%x", h)
                for ln in raw[max(0, i - a.window) : i + a.window]:
                    logging.info("%s", ln)
    return 0


if __name__ == "__main__":
    sys.exit(main())
