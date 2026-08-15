#!/usr/bin/env python3
"""Carve every FDT blob out of a binary and report soc/nand-flash partitions.

Scans for the 0xd00dfeed magic, validates the header, writes each blob to
outdir, decompiles with dtc and extracts board-cfg + nand partition table.
"""
from __future__ import annotations

import argparse
import logging
import re
import struct
import subprocess
import sys
from pathlib import Path

MAGIC = b"\xd0\x0d\xfe\xed"
REPO = Path(__file__).resolve().parent.parent
LOG = REPO / "tmp" / "logs" / "carve-dtb.log"


def setup_log() -> None:
    LOG.parent.mkdir(parents=True, exist_ok=True)
    h = [logging.FileHandler(LOG), logging.StreamHandler(sys.stdout)]
    logging.basicConfig(level=logging.INFO, format="%(message)s", handlers=h)


def find_fdts(blob: bytes) -> list[tuple[int, int]]:
    """Return (offset, totalsize) for each plausible FDT header."""
    out = []
    for m in re.finditer(re.escape(MAGIC), blob):
        off = m.start()
        if off + 40 > len(blob):
            continue
        (totalsize, off_struct, off_strings, off_rsvmap, version,
         last_comp, _boot_cpu, size_strings, size_struct) = struct.unpack(
            ">9I", blob[off + 4:off + 40])
        if not (0x100 <= totalsize <= 0x100000) or off + totalsize > len(blob):
            continue
        if version < 16 or version > 17 or last_comp > version:
            continue
        if off_struct >= totalsize or off_strings >= totalsize:
            continue
        if off_rsvmap >= totalsize or size_struct + size_strings >= totalsize:
            continue
        out.append((off, totalsize))
    return out


def dts_of(path: Path) -> str:
    r = subprocess.run(["dtc", "-I", "dtb", "-O", "dts", "-q", str(path)],
                       capture_output=True, text=True)
    return r.stdout


def summarise(dts: str) -> tuple[str, str, list[tuple[str, int, int]]]:
    model = ""
    boardcfg = ""
    m = re.search(r'^\s*model\s*=\s*"([^"]*)"', dts, re.M)
    if m:
        model = m.group(1)
    m = re.search(r'board-cfg\s*=\s*"([^"]*)"', dts)
    if m:
        boardcfg = m.group(1)

    parts: list[tuple[str, int, int]] = []
    # locate nand-flash node body, then walk its partition@ children
    nand = re.search(r"nand-flash[^\{]*\{", dts)
    if nand:
        depth = 0
        i = nand.end() - 1
        start = i
        while i < len(dts):
            if dts[i] == "{":
                depth += 1
            elif dts[i] == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        body = dts[start:i]
        for pm in re.finditer(
                r"partition@([0-9a-fA-F]+)\s*\{(.*?)\n(\s*)\};", body, re.S):
            sub = pm.group(2)
            lab = re.search(r'label\s*=\s*"([^"]*)"', sub)
            reg = re.search(r"reg\s*=\s*<([^>]*)>", sub)
            off = size = -1
            if reg:
                vals = [int(x, 0) for x in reg.group(1).split()]
                if len(vals) == 2:
                    off, size = vals
                elif len(vals) == 4:  # #address-cells=2
                    off = (vals[0] << 32) | vals[1]
                    size = (vals[2] << 32) | vals[3]
            parts.append((lab.group(1) if lab else f"@{pm.group(1)}", off, size))
    return model, boardcfg, parts


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("binary", type=Path)
    ap.add_argument("--outdir", type=Path, required=True)
    a = ap.parse_args()
    setup_log()

    blob = a.binary.read_bytes()
    a.outdir.mkdir(parents=True, exist_ok=True)
    hits = find_fdts(blob)
    logging.info("== %s (%d B): %d FDT candidates", a.binary, len(blob), len(hits))

    for n, (off, size) in enumerate(hits):
        out = a.outdir / f"dtb{n:02d}-0x{off:06x}-{size}B.dtb"
        out.write_bytes(blob[off:off + size])
        dts = dts_of(out)
        (out.with_suffix(".dts")).write_text(dts)
        model, cfg, parts = summarise(dts)
        logging.info("")
        logging.info("--- dtb%02d @0x%06x size %d", n, off, size)
        logging.info("    model      = %s", model)
        logging.info("    board-cfg  = %s", cfg)
        logging.info("    nand parts = %d", len(parts))
        for lab, o, s in parts:
            logging.info("      %-14s off 0x%08x  size 0x%08x (%8.2f MB)",
                         lab, o, s, s / 1048576)
    return 0


if __name__ == "__main__":
    sys.exit(main())
