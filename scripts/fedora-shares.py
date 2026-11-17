#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Idempotent storage mounts + NFS + SMB for woomera on Fedora (#372, docs/fedora-shares.md).

  ./dev.py fedora-shares plan                        # print the steps, touch nothing
  ./dev.py fedora-shares apply [--i-accept-wipe]     # converge (re-runnable)
  ./dev.py fedora-shares verify                      # mounts, exports, NFS/SMB put+get from here

- Mirror (WD pair) is never touched: only found by serial, fstab'd by its current UUID.
- Single drive (Seagate WCT3LCS1): wiped + mkfs.btrfs only when no `single-5t` fs exists AND
  --i-accept-wipe is given; refuses any other model/serial or a mounted disk.
- smb password: ~/.config/awto/secrets.yaml section `woomera-fedora-smb`, generated once, never printed.
"""

from __future__ import annotations

import argparse
import os
import secrets as pysecrets
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _box  # noqa: E402
from _net import LAN_SUBNET  # noqa: E402
from _repo import TMP, make_log  # noqa: E402

log = make_log("fedora-shares")

MIRROR_SERIALS = ("VGG8B0LG", "VGG8ASZG")  # WD82PURZ; the fs is found via the first
MIRROR_MNT = "/srv/mirror"
SINGLE_SERIAL, SINGLE_MODEL, SINGLE_LABEL = "WCT3LCS1", "ST6000VX001", "single-5t"
SINGLE_MNT = "/srv/single"
# nofail: a missing disk must not block boot; device-timeout 30 s: spin-up of a cold HDD is
# <20 s on this box, 30 s = 1.5x, then systemd gives up on that mount only.
MOUNT_OPTS = "noatime,space_cache=v2,nofail,x-systemd.device-timeout=30s"
USER, UID = "dan", 1000
LAN = LAN_SUBNET
SECRETS = Path(os.path.expanduser("~/.config/awto/secrets.yaml"))
SMB_SECTION = "woomera-fedora-smb"
EXPORTS_FILE = "/etc/exports.d/awto.exports"
SHARES = {"mirror": MIRROR_MNT, "single": SINGLE_MNT}  # SMB: data drives only
# NFS: path -> fsid. Explicit fsids keep file handles stable across fs re-creation order.
NFS_EXPORTS = {"/": 1, MIRROR_MNT: 2, SINGLE_MNT: 3}
NFS_OPTS = "rw,sync,no_subtree_check,root_squash"
DRY = False


def exports_text() -> str:
    lines = [
        "# managed by scripts/fedora-shares.py (docs/fedora-shares.md); no crossmnt on purpose",
    ]
    lines += [
        f"{p:<12} {LAN}({NFS_OPTS},fsid={fsid})" for p, fsid in NFS_EXPORTS.items()
    ]
    return "\n".join(lines) + "\n"


def smb_conf() -> str:
    g = f"""# managed by scripts/fedora-shares.py (docs/fedora-shares.md)
[global]
    workgroup = WORKGROUP
    server string = woomera
    server role = standalone server
    security = user
    passdb backend = tdbsam
    server min protocol = SMB3
    map to guest = Never
    restrict anonymous = 2
    hosts allow = 127. {LAN.rsplit(".", 2)[0]}.
    hosts deny = ALL
    disable netbios = yes
    smb ports = 445
    load printers = no
    printing = bsd
    printcap name = /dev/null
    disable spoolss = yes
"""
    for name, path in SHARES.items():
        g += f"""
[{name}]
    path = {path}
    valid users = {USER}
    read only = no
    browseable = yes
    create mask = 0664
    directory mask = 0775
"""
    return g


def load_secrets() -> dict:
    import yaml

    return yaml.safe_load(SECRETS.read_text()) or {}


def smb_password(create: bool) -> str | None:
    sec = load_secrets().get(SMB_SECTION) or {}
    if sec.get("password"):
        return sec["password"]
    if not create:
        return None
    import yaml

    pw = pysecrets.token_urlsafe(24)
    sec = {
        "url": "smb://woomera/{mirror,single} (LAN IP via ./dev.py ssh --print)",
        "purpose": "woomera Fedora Samba user dan (scripts/fedora-shares.py)",
        "username": USER,
        "password": pw,
    }
    text = SECRETS.read_text()
    if not text.endswith("\n"):
        text += "\n"
    SECRETS.write_text(
        text + "\n" + yaml.safe_dump({SMB_SECTION: sec}, sort_keys=False)
    )
    SECRETS.chmod(0o600)
    log(f"secrets: created section {SMB_SECTION}")
    return pw


class Box:
    def __init__(self) -> None:
        self.ip = _box.require()

    def sh(
        self, cmd: str, stdin: str | None = None, timeout: int = 120, check: bool = True
    ) -> subprocess.CompletedProcess:
        log(f"box$ {cmd.splitlines()[0][:150]}")
        if DRY:
            return subprocess.CompletedProcess(cmd, 0, "", "")
        r = subprocess.run(
            _box.ssh_argv(self.ip, cmd=["bash", "-c", _q(cmd)], batch=True),
            input=stdin,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        if check and r.returncode:
            raise RuntimeError(
                f"rc={r.returncode}: {cmd[:120]}\n{(r.stdout + r.stderr)[-400:]}"
            )
        return r

    def out(self, cmd: str, timeout: int = 60) -> str:
        return self.sh(cmd, timeout=timeout, check=False).stdout.strip()

    def put(self, path: str, text: str, mode: str = "644") -> None:
        cur = self.out(f"cat {path} 2>/dev/null")
        if cur == text.strip():
            log(f"  {path}: up to date")
            return
        self.sh(
            f"mkdir -p $(dirname {path}) && cat > {path} && chmod {mode} {path}",
            stdin=text,
        )
        log(f"  {path}: written")


def _q(s: str) -> str:
    import shlex

    return shlex.quote(s)


def by_id(b: Box, serial: str) -> str:
    p = b.out(f"ls /dev/disk/by-id/ata-*_{serial} 2>/dev/null | head -1")
    if not p:
        raise RuntimeError(f"no /dev/disk/by-id/ata-*_{serial}")
    return p


def fs_uuid(b: Box, dev: str) -> str:
    return b.out(f"blkid -s UUID -o value {dev}")


def nfsd_available(b: Box) -> bool:
    """nfsd registers in /proc/filesystems once built in or after modprobe nfsd."""
    if DRY:
        return True
    return (
        b.sh("grep -qw nfsd /proc/filesystems || modprobe nfsd", check=False).returncode
        == 0
    )


def ensure_fstab(b: Box, uuid: str, mnt: str, label: str) -> None:
    line = f"UUID={uuid}  {mnt}  btrfs  {MOUNT_OPTS}  0 0  # {label} (fedora-shares.py)"
    cur = b.out("cat /etc/fstab")
    keep = [ln for ln in cur.splitlines() if f" {mnt} " not in ln]
    new = "\n".join(keep + [line]) + "\n"
    if cur.strip() == new.strip():
        log(f"  fstab {mnt}: up to date")
        return
    b.sh("cp -n /etc/fstab /etc/fstab.pre-fedora-shares; cat > /etc/fstab", stdin=new)
    log(f"  fstab {mnt}: set to UUID={uuid}")


def ensure_mount(b: Box, mnt: str) -> None:
    b.sh(f"mkdir -p {mnt}; systemctl daemon-reload")
    if b.sh(f"findmnt -n {mnt}", check=False).returncode:
        b.sh(f"mount {mnt}")
    b.sh(f"findmnt -n {mnt}")


def unmount_old_test(b: Box) -> None:
    if b.sh("findmnt -n /mnt/btrfs-test", check=False).returncode == 0:
        b.sh("umount /mnt/btrfs-test")
        log("  unmounted /mnt/btrfs-test")


def make_single(b: Box, accept: bool) -> None:
    dev = by_id(b, SINGLE_SERIAL)
    if SINGLE_MODEL not in dev:
        raise RuntimeError(f"{dev}: not a {SINGLE_MODEL}")
    have = b.out(f"blkid -s LABEL -o value {dev}")
    if have == SINGLE_LABEL:
        log(f"  single: {dev} already btrfs {SINGLE_LABEL}")
        return
    log(b.out("lsblk -o NAME,SIZE,MODEL,SERIAL,FSTYPE,LABEL,MOUNTPOINTS"))
    if not accept:
        raise RuntimeError(
            f"single drive needs wipe: rerun with --i-accept-wipe ({dev})"
        )
    mounted = b.out(f"lsblk -nlo MOUNTPOINTS {dev} | grep -v '^$'")
    if mounted:
        raise RuntimeError(f"{dev} in use: {mounted}")
    b.sh(
        f"""set -e
D=$(readlink -f {dev})
[ "$(lsblk -ndo SERIAL $D)" = {SINGLE_SERIAL} ]
for p in $(lsblk -nlo PATH "$D" | tail -n +2) "$D"; do
  sz=$(blockdev --getsz "$p")
  wipefs -a "$p" >/dev/null
  dd if=/dev/zero of="$p" bs=1M count=4 oflag=direct status=none
  dd if=/dev/zero of="$p" bs=512 seek=$((sz-8192)) count=8192 oflag=direct status=none
done
partprobe "$D" || true
udevadm settle
mkfs.btrfs -f -L {SINGLE_LABEL} -d single -m dup --csum crc32c {dev}
""",
        timeout=300,
    )
    log(f"  single: {dev} wiped + mkfs.btrfs {SINGLE_LABEL}")


def cmd_apply(plan: bool, accept_wipe: bool) -> int:
    global DRY
    DRY = plan
    b = Box()
    log(f"== fedora-shares {'plan' if plan else 'apply'} on {b.ip}")
    if not plan and b.sh("rpm -q nfs-utils samba >/dev/null", check=False).returncode:
        raise RuntimeError(
            "dnf install nfs-utils samba samba-client first (box has no default route: ip route add default via 192.168.25.254 metric 500 + a nameserver, temporary)"
        )
    unmount_old_test(b)
    if not plan:
        make_single(b, accept_wipe)
    else:
        log(
            "  single: would wipe+mkfs only if no btrfs label single-5t (needs --i-accept-wipe)"
        )
    mdev = by_id(b, MIRROR_SERIALS[0]) if not plan else "<mirror>"
    for mnt, dev, label in (
        (MIRROR_MNT, mdev, "mirror RAID1"),
        (SINGLE_MNT, by_id(b, SINGLE_SERIAL) if not plan else "<single>", "single-5t"),
    ):
        uuid = fs_uuid(b, dev) if not plan else "<uuid>"
        ensure_fstab(b, uuid, mnt, label)
        ensure_mount(b, mnt)
        b.sh(f'[ "$(stat -c %u {mnt})" = {UID} ] || chown {UID}:{UID} {mnt}')
    # user dan: no login shell (box is root-ssh only); uid matches the LAN workstation
    b.sh(f"id {USER} >/dev/null 2>&1 || useradd -M -u {UID} -s /sbin/nologin {USER}")
    # NFS
    b.put(EXPORTS_FILE, exports_text())
    if nfsd_available(b):
        b.sh("systemctl enable --now nfs-server && exportfs -ra")
    else:
        log(
            "  NFS SKIPPED: kernel has no NFSD/SUNRPC (CONFIG_NFSD not set in the Fedora config); exports file staged only"
        )
        b.sh(
            "systemctl disable nfs-server >/dev/null 2>&1; systemctl reset-failed",
            check=False,
        )
    # SMB
    pw = smb_password(create=not plan) if not plan else "x"
    b.put("/etc/samba/smb.conf", smb_conf())
    b.sh("testparm -s >/dev/null 2>&1")
    b.sh(f"smbpasswd -s -a {USER}", stdin=f"{pw}\n{pw}\n")
    b.sh("smbpasswd -e dan; systemctl enable --now smb; systemctl restart smb")
    b.sh("systemctl is-active smb unvr-fand")
    log("== done")
    return 0


# ---- verify -------------------------------------------------------------------------
def sh(cmd: list[str], **kw) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, capture_output=True, text=True, check=False, **kw)


def cmd_verify() -> int:
    b = Box()
    fails: list[str] = []

    def check(name: str, ok: bool, detail: str = "") -> None:
        log(f"  {'PASS' if ok else 'FAIL'} {name} {detail}".rstrip())
        if not ok:
            fails.append(name)

    for mnt in (MIRROR_MNT, SINGLE_MNT):
        check(f"mounted {mnt}", b.sh(f"findmnt -n {mnt}", check=False).returncode == 0)
        check(f"fstab {mnt}", f" {mnt} " in b.out("cat /etc/fstab"))
    nfs = nfsd_available(b)
    if not nfs:
        log("  SKIP nfs checks: kernel has no NFSD")
    for unit in ("nfs-server", "smb", "unvr-fand") if nfs else ("smb", "unvr-fand"):
        check(
            f"{unit} active+enabled",
            b.out(f"systemctl is-active {unit}") == "active"
            and b.out(f"systemctl is-enabled {unit}") == "enabled",
        )
    ex = b.out("exportfs -v") if nfs else ""
    for p in NFS_EXPORTS if nfs else ():
        check(f"exported {p}", any(ln.startswith(p) for ln in ex.splitlines()))
    check("no crossmnt", "crossmnt" not in ex)
    log(
        "box MAC for traffic: "
        + b.out(f"ip -br link show; ip neigh | grep {LAN.rsplit('.', 2)[0]}")
    )
    mnt_root = TMP / "shares-mnt"
    me = os.getuid()
    marker = f"awto-verify-{pysecrets.token_hex(3)}.txt"
    for path, sub in (
        (("/", "/var/tmp"), (MIRROR_MNT, ""), (SINGLE_MNT, "")) if nfs else ()
    ):
        m = mnt_root / (path.strip("/").replace("/", "_") or "root")
        m.mkdir(parents=True, exist_ok=True)
        r = sh(
            ["sudo", "mount", "-t", "nfs4", "-o", "vers=4.2", f"{b.ip}:{path}", str(m)]
        )
        check(f"nfs mount {path}", r.returncode == 0, r.stderr.strip()[-120:])
        if r.returncode:
            continue
        try:
            f = m / sub.strip("/") / marker
            try:
                f.write_text("nfs-ok")
                check(f"nfs write+read {path}", f.read_text() == "nfs-ok")
                owner = b.out(f"stat -c %U {path.rstrip('/')}{sub}/{marker}")
                check(f"nfs file owner on box {path}", owner == USER, owner)
                f.unlink()
            except OSError as e:
                check(f"nfs write {path}", False, f"uid {me}: {e}")
        finally:
            sh(["sudo", "umount", str(m)])
    pw = smb_password(create=False)
    check("secrets section present", bool(pw))
    if pw:
        auth = TMP / "smb-auth"
        auth.write_text(f"username = {USER}\npassword = {pw}\n")
        auth.chmod(0o600)
        src, back = TMP / "smb-test.txt", TMP / "smb-back.txt"
        src.write_text("smb-ok")
        try:
            for name in SHARES:
                base = ["smbclient", f"//{b.ip}/{name}", "-A", str(auth), "-m", "SMB3"]
                r = sh(
                    base
                    + ["-c", f"put {src} {marker}; get {marker} {back}; del {marker}"]
                )
                check(
                    f"smb put/get {name}",
                    r.returncode == 0
                    and back.exists()
                    and back.read_text() == "smb-ok",
                    "" if r.returncode == 0 else r.stderr.strip()[-100:],
                )
                back.unlink(missing_ok=True)
            r = sh(["smbclient", f"//{b.ip}/mirror", "-N", "-c", "ls"])
            check("smb anonymous refused", r.returncode != 0)
        finally:
            for f in (auth, src, back):
                f.unlink(missing_ok=True)
    log("verify: " + ("OK" if not fails else f"FAILED {fails}"))
    return 1 if fails else 0


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter
    )
    ap.add_argument("cmd", choices=["plan", "apply", "verify"])
    ap.add_argument(
        "--i-accept-wipe",
        action="store_true",
        help=f"allow wiping {SINGLE_SERIAL} (only that serial)",
    )
    a = ap.parse_args()
    if a.cmd == "verify":
        return cmd_verify()
    return cmd_apply(a.cmd == "plan", a.i_accept_wipe)


if __name__ == "__main__":
    sys.exit(main())
