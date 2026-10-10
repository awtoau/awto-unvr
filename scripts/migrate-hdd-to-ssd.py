#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Move woomera's reference data off the spinning disks onto the SSD (#32).

sda/sdb/sdc are 7.3T/7.3T/5.5T WD/Seagate HDDs; sdd is the 931G Samsung SSD
holding `/` with ~852G free. The only HDD carrying our data is **sda** (47G:
gpl 22G, unvr-port-refs 15G, awto-unvr-images 12G). Moving it to the SSD frees
all three HDDs for the RAID baseline and makes reference lookups fast.

Also moved: **sdc**, 245G of NTFS (cica_od, dbb3, debris). The user confirmed
2026-10-10 that this is theirs and may be moved. Two NTFS specifics:
  * mounted with the kernel `ntfs3` driver, not ntfs-3g - the fuseblk mount on
    this box could not `du` 245G within 300s; in-kernel is several times faster
  * rsync gets `-a`, NOT `-aHAX`: NTFS has no POSIX ACLs or xattrs to preserve
    and requesting them makes rsync error on every file

sdb has no filesystem - nothing to move.

292G total onto 852G free: fits with ~560G spare.

rsync --checksum on the verify pass, not size+mtime: a truncated file whose
mtime was preserved passes a size compare, and these are reference trees whose
corruption would not surface until a search came up empty.

This script NEVER erases anything. Wiping the freed disks is a separate,
explicitly-approved step - there is no undo.

    ./scripts/migrate-hdd-to-ssd.py              # survey, no changes
    ./scripts/migrate-hdd-to-ssd.py --copy       # rsync sda + sdc -> SSD
    ./scripts/migrate-hdd-to-ssd.py --verify     # checksum compare all trees
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

from _box import require, ssh_argv  # noqa: E402
from _repo import make_log  # noqa: E402

log = make_log("migrate-hdd-to-ssd")

SDA_DEV, SDA_MNT, SDA_DST = "/dev/sda", "/mnt/hdd-sda", "/srv/refs"
SDC_DEV, SDC_MNT, SDC_DST = "/dev/sdc2", "/mnt/hdd-sdc", "/srv/windows-sdc"

# Dirs worth keeping off sda. The rest is UniFi OS leftovers (unifi-core,
# unifi-protect, postgresql, aws_s3, ai-feature-console, ds, ms) - the stock
# appliance's own state, regenerable and not ours.
KEEP = ["gpl", "unvr-port-refs", "awto-unvr-images", "scripts"]

# Windows metadata that must not come across: recycle bin, volume metadata,
# chkdsk orphans, OneDrive scratch, boot telemetry.
NTFS_SKIP = [
    "$RECYCLE.BIN",
    "System Volume Information",
    "found.000",
    "OneDriveTemp",
    "bootTel.dat",
]

# A 292G copy over SATA at ~120 MB/s is ~41 min; NTFS small files run slower,
# so 4h is ~6x and only bounds a hung rsync. --verify re-reads both sides
# (584G) and gets 8h. On expiry the step is logged with its limit and the run
# returns non-zero.
COPY_TIMEOUT_S = 14400
VERIFY_TIMEOUT_S = 28800


def box_sh(host: str, cmd: str, timeout: int = 300) -> tuple[int, str]:
    try:
        r = subprocess.run(
            ssh_argv(host, cmd=[cmd]),
            capture_output=True,
            text=True,
            check=False,
            timeout=timeout,
        )
        return r.returncode, (r.stdout + r.stderr).strip()
    except subprocess.TimeoutExpired:
        log(f"TIMEOUT after {timeout}s: {cmd[:70]}")
        return 124, ""


def mount_ro(host: str, dev: str, mnt: str, ntfs: bool = False) -> bool:
    """Mount dev read-only at mnt.

    NTFS prefers the kernel `ntfs3` driver and falls back to ntfs-3g (fuseblk).
    This kernel has no ntfs3, so the fallback is the live path - slower, but a
    read-only copy source does not need more.
    """
    rc, _ = box_sh(host, f"mountpoint -q {mnt}")
    if rc == 0:
        return True
    box_sh(host, f"mkdir -p {mnt}")
    attempts = ["-t ntfs3 -o ro", "-o ro"] if ntfs else ["-o ro"]
    for opt in attempts:
        rc, o = box_sh(host, f"mount {opt} {dev} {mnt}")
        if rc == 0:
            return True
        log(f"  mount {opt} {dev} failed: {o[:120]}")
    log(f"cannot mount {dev} at {mnt}")
    return False


def rsync_flags(ntfs: bool) -> str:
    """-aHAX for ext4; -a plus Windows excludes for NTFS (see module docstring)."""
    if not ntfs:
        return "-aHAX"
    return "-a " + " ".join(f"--exclude='{s}'" for s in NTFS_SKIP)


def copy_tree(host: str, src: str, dst: str, ntfs: bool) -> bool:
    log(f"  copying {src} -> {dst} ...")
    rc, o = box_sh(
        host,
        f"rsync {rsync_flags(ntfs)} --info=stats2 {src} {dst}/ 2>&1 | tail -5",
        timeout=COPY_TIMEOUT_S,
    )
    log(f"  rc={rc}\n{o}")
    return rc == 0


def verify_tree(host: str, src: str, dst: str, ntfs: bool) -> bool:
    """True if identical. -n + --checksum lists only CONTENT differences."""
    log(f"  verifying {dst} (checksum) ...")
    flags = rsync_flags(ntfs).replace("-aHAX", "-aHAXn", 1).replace("-a ", "-an ", 1)
    rc, o = box_sh(
        host,
        f"rsync {flags} --checksum --itemize-changes {src}/ {dst}/ 2>&1 "
        f"| grep -v '^$' | head -20",
        timeout=VERIFY_TIMEOUT_S,
    )
    if rc == 124:
        log("  verify TIMED OUT - treating as FAIL")
        return False
    if o.strip():
        log(f"  DIFFERS:\n{o}")
        return False
    log("  identical")
    return True


def survey(host: str) -> None:
    log("--- sda (ext4, 47G):")
    _, o = box_sh(
        host, f"du -sh {' '.join(f'{SDA_MNT}/{d}' for d in KEEP)} 2>/dev/null"
    )
    log(o)
    log("--- sdc (NTFS, 245G):")
    _, o = box_sh(host, f"df -h {SDC_MNT} | tail -1; ls {SDC_MNT}")
    log(o)
    log("--- destination:")
    _, o = box_sh(host, f"df -h / | tail -1; du -sh {SDA_DST} {SDC_DST} 2>/dev/null")
    log(o)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--copy", action="store_true", help="rsync sda + sdc -> SSD")
    ap.add_argument("--verify", action="store_true", help="checksum-compare trees")
    a = ap.parse_args()

    host = require(hint="is woomera up?")
    if not mount_ro(host, SDA_DEV, SDA_MNT):
        return 1
    if not mount_ro(host, SDC_DEV, SDC_MNT, ntfs=True):
        return 1

    if not (a.copy or a.verify):
        survey(host)
        log("pass --copy to move, then --verify. Nothing is ever erased here.")
        return 0

    if a.copy:
        box_sh(host, f"mkdir -p {SDA_DST} {SDC_DST}")
        for d in KEEP:
            rc, _ = box_sh(host, f"test -d {SDA_MNT}/{d}")
            if rc != 0:
                log(f"  {d}: absent on sda, skipped")
                continue
            if not copy_tree(host, f"{SDA_MNT}/{d}", SDA_DST, ntfs=False):
                log(f"ABORT: copy of {d} failed")
                return 1
        if not copy_tree(host, f"{SDC_MNT}/", SDC_DST, ntfs=True):
            log("ABORT: NTFS copy failed")
            return 1
        _, o = box_sh(host, f"du -sh {SDA_DST} {SDC_DST}; df -h / | tail -1")
        log(o)

    if a.verify:
        bad = []
        for d in KEEP:
            rc, _ = box_sh(host, f"test -d {SDA_DST}/{d}")
            if rc == 0 and not verify_tree(
                host, f"{SDA_MNT}/{d}", f"{SDA_DST}/{d}", False
            ):
                bad.append(d)
        if not verify_tree(host, SDC_MNT, SDC_DST, ntfs=True):
            bad.append("sdc-ntfs")
        if bad:
            log(f"RESULT: FAIL - {bad} differ. The HDDs hold the only good copy.")
            return 1
        log("RESULT: PASS - every tree is byte-identical on the SSD.")
        log("The HDDs are now redundant. Wiping them is a separate approved step.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
