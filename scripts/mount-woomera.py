#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Mount woomera's NFS + SMB shares on this workstation (for Nautilus).

  ./dev.py mount-woomera install    # fstab block + systemd automount, survives reboot
  ./dev.py mount-woomera uninstall  # remove block, creds, automount units
  ./dev.py mount-woomera up         # mount now (manual, or trigger automounts if installed)
  ./dev.py mount-woomera down       # unmount all, remove the manual GTK bookmarks
  ./dev.py mount-woomera status

- Mount points /mnt/woomera/<proto>-<share>.
- install: marked block in /etc/fstab (backup in ./tmp, validated with findmnt --verify),
  /etc/samba/woomera.creds (root 0600) from secrets.yaml section `woomera-fedora-smb`
  (never logged). Host = registry address (_net.WOOMERA_DATA_IP). x-systemd.automount +
  mount-timeout 15: a powered-off box never blocks boot or Nautilus. x-gvfs-show lists them
  in Nautilus.
- up/down without install: host resolved by MAC (_box.require), creds file in ./tmp.
- NFS: `/` (system SSD), mirror, single. SMB: mirror, single (data drives only).
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _box  # noqa: E402
from _net import WOOMERA_DATA_IP  # noqa: E402
from _repo import TMP, make_log  # noqa: E402

log = make_log("mount-woomera")

BASE = Path("/mnt/woomera")
NFS = {"nfs-system": "/", "nfs-mirror": "/srv/mirror", "nfs-single": "/srv/single"}
SMB = {"smb-mirror": "mirror", "smb-single": "single"}
SECRETS = Path(os.path.expanduser("~/.config/awto/secrets.yaml"))
SECTION = "woomera-fedora-smb"
CREDS = TMP / "woomera-smb-creds"
BOOKMARKS = Path.home() / ".config" / "gtk-3.0" / "bookmarks"
FSTAB = Path("/etc/fstab")
SYS_CREDS = Path("/etc/samba/woomera.creds")
BEGIN, END = "# BEGIN awto-woomera", "# END awto-woomera"
# Automount: LAN mount normally <1 s; 15 s = 15x. Expiry: mount fails, boot/Nautilus continue.
OPTS_COMMON = "noauto,x-systemd.automount,x-systemd.idle-timeout=60,x-systemd.mount-timeout=15,_netdev"
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


def fstab_block() -> str:
    uid, gid = os.getuid(), os.getgid()
    ip = WOOMERA_DATA_IP
    rows = [BEGIN + " (managed by scripts/mount-woomera.py install)"]
    for name, path in NFS.items():
        # nosharecache: another mount of the same export under a different address
        # (e.g. the old .127) makes mount(2) fail EBUSY without it.
        o = f"{OPTS_COMMON},nosharecache,x-gvfs-show,x-gvfs-name=woomera-{name}"
        rows.append(f"{ip}:{path} {BASE / name} nfs4 {o} 0 0")
    for name, share in SMB.items():
        o = (
            f"{OPTS_COMMON},credentials={SYS_CREDS},uid={uid},gid={gid},vers=3.1.1,"
            f"x-gvfs-show,x-gvfs-name=woomera-{name}"
        )
        rows.append(f"//{ip}/{share} {BASE / name} cifs {o} 0 0")
    rows.append(END)
    return "\n".join(rows) + "\n"


def strip_block(text: str) -> str:
    out, skip = [], False
    for line in text.splitlines():
        if line.startswith(BEGIN):
            skip = True
        elif line.startswith(END):
            skip = False
        elif not skip:
            out.append(line)
    return "\n".join(out).rstrip("\n") + "\n"


def automount_units() -> list[str]:
    return [
        subprocess.run(
            ["systemd-escape", "--suffix=automount", "-p", str(BASE / n)],
            capture_output=True,
            text=True,
            check=True,
        ).stdout.strip()
        for n in [*NFS, *SMB]
    ]


def write_fstab(text: str) -> None:
    stamp = time.strftime("%Y%m%d-%H%M%S")
    TMP.mkdir(parents=True, exist_ok=True)
    (TMP / f"fstab.{stamp}.bak").write_text(FSTAB.read_text())
    new = TMP / "fstab.new"
    new.write_text(text)
    v = sh(["findmnt", "--verify", "--tab-file", str(new)], check=False)
    bad = [
        x
        for x in (v.stdout + v.stderr).splitlines()
        if "woomera" in x and "error" in x.lower()
    ]
    if bad:
        raise SystemExit("findmnt --verify rejected the block:\n" + "\n".join(bad))
    sh(
        [
            "sudo",
            "install",
            "-m",
            "0644",
            "-o",
            "root",
            "-g",
            "root",
            str(new),
            str(FSTAB),
        ]
    )
    new.unlink()


def install() -> int:
    down()  # old manual mounts + bookmarks first
    creds = smb_creds()
    sh(["sudo", "mkdir", "-p", str(SYS_CREDS.parent)])
    sh(
        [
            "sudo",
            "install",
            "-m",
            "0600",
            "-o",
            "root",
            "-g",
            "root",
            str(creds),
            str(SYS_CREDS),
        ]
    )
    creds.unlink(missing_ok=True)
    for name in [*NFS, *SMB]:
        sh(["sudo", "mkdir", "-p", str(BASE / name)])
    write_fstab(strip_block(FSTAB.read_text()) + "\n" + fstab_block())
    sh(["sudo", "systemctl", "daemon-reload"])
    sh(["sudo", "systemctl", "start", *automount_units()])
    return status()


def uninstall() -> int:
    units = automount_units()
    sh(["sudo", "systemctl", "stop", *units], check=False)
    for name in [*NFS, *SMB]:
        if mounted(BASE / name):
            sh(["sudo", "umount", str(BASE / name)], check=False)
    write_fstab(strip_block(FSTAB.read_text()))
    sh(["sudo", "rm", "-f", str(SYS_CREDS)])
    sh(["sudo", "systemctl", "daemon-reload"])
    return 0


def installed() -> bool:
    return BEGIN in FSTAB.read_text()


def up() -> int:
    if installed():
        for name in [*NFS, *SMB]:
            sh(["ls", str(BASE / name)], check=False)  # triggers the automount
        return status()
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
    ap.add_argument("cmd", choices=["install", "uninstall", "up", "down", "status"])
    a = ap.parse_args()
    return {
        "install": install,
        "uninstall": uninstall,
        "up": up,
        "down": down,
        "status": status,
    }[a.cmd]()


if __name__ == "__main__":
    sys.exit(main())
