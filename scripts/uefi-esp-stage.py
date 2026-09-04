#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Stage Image.efi + unvr.dtb on the SSD's ESP for docs/uefi.md P4.

P4 boots the kernel as a UEFI application. EDK2's Fat.inf cannot read the
ext4 root, so the kernel and DTB must live on a FAT volume - and the boot
SSD already carries one: a 200 MB `EFI System` partition (a leftover Apple
ESP, EFI/APPLE/EXTENSIONS/Firmware.scap) beside the ext4 root. No
repartitioning needed; ~105 MB free against a 62 MB kernel.

What lands on it:
  \\Image.efi   the arm64 kernel (MZ at 0, PE\\0\\0 at 0x40 - the EFI stub
                makes Image a dual-format PE/COFF UEFI application, which
                `file`'s "Linux kernel ARM64 boot executable" label hides)
  \\unvr.dtb    our board DT, loaded by the stub's own `dtb=` handler
                (CONFIG_EFI_ARMSTUB_DTB_LOADER=y)
No initramfs: this kernel has none by design (build-linux-fedora.py) and
goes straight to root=PARTUUID with the needed drivers built in.

Mounts with `iocharset=ascii`: vfat's default iso8859-1 NLS was missing on
older builds (#250), and ascii needs no NLS module either way.

The ESP is found by GPT partition type, never by /dev/sdX - device letters
shift with probe order across boots (CLAUDE.md).

Usage:
    ./dev.py uefi-esp-stage            # dry run, shows the target + free space
    ./dev.py uefi-esp-stage --yes      # copy the kernel + DTB
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

import _box  # noqa: E402
from _repo import LOGS  # noqa: E402

BUILD_OUT = Path("/mnt/2tb/unvr-port-refs/build-out-fedora")
KERNEL = BUILD_OUT / "Image"
DTB = BUILD_OUT / "alpine-v2-ubnt-unvr-ea16-7.3.dtb"

MNT = "/mnt/awto-esp"
SSH_OPTS = [
    "-o",
    "StrictHostKeyChecking=no",
    "-o",
    "UserKnownHostsFile=/dev/null",
    "-o",
    "LogLevel=ERROR",
]


def log(m: str) -> None:
    print(m, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    (LOGS / "uefi-esp-stage.log").open("a").write(m + "\n")


def box_ip() -> str:
    return _box.require(hint="is it up and in Linux?")


def sh(host: str, cmd: str, check: bool = True) -> str:
    r = subprocess.run(
        ["ssh", *SSH_OPTS, host, cmd], capture_output=True, text=True, check=False
    )
    if check and r.returncode:
        sys.exit(f"FAILED on box: {cmd}\n{r.stdout}{r.stderr}")
    return r.stdout


def find_esp(host: str, explicit: str | None) -> str:
    """Locate the ESP by GPT partition type, never by device letter."""
    if explicit:
        return explicit
    out = sh(host, "lsblk -no PATH,PARTTYPENAME,FSTYPE")
    esps = [
        f[0]
        for line in out.splitlines()
        if (f := line.split()) and "EFI" in line and "vfat" in line
    ]
    log(f"lsblk:\n{out}")
    log(f"ESP candidates: {esps or 'none'}")
    if len(esps) != 1:
        sys.exit(f"want exactly 1 vfat EFI System partition, got {esps}. Use --device.")
    return esps[0]


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--device", help="override the auto-detected ESP partition")
    ap.add_argument("--yes", action="store_true", help="actually copy")
    a = ap.parse_args()

    for p in (KERNEL, DTB):
        if not p.exists():
            sys.exit(f"missing artifact: {p} (run ./dev.py build-fedora)")

    host = f"root@{box_ip()}"
    log(f"uefi-esp-stage -> {host}")
    esp = find_esp(host, a.device)
    need = KERNEL.stat().st_size + DTB.stat().st_size
    log(f"ESP: {esp}, need {need} B")

    sh(host, f"mkdir -p {MNT}")
    sh(host, f"mountpoint -q {MNT} || mount -o iocharset=ascii {esp} {MNT}")
    try:
        log(sh(host, f"df -h {MNT}; ls -l {MNT}"))
        avail = int(sh(host, f"df -B1 --output=avail {MNT} | tail -1").strip())
        if avail < need:
            sys.exit(f"ESP has {avail} B free, need {need} B")

        if not a.yes:
            log("DRY RUN - pass --yes to copy. Nothing written.")
            return 0

        for src, dst in ((KERNEL, "Image.efi"), (DTB, "unvr.dtb")):
            r = subprocess.run(
                ["scp", *SSH_OPTS, str(src), f"{host}:{MNT}/{dst}"], check=False
            )
            if r.returncode:
                sys.exit(f"FAILED: scp {src.name}")
            log(rf"  {src.name} -> \{dst}")
        sh(host, "sync")
        # Verify on the box: a short read is silent otherwise, and this
        # exact class of bug has bitten deploys before (_fedora_deploy.py).
        got = sh(host, f"md5sum {MNT}/Image.efi {MNT}/unvr.dtb")
        log(got)
        for src, dst in ((KERNEL, "Image.efi"), (DTB, "unvr.dtb")):
            want = subprocess.run(
                ["md5sum", str(src)], capture_output=True, text=True, check=True
            ).stdout.split()[0]
            if want not in got:
                sys.exit(f"md5 mismatch for {dst} - copy is short or corrupt")
        log("md5 verified for both files")
    finally:
        sh(host, f"umount {MNT}", check=False)

    log("DONE. From the UEFI Shell, after `connect -r` and `map -r`:")
    log("  fsN:")
    log(r"  Image.efi dtb=\unvr.dtb console=ttyS0,115200 root=PARTUUID=... rw rootwait")
    return 0


if __name__ == "__main__":
    sys.exit(main())
