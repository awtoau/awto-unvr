#!/usr/bin/env python3
"""Format a USB stick as the UNVR's USERDEV (whole-device ext4, no partitions).

The UNVR's USERDEV is the overlay upper layer - `data/` + `.workdir/`, mounted
as `upperdir=`. `recovering_userdev` runs `mkfs_p ${USERDEV}` against the WHOLE
DEVICE, so it wants a bare ext4 filesystem with no partition table. That is
exactly what the original 8 GB stick had. `mountroot` creates `data/` and
`.workdir/` itself, so a blank filesystem is sufficient.

DESTRUCTIVE. Guarded by serial, not by /dev/sdX - device letters shift between
plugs, and a mkfs on the wrong one destroys a 1 TB SSD. The target is matched by
USB serial and cross-checked against size and transport; the failing original is
named in a denylist so it can never be the target.

Run: ./scripts/prep-userdev.py --serial A20043FE1501C484 --yes
     ./scripts/prep-userdev.py --list
"""

import argparse
import json
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
LOGS = REPO / "tmp" / "logs"

# Never format these, whatever else matches. The original UNVR stick is being
# imaged and is the only copy of the old config.
DENY_SERIALS = {"07190176402AEE98"}

# A USERDEV larger than this is almost certainly not the stick you meant.
# The stock part is 8 GB; 128 GB is generous headroom while still excluding
# any plausible SSD/NVMe mis-selection.
MAX_SIZE_BYTES = 128 * 1024**3

# Features modern mkfs.ext4 enables by default that the UNVR's 4.1.37 kernel
# (2020) cannot mount. Verified on hardware: a default mkfs.ext4 produced
#   EXT4-fs (sdq): Couldn't mount because of unsupported optional features (2000)
# 0x2000 is INCOMPAT_CSUM_SEED (metadata_csum_seed). orphan_file is the same
# class of problem - a newer default the device predates. The original stock
# stick had neither; its feature set was:
#   has_journal ext_attr resize_inode dir_index filetype extent 64bit flex_bg
#   sparse_super large_file huge_file dir_nlink extra_isize metadata_csum
# Note metadata_csum (0x400 rocompat) IS supported and present on the original -
# it is only the _seed variant that is not.
DISABLE_FEATURES = ["metadata_csum_seed", "orphan_file"]


def log(msg, level="INFO"):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {level:<5} {msg}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    with (LOGS / "prep-userdev.log").open("a") as fh:
        fh.write(line + "\n")


def block_devices():
    p = subprocess.run(
        ["lsblk", "-J", "-b", "-d", "-o", "NAME,SIZE,TRAN,MODEL,SERIAL,TYPE"],
        capture_output=True, text=True)
    if p.returncode != 0:
        sys.exit(f"lsblk failed: {p.stderr}")
    return json.loads(p.stdout)["blockdevices"]


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--list", action="store_true", help="list USB block devices and exit")
    ap.add_argument("--serial", help="USB serial of the target stick")
    ap.add_argument("--label", default="", help="filesystem label (default: none, matching stock)")
    ap.add_argument("--yes", action="store_true", help="actually do it (otherwise dry run)")
    a = ap.parse_args()

    devs = block_devices()

    if a.list or not a.serial:
        log("USB block devices:")
        for d in devs:
            if d.get("tran") != "usb":
                continue
            deny = " [DENYLISTED]" if d.get("serial") in DENY_SERIALS else ""
            log(f"  /dev/{d['name']:6s} {d['size']:>14} B  {d.get('model','?'):22s} "
                f"serial={d.get('serial','?')}{deny}")
        if not a.serial:
            log("give --serial to select a target", "WARN")
            return 2
        return 0

    if a.serial in DENY_SERIALS:
        sys.exit(f"REFUSING: serial {a.serial} is denylisted (it is the original UNVR stick)")

    match = [d for d in devs if d.get("serial") == a.serial]
    if not match:
        sys.exit(f"no block device with serial {a.serial}. Run --list.")
    if len(match) > 1:
        sys.exit(f"ambiguous: {len(match)} devices with serial {a.serial}")
    d = match[0]
    dev = f"/dev/{d['name']}"

    if d.get("tran") != "usb":
        sys.exit(f"REFUSING: {dev} transport is {d.get('tran')!r}, not usb")
    if d.get("type") != "disk":
        sys.exit(f"REFUSING: {dev} type is {d.get('type')!r}, not disk")
    size = int(d["size"])
    if size > MAX_SIZE_BYTES:
        sys.exit(f"REFUSING: {dev} is {size} B, over the {MAX_SIZE_BYTES} B ceiling")

    mounts = Path("/proc/mounts").read_text()
    if dev in mounts:
        sys.exit(f"REFUSING: {dev} or a partition of it is mounted. Unmount first.")

    log(f"target : {dev}  ({d.get('model','?')}, serial {a.serial}, {size} B)")
    log("plan   : wipefs -a, then mkfs.ext4 across the WHOLE device (no partition table)")

    if not a.yes:
        log("dry run - pass --yes to proceed", "WARN")
        return 0

    mkfs = ["mkfs.ext4", "-F", "-O", "^" + ",^".join(DISABLE_FEATURES)]
    for cmd in (["wipefs", "-a", dev],
                mkfs + (["-L", a.label] if a.label else []) + [dev]):
        log("run: " + " ".join(cmd))
        p = subprocess.run(["sudo", "-n"] + cmd, capture_output=True, text=True)
        if p.stdout.strip():
            log(p.stdout.strip())
        if p.returncode != 0:
            log(p.stderr.strip(), "ERROR")
            sys.exit(f"failed: {' '.join(cmd)}")

    p = subprocess.run(["lsblk", "-o", "NAME,SIZE,FSTYPE,LABEL", dev],
                       capture_output=True, text=True)
    log("result:\n" + p.stdout.strip())
    log("done - fit this stick in the UNVR and power on", "INFO")
    return 0


if __name__ == "__main__":
    sys.exit(main())
