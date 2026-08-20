#!/usr/bin/env python3
"""Dump the U-Boot sysid->board table baked into an Alpine SPI boot image.

Both builds carry a data-driven board_info[] table (matched at runtime against
the EEPROM sysid to setenv bootfrom/sysid/fanspeed/model/...). Two layouts:

- OLD (2020-12-16): stride 0x38, 5 entries, sysid stored BYTE-SWAPPED as u16
  (0x16ea == ea16). Fields: +0x00 sysid  +0x08 name  +0x29 fanspeed
  +0x28 resetled-flag  +0x30 bootfrom. (DTB multi-DT index is chosen by a
  separate code compare-switch @file 0xa318c: ea16->0 ea20->1 ea21->2 ea1a->3
  ea2c->5.)
- NEW (>=5.1.25): stride 0x70, 13 entries, sysid stored NATIVE u16 (0xea16).
  Fields: +0x00 sysid  +0x08 name  +0x2a fanspeed  +0x30 bootfrom
  +0x38 signed-boot flag (1 -> dobootm='run bootsign', 0 -> 'run bootunsign')
  +0x40 short-model ($model)  +0x58 ethprime  +0x20 flag bytes.

Mechanics:
- Locate the `uboot` object in the AL TOC (magic 0x070c070c @0x80000). Its
  Annapurna image header (magic 0x000b9ec7) gives the load VA (0x1100000);
  the payload starts at header + 0x48 (NOT +0x1000).
- VA -> file offset = payload_file_start + (VA - load_va).
- Auto-scan the payload for a run of >=4 records (either layout) whose sysid u16
  is in 0xea00..0xeaff and whose name(+0x08)/bootfrom(+0x30) pointers resolve to
  printable strings. --start/--count/--stride/--swap override the scan.
"""

from __future__ import annotations

import argparse
import logging
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
LOG = REPO / "tmp" / "logs" / "dump-uboot-board-table.log"
TOC_MAGIC = 0x070C070C
IMG_MAGIC = 0x000B9EC7
PAYLOAD_HDR_SKIP = 0x48  # payload begins header+0x48, proven empirically
STRIDE = 0x70


def find_uboot_payload(b: bytes, toc: int) -> tuple[int, int]:
    """Return (payload_file_start, load_va) for the `uboot` TOC object."""
    if struct.unpack_from("<I", b, toc)[0] != TOC_MAGIC:
        raise SystemExit(f"no TOC magic @0x{toc:x}")
    count = struct.unpack_from("<I", b, toc + 8)[0]
    for n in range(count):
        e = toc + 16 + n * 0x20
        name = b[e + 8 : e + 20].split(b"\0")[0].decode("latin1")
        ooff = struct.unpack_from("<I", b, e + 20)[0]
        if name == "uboot":
            if struct.unpack_from("<I", b, ooff)[0] != IMG_MAGIC:
                raise SystemExit(f"uboot obj @0x{ooff:x} lacks image magic")
            load_va = struct.unpack_from("<Q", b, ooff + 0x30)[0]
            return ooff + PAYLOAD_HDR_SKIP, load_va
    raise SystemExit("no `uboot` object in TOC")


def make_resolver(b: bytes, pstart: int, load_va: int):
    def s(va: int):
        if va == 0:
            return None
        o = pstart + (va - load_va)
        if not (0 <= o < len(b)):
            return None
        end = b.find(b"\0", o)
        if end < 0 or end - o > 64:
            return None
        try:
            t = b[o:end].decode("ascii")
        except UnicodeDecodeError:
            return None
        return t if t.isprintable() else None

    return s


LAYOUTS = [(0x70, False), (0x38, True)]  # (stride, sysid byte-swapped)


def read_sysid(b: bytes, off: int, swap: bool) -> int:
    v = struct.unpack_from("<H", b, off)[0]
    return ((v & 0xFF) << 8) | (v >> 8) if swap else v


def looks_like_entry(b: bytes, off: int, s, stride: int, swap: bool) -> bool:
    if off + stride > len(b):
        return False
    if not 0xEA00 <= read_sysid(b, off, swap) <= 0xEAFF:
        return False
    name = s(struct.unpack_from("<Q", b, off + 0x08)[0])
    boot = s(struct.unpack_from("<Q", b, off + 0x30)[0])
    return bool(name) and bool(boot)


def run_len(b: bytes, off: int, s, stride: int, swap: bool) -> int:
    n = 0
    while looks_like_entry(b, off + n * stride, s, stride, swap):
        n += 1
    return n


def scan_table(b, pstart, load_va, s, stride, swap):
    best = None
    off = pstart
    while off + stride <= len(b):
        if looks_like_entry(b, off, s, stride, swap):
            n = run_len(b, off, s, stride, swap)
            if n >= 4 and (best is None or n > best[1]):
                best = (off, n)
            off += n * stride
        else:
            off += 4
    return best[0] if best else None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "image", type=Path, help="AL SPI boot image (01-uboot.bin or mtdXX u-boot)"
    )
    ap.add_argument("--toc", type=lambda x: int(x, 0), default=0x80000)
    ap.add_argument(
        "--start",
        type=lambda x: int(x, 0),
        default=None,
        help="file offset of first entry (default: auto-scan)",
    )
    ap.add_argument("--count", type=int, default=None)
    ap.add_argument("--stride", type=lambda x: int(x, 0), default=None)
    ap.add_argument("--swap", action="store_true", help="sysid stored byte-swapped")
    a = ap.parse_args()

    LOG.parent.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(
        level=logging.INFO,
        format="%(message)s",
        handlers=[logging.FileHandler(LOG), logging.StreamHandler(sys.stdout)],
    )

    b = a.image.read_bytes()
    pstart, load_va = find_uboot_payload(b, a.toc)
    s = make_resolver(b, pstart, load_va)
    logging.info("== %s (%d B)", a.image, len(b))
    logging.info("   uboot payload file 0x%x  load VA 0x%x", pstart, load_va)

    # pick layout: explicit args, else best-scoring auto-scan over LAYOUTS
    start, stride, swap = a.start, a.stride, a.swap
    if start is None:
        best = None  # (nrun, start, stride, swap)
        for st, sw in LAYOUTS:
            if a.stride is not None and st != a.stride:
                continue
            off = scan_table(b, pstart, load_va, s, st, sw)
            if off is not None:
                n = run_len(b, off, s, st, sw)
                if best is None or n > best[0]:
                    best = (n, off, st, sw)
        if best is None:
            logging.info("   NO board table found")
            return 0
        _, start, stride, swap = best
    else:
        stride = stride or STRIDE

    dt_note = (
        (
            "  (OLD: DTB multi-DT idx from a separate code switch @file 0xa318c: "
            "ea16->0 ea20->1 ea21->2 ea1a->3 ea2c->5)"
        )
        if stride == 0x38
        else ""
    )
    logging.info(
        "   table @file 0x%x (VA 0x%x) stride 0x%x swap=%s%s",
        start,
        load_va + (start - pstart),
        stride,
        swap,
        dt_note,
    )
    fan_off = 0x29 if stride == 0x38 else 0x2A
    wide = stride >= 0x70
    logging.info(
        "%-3s %-7s %-12s %-10s %-4s %-9s %-9s %s",
        "idx",
        "sysid",
        "name",
        "bootfrom",
        "fan",
        "model",
        "ethprime",
        "dobootm",
    )
    off, i = start, 0
    while (a.count is None and looks_like_entry(b, off, s, stride, swap)) or (
        a.count is not None and i < a.count
    ):
        sysid = read_sysid(b, off, swap)
        name = s(struct.unpack_from("<Q", b, off + 0x08)[0]) or "-"
        boot = s(struct.unpack_from("<Q", b, off + 0x30)[0]) or "-"
        fan = b[off + fan_off]
        model = eth = dob = "-"
        if wide:
            model = s(struct.unpack_from("<Q", b, off + 0x40)[0]) or "-"
            eth = s(struct.unpack_from("<Q", b, off + 0x58)[0]) or "-"
            dob = "bootsign" if b[off + 0x38] else "bootunsign"
        logging.info(
            "%-3d 0x%04x  %-12s %-10s 0x%02x %-9s %-9s %s",
            i,
            sysid,
            name,
            boot,
            fan,
            model,
            eth,
            dob,
        )
        off += stride
        i += 1
    logging.info("   %d entries", i)
    return 0


if __name__ == "__main__":
    sys.exit(main())
