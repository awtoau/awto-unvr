#!/usr/bin/env python3
"""Walk rpsd .data.rel.ro as an 8-byte LE pointer array; resolve pointers into
.rodata to strings. Find the ea16 group descriptor + pin sub-table.

Sections (from readelf):
  .rodata       vaddr 0x1d728 == fileoff 0x1d728  len 0x50be
  .data.rel.ro  vaddr 0x36b88 ,  fileoff 0x26b88  len 0x1148
  .data         vaddr 0x383e8 ,  fileoff 0x283e8
rodata vaddr==fileoff so rodata pointers read straight as file offsets.
data.rel.ro vaddr - fileoff = 0x10000.
"""

import struct
import sys

BIN = "/mnt/2tb/git/awto-unvr/tmp/fw-rootfs/sbin/rpsd"
data = open(BIN, "rb").read()

ROD_V, ROD_F, ROD_L = 0x1D728, 0x1D728, 0x50BE
DRO_V, DRO_F, DRO_L = 0x36B88, 0x26B88, 0x1148
DAT_V, DAT_F = 0x383E8, 0x283E8


def rod_end_v():
    return ROD_V + ROD_L


def in_rodata(v):
    return ROD_V <= v < ROD_V + ROD_L


def in_dro(v):
    return DRO_V <= v < DRO_V + DRO_L


def in_data(v):
    return DAT_V <= v < DAT_V + 0x4000


def v2f(v):
    if in_rodata(v):
        return v  # vaddr==fileoff
    if in_dro(v):
        return v - (DRO_V - DRO_F)
    if in_data(v):
        return v - (DAT_V - DAT_F)
    return None


def cstr(fileoff, maxlen=64):
    if fileoff is None or fileoff < 0 or fileoff >= len(data):
        return None
    end = data.find(b"\x00", fileoff)
    if end < 0:
        end = fileoff + maxlen
    s = data[fileoff:end]
    try:
        return s.decode("ascii")
    except:
        return repr(s)


def rd_ptrs(fileoff, count):
    return list(struct.unpack_from("<{}Q".format(count), data, fileoff))


def annotate(v):
    if v == 0:
        return "NULL"
    f = v2f(v)
    if f is None:
        return "0x{:x}".format(v)
    if in_rodata(v):
        s = cstr(f)
        if s is not None and all(32 <= ord(c) < 127 for c in s) and len(s) >= 1:
            return "0x{:x} -> rodata str {!r}".format(v, s)
        return "0x{:x} -> rodata(bin)".format(v)
    if in_dro(v):
        return "0x{:x} -> DRO(file 0x{:x})".format(v, f)
    if in_data(v):
        return "0x{:x} -> DATA(file 0x{:x})".format(v, f)
    return "0x{:x}".format(v)


def main():
    cmd = sys.argv[1] if len(sys.argv) > 1 else "scan"
    if cmd == "scan":
        # walk data.rel.ro as pointer array, print entries that resolve into rodata
        print("== .data.rel.ro pointer scan (entries pointing into rodata) ==")
        n = DRO_L // 8
        for i in range(n):
            fo = DRO_F + i * 8
            v = struct.unpack_from("<Q", data, fo)[0]
            if in_rodata(v):
                print(
                    "DRO file 0x{:05x} vaddr 0x{:05x} : {}".format(
                        fo, DRO_V + i * 8, annotate(v)
                    )
                )
    elif cmd == "findref":
        # find pointer array entries (anywhere in file, 8-aligned) whose value == target vaddr
        target = int(sys.argv[2], 0)
        print("== searching whole file for LE64 == 0x{:x} ==".format(target))
        off = 0
        while True:
            idx = data.find(struct.pack("<Q", target), off)
            if idx < 0:
                break
            sect = (
                "rodata"
                if ROD_F <= idx < ROD_F + ROD_L
                else (
                    "DRO"
                    if DRO_F <= idx < DRO_F + DRO_L
                    else ("data" if DAT_F <= idx else "?")
                )
            )
            print("  at file 0x{:05x} ({})".format(idx, sect))
            off = idx + 1
    elif cmd == "struct":
        # dump a run of pointers starting at a file offset
        fo = int(sys.argv[2], 0)
        cnt = int(sys.argv[3], 0) if len(sys.argv) > 3 else 16
        for i in range(cnt):
            v = struct.unpack_from("<Q", data, fo + i * 8)[0]
            print(
                "  [{:2d}] file 0x{:05x} vaddr 0x{:05x} = {}".format(
                    i,
                    fo + i * 8,
                    (fo + i * 8)
                    + (DRO_V - DRO_F if DRO_F <= fo < DRO_F + DRO_L else 0),
                    annotate(v),
                )
            )
    elif cmd == "dump":
        fo = int(sys.argv[2], 0)
        cnt = int(sys.argv[3], 0) if len(sys.argv) > 3 else 128
        import binascii

        for i in range(0, cnt, 16):
            chunk = data[fo + i : fo + i + 16]
            print("0x{:05x}  {}".format(fo + i, binascii.hexlify(chunk, " ").decode()))


if __name__ == "__main__":
    main()
