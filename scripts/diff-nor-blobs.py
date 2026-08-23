#!/usr/bin/env python3
"""Carve and diff the SPI-NOR boot sub-blobs of two Annapurna Alpine-V2 boot
containers (Ubiquiti UNVR mtd00 / u-boot partition).

Sub-blobs (per Annapurna flash layout):
  S2 SPI loader   file 0x0      header "S2\\0\\0", size @+0x0c, code @+0x20
  al_boot preboot image hdr @0x20000 (magic 0x000b9ec7), payload @0x21000
  TOC             @0x80000 magic 0x070c070c -> dt/dt_pro/dt_ai/dt_bt[/dt_hd], uboot
Each TOC object begins with an image header (magic 0x000b9ec7); payload size @+0x28,
payload starts +0x48.

Reports per sub-blob: present in both?, size delta, identical / changed, and for
changed blobs a byte-diff summary (n differing bytes, first/last diff offset).

Usage: diff-nor-blobs.py OLD.img NEW.img [--toc 0x80000]
"""

from __future__ import annotations

import argparse
import logging
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
LOG = REPO / "tmp" / "logs" / "diff-nor-blobs.log"
TOC_MAGIC = 0x070C070C
IMG_MAGIC = 0x000B9EC7
S2_MAGIC = b"S2\0\0"


def carve_s2(b: bytes) -> tuple[int, int] | None:
    if b[:4] != S2_MAGIC:
        return None
    size = struct.unpack_from("<I", b, 0x0C)[0]  # code size, code starts @0x20
    return (0, 0x20 + size)


def carve_preboot(b: bytes, at: int = 0x20000) -> tuple[int, int] | None:
    if struct.unpack_from("<I", b, at)[0] != IMG_MAGIC:
        return None
    size = struct.unpack_from("<I", b, at + 0x28)[0]
    payload = at + 0x1000  # payload @0x21000
    end = payload + size
    if end > len(b):  # header size can exceed carved container; clamp
        end = min(len(b), 0x80000)
    return (payload, end)


def parse_toc(b: bytes, toc: int) -> list[dict]:
    if struct.unpack_from("<I", b, toc)[0] != TOC_MAGIC:
        return []
    count = struct.unpack_from("<I", b, toc + 8)[0]
    out = []
    for n in range(count):
        e = toc + 16 + n * 0x20
        name = b[e + 8 : e + 20].split(b"\0")[0].decode("latin1")
        ooff, osize = struct.unpack_from("<2I", b, e + 20)
        psize = ppayload = None
        if ooff + 0x48 <= len(b) and struct.unpack_from("<I", b, ooff)[0] == IMG_MAGIC:
            psize = struct.unpack_from("<I", b, ooff + 0x28)[0]
            ppayload = ooff + 0x48
        out.append(
            {
                "name": name,
                "off": ooff,
                "size": osize,
                "psize": psize,
                "ppayload": ppayload,
            }
        )
    return out


def diff_region(a: bytes, b: bytes, ra: tuple[int, int], rb: tuple[int, int]) -> str:
    sa, sb = a[ra[0] : ra[1]], b[rb[0] : rb[1]]
    la, lb = len(sa), len(sb)
    if sa == sb:
        return f"IDENTICAL  ({la} B, sha match)"
    n = min(la, lb)
    diffs = [i for i in range(n) if sa[i] != sb[i]]
    tail = abs(la - lb)
    first = diffs[0] if diffs else n
    last = diffs[-1] if diffs else n
    return (
        f"CHANGED    old={la} new={lb} (delta {lb - la:+d}); "
        f"{len(diffs)} of {n} common bytes differ, "
        f"first@0x{first:x} last@0x{last:x}"
        + (f", +{tail} trailing bytes only in longer" if tail else "")
    )


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("old", type=Path)
    ap.add_argument("new", type=Path)
    ap.add_argument("--toc", type=lambda s: int(s, 0), default=0x80000)
    ap.add_argument(
        "--carve",
        type=Path,
        default=None,
        help="dir to carve NEW's sub-blobs into (S2, preboot, each TOC obj)",
    )
    a = ap.parse_args()
    LOG.parent.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(
        level=logging.INFO,
        format="%(message)s",
        handlers=[logging.FileHandler(LOG), logging.StreamHandler(sys.stdout)],
    )
    log = logging.info

    ob, nb = a.old.read_bytes(), a.new.read_bytes()
    log("OLD %s (%d B)", a.old, len(ob))
    log("NEW %s (%d B)", a.new, len(nb))
    log("")

    # S2
    ra, rb = carve_s2(ob), carve_s2(nb)
    if ra and rb:
        log("%-10s %s", "S2", diff_region(ob, nb, ra, rb))
    else:
        log("%-10s missing S2 header (old=%s new=%s)", "S2", bool(ra), bool(rb))

    # preboot / al_boot payload
    ra, rb = carve_preboot(ob), carve_preboot(nb)
    if ra and rb:
        log("%-10s %s", "preboot", diff_region(ob, nb, ra, rb))
    else:
        log("%-10s missing al_boot header", "preboot")

    # TOC objects (payload-level)
    ot = {o["name"]: o for o in parse_toc(ob, a.toc)}
    nt = {o["name"]: o for o in parse_toc(nb, a.toc)}
    order = list(dict.fromkeys(list(ot) + list(nt)))
    log("")
    for name in order:
        o, n = ot.get(name), nt.get(name)
        if not o or not n:
            log("%-10s only in %s", name, "OLD" if o else "NEW")
            continue
        if o["ppayload"] is None or n["ppayload"] is None:
            # no image header (e.g. env objects): compare whole TOC region if present
            ra = (o["off"], min(o["off"] + o["size"], len(ob)))
            rb = (n["off"], min(n["off"] + n["size"], len(nb)))
            if ra[1] <= ra[0] or rb[1] <= rb[0]:
                log(
                    "%-10s TOC off 0x%06x size 0x%06x (outside carved file)",
                    name,
                    o["off"],
                    o["size"],
                )
                continue
            log("%-10s [region] %s", name, diff_region(ob, nb, ra, rb))
            continue
        oe = min(o["ppayload"] + o["psize"], len(ob))
        ne = min(n["ppayload"] + n["psize"], len(nb))
        log(
            "%-10s off 0x%06x->0x%06x  %s",
            name,
            o["off"],
            n["off"],
            diff_region(ob, nb, (o["ppayload"], oe), (n["ppayload"], ne)),
        )

    if a.carve:
        a.carve.mkdir(parents=True, exist_ok=True)
        r = carve_s2(nb)
        if r:
            (a.carve / "s2-loader.bin").write_bytes(nb[r[0] : r[1]])
        r = carve_preboot(nb)
        if r:
            (a.carve / "al_boot-preboot.bin").write_bytes(nb[r[0] : r[1]])
        for name, o in nt.items():
            if o["ppayload"] is None:
                continue
            end = min(o["ppayload"] + o["psize"], len(nb))
            (a.carve / f"{name}.bin").write_bytes(nb[o["ppayload"] : end])
        log("\ncarved NEW sub-blobs -> %s", a.carve)
    return 0


if __name__ == "__main__":
    sys.exit(main())
