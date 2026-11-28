#!/usr/bin/env python3
# Stage TrueNAS-image ZFS userland (zdb/zpool/zfs + shared-lib closure, incl. Debian ld.so)
# into a dir, then rsync to woomera /opt/zfs. Run via ld.so --library-path, no chroot.
# Source rootfs: tmp/truenas-install/rootfs (extracted .update). See docs/zfs-p3.md.
import os, re, shutil, subprocess, sys
from pathlib import Path

SRC = Path(os.environ.get("TRUENAS_ROOTFS", "/mnt/2tb/git/awto-unvr/tmp/truenas-install/rootfs"))
DST = Path(sys.argv[1] if len(sys.argv) > 1 else "tmp/zfs-stage")
BINS = ["usr/sbin/zdb", "usr/sbin/zpool", "usr/sbin/zfs"]
LIBDIRS = ["usr/lib/aarch64-linux-gnu", "lib/aarch64-linux-gnu", "usr/lib", "lib"]


def needed(p):
    out = subprocess.run(["readelf", "-d", str(p)], capture_output=True, text=True).stdout
    return re.findall(r"NEEDED.*\[(.+?)\]", out)


def find(name):
    for d in LIBDIRS:
        c = SRC / d / name
        if c.exists():
            return c
    raise SystemExit(f"missing {name}")


seen, todo = {}, []
for b in BINS:
    todo += needed(SRC / b)
while todo:
    n = todo.pop()
    if n in seen:
        continue
    p = find(n)
    seen[n] = p
    todo += needed(p)

(DST / "lib").mkdir(parents=True, exist_ok=True)
(DST / "sbin").mkdir(exist_ok=True)
for b in BINS:
    shutil.copy2(SRC / b, DST / "sbin" / Path(b).name)
for n, p in seen.items():
    shutil.copy2(p.resolve(), DST / "lib" / n)
print("libs:", " ".join(sorted(seen)))
