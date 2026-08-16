#!/usr/bin/env python3
"""Wrap an ARM64 kernel Image as a U-Boot legacy uImage (optionally gzip).

Pure-python (no mkimage dep). gzip matters for the full-Fedora-config kernel:
uncompressed it is ~56 MB (slow tftp, and its load span overlaps the DTB
address). Compressed it is ~18 MB; U-Boot decompresses to the load address, so
tftp is fast and the standard DTB address (0x04078000) is clear again.

  mkuimage.py Image out.uImage --gzip --name unvr-ea16-7.1-fedora
"""
import argparse, gzip, struct, sys, time, zlib
from pathlib import Path

IH_MAGIC = 0x27051956
IH_OS_LINUX, IH_ARCH_ARM64, IH_TYPE_KERNEL = 5, 22, 2
IH_COMP_NONE, IH_COMP_GZIP = 0, 1
LOAD = ENTRY = 0x08080000


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("image")
    ap.add_argument("out")
    ap.add_argument("--gzip", action="store_true")
    ap.add_argument("--name", default="linux")
    ap.add_argument("--load", type=lambda x: int(x, 0), default=LOAD)
    ap.add_argument("--entry", type=lambda x: int(x, 0), default=ENTRY)
    a = ap.parse_args()

    raw = Path(a.image).read_bytes()
    data = gzip.compress(raw, 9) if a.gzip else raw
    comp = IH_COMP_GZIP if a.gzip else IH_COMP_NONE
    dcrc = zlib.crc32(data) & 0xffffffff
    nm = a.name.encode()[:31].ljust(32, b"\0")

    def hdr(hc):
        return struct.pack(">IIIIIIIBBBB32s", IH_MAGIC, hc, int(time.time()),
                           len(data), a.load, a.entry, dcrc,
                           IH_OS_LINUX, IH_ARCH_ARM64, IH_TYPE_KERNEL, comp, nm)
    h = hdr(zlib.crc32(hdr(0)) & 0xffffffff)
    Path(a.out).write_bytes(h + data)
    print(f"{a.out}: {len(h)+len(data)} bytes "
          f"({'gzip' if a.gzip else 'none'}, raw {len(raw)} -> {len(data)}), "
          f"load/entry=0x{a.load:08x}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
