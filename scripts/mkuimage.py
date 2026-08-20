#!/usr/bin/env python3
"""Wrap an ARM64 kernel Image as a U-Boot legacy uImage (optionally gzip).

Pure-python (no mkimage dep). gzip matters for the full-Fedora-config kernel:
uncompressed it is ~56 MB (slow tftp, and its load span overlaps the DTB
address). Compressed it is ~18 MB; U-Boot decompresses to the load address, so
tftp is fast and the standard DTB address (0x04078000) is clear again.

  mkuimage.py Image out.uImage --gzip --name unvr-ea16-7.1-fedora
  mkuimage.py initramfs.img out.uimg --ramdisk   # wrap an initrd (this U-Boot has
                                                 # no raw addr:size support)
"""

import argparse
import gzip
import struct
import sys
import time
import zlib
from pathlib import Path

IH_MAGIC = 0x27051956
IH_OS_LINUX, IH_ARCH_ARM64, IH_TYPE_KERNEL, IH_TYPE_RAMDISK = 5, 22, 2, 3
IH_COMP_NONE, IH_COMP_GZIP = 0, 1
LOAD = ENTRY = 0x08000000


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("image")
    ap.add_argument("out")
    ap.add_argument("--gzip", action="store_true")
    ap.add_argument(
        "--ramdisk",
        action="store_true",
        help="wrap as an initrd (type=RAMDISK, load/entry=0, comp=NONE)",
    )
    ap.add_argument("--name", default="linux")
    ap.add_argument("--load", type=lambda x: int(x, 0), default=None)
    ap.add_argument("--entry", type=lambda x: int(x, 0), default=None)
    a = ap.parse_args()

    ih_type = IH_TYPE_RAMDISK if a.ramdisk else IH_TYPE_KERNEL
    # ramdisk: load/entry 0 (U-Boot places it, passes addr/size to the kernel).
    load = a.load if a.load is not None else (0 if a.ramdisk else LOAD)
    entry = a.entry if a.entry is not None else (0 if a.ramdisk else ENTRY)

    raw = Path(a.image).read_bytes()
    data = gzip.compress(raw, 9) if a.gzip else raw
    comp = IH_COMP_GZIP if a.gzip else IH_COMP_NONE
    dcrc = zlib.crc32(data) & 0xFFFFFFFF
    nm = a.name.encode()[:31].ljust(32, b"\0")

    def hdr(hc):
        return struct.pack(
            ">IIIIIIIBBBB32s",
            IH_MAGIC,
            hc,
            int(time.time()),
            len(data),
            load,
            entry,
            dcrc,
            IH_OS_LINUX,
            IH_ARCH_ARM64,
            ih_type,
            comp,
            nm,
        )

    h = hdr(zlib.crc32(hdr(0)) & 0xFFFFFFFF)
    Path(a.out).write_bytes(h + data)
    print(
        f"{a.out}: {len(h) + len(data)} bytes "
        f"({'ramdisk' if a.ramdisk else 'kernel'}, "
        f"{'gzip' if a.gzip else 'none'}, raw {len(raw)} -> {len(data)}), "
        f"load/entry=0x{load:08x}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
