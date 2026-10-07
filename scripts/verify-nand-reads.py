#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Prove al_nand's reads are correct before anyone trusts a write (#208).

READ-ONLY. Never opens an mtd node for writing, never erases. NAND 0x1300000
holds awto-uboot since #216 and 0x300000 holds the only recovery image there is
(#166) - a bad write to either is a bricked box, so writes are a separate task
that does not start until this passes.

Method: read a region off the live NAND through the new driver and compare it,
byte for byte, against a full-chip backup taken through U-Boot. Two independent
read paths, two different decades of driver code; if they agree, the driver's
ECC, timing and addressing are right.

WHICH backup matters, and the -164356 one is the WRONG reference for the kernel
partition. It predates the stock-firmware upgrade ladder that was run the same
evening, so its NAND kernel is `4.1.37-ubnt @ 20201216` while the chip now
holds `4.19.152-alpine-unvr`. Comparing against it reports ~99.6% of bytes
differing when the read is in fact perfect. The reference has to be the
post-upgrade capture:

  kernel: ...-20260815-212945-post-5.1.25-final/mtd09-linux_kernel
          @ NAND 0x300000, 16 MiB - taken AFTER the whole upgrade ladder

The ROOTFS cannot be verified this way and is not attempted. The upgrade ladder
rewrote it as well, and the only full rootfs capture (-164356, 1004 MiB) is
pre-upgrade; its bytes and the chip's are both high-entropy squashfs but are
different images, so a mismatch there says nothing about the driver. Verifying
it needs a post-upgrade rootfs capture, which does not exist. Re-adding the
pre-upgrade one as a "region" only manufactures a failure.

A second read path over the SAME region is a weaker check (it shares the
driver), so it is not a substitute - the point of the kernel comparison is that
the reference came off the chip through U-Boot's own, independent driver.

Partitions are found by LABEL, not by /dev/mtdN - the numbers shift with probe
order (CLAUDE.md), and reading the wrong node would compare the wrong bytes.

Usage (via dev.py, or AWTO_ALLOW_DIRECT_SCRIPT=1):
    scripts/verify-nand-reads.py
    scripts/verify-nand-reads.py --mib 8     # read more per region
"""

from __future__ import annotations

import argparse
import hashlib
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import _box  # noqa: E402
import _repo  # noqa: E402

REPO = Path(__file__).resolve().parent.parent
MTD_BACKUPS = Path("/mnt/2tb/git_debris/woomera-mtd")

# The backup filenames embed the box's base MAC, which lives ONLY in _box.py
# (#260 guard test). Built from _box.MAC_1G rather than spelled out.
_MAC = _box.MAC_1G.replace(":", "")
_UNIT = f"UNVR-{_MAC}"

# The post-upgrade capture: the only NAND backup that matches what is on the
# chip now - see the module docstring.
POST_UPGRADE = f"{_UNIT}-sysidea16-20260815-212945-post-5.1.25-final"

# SSH is a LAN round trip plus a NAND read. A 4 MiB read at even 1 MB/s is 4 s;
# 300 s is ~75x that and the point at which a wedged controller is the only
# explanation left. On expiry: CalledProcessError, named region, exit 1.
SSH_TIMEOUT_S = 300

# One region per backup image. skip = bytes into the partition to start at,
# which is also bytes into the backup file (both are partition-relative).
REGIONS = (
    {
        "label": "linux_kernel",
        "nand_off": 0x300000,
        "backup": f"{POST_UPGRADE}/{_UNIT}-ea16-mtd09-linux_kernel"
        "-16777216B-20260815-212945.img",
        "skip": 0,
        "why": "stock 4.19.152 uImage, post-upgrade-ladder - a straight match",
    },
)


def mtd_index(ip: str, label: str) -> int:
    """/proc/mtd index for `label`. Device letters are not stable (CLAUDE.md),
    so never hardcode mtdN."""
    out = subprocess.run(
        _box.ssh_argv(ip, cmd=["cat", "/proc/mtd"], batch=True),
        capture_output=True,
        text=True,
        check=True,
        timeout=SSH_TIMEOUT_S,
    ).stdout
    for line in out.splitlines():
        if f'"{label}"' in line:
            return int(line.split(":")[0].removeprefix("mtd"))
    raise SystemExit(f"no mtd partition labelled {label!r} in /proc/mtd on the box")


def read_nand(ip: str, idx: int, skip: int, length: int) -> bytes:
    """Read `length` bytes at `skip` from /dev/mtdNro, over ssh, as raw bytes.

    The `ro` node, not /dev/mtdN: an accidental write to a read-only node fails
    at open() instead of reaching the chip. dd with iflag=skip_bytes so the
    offset is in bytes rather than block multiples.
    """
    cmd = [
        "dd",
        f"if=/dev/mtd{idx}ro",
        f"skip={skip}",
        f"count={length}",
        "bs=1M",
        "iflag=skip_bytes,count_bytes",
        "status=none",
    ]
    p = subprocess.run(
        _box.ssh_argv(ip, cmd=cmd, batch=True),
        capture_output=True,
        check=True,
        timeout=SSH_TIMEOUT_S,
    )
    return p.stdout


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--mib", type=int, default=4, help="MiB to read per region (default 4)"
    )
    args = ap.parse_args()
    length = args.mib * 1024 * 1024

    if not MTD_BACKUPS.is_dir():
        return _fail(f"MTD backup directory missing: {MTD_BACKUPS}")

    ip = _box.require(hint="deploy the al_nand kernel first")
    print(f"# woomera at {ip}, comparing {args.mib} MiB per region")

    failures = 0
    for r in REGIONS:
        img = MTD_BACKUPS / r["backup"]
        if not img.is_file():
            print(f"FAIL {r['label']}: backup image missing: {img}")
            failures += 1
            continue

        want = img.read_bytes()[r["skip"] : r["skip"] + length]
        if len(want) < length:
            print(f"FAIL {r['label']}: backup has only {len(want)} bytes to compare")
            failures += 1
            continue

        idx = mtd_index(ip, r["label"])
        got = read_nand(ip, idx, r["skip"], length)

        if len(got) != length:
            print(f"FAIL {r['label']}: read {len(got)} bytes, wanted {length}")
            failures += 1
            continue

        wd = hashlib.sha256(want).hexdigest()
        gd = hashlib.sha256(got).hexdigest()
        where = f"mtd{idx} NAND 0x{r['nand_off'] + r['skip']:x}"
        if wd == gd:
            print(f"OK   {r['label']:<13} {where}  sha256 {gd[:16]}  ({r['why']})")
        else:
            failures += 1
            first = next((i for i, (a, b) in enumerate(zip(want, got)) if a != b), None)
            diff = sum(a != b for a, b in zip(want, got))
            print(
                f"FAIL {r['label']:<13} {where}\n"
                f"       backup sha256 {wd}\n"
                f"       live   sha256 {gd}\n"
                f"       first differing byte at +0x{first:x},"
                f" {diff} of {length} bytes differ"
            )

    if failures:
        print(f"\n{failures} region(s) MISMATCHED - reads are NOT proven, do not write")
        return 1
    print(f"\nall {len(REGIONS)} regions match their U-Boot backup")
    return 0


def _fail(msg: str) -> int:
    print(msg, file=sys.stderr)
    return 1


if __name__ == "__main__":
    _repo.require_devpy() if hasattr(_repo, "require_devpy") else None
    os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
    sys.exit(main())
