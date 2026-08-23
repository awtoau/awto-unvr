#!/usr/bin/env python3
"""Unpack MTD dumps into browsable directories.

Identifies each .img and extracts what it can, so the contents can be read with
an ordinary file manager instead of a hex editor:

  ext4      -> loop-mounted read-only and copied out to a plain directory
  squashfs  -> unsquashfs
  uImage    -> header decoded, payload decompressed
  JPEG/PNG  -> copied out with the right extension
  raw       -> left alone, reported with its identification

ext4 is mounted with `ro,noload`. The UNVR's config partition reports
"needs journal recovery"; a plain mount would REPLAY the journal and modify the
image, destroying the evidence we just captured. noload skips it.

Run: ./scripts/unpack-mtd.py                       # newest dump set
     ./scripts/unpack-mtd.py --set <dirname>
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import IMAGES, LOGS, rel

MTD_ROOT = IMAGES / "mtd"


def log(msg, level="INFO"):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {level:<5} {msg}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    with (LOGS / "unpack-mtd.log").open("a") as fh:
        fh.write(line + "\n")


def identify(p: Path) -> str:
    r = subprocess.run(
        ["file", "-b", str(p)], capture_output=True, text=True, check=False
    )
    return r.stdout.strip()


def unpack_ext4(img: Path, dest: Path) -> bool:
    """Loop-mount read-only and copy out. Needs sudo for mount only."""
    dest.mkdir(parents=True, exist_ok=True)
    mnt = dest.parent / f".mnt-{img.stem}"
    mnt.mkdir(parents=True, exist_ok=True)
    # ro,noload: the image says "needs journal recovery" and a normal mount would
    # replay the journal, writing to the dump we are trying to preserve.
    r = subprocess.run(
        ["sudo", "-n", "mount", "-o", "ro,noload,loop", str(img), str(mnt)],
        capture_output=True,
        text=True,
        check=False,
    )
    if r.returncode != 0:
        log(f"  mount failed: {r.stderr.strip()}", "ERROR")
        mnt.rmdir()
        return False
    try:
        r = subprocess.run(
            ["sudo", "-n", "cp", "-a", f"{mnt}/.", str(dest)],
            capture_output=True,
            text=True,
            check=False,
        )
        if r.returncode != 0:
            log(f"  copy failed: {r.stderr.strip()}", "ERROR")
            return False
        subprocess.run(
            ["sudo", "-n", "chown", "-R", f"{os_uid()}:{os_gid()}", str(dest)],
            capture_output=True,
            text=True,
            check=False,
        )
    finally:
        subprocess.run(
            ["sudo", "-n", "umount", str(mnt)],
            capture_output=True,
            text=True,
            check=False,
        )
        mnt.rmdir()
    n = sum(1 for _ in dest.rglob("*"))
    log(f"  extracted {n} entries -> {rel(dest)}")
    return True


def os_uid() -> int:
    import os

    return os.getuid()


def os_gid() -> int:
    import os

    return os.getgid()


def unpack_squashfs(img: Path, dest: Path) -> bool:
    if dest.exists():
        shutil.rmtree(dest)
    r = subprocess.run(
        ["unsquashfs", "-d", str(dest), str(img)],
        capture_output=True,
        text=True,
        check=False,
    )
    if r.returncode != 0:
        log(f"  unsquashfs failed: {r.stderr.strip()[:200]}", "ERROR")
        return False
    n = sum(1 for _ in dest.rglob("*"))
    log(f"  extracted {n} entries -> {rel(dest)}")
    return True


def unpack_image(img: Path, dest: Path, kind: str) -> bool:
    """JPEG/PNG and friends: copy out with a sensible extension."""
    dest.mkdir(parents=True, exist_ok=True)
    ext = "jpg" if "JPEG" in kind else "png" if "PNG" in kind else "bin"
    out = dest / f"{img.stem}.{ext}"
    shutil.copy2(img, out)
    log(f"  copied -> {rel(out)}")
    return True


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--set", help="dump-set directory name under images/mtd/")
    a = ap.parse_args()

    if not MTD_ROOT.is_dir():
        sys.exit(f"no dumps at {rel(MTD_ROOT)}")
    sets = sorted(
        [d for d in MTD_ROOT.iterdir() if d.is_dir()],
        key=lambda d: d.name,
        reverse=True,
    )
    if not sets:
        sys.exit(f"no dump sets under {rel(MTD_ROOT)}")
    if a.set:
        chosen = [d for d in sets if d.name == a.set]
        if not chosen:
            sys.exit(f"no such set: {a.set}")
        targets = chosen
    else:
        targets = sets  # unpack every set; they are per-run and disjoint

    for s in targets:
        imgs = sorted(s.glob("*.img"))
        if not imgs:
            continue
        log(f"=== {s.name} ({len(imgs)} image(s)) ===")
        outroot = s / "unpacked"
        for img in imgs:
            kind = identify(img)
            log(f"{img.name}")
            log(f"  {kind}")
            stem = img.stem.split("-")[3] if len(img.stem.split("-")) > 3 else img.stem
            dest = outroot / stem
            if "ext4" in kind or "ext2" in kind or "ext3" in kind:
                unpack_ext4(img, dest)
            elif "Squashfs" in kind or "squashfs" in kind:
                unpack_squashfs(img, dest)
            elif "JPEG" in kind or "PNG" in kind:
                unpack_image(img, dest, kind)
            elif "u-boot legacy uImage" in kind or "uImage" in kind:
                log("  uImage - use scripts/analyse-unvr-firmware.py to split")
            else:
                log("  raw - left as-is")
    return 0


if __name__ == "__main__":
    sys.exit(main())
