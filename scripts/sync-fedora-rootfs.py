#!/usr/bin/env python3
"""Sync a freshly-built Fedora rootfs tree onto woomera's LIVE running SSD
root, over SSH - no reboot, no reformat, no console. rsync --delete strips
any accumulated cruft (stray drop-ins, one-off debugging files) that isn't
in the fresh tree, while leaving in-progress work and data untouched.

Excludes: pseudo-filesystems (proc/sys/dev/run), user/work data
(root/home/mnt/tmp), and things owned by a separate deploy pipeline (boot -
kernel/DTB via publish-fedora+flash; SSH host keys/machine-id/hostname - so
this doesn't force a "host identity changed" SSH warning or reset identity
for a live box that isn't being reinstalled).

Run: ./dev.py sync-fedora-rootfs [--dry-run] [--yes]
Prereq: ./dev.py build-fedora-rootfs (produces tmp/fedora-rootfs-ea16.tar)
"""

from __future__ import annotations

import argparse
import subprocess
import sys
import tarfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import LOGS, REPO, log_path

ROOTFS_TAR = REPO / "tmp" / "fedora-rootfs-ea16.tar"
EXTRACT_DIR = REPO / "tmp" / "fedora-rootfs-extracted"
ROOT_PASSWORD = "unvr"  # documented default, see docs/fedora-on-ssd.md

EXCLUDES = [
    "/proc/",
    "/sys/",
    "/dev/",
    "/run/",
    "/tmp/",
    "/root/",
    "/home/",
    "/mnt/",
    "/media/",
    "/boot/",  # owned by publish-fedora + ./dev.py flash, not this script
    "/efi/",  # ESP automount (unused - no UEFI boot on this board), live mount point
    "/usr/lib/modules/",
    "/lib/modules/",  # owned by publish-fedora's sync_modules() - a plain
    # dnf installroot has no OOT kernel modules, so --delete wiped the just-deployed tree
    # (confirmed live 2026-08-28: /lib/modules/7.1.8-dirty gone after the first real sync)
    "/etc/machine-id",
    "/etc/hostname",
    "/etc/ssh/ssh_host_*",
]

LOG = log_path("sync-fedora-rootfs")


def log(m: str) -> None:
    from datetime import datetime, timezone

    line = (
        f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    )
    print(line, flush=True)
    LOG.open("a").write(line + "\n")


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--dry-run", action="store_true", help="rsync -n, report only")
    ap.add_argument("--yes", action="store_true", help="skip the confirmation prompt")
    a = ap.parse_args()

    if not ROOTFS_TAR.is_file():
        log(
            f"ABORT: no {ROOTFS_TAR} - run ./dev.py build-fedora-rootfs first",
        )
        return 1

    if EXTRACT_DIR.is_dir():
        # root-owned files from a prior `dnf --installroot` extraction need
        # sudo to remove - a plain rm -rf fails partway through (Permission
        # denied), same reason build-fedora-rootfs.py uses sudo throughout.
        subprocess.run(["sudo", "-n", "rm", "-rf", str(EXTRACT_DIR)], check=True)
    EXTRACT_DIR.mkdir(parents=True)
    log(f"extracting {ROOTFS_TAR.name} -> {EXTRACT_DIR.relative_to(REPO)}")
    subprocess.run(
        ["sudo", "-n", "tar", "-C", str(EXTRACT_DIR), "-xf", str(ROOTFS_TAR)],
        check=True,
    )

    host = subprocess.run(
        [sys.executable, "scripts/ssh-woomera.py", "--print"],
        cwd=REPO,
        capture_output=True,
        text=True,
        timeout=15,
        check=False,
    ).stdout.strip()
    if not host:
        log("ABORT: woomera not reachable over SSH")
        return 1
    log(f"target: root@{host}:/ (live, running system)")

    ssh_opts = (
        "ssh -o StrictHostKeyChecking=accept-new"
        " -o PreferredAuthentications=password -o PubkeyAuthentication=no"
    )
    rsync_cmd = [
        # sudo: the extracted source tree is root-owned (see extraction
        # above) - a plain-user rsync can't read files it doesn't have
        # permission on.
        "sudo",
        "-n",
        "sshpass",
        "-p",
        ROOT_PASSWORD,
        "rsync",
        "-a",
        "--delete",
        "-e",
        ssh_opts,
    ]
    for e in EXCLUDES:
        rsync_cmd += ["--exclude", e]
    if a.dry_run:
        rsync_cmd.append("-n")
    rsync_cmd += [f"{EXTRACT_DIR}/", f"root@{host}:/"]

    if not a.dry_run:
        log("DRY RUN first (always) - review before the real sync")
        preview = subprocess.run(
            rsync_cmd[:-1] + ["-n"] + rsync_cmd[-1:], capture_output=True, text=True
        )
        changed = [l for l in preview.stdout.splitlines() if l and not l.endswith("/")]
        log(
            f"dry run: {len(changed)} file(s) would change (see {LOG.relative_to(REPO)} for full list)"
        )
        LOG.open("a").write(preview.stdout)
        if not a.yes:
            resp = input(f"Proceed with the real sync onto root@{host} now? [y/N] ")
            if resp.strip().lower() != "y":
                log("aborted by user before the real sync")
                return 1

    log("running rsync" + (" (dry run)" if a.dry_run else ""))
    rc = subprocess.run(rsync_cmd).returncode
    if rc != 0:
        log(f"ABORT: rsync failed (rc={rc})")
        return rc
    log(
        "DONE - rootfs synced. Kernel/modules already fresh from ./dev.py publish-fedora."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
