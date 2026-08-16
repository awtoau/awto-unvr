#!/usr/bin/env python3
"""Dump the static board table inside ubnt-tools (5.1.25 rootfs /sbin/ubnt-tools).

Entry = 552 B: name[0x40] shortname[0x40] … sysid u32 @+0x6C … uboot-device
str @+0xC0, main-device str @+0x100. Names are XOR'd with a 4-byte key applied
from each string's start; device paths are plain.
Empty device fields mean `fwupdate <file>` (write mode) cannot target that board.
"""
from __future__ import annotations

import argparse
import logging
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
LOG = REPO / "tmp" / "logs" / "dump-ubnt-board-table.log"
KEY = (0x81, 0x93, 0xE0, 0xC4)
STRIDE = 552
DATA_VA_BIAS = 0x10000  # .data vaddr 0x30000 vs file offset 0x20000


def cstr(b: bytes, off: int) -> str:
    return b[off:b.index(b"\0", off)].decode("latin1")


def dstr(b: bytes, off: int) -> str:
    r = b[off:b.index(b"\0", off)]
    return "".join(chr(c ^ KEY[i % 4]) for i, c in enumerate(r))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("elf", type=Path)
    ap.add_argument("--start", type=lambda s: int(s, 0), default=0x22068,
                    help="file offset of the first entry")
    a = ap.parse_args()
    LOG.parent.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(level=logging.INFO, format="%(message)s",
                        handlers=[logging.FileHandler(LOG),
                                  logging.StreamHandler(sys.stdout)])

    b = a.elf.read_bytes()
    logging.info("== %s", a.elf)
    logging.info("%-10s %-7s %-11s %-14s %-14s %s",
                 "vaddr", "sysid", "short", "uboot-dev", "main-dev", "name")
    off = a.start
    while off + STRIDE <= len(b):
        name = dstr(b, off)
        if not name or not name.isprintable():
            break
        sysid = struct.unpack_from("<I", b, off + 0x6C)[0]
        if not 0x1000 <= sysid <= 0xFFFF:  # past the table into the RSA blob
            break
        logging.info("0x%08x 0x%04x  %-11s %-14s %-14s %s",
                     off + DATA_VA_BIAS, sysid, dstr(b, off + 0x40),
                     cstr(b, off + 0xC0) or "-", cstr(b, off + 0x100) or "-",
                     name)
        off += STRIDE
    return 0


if __name__ == "__main__":
    sys.exit(main())
