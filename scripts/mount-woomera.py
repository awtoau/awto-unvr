#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Mount woomera's NFS + SMB shares on this workstation (for Nautilus).

  ./dev.py mount-woomera up       # mount all, add Nautilus sidebar bookmarks
  ./dev.py mount-woomera down     # unmount all, remove the bookmarks
  ./dev.py mount-woomera status

- Mount points /mnt/woomera/<proto>-<share>; not persistent across a workstation reboot.
- Host resolved by MAC (_box.require); SMB password read from secrets.yaml section
  `woomera-fedora-smb`, written to a 0600 creds file in ./tmp, never logged.
- NFS: `/` (system SSD), mirror, single. SMB: mirror, single (data drives only).
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _box  # noqa: E402
from _repo import TMP, make_log  # noqa: E402

log = make_log("mount-woomera")

BASE = Path("/mnt/woomera")
NFS = {"nfs-system": "/", "nfs-mirror": "/srv/mirror", "nfs-single": "/srv/single"}
SMB = {"smb-mirror": "mirror", "smb-single": "single"}
SECRETS = Path(os.path.expanduser("~/.config/awto/secrets.yaml"))
SECTION = "woomera-fedora-smb"
CREDS = TMP / "woomera-smb-creds"
BOOKMARKS = Path.home() / ".config" / "gtk-3.0" / "bookmarks"
# One mount: LAN round trip, expected <1 s; 20 s = 20x backstop. Expiry: subprocess raises, named below.
MOUNT_TIMEOUT_S = 20


def sh(argv: list[str], check: bool = True) -> subprocess.CompletedProcess:
    log("$ " + " ".join(argv))
    r = subprocess.run(
        argv, capture_output=True, text=True, timeout=MOUNT_TIMEOUT_S, check=False
    )
    if r.stdout.strip():
        log(r.stdout.strip())
    if r.stderr.strip():
        log(r.stderr.strip())
    if check and r.returncode:
        raise SystemExit(f"{argv[0]} failed rc={r.returncode}")
    return r


def mounted(path: Path) -> bool:
    return subprocess.run(["mountpoint", "-q", str(path)]).returncode == 0


def smb_creds() -> Path:
    import yaml

    sec = (yaml.safe_load(SECRETS.read_text()) or {}).get(SECTION) or {}
    if not sec.get("password"):
        raise SystemExit(f"no [{SECTION}] password in secrets.yaml")
    CREDS.parent.mkdir(parents=True, exist_ok=True)
    CREDS.touch(mode=0o600)
    CREDS.chmod(0o600)
    CREDS.write_text(f"username={sec['username']}\npassword={sec['password']}\n")
    return CREDS


def bookmark_lines() -> list[str]:
    return [f"file://{BASE}/{n} woomera {n}" for n in {**NFS, **SMB}]


def set_bookmarks(add: bool) -> None:
    mine = set(bookmark_lines())
    cur = BOOKMARKS.read_text().splitlines() if BOOKMARKS.exists() else []
    keep = [x for x in cur if x not in mine and not x.startswith(f"file://{BASE}/")]
    BOOKMARKS.parent.mkdir(parents=True, exist_ok=True)
    BOOKMARKS.write_text("\n".join(keep + (sorted(mine) if add else [])) + "\n")


def up() -> int:
    ip = _box.require()
    uid, gid = os.getuid(), os.getgid()
    creds = smb_creds()
    for name, path in NFS.items():
        mp = BASE / name
        sh(["sudo", "mkdir", "-p", str(mp)])
        if not mounted(mp):
            sh(
                [
                    "sudo",
                    "mount",
                    "-t",
                    "nfs4",
                    "-o",
                    "vers=4.2",
                    f"{ip}:{path}",
                    str(mp),
                ]
            )
    for name, share in SMB.items():
        mp = BASE / name
        sh(["sudo", "mkdir", "-p", str(mp)])
        if not mounted(mp):
            sh(
                [
                    "sudo",
                    "mount",
                    "-t",
                    "cifs",
                    f"//{ip}/{share}",
                    str(mp),
                    "-o",
                    f"credentials={creds},uid={uid},gid={gid},vers=3.1.1",
                ]
            )
    set_bookmarks(True)
    return status()


def down() -> int:
    for name in [*NFS, *SMB]:
        mp = BASE / name
        if mounted(mp):
            sh(["sudo", "umount", str(mp)])
    set_bookmarks(False)
    CREDS.unlink(missing_ok=True)
    return status()


def status() -> int:
    bad = 0
    for name in [*NFS, *SMB]:
        ok = mounted(BASE / name)
        log(f"{'MOUNTED' if ok else 'not mounted':12} {BASE / name}")
        bad += not ok
    return 0 if not bad else 1


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("cmd", choices=["up", "down", "status"])
    a = ap.parse_args()
    return {"up": up, "down": down, "status": status}[a.cmd]()


if __name__ == "__main__":
    sys.exit(main())
