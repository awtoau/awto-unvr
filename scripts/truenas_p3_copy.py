#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Copy the installed boot-pool from its image partition onto p3 - runs ON the box (Fedora), stdlib only.

- Piped by `./dev.py truenas-install commit-p3 --go`.
- Args: <image> <p1 offset> <p3 bytes> <p3 PARTUUID> <p3 PARTNAME>.
- Refuses unless the target is exactly that partition: PARTUUID link, PARTNAME, size, not mounted,
  no holders. Only the image's data extents (SEEK_DATA) are written; holes = never-written
  pool space. Then every written chunk is read back from the device and compared.
"""

from __future__ import annotations

import hashlib
import os
import sys

CHUNK = 4 << 20


def extents(fd: int, start: int, end: int):
    pos = start
    while pos < end:
        try:
            d = os.lseek(fd, pos, os.SEEK_DATA)
        except OSError:  # ENXIO: no data after pos
            return
        if d >= end:
            return
        h = min(os.lseek(fd, d, os.SEEK_HOLE), end)
        yield d, h
        pos = h


def main() -> int:
    img, off, size, partuuid, partname = (
        sys.argv[1],
        int(sys.argv[2]),
        int(sys.argv[3]),
        sys.argv[4],
        sys.argv[5],
    )
    dev = os.path.realpath(f"/dev/disk/by-partuuid/{partuuid}")
    n = os.path.basename(dev)
    sysd = f"/sys/class/block/{n}"
    uevent = open(f"{sysd}/uevent").read()
    if f"PARTNAME={partname}\n" not in uevent:
        sys.exit(f"{dev}: PARTNAME is not {partname}")
    if int(open(f"{sysd}/size").read()) * 512 != size:
        sys.exit(f"{dev}: size != {size}")
    if os.listdir(f"{sysd}/holders") or any(
        line.split()[0] == dev for line in open("/proc/mounts")
    ):
        sys.exit(f"{dev}: in use")
    if os.path.getsize(img) < off + size:
        sys.exit(f"{img}: smaller than offset + size")
    src = os.open(img, os.O_RDONLY)
    dst = os.open(dev, os.O_RDWR | os.O_EXCL)
    written, sums = 0, []
    try:
        for a, b in extents(src, off, off + size):
            p = a
            while p < b:
                n_ = min(CHUNK, b - p)
                buf = os.pread(src, n_, p)
                if os.pwrite(dst, buf, p - off) != len(buf):
                    sys.exit(f"short write at {p - off}")
                sums.append((p - off, len(buf), hashlib.sha256(buf).digest()))
                written += len(buf)
                p += n_
        os.fsync(dst)
        os.posix_fadvise(dst, 0, 0, os.POSIX_FADV_DONTNEED)
        for o, ln, h in sums:
            if hashlib.sha256(os.pread(dst, ln, o)).digest() != h:
                sys.exit(f"read-back mismatch at {o}")
    finally:
        os.close(src)
        os.close(dst)
    print(f"{dev}: wrote + verified {written} B in {len(sums)} chunks from {img}+{off}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
