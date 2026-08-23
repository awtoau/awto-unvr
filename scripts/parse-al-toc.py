#!/usr/bin/env python3
"""Parse the Annapurna flash TOC (magic 0x070c070c) in an Alpine SPI boot image.

Prints each object's name, type, offset and size, plus the per-image header
(magic 0x000b9ec7) fields that follow at the object offset.
"""

from __future__ import annotations

import argparse
import logging
import struct
import sys
from pathlib import Path

log = logging.getLogger(__name__)

REPO = Path(__file__).resolve().parent.parent
LOG = REPO / "tmp" / "logs" / "parse-al-toc.log"
TOC_MAGIC = 0x070C070C
IMG_MAGIC = 0x000B9EC7


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("image", type=Path)
    ap.add_argument("--toc", type=lambda s: int(s, 0), default=0x80000)
    a = ap.parse_args()
    LOG.parent.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(
        level=logging.INFO,
        format="%(message)s",
        handlers=[logging.FileHandler(LOG), logging.StreamHandler(sys.stdout)],
    )

    b = a.image.read_bytes()
    magic = struct.unpack_from("<I", b, a.toc)[0]
    log.info(
        "== %s (%d B) TOC@0x%x magic 0x%08x %s",
        a.image,
        len(b),
        a.toc,
        magic,
        "OK" if magic == TOC_MAGIC else "MISMATCH",
    )
    if magic != TOC_MAGIC:
        return 1

    # header: magic, rsvd, count, crc. entry (0x20 B): id[4] type[4] name[12]
    # off[4] size[4] rsvd[4]. type high nibble is the multi-DT index.
    count = struct.unpack_from("<I", b, a.toc + 8)[0]
    log.info("   %d entries", count)
    for n in range(count):
        e = a.toc + 16 + n * 0x20
        oid, otype = struct.unpack_from("<2I", b, e)
        name = b[e + 8 : e + 20].split(b"\0")[0].decode("latin1")
        ooff, osize = struct.unpack_from("<2I", b, e + 20)
        hdr = ""
        if ooff + 0x40 <= len(b) and struct.unpack_from("<I", b, ooff)[0] == IMG_MAGIC:
            sz = struct.unpack_from("<I", b, ooff + 0x28)[0]
            ld, en = struct.unpack_from("<2Q", b, ooff + 0x30)
            hdr = f"  payload 0x{sz:x} load 0x{ld:x} entry 0x{en:x}"
        present = "" if ooff + osize <= len(b) else "  [outside this file]"
        log.info(
            "  [%d] %-10s type 0x%08x off 0x%06x size 0x%06x%s%s",
            n,
            name,
            otype,
            ooff,
            osize,
            hdr,
            present,
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
