#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Prepare the USB SSD as an awto-uboot `usb` boot target (#360), run on the box over ssh.

  ./dev.py usb-ssd inspect              # read-only: lsblk/blkid/sfdisk of every USB disk, guard verdict
  ./dev.py usb-ssd prepare              # plan only (what prepare --go would do)
  ./dev.py usb-ssd prepare --go         # wipe + partition + copy Fedora root + /boot/awto/usb/*
  ./dev.py usb-ssd verify               # read-only: layout, fsck, profile files, PARTUUID match
  ./dev.py usb-ssd rehearse             # same scripts on a loop device on THIS host (sudo)

- Guard (python AND re-checked inside the remote script): disk, tran=usb, matches an entry in
  scripts/usb-ssd-devices.json, not in never_serials, nothing mounted, under max_bytes.
- Kernel/DTB = what deploy-ssd put in the box's /boot (publish-fedora + deploy-ssd stay the only builders).
- Boot profile: awto-uboot reads usb <d>:2 /boot/awto/usb/{uEnv.txt,uImage,unvr.dtb,initrd}.
"""

from __future__ import annotations

import argparse
import base64
import json
import os
import re
import select
import shlex
import subprocess
import sys
import time
import uuid
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _bootsel as bs  # noqa: E402

import _box  # noqa: E402
from _repo import REPO, TMP, make_log  # noqa: E402

log = make_log("usb-ssd")

DEVICES = REPO / "scripts" / "usb-ssd-devices.json"
STAGE = TMP / "usb-ssd"
MNT = "/mnt/usb-ssd-root"
PROFILE = "usb"
LSBLK = (
    "lsblk -J -b -o NAME,PATH,TRAN,MODEL,VENDOR,SERIAL,SIZE,TYPE,RM,"
    "MOUNTPOINTS,PTTYPE,PARTUUID,FSTYPE,LABEL"
)
ESP_MIB = 200  # same as the SATA SSD's p1 (409600 sectors)
ESP_TYPE, LINUX_TYPE = (
    "C12A7328-F81F-11D2-BA4B-00A0C93EC93B",
    "0FC63DAF-8483-4772-8E79-3D69D8477DE4",
)
ROOT_LABEL = (
    "unvr-usb"  # the SATA root is LABEL=unvr-root: a second one would be ambiguous
)
MAX_WORDS = (
    62  # U-Boot `setenv bootargs ${bootargs_alt}`: SYS_MAXARGS 64 incl. setenv + name
)

# Quiet-time limits for a streamed remote step. rsync --info=progress2 prints every second and
# mkfs/e2fsck finish in seconds on 238 GB, so 60 s of silence = hung (global host-process rule).
STALL_S = 60
# Backstop: ~5 GB copy at a pessimistic 10 MB/s USB2 write = 500 s, x3 = 1500 s. Expiry = kill + fail.
BACKSTOP_S = 1500
# One short ssh exec (lsblk/blkid/cat): LAN ~1 s. 30 s = _bootsel.SSH_TIMEOUT_S.

# Excluded from the root copy: data stores and per-boot state, not the OS (USB SSD is 256 GB).
RSYNC_FILTERS = [
    "--include=/root/.ssh",
    "--include=/root/.ssh/**",
    "--exclude=/root/*",
    "--exclude=/srv/*",
    "--exclude=/mnt/*",
    "--exclude=/tmp/*",
    "--exclude=/var/tmp/*",
    "--exclude=/var/log/journal/*",
    "--exclude=/boot/awto/*",
    "--exclude=/lost+found",
]


def load_devices(path: Path = DEVICES) -> dict:
    return json.loads(path.read_text())


# ---------------------------------------------------------------- guard
def disks_of(lsblk: dict) -> list[dict]:
    return [d for d in lsblk.get("blockdevices", []) if d.get("type") == "disk"]


def _mounted(d: dict) -> list[str]:
    mps = [m for m in (d.get("mountpoints") or []) if m]
    for c in d.get("children") or []:
        mps += _mounted(c)
    return mps


def match_entry(d: dict, dev: dict) -> dict | None:
    """First allow-list entry matching model (+ serial pin); serial_required needs a pinned serial."""
    serial = (d.get("serial") or "").strip()
    ident = (
        f"{(d.get('vendor') or '').strip()} {(d.get('model') or '').strip()}".strip()
    )
    for e in dev["allow"]:
        if not re.search(e["model_re"], ident, re.I):
            continue
        if e.get("serial") is not None and e["serial"] != serial:
            continue
        if e.get("serial_required") and e.get("serial") is None:
            continue
        return e
    return None


def check_target(d: dict, dev: dict) -> list[str]:
    """Reasons `d` (one lsblk disk node) must NOT be touched; [] = allowed."""
    errs = []
    serial = (d.get("serial") or "").strip()
    ident = (
        f"{(d.get('vendor') or '').strip()} {(d.get('model') or '').strip()}".strip()
    )
    if d.get("type") != "disk":
        errs.append(f"type {d.get('type')!r} is not a whole disk")
    if d.get("tran") != "usb":
        errs.append(f"transport {d.get('tran')!r} is not usb")
    if serial and serial in dev.get("never_serials", []):
        errs.append(f"serial {serial} is on the never list")
    entry = match_entry(d, dev)
    if entry is None:
        errs.append(f"{ident!r} serial {serial!r} matches no allow-list entry")
    elif not 0 < int(d.get("size") or 0) <= entry["max_bytes"]:
        errs.append(f"size {d.get('size')} outside 0..{entry['max_bytes']}")
    if m := _mounted(d):
        errs.append(f"mounted: {', '.join(m)}")
    return errs


def pick_target(
    lsblk: dict, dev: dict, name: str | None = None
) -> tuple[dict | None, list[str]]:
    """The one allowed USB disk (or the named one if it passes). (disk, errors)."""
    ds = disks_of(lsblk)
    if name:
        ds = [d for d in ds if d["name"] == name]
        if not ds:
            return None, [f"no disk named {name}"]
        errs = check_target(ds[0], dev)
        return (None if errs else ds[0]), errs
    usb = [d for d in ds if d.get("tran") == "usb"]
    if not usb:
        return None, ["no USB-transport disk attached"]
    ok = [d for d in usb if not check_target(d, dev)]
    if len(ok) == 1:
        return ok[0], []
    if not ok:
        return None, [f"{d['name']}: " + "; ".join(check_target(d, dev)) for d in usb]
    return None, [
        f"{len(ok)} allowed USB disks ({', '.join(d['name'] for d in ok)}); pass --dev"
    ]


def part(dev: str, n: int) -> str:
    """/dev/sdb -> /dev/sdb1; /dev/loop3 -> /dev/loop3p1."""
    return f"{dev}p{n}" if dev[-1].isdigit() else f"{dev}{n}"


# ---------------------------------------------------------------- boot profile content
def usb_bootargs(cmdline: str, p2_partuuid: str) -> str:
    """Running cmdline with root= pointed at the USB p2; awto.profile=usb (as stage_profile does)."""
    words = [w for w in cmdline.split() if not w.startswith(("root=", "awto.profile="))]
    out = " ".join(words + [f"root=PARTUUID={p2_partuuid}"])
    if len((out + f" awto.profile={PROFILE}").split()) + 1 > MAX_WORDS:
        raise ValueError("bootargs: too many words for one setenv")
    return out


def uenv_text(bootargs: str) -> str:
    return f"bootargs_alt={bootargs} awto.profile={PROFILE}\n"


def empty_initrd() -> bytes:
    """newc archive holding only TRAILER!!! (same 512 B as tmp/bootsel/empty.cpio)."""
    name = b"TRAILER!!!\0"
    hdr = ("070701" + "00000000" * 11 + f"{len(name):08x}" + "00000000").encode()
    blob = hdr + name
    blob += b"\0" * (-len(blob) % 4)
    return blob + b"\0" * (512 - len(blob))


# ---------------------------------------------------------------- remote scripts (bash)
def _guard_sh(dev_path: str, e: dict, mode: str, loopfile: str = "") -> str:
    """Re-check on the box, immediately before touching it: device letters shift."""
    q = shlex.quote
    if mode == "loop":
        return f'[ "$(losetup -nO BACK-FILE {q(dev_path)})" = {q(loopfile)} ] || die "not our loop image"\n'
    return (
        f'[ "$(lsblk -dno TRAN {q(dev_path)})" = usb ] || die "not usb transport"\n'
        f'ID="$(lsblk -dno VENDOR,MODEL {q(dev_path)} | xargs)"\n'
        f'printf %s "$ID" | grep -Eiq {q(e["model_re"])} || die "model $ID not allowed"\n'
        f'SER="$(lsblk -dno SERIAL {q(dev_path)} | xargs)"\n'
        f'case " {" ".join(e["never"])} " in *" $SER "*) die "serial on never list";; esac\n'
        + (
            f'[ "$SER" = {q(e["serial"])} ] || die "serial $SER != expected"\n'
            if e.get("serial")
            else ""
        )
        + (
            '[ -n "$SER" ] || die "serial required but empty"\n'
            if e.get("serial_required")
            else ""
        )
        + f'[ "$(blockdev --getsize64 {q(dev_path)})" -le {int(e["max_bytes"])} ] || die "too big"\n'
        f'[ -z "$(lsblk -nro MOUNTPOINTS {q(dev_path)} | xargs)" ] || die "has mounted partitions"\n'
    )


PRE = 'set -euo pipefail\ndie() { echo "REFUSE: $*" >&2; exit 9; }\n'


def render_prepare(
    dev_path: str,
    e: dict,
    *,
    p2_uuid: str,
    uenv: str,
    initrd: bytes,
    src: str = "/",
    guard: str = "usb",
    loopfile: str = "",
) -> str:
    q = shlex.quote
    p1, p2 = part(dev_path, 1), part(dev_path, 2)
    sfdisk_in = (
        "label: gpt\n"
        f"size={ESP_MIB}MiB, type={ESP_TYPE}, name=ESP\n"
        f"type={LINUX_TYPE}, uuid={p2_uuid}, name=unvr-usb-root\n"
    )
    flt = " ".join(q(f) for f in RSYNC_FILTERS)
    root_line = f"PARTUUID={p2_uuid} / ext4 defaults,noatime 0 1"
    return (
        PRE
        + _guard_sh(dev_path, e, guard, loopfile)
        + f"""SRC={q(src)}
for f in boot/uImage boot/unvr.dtb etc/fstab; do [ -s "$SRC/$f" ] || die "missing $SRC/$f"; done
mkdir -p {MNT}; mountpoint -q {MNT} && die "{MNT} already mounted"
echo "== partition {dev_path}"
wipefs -a {q(dev_path)}
sfdisk --wipe always --no-reread {q(dev_path)} <<'EOF_SFDISK'
{sfdisk_in}EOF_SFDISK
partprobe {q(dev_path)}; udevadm settle --timeout=15
[ -b {p1} ] && [ -b {p2} ] || die "partition nodes missing"
[ "$(blkid -s PARTUUID -o value {p2})" = {p2_uuid} ] || die "p2 PARTUUID mismatch"
echo "== mkfs"
mkfs.vfat -F32 -n AWTO-ESP {p1}
mkfs.ext4 -q -F -L {ROOT_LABEL} {p2}
mount {p2} {MNT}
trap 'umount {MNT} 2>/dev/null || true' EXIT
echo "== copy root"
rsync -aHAXx --numeric-ids --info=progress2,stats1 {flt} "$SRC"/ {MNT}/
awk -v r={q(root_line)} '$2=="/"{{print r; next}} {{print}}' "$SRC/etc/fstab" > {MNT}/etc/fstab
echo "== boot profile {PROFILE}"
D={MNT}/boot/awto/{PROFILE}; rm -rf "$D.new"; mkdir -p "$D.new"
cp "$SRC/boot/uImage" "$SRC/boot/unvr.dtb" "$D.new/"
printf %s {q(base64.b64encode(uenv.encode()).decode())} | base64 -d > "$D.new/uEnv.txt"
printf %s {q(base64.b64encode(initrd).decode())} | base64 -d > "$D.new/initrd"
sync
for f in uImage unvr.dtb; do cmp "$SRC/boot/$f" "$D.new/$f"; done
rm -rf "$D"; mv "$D.new" "$D"; sync
umount {MNT}; trap - EXIT
echo "== done"
sfdisk -d {q(dev_path)}; blkid {p1} {p2}
"""
    )


def render_mount_ro(
    dev_path: str, e: dict, guard: str = "usb", loopfile: str = ""
) -> str:
    p2 = part(dev_path, 2)
    return (
        PRE
        + _guard_sh(dev_path, e, guard, loopfile)
        + f"""mkdir -p {MNT}; mountpoint -q {MNT} && die "{MNT} already mounted"
e2fsck -fn {p2}
mount -o ro {p2} {MNT}
echo "PARTUUID $(blkid -s PARTUUID -o value {p2})"
echo "SFDISK_BEGIN"; sfdisk -d {shlex.quote(dev_path)}; echo "SFDISK_END"
echo "UENV $(cat {MNT}/boot/awto/{PROFILE}/uEnv.txt)"
echo "FSTAB_BEGIN"; cat {MNT}/etc/fstab; echo "FSTAB_END"
cd {MNT}/boot/awto/{PROFILE} && sha256sum uEnv.txt uImage unvr.dtb initrd
[ -s {MNT}/etc/os-release ] && echo "OSREL $(. {MNT}/etc/os-release; echo $PRETTY_NAME)"
"""
    )


UMOUNT = f"umount {MNT}\n"


# ---------------------------------------------------------------- running remote scripts
def stream(argv: list[str], script: str) -> int:
    """Run `argv` with `script` on stdin; log every line; kill on STALL_S silence or BACKSTOP_S."""
    t0 = time.monotonic()
    p = subprocess.Popen(
        argv, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
    )
    p.stdin.write(script.encode())
    p.stdin.close()
    fd, buf, last = p.stdout.fileno(), b"", "(none)"
    while True:
        r, _, _ = select.select([fd], [], [], STALL_S)
        el = time.monotonic() - t0
        if not r or el > BACKSTOP_S:
            p.kill()
            why = f"silent {STALL_S} s" if not r else "backstop"
            log(f"TIMEOUT: {why}, elapsed {el:.0f} s, last line: {last}")
            return 124
        chunk = os.read(fd, 65536)
        if not chunk:
            return p.wait()
        buf += chunk
        *lines, buf = re.split(rb"[\r\n]+", buf)  # rsync progress2 uses \r
        for ln in lines:
            last = ln.decode(errors="replace")
            log("  " + last)


def remote_argv(ip: str) -> list[str]:
    return _box.ssh_argv(ip, cmd=["bash", "-s"], batch=True)


def ssh_out(ip: str, cmd: str) -> str:
    return bs.ssh(ip, cmd)


def fetch_lsblk(ip: str | None, json_file: str | None) -> dict:
    if json_file:
        return json.loads(Path(json_file).read_text())
    return json.loads(ssh_out(ip, LSBLK))


def allow_entry(d: dict, dev: dict) -> dict:
    """The matching allow entry plus the never list, shaped for _guard_sh."""
    e = match_entry(d, dev)
    if e is None:
        raise RuntimeError("no allow-list entry (check_target should have refused)")
    return {**e, "never": dev["never_serials"]}


# ---------------------------------------------------------------- commands
def cmd_inspect(a) -> int:
    dev = load_devices()
    ip = None if a.lsblk_json else _box.require()
    ls = fetch_lsblk(ip, a.lsblk_json)
    log("inspect: disks")
    for d in disks_of(ls):
        errs = check_target(d, dev)
        kids = ", ".join(
            f"{c['name']} {c.get('fstype') or '-'} {c.get('label') or '-'} {c.get('partuuid') or '-'}"
            for c in d.get("children") or []
        )
        log(
            f"  {d['name']:5} tran={d.get('tran')} {(d.get('vendor') or '').strip()} "
            f"{(d.get('model') or '').strip()} serial={(d.get('serial') or '').strip()} "
            f"size={d.get('size')} -> {'ALLOWED' if not errs else 'refused: ' + '; '.join(errs)}"
        )
        if kids:
            log(f"        parts: {kids}")
    tgt, errs = pick_target(ls, dev, a.dev)
    if not tgt:
        log("inspect: no usable target: " + " | ".join(errs))
        return 2
    if ip:
        path = tgt.get("path") or f"/dev/{tgt['name']}"
        for cmd in (f"sfdisk -d {path}", f"blkid {path}*"):
            log(f"  $ {cmd}")
            r = subprocess.run(
                _box.ssh_argv(ip, cmd=[cmd], batch=True),
                capture_output=True,
                text=True,
                timeout=bs.SSH_TIMEOUT_S,
                check=False,
            )
            for line in (r.stdout + r.stderr).splitlines():
                log("    " + line)
    log(f"inspect: target {tgt['name']} is allowed")
    return 0


def cmd_prepare(a) -> int:
    dev = load_devices()
    ip = _box.require()
    tgt, errs = pick_target(fetch_lsblk(ip, None), dev, a.dev)
    if not tgt:
        log("REFUSED: " + " | ".join(errs))
        return 2
    path = tgt.get("path") or f"/dev/{tgt['name']}"
    p2_uuid = str(uuid.uuid4())
    sata = ssh_out(ip, "lsblk -nro PARTUUID").lower()
    while p2_uuid in sata:  # pragma: no cover - uuid4 collision
        p2_uuid = str(uuid.uuid4())
    cmdline = ssh_out(ip, "cat /proc/cmdline").strip()
    uenv = uenv_text(usb_bootargs(cmdline, p2_uuid))
    log(
        f"prepare: target {path} ({(tgt.get('model') or '').strip()}, serial {(tgt.get('serial') or '').strip()}, {tgt['size']} B)"
    )
    log(
        f"  layout: p1 {ESP_MIB} MiB ESP | p2 ext4 {ROOT_LABEL} PARTUUID {p2_uuid} (rest)"
    )
    log(
        f"  root = copy of the running box root (excl. /srv /root /mnt /boot/awto); profile {PROFILE}: {uenv.strip()}"
    )
    # Fetched copies are checked on the host before anything is wiped.
    STAGE.mkdir(parents=True, exist_ok=True)
    for f, chk in (("uImage", bs.uimage_info), ("unvr.dtb", None)):
        subprocess.run(
            ["scp", "-q", *_box.SSH_OPTS_BATCH, f"root@{ip}:/boot/{f}", str(STAGE / f)],
            check=True,
            timeout=bs.SCP_TIMEOUT_S,
        )
        blob = (STAGE / f).read_bytes()
        if chk:
            info = chk(blob)
            log(f"  /boot/{f}: '{info['name']}' {len(blob)} B, CRC ok")
        elif blob[:4] != b"\xd0\x0d\xfe\xed":
            log("REFUSED: /boot/unvr.dtb is not an FDT")
            return 2
    script = render_prepare(
        path, allow_entry(tgt, dev), p2_uuid=p2_uuid, uenv=uenv, initrd=empty_initrd()
    )
    (STAGE / "prepare.sh").write_text(script)
    if not a.go:
        log(
            "prepare: plan only (script at tmp/usb-ssd/prepare.sh); pass --go to run it"
        )
        return 0
    log("prepare --go: running on the box")
    rc = stream(remote_argv(ip), script)
    log(f"prepare: rc={rc}")
    return rc


def parse_verify(out: str) -> dict:
    d = {"sha": {}}
    for m in re.finditer(r"^([0-9a-f]{64})  (\S+)$", out, re.M):
        d["sha"][m.group(2)] = m.group(1)
    for key in ("PARTUUID", "UENV", "OSREL"):
        m = re.search(rf"^{key} (.*)$", out, re.M)
        d[key] = m.group(1).strip() if m else ""
    for key in ("SFDISK", "FSTAB"):
        m = re.search(rf"^{key}_BEGIN\n(.*?)^{key}_END$", out, re.M | re.S)
        d[key] = m.group(1) if m else ""
    d["fsck_ok"] = bool(re.search(r"^\S+: \d+/\d+ files .*blocks$", out, re.M))
    return d


def check_verify(d: dict, ls_part_types: dict[int, str] | None = None) -> list[str]:
    """Problems with a parsed verify (empty = pass). Pure, unit-tested."""
    errs = []
    pu = d["PARTUUID"].lower()
    if not pu:
        errs.append("no p2 PARTUUID")
    if not d["fsck_ok"]:
        errs.append("e2fsck -fn did not finish clean")
    m = re.search(r"root=PARTUUID=(\S+)", d["UENV"])
    if not m or m.group(1).lower() != pu:
        errs.append(f"uEnv root={m.group(1) if m else None} != p2 PARTUUID {pu}")
    if f"awto.profile={PROFILE}" not in d["UENV"]:
        errs.append("uEnv lacks awto.profile=usb")
    roots = [ln.split() for ln in d["FSTAB"].splitlines() if ln.split()[1:2] == ["/"]]
    if len(roots) != 1 or roots[0][0].lower() != f"partuuid={pu}":
        errs.append(f"fstab root entry {roots} is not PARTUUID={pu}")
    if "unvr-root" in d["FSTAB"]:
        errs.append("fstab still mentions LABEL=unvr-root (would mount the SATA root)")
    for f in ("uEnv.txt", "uImage", "unvr.dtb", "initrd"):
        if f not in d["sha"]:
            errs.append(f"profile file {f} missing")
    sf = d["SFDISK"].upper()
    if ESP_TYPE not in sf or LINUX_TYPE not in sf:
        errs.append("partition types are not ESP + Linux")
    if not d["OSREL"]:
        errs.append("no /etc/os-release on p2")
    return errs


def verify_target(
    argv_run, tgt: dict, dev: dict, *, guard="usb", loopfile="", fetch=None
) -> int:
    """argv_run(script)->(rc,out). fetch(path)->bytes for the profile files while mounted."""
    path = tgt.get("path") or f"/dev/{tgt['name']}"
    e = allow_entry(tgt, dev) if guard == "usb" else {"never": []}
    rc, out = argv_run(render_mount_ro(path, e, guard, loopfile))
    if rc:
        log(f"verify: mount/fsck step failed rc={rc}:\n{out}")
        return 1
    try:
        d = parse_verify(out)
        errs = check_verify(d)
        if fetch:
            for f in ("uImage", "unvr.dtb"):
                blob = fetch(f"{MNT}/boot/awto/{PROFILE}/{f}")
                if bs.sha256(blob) != d["sha"].get(f):
                    errs.append(f"{f}: fetched sha256 != on-disk sha256")
                elif f == "uImage":
                    bs.uimage_info(blob)
                elif blob[:4] != b"\xd0\x0d\xfe\xed":
                    errs.append("unvr.dtb: bad FDT magic")
    except ValueError as ex:
        errs = [str(ex)]
    finally:
        argv_run(PRE + UMOUNT)
    for er in errs:
        log(f"verify FAIL: {er}")
    if not errs:
        log(f"verify OK: {d['OSREL']}, p2 PARTUUID {d['PARTUUID']}, uEnv: {d['UENV']}")
    return 1 if errs else 0


def cmd_verify(a) -> int:
    dev = load_devices()
    ip = _box.require()
    tgt, errs = pick_target(fetch_lsblk(ip, None), dev, a.dev)
    if not tgt:
        log("REFUSED: " + " | ".join(errs))
        return 2

    def run(script):
        r = subprocess.run(
            remote_argv(ip),
            input=script,
            capture_output=True,
            text=True,
            timeout=BACKSTOP_S,
            check=False,
        )
        return r.returncode, r.stdout + r.stderr

    def fetch(p):
        return subprocess.run(
            _box.ssh_argv(ip, cmd=[f"cat {shlex.quote(p)}"], batch=True),
            capture_output=True,
            timeout=bs.SCP_TIMEOUT_S,
            check=True,
        ).stdout

    return verify_target(run, tgt, dev, fetch=fetch)


# ---------------------------------------------------------------- rehearsal on a loop device
def fake_root(root: Path) -> None:
    """Minimal source tree: valid-looking boot files + fstab with the SATA label line."""
    import struct
    import zlib

    (root / "boot").mkdir(parents=True)
    (root / "etc").mkdir()
    data = b"\1" * 4096
    h = struct.pack(
        ">IIIIIIIBBBB32s",
        0x27051956,
        0,
        0,
        len(data),
        0x02000000 + 0x40,
        0x02000000 + 0x40,
        zlib.crc32(data),
        5,
        22,
        2,
        0,
        b"rehearsal",
    )
    (root / "boot/uImage").write_bytes(
        h[:4] + struct.pack(">I", zlib.crc32(h)) + h[8:] + data
    )
    (root / "boot/unvr.dtb").write_bytes(b"\xd0\x0d\xfe\xed" + b"\0" * 60)
    (root / "etc/fstab").write_text("LABEL=unvr-root  /  ext4  defaults,noatime  0 1\n")
    (root / "etc/os-release").write_text('PRETTY_NAME="rehearsal-root"\n')
    (root / "root/.ssh").mkdir(parents=True)
    (root / "root/.ssh/authorized_keys").write_text("k\n")
    (root / "root/junk").write_text("x")


def cmd_rehearse(a) -> int:
    dev = load_devices()
    STAGE.mkdir(parents=True, exist_ok=True)
    img, src = STAGE / "rehearse.img", STAGE / "rehearse-src"
    subprocess.run(["rm", "-rf", str(src), str(img)], check=True)
    fake_root(src)
    subprocess.run(["truncate", "-s", "4G", str(img)], check=True)
    loop = subprocess.run(
        ["sudo", "losetup", "-f", "--show", str(img)],
        capture_output=True,
        text=True,
        check=True,
    ).stdout.strip()
    log(f"rehearse: {loop} <- {img}")
    rc = 1
    try:
        # The real guard must refuse this device: tran is not usb.
        fake = {
            "name": Path(loop).name,
            "type": "disk",
            "tran": None,
            "model": "loop",
            "size": 4 << 30,
        }
        assert check_target(fake, dev), "guard accepted a loop device"
        log("rehearse: guard refuses the loop device as a real target: ok")
        p2_uuid = str(uuid.uuid4())
        cmdline = "console=ttyS0,115200 root=PARTUUID=dcdc291e-9956-48cd-9d7c-48219877881a rootfstype=ext4 rw rootwait"
        script = render_prepare(
            loop, {"never": []}, p2_uuid=p2_uuid,
            uenv=uenv_text(usb_bootargs(cmdline, p2_uuid)), initrd=empty_initrd(),
            src=str(src), guard="loop", loopfile=str(img),
        )  # fmt: skip
        sudo = ["sudo", "bash", "-s"]
        if rc := stream(sudo, script):
            return rc
        tgt = {"name": Path(loop).name, "path": loop}

        def run(s):
            r = subprocess.run(
                sudo,
                input=s,
                capture_output=True,
                text=True,
                timeout=BACKSTOP_S,
                check=False,
            )
            return r.returncode, r.stdout + r.stderr

        def fetch(p):
            return subprocess.run(
                ["sudo", "cat", p], capture_output=True, check=True
            ).stdout

        rc = verify_target(run, tgt, dev, guard="loop", loopfile=str(img), fetch=fetch)
    finally:
        subprocess.run(["sudo", "umount", MNT], capture_output=True, check=False)
        subprocess.run(["sudo", "losetup", "-d", loop], check=False)
        img.unlink(missing_ok=True)
    log(f"rehearse: {'PASS' if rc == 0 else 'FAIL'}")
    return rc


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name, fn in (
        ("inspect", cmd_inspect),
        ("prepare", cmd_prepare),
        ("verify", cmd_verify),
        ("rehearse", cmd_rehearse),
    ):
        s = sub.add_parser(name)
        s.set_defaults(fn=fn)
        if name != "rehearse":
            s.add_argument(
                "--dev", help="disk name (e.g. sdb); still must pass the guard"
            )
        if name == "inspect":
            s.add_argument(
                "--lsblk-json", help="canned lsblk -J output instead of the box"
            )
        if name == "prepare":
            s.add_argument("--go", action="store_true", help="actually wipe and write")
    a = ap.parse_args()
    return a.fn(a)


if __name__ == "__main__":
    sys.exit(main())
