#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Repartition woomera's boot SSD for Fedora + TrueNAS, no serial console needed.

- p2 (ext4 root, PARTUUID kept) shrinks to FS_TARGET (800 GiB) + margin; p3 = rest, ZFS type.
- ext4 cannot shrink mounted -> one boot of a maint uImage whose embedded initramfs
  runs scripts/initramfs/ssd-repartition-init. That /init restores the normal uImage first.

  ./dev.py ssd-repartition collect     # initramfs userland off the box (read-only ssh)
  ./dev.py ssd-repartition build       # cpio + maint kernel + gzip uImage
  ./dev.py ssd-repartition rehearse    # same /init + same aarch64 binaries, chroot, loop device
  ./dev.py ssd-repartition preflight   # read-only checks on the box
  ./dev.py ssd-repartition run --go    # install, reboot, wait, verify, clean up
  ./dev.py ssd-repartition verify --id YYYYMMDD-HHMMSS   # post-run checks only
"""

from __future__ import annotations

import argparse
import base64
import gzip
import hashlib
import json
import os
import random
import re
import shlex
import shutil
import stat
import struct
import subprocess
import sys
import time
import zlib
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _box  # noqa: E402
from _repo import REPO, TMP, kernel_build_out, kernel_build_ver, make_log  # noqa: E402

log = make_log("ssd-repartition")

STAGE = TMP / "ssd-repartition"
ROOTFS = STAGE / "root"  # collected aarch64 userland
CPIO = STAGE / "initramfs.cpio"
UIMAGE = STAGE / "uImage-maint-gz"
BUILD_JSON = STAGE / "build.json"
REHEARSAL_JSON = STAGE / "rehearsal.json"
INIT_SRC = REPO / "scripts" / "initramfs" / "ssd-repartition-init"

GiB = 1 << 30
SECTOR = 512
ALIGN = 2048  # 1 MiB in sectors; sfdisk's default grain for p3
MARGIN_SECTORS = (
    64 * (1 << 20) // SECTOR
)  # p2 = fs + >= 64 MiB: fs never exceeds partition
P3_TYPE = "6A898CC3-1DD2-11B2-99A6-080020736631"  # Solaris/ZFS, TrueNAS boot-pool
P3_NAME = "truenas-boot"

# awto-uboot bootcmd: ext4load uImage @0x02000000, dtb @0x04078000, gunzip to
# CONFIG_SYS_BOOTM_LEN. Overlap or overflow = bootm fails = box parked at U-Boot.
UIMAGE_LOAD, DTB_LOAD, BOOTM_LEN = 0x02000000, 0x04078000, 0x4000000
UIMAGE_MAX = DTB_LOAD - UIMAGE_LOAD - 0x10000
IMAGE_MAX = BOOTM_LEN - 0x100000

BINS = (
    "/usr/bin/bash",
    "/usr/sbin/e2fsck",
    "/usr/sbin/resize2fs",
    "/usr/sbin/dumpe2fs",
    "/usr/sbin/sfdisk",
    "/usr/sbin/blkid",
    "/usr/sbin/blockdev",
    "/usr/bin/mount",
    "/usr/bin/umount",
    "/usr/bin/dmesg",
    "/usr/bin/cp",
    "/usr/bin/mv",
    "/usr/bin/rm",
    "/usr/bin/ln",
    "/usr/bin/cmp",
    "/usr/bin/sync",
    "/usr/bin/cat",
    "/usr/bin/ls",
    "/usr/bin/head",
    "/usr/bin/grep",
    "/usr/bin/sleep",
    "/usr/bin/date",
    "/usr/bin/touch",
    "/usr/bin/mknod",
    "/usr/bin/mkdir",
    "/usr/bin/dd",
)


@dataclass(frozen=True)
class Geometry:
    """The layout /init is built for; anything else on the disk -> it changes nothing."""

    disk_sectors: int
    p2_start: int
    p2_sectors: int
    fs_target_kib: int
    partuuid: str = "dcdc291e-9956-48cd-9d7c-48219877881a"
    p2_type: str = "7C3457EF-0000-11AA-AA11-00306543ECAC"  # what the SSD has today
    model: str = "Samsung SSD 850"

    @property
    def p2_new_sectors(self) -> int:
        """fs + MARGIN, rounded up so p2 ends on a 1 MiB boundary (p3 aligned)."""
        end = self.p2_start + self.fs_target_kib * 2 + MARGIN_SECTORS
        return -(-end // ALIGN) * ALIGN - self.p2_start

    @property
    def fs_target_bytes(self) -> int:
        return self.fs_target_kib * 1024


# sfdisk -d of the SSD, 2026-11-13: p2 = 931.3 GiB, 359 GiB used.
REAL = Geometry(
    disk_sectors=1953525168,
    p2_start=409640,
    p2_sectors=1953115488,
    fs_target_kib=800 * (1 << 20),
)
# Same shape scaled to a 4 GiB image: p1 identical, p2 ends 7 sectors short of last-lba like the SSD.
_RD = 4 * GiB // SECTOR
REHEARSE = Geometry(
    disk_sectors=_RD,
    p2_start=409640,
    p2_sectors=(_RD - 34 - 7) - 409640 + 1,
    fs_target_kib=3 * (1 << 20),
)

# /init step deadlines (s). Each: expected worst, x1.5, expiry = feeder stops, SP805 resets.
TIMEOUTS = {
    # SSD probed by uptime 4.7-21.6 s (console logs). Expiry here = no restore = boot loop
    # needing the serial console, so this one is deliberately long.
    "T_DISK": 120,
    "T_RESTORE": 60,  # mount + 19 MB cp + cmp: ~2 s
    # e2fsck -fn on the live fs: 13 s (2026-11-13, warm); cold + repairs ~120 s
    "T_FSCK": 180,
    "T_RESIZE": 1800,  # 1.5x the 20 min estimate in RESIZE_ESTIMATE below
    "T_SFDISK": 60,
    "T_VERIFY": 180,
    "T_SAVE": 60,
    # No I/O and no CPU for this long = hung (libata EH worst ~2 min x1.5).
    "STALL_S": 180,
    "FEED_EVERY_S": 5,  # SP805 hw timeout is 17 s (LOAD_MAX at 500 MHz)
}


def render_conf(g: Geometry, mode: str, **over) -> str:
    v = {
        "MODE": mode,
        "SSD_MODEL": g.model,
        "DISK_OVERRIDE": "",
        "WDT_DEV": "/dev/watchdog",
        "DISK_SECTORS": g.disk_sectors,
        "P2_START": g.p2_start,
        "P2_SECTORS_ORIG": g.p2_sectors,
        "P2_NEW_SECTORS": g.p2_new_sectors,
        "P2_PARTUUID": g.partuuid,
        "P2_TYPE": g.p2_type,
        "FS_TARGET_KIB": g.fs_target_kib,
        "P3_TYPE": P3_TYPE,
        "P3_NAME": P3_NAME,
        "P3_ALIGN": ALIGN,
        **TIMEOUTS,
    }
    v.update(over)
    lines = ["# generated by scripts/ssd-repartition.py - do not edit"]
    lines += [f"{k}={shlex.quote(str(x))}" for k, x in v.items()]
    return "\n".join(lines) + "\n"


# --- sfdisk -d parsing (host side; /init has its own bash copy) ----------
_PART = re.compile(r"^/dev/\S*?(\d+) : (.*)$")


def parse_dump(text: str) -> dict:
    out: dict = {"parts": {}}
    for line in text.splitlines():
        if line.startswith("label-id:"):
            out["label_id"] = line.split(":", 1)[1].strip()
        m = _PART.match(line)
        if not m:
            continue
        f = {}
        for kv in m.group(2).split(","):
            k, _, val = kv.strip().partition("=")
            f[k] = val.strip().strip('"')
        for k in ("start", "size"):
            f[k] = int(f[k])
        out["parts"][int(m.group(1))] = f
    return out


def check_table(d: dict, g: Geometry, *, after: bool) -> list[str]:
    """Problems with a parsed dump: before (= original layout) or after the run."""
    errs = []
    p = d["parts"]
    if 2 not in p:
        return ["no p2"]
    p2 = p[2]
    if p2["start"] != g.p2_start:
        errs.append(f"p2 start {p2['start']} != {g.p2_start}")
    if p2.get("uuid", "").lower() != g.partuuid:
        errs.append(f"p2 PARTUUID {p2.get('uuid')} != {g.partuuid}")
    if p2.get("type", "").upper() != g.p2_type:
        errs.append(f"p2 type {p2.get('type')} != {g.p2_type}")
    want = g.p2_new_sectors if after else g.p2_sectors
    if p2["size"] != want:
        errs.append(f"p2 size {p2['size']} != {want}")
    if not after:
        if 3 in p:
            errs.append("p3 already exists")
        return errs
    p3 = p.get(3)
    if not p3:
        return errs + ["no p3"]
    if p3.get("type", "").upper() != P3_TYPE:
        errs.append(f"p3 type {p3.get('type')} != {P3_TYPE}")
    if p3.get("name") != P3_NAME:
        errs.append(f"p3 name {p3.get('name')!r} != {P3_NAME!r}")
    if p3["start"] < g.p2_start + g.p2_new_sectors or p3["start"] % ALIGN:
        errs.append(f"p3 start {p3['start']} overlaps p2 or is unaligned")
    if p3["start"] + p3["size"] > g.disk_sectors - 33:
        errs.append("p3 runs past the last usable LBA")
    return errs


def fs_bytes(dumpe2fs_h: str) -> int:
    bc = int(re.search(r"^Block count:\s+(\d+)", dumpe2fs_h, re.M).group(1))
    bs = int(re.search(r"^Block size:\s+(\d+)", dumpe2fs_h, re.M).group(1))
    return bc * bs


# awk over `dumpe2fs` group lines: used blocks/inodes in groups at/after the cut.
# Must stay one shell word for ssh; braces/quotes escaped for the remote bash.
def tail_usage_cmd(dev: str, cut_group: int) -> str:
    awk = (
        r"/^Group [0-9]+:/{g=$2+0} / free blocks, .* free inodes,/"
        r"{if(g>=CUT){n++; ub+=BPG-$1; ui+=IPG-$4}}"
        r" /^Blocks per group:/{BPG=$4} /^Inodes per group:/{IPG=$4}"
        r" END{print n+0, ub+0, ui+0}"
    ).replace("CUT", str(cut_group))
    return f"dumpe2fs {dev} 2>/dev/null | awk {shlex.quote(awk)}"


# --- newc cpio ----------------------------------------------------------
def _newc(name: str, mode: int, data: bytes = b"", rdev=(0, 0), ino=0) -> bytes:
    nb = name.encode() + b"\0"
    hdr = "070701" + "".join(
        f"{x:08x}"
        for x in (ino, mode, 0, 0, 1, 0, len(data), 0, 0, rdev[0], rdev[1], len(nb), 0)
    )
    out = hdr.encode() + nb
    out += b"\0" * (-len(out) % 4) + data
    return out + b"\0" * (-len(out) % 4)


def build_cpio(files: dict[str, tuple[int, bytes | str | tuple]]) -> bytes:
    """files: path -> (mode, payload). Dirs payload b"", symlinks target str,
    char devices (major, minor). Reproducible: uid/gid 0, mtime 0, sorted."""
    out = b""
    for ino, name in enumerate(sorted(files), start=1):
        mode, payload = files[name]
        if stat.S_ISLNK(mode):
            out += _newc(name, mode, payload.encode(), ino=ino)
        elif stat.S_ISCHR(mode):
            out += _newc(name, mode, rdev=payload, ino=ino)
        else:
            out += _newc(name, mode, payload, ino=ino)
    return out + _newc("TRAILER!!!", 0)


def read_cpio(blob: bytes) -> dict[str, tuple[int, bytes]]:
    out, i = {}, 0
    while True:
        h = blob[i : i + 110]
        f = [int(h[6 + 8 * k : 14 + 8 * k], 16) for k in range(13)]
        mode, size, nsz = f[1], f[6], f[11]
        name = blob[i + 110 : i + 110 + nsz - 1].decode()
        i += 110 + nsz
        i += -i % 4
        data = blob[i : i + size]
        i += size
        i += -i % 4
        if name == "TRAILER!!!":
            return out
        out[name] = (mode, data)


def initramfs_files(conf: str) -> dict:
    d, x, r = stat.S_IFDIR | 0o755, stat.S_IFREG | 0o755, stat.S_IFREG | 0o644
    files: dict = {
        p: (d, b"")
        for p in ("dev", "proc", "sys", "run", "tmp", "mnt", "mnt/p2", "etc", "root")
    }
    files["dev/console"] = (stat.S_IFCHR | 0o600, (5, 1))
    files["dev/null"] = (stat.S_IFCHR | 0o666, (1, 3))
    files["bin"] = (stat.S_IFLNK | 0o777, "usr/bin")
    files["sbin"] = (stat.S_IFLNK | 0o777, "usr/sbin")
    files["etc/mtab"] = (stat.S_IFLNK | 0o777, "/proc/self/mounts")
    files["init"] = (x, INIT_SRC.read_bytes())
    files["etc/ssdrep.conf"] = (r, conf.encode())
    for p in sorted(ROOTFS.rglob("*")):
        rel = p.relative_to(ROOTFS).as_posix()
        if p.is_dir():
            files[rel] = (d, b"")
        else:
            files[rel] = (x if os.access(p, os.X_OK) else r, p.read_bytes())
    return files


def sha256(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


# --- ssh helpers --------------------------------------------------------
# One-shot ssh to the box: handshake + exec measured ~1 s on the LAN; the
# slowest read here (full dumpe2fs + resize2fs -P) ~3 s. 30 s = backstop.
SSH_TIMEOUT_S = 30


def ssh(
    ip: str, cmd: str, timeout: float = SSH_TIMEOUT_S, check=True, stdin=None
) -> str:
    r = subprocess.run(
        _box.ssh_argv(ip, cmd=[cmd], batch=True),
        input=stdin,
        capture_output=True,
        text=True,
        timeout=timeout,
        check=False,
    )
    if check and r.returncode:
        raise RuntimeError(f"ssh rc={r.returncode}: {cmd}\n{r.stderr.strip()}")
    return r.stdout


# --- collect ------------------------------------------------------------
def cmd_collect(_a) -> int:
    ip = _box.require()
    log(f"collect: from {ip} (read-only)")
    out = ssh(ip, "ldd " + " ".join(BINS))
    paths = set(BINS)
    for line in out.splitlines():
        m = re.search(r"(?:=> )?(/\S+) \(0x", line)
        if m:
            paths.add(m.group(1))
    missing = [b for b in BINS if f"{b}:" not in out]
    if missing:
        log(f"FATAL: ldd did not report {missing}")
        return 1
    # No tar on the box: cpio -L (dereference) of ~15 MB over 1G, < 1 s; 60 s backstop.
    names = "\\n".join(p.lstrip("/") for p in sorted(paths))
    r = subprocess.run(
        _box.ssh_argv(
            ip,
            cmd=[f"cd / && printf '{names}\\n' | cpio -o -H newc -L --quiet"],
            batch=True,
        ),
        capture_output=True,
        timeout=60,
        check=False,
    )
    if r.returncode:
        log(f"FATAL: remote cpio rc={r.returncode}: {r.stderr.decode()[-300:]}")
        return 1
    if ROOTFS.exists():
        shutil.rmtree(ROOTFS)
    for name, (mode, data) in read_cpio(r.stdout).items():
        if not stat.S_ISREG(mode) or ".." in name.split("/"):
            log(f"FATAL: unexpected cpio member {name} mode {mode:o}")
            return 1
        dest = ROOTFS / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(data)
        dest.chmod(stat.S_IMODE(mode) & 0o755)
    pkgs = ssh(ip, "rpm -qf --qf '%{NAME}-%{VERSION}-%{RELEASE}\\n' " + " ".join(BINS))
    manifest = {
        "box_kernel": ssh(ip, "uname -r").strip(),
        "packages": sorted(set(pkgs.split())),
        "files": {
            p.relative_to(ROOTFS).as_posix(): sha256(p.read_bytes())
            for p in sorted(ROOTFS.rglob("*"))
            if p.is_file()
        },
    }
    (STAGE / "collect.json").write_text(json.dumps(manifest, indent=1))
    total = sum(p.stat().st_size for p in ROOTFS.rglob("*") if p.is_file())
    log(f"collect: {len(manifest['files'])} files, {total} B -> {ROOTFS}")
    log("collect: packages " + " ".join(manifest["packages"]))
    return 0


# --- build --------------------------------------------------------------
def uimage_check(blob: bytes) -> tuple[str, int]:
    """Header + CRCs of a gzip legacy uImage; returns (name, gunzipped size)."""
    if len(blob) > UIMAGE_MAX:
        raise ValueError(
            f"uImage {len(blob)} B > {UIMAGE_MAX} B (would overlap the DTB load)"
        )
    hdr = blob[:64]
    magic, hcrc, _t, size, load, _e, dcrc = struct.unpack(">IIIIIII", hdr[:28])
    if magic != 0x27051956:
        raise ValueError("bad uImage magic")
    if zlib.crc32(hdr[:4] + b"\0" * 4 + hdr[8:]) & 0xFFFFFFFF != hcrc:
        raise ValueError("bad header CRC")
    data = blob[64:]
    if len(data) != size or zlib.crc32(data) & 0xFFFFFFFF != dcrc:
        raise ValueError("bad data size/CRC")
    if hdr[31] != 1:
        raise ValueError("not gzip-compressed")
    raw = gzip.decompress(data)
    if len(raw) > IMAGE_MAX:
        raise ValueError(f"Image {len(raw)} B > {IMAGE_MAX} B (BOOTM_LEN - 1 MiB)")
    return hdr[32:64].rstrip(b"\0").decode(), len(raw)


def cmd_build(a) -> int:
    if not (STAGE / "collect.json").exists():
        log("FATAL: run `./dev.py ssd-repartition collect` first")
        return 1
    conf = render_conf(REAL, "real")
    blob = build_cpio(initramfs_files(conf))
    CPIO.write_bytes(blob)
    log(f"build: {CPIO} {len(blob)} B sha256 {sha256(blob)}")
    if a.cpio_only:
        return 0
    env = {
        k: v
        for k, v in os.environ.items()
        if k not in ("AWTO_PERF_BUILD", "AWTO_KASAN_BUILD")
    }
    env.update(
        AWTO_MAINT_BUILD="1", AWTO_INITRAMFS_SOURCE=str(CPIO), AWTO_VIA_DEVPY="1"
    )
    rc = subprocess.call(
        [sys.executable, str(REPO / "scripts/build-linux-fedora.py")], cwd=REPO, env=env
    )
    if rc:
        log(f"FATAL: maint kernel build rc={rc}")
        return rc
    out = kernel_build_out("maint")
    name = f"unvr-ea16-{kernel_build_ver()}-maint"
    rc = subprocess.call(
        [sys.executable, str(REPO / "scripts/mkuimage.py"), str(out / "Image")]
        + [str(UIMAGE), "--gzip", "--name", name],
        cwd=REPO,
        env=env,
    )
    if rc:
        return rc
    ub = UIMAGE.read_bytes()
    try:
        nm, raw = uimage_check(ub)
    except ValueError as e:
        log(f"FATAL: {e}")
        return 1
    # The kernel's own copy of the initramfs must be byte-identical to the rehearsed cpio.
    import lzma

    inc = lzma.decompress((out / "kbuild/usr/initramfs_inc_data").read_bytes())
    if sha256(inc) != sha256(blob):
        log("FATAL: embedded initramfs != the cpio just built")
        return 1
    kver = (out / "kbuild/include/config/kernel.release").read_text().strip()
    info = {
        "kver": kver,
        "uimage": str(UIMAGE),
        "uimage_sha256": sha256(ub),
        "uimage_bytes": len(ub),
        "image_bytes": raw,
        "cpio_sha256": sha256(blob),
        "init_sha256": sha256(INIT_SRC.read_bytes()),
        "conf_sha256": sha256(conf.encode()),
    }
    BUILD_JSON.write_text(json.dumps(info, indent=1))
    log(
        f"build: {nm} kver {kver}: uImage {len(ub)} B (max {UIMAGE_MAX}), Image {raw} B (max {IMAGE_MAX})"
    )
    return 0


# --- rehearse -----------------------------------------------------------
def sudo(*cmd, check=True, timeout=120, **kw) -> subprocess.CompletedProcess:
    """Privileged host step. 120 s: every caller is a mount/losetup/mkfs on a 4 GiB image."""
    r = subprocess.run(
        ["sudo", "-n", *map(str, cmd)],
        capture_output=True,
        text=True,
        timeout=timeout,
        check=False,
        **kw,
    )
    if check and r.returncode:
        raise RuntimeError(
            f"sudo {' '.join(map(str, cmd))} rc={r.returncode}: {r.stderr.strip()}"
        )
    return r


def _mounts_under(root: Path) -> list[str]:
    pts = []
    for line in Path("/proc/self/mountinfo").read_text().splitlines():
        mp = line.split()[4].replace("\\040", " ")
        if mp == str(root) or mp.startswith(str(root) + "/"):
            pts.append(mp)
    return sorted(pts, key=len, reverse=True)


@dataclass
class Scenario:
    name: str
    data_bytes: int
    over: dict
    expect_ok: bool
    expect_log: tuple[str, ...] = ()
    expect_fed: bool = True
    resized: bool | None = None  # fs at target afterwards; default = expect_ok


SCENARIOS = {
    # 2.2 GiB in 3.8 GiB, target 3 GiB: forces block + inode moves out of the cut groups.
    "ok": Scenario("ok", 2200 << 20, {}, True),
    # Wrong original size in the conf: restore must still happen, nothing else may change.
    "abort": Scenario(
        "abort",
        300 << 20,
        {"P2_SECTORS_ORIG": REHEARSE.p2_sectors - 2048},
        False,
        ("RESULT: FAIL geometry", "restored=1"),
    ),
    # Negative limits trip on the feeder's first pass, before its first feed: on
    # the box that is an SP805 reset; here the run carries on unfed.
    "wdt-deadline": Scenario(
        "wdt-deadline",
        300 << 20,
        {"T_DISK": -5},
        True,
        ("passed its deadline - stop feeding",),
        expect_fed=False,
    ),
    "wdt-stall": Scenario(
        "wdt-stall",
        300 << 20,
        {"STALL_S": -1},
        True,
        ("no progress for -1s in '",),
        expect_fed=False,
    ),
    # p3 append fails after the fs shrank and p2 was cut: repair e2fsck must run,
    # p2/fs stay at target, data intact, normal uImage restored.
    "late-fail": Scenario(
        "late-fail",
        300 << 20,
        {"P3_TYPE": "not-a-guid"},
        False,
        ("RESULT: FAIL sfdisk --append", "== step repair", "restored=1"),
        resized=True,
    ),
}


def _populate(mnt: Path, nbytes: int, seed: int) -> dict[str, str]:
    """Deterministic tree: many top-level dirs (ext4 Orlov spreads them over all
    groups) of 16 KiB-8 MiB files. Returns path -> sha256."""
    rng = random.Random(seed)
    hashes, total, i = {}, 0, 0
    (mnt / "boot").mkdir()
    (mnt / "root").mkdir()
    for name, size in (("boot/uImage.normal", 19 << 20), ("boot/uImage", 23 << 20)):
        b = rng.randbytes(size)
        (mnt / name).write_bytes(b)
        hashes[name] = sha256(b)
    (mnt / "boot/ssd-repartition.id").write_text(
        f"ID=20261113-000000\nEPOCH={int(time.time())}\n"
    )
    while total < nbytes:
        d = mnt / f"d{i % 400:03d}"
        d.mkdir(exist_ok=True)
        size = min(rng.randint(16 << 10, 8 << 20), nbytes - total)
        b = rng.randbytes(size)
        rel = f"{d.name}/f{i:05d}"
        (mnt / rel).write_bytes(b)
        hashes[rel] = sha256(b)
        total += size
        i += 1
    return hashes


def rehearse_one(sc: Scenario, cpio: bytes) -> dict:
    g = REHEARSE
    work = STAGE / "rehearse" / sc.name
    if work.exists():
        for mp in _mounts_under(work):
            sudo("umount", mp)
        sudo("rm", "-rf", work)
    work.mkdir(parents=True)
    img, root, dmnt = work / "disk.img", work / "chroot", work / "data"
    with open(img, "wb") as f:
        f.truncate(g.disk_sectors * SECTOR)
    table = (
        "label: gpt\nunit: sectors\nfirst-lba: 34\n"
        f'start=40, size=409600, type=C12A7328-F81F-11D2-BA4B-00A0C93EC93B, name="EFI System Partition"\n'
        f"start={g.p2_start}, size={g.p2_sectors}, type={g.p2_type}, uuid={g.partuuid}\n"
    )
    subprocess.run(["sfdisk", "-q", str(img)], input=table, text=True, check=True)
    loop = sudo("losetup", "-fP", "--show", img).stdout.strip()
    res: dict = {"scenario": sc.name, "loop": loop}
    try:
        p2 = f"{loop}p2"
        # -G 4: real fs is flex 16 over 7451 groups; at 31 groups flex 16 would keep
        # every inode below the cut and leave the inode-move path untested.
        sudo(
            "mkfs.ext4",
            "-q",
            "-L",
            "unvr-root",
            "-b",
            "4096",
            "-i",
            "16384",
            "-G",
            "4",
            p2,
        )
        dmnt.mkdir()
        sudo("mount", p2, dmnt)
        sudo("chown", f"{os.getuid()}:{os.getgid()}", dmnt)
        t = time.monotonic()
        hashes = _populate(dmnt, sc.data_bytes, seed=353)
        res["populate_s"] = round(time.monotonic() - t, 1)
        sudo("umount", dmnt)
        pre = parse_dump(sudo("sfdisk", "-d", loop).stdout)
        res["fs_bytes_before"] = fs_bytes(sudo("dumpe2fs", "-h", p2).stdout)
        cut = g.fs_target_bytes // 4096 // 32768
        n, ub, ui = map(int, sudo("sh", "-c", tail_usage_cmd(p2, cut)).stdout.split())
        res["tail"] = {"groups": n, "used_blocks": ub, "used_inodes": ui}
        log(
            f"rehearse[{sc.name}]: {len(hashes)} files; groups >= {cut}: {ub} used blocks, {ui} used inodes"
        )
        if sc.name == "ok" and (ub == 0 or ui == 0):
            raise RuntimeError("rehearsal data does not reach the cut groups")

        # chroot = the real cpio + the rehearsal conf; /dev holds only the loop disk.
        root.mkdir()
        (work / "initramfs.cpio").write_bytes(cpio)
        sudo(
            "sh",
            "-c",
            f"cd {shlex.quote(str(root))} && cpio -idm --quiet < ../initramfs.cpio",
        )
        conf = render_conf(
            g, "rehearse", DISK_OVERRIDE=loop, WDT_DEV="/run/wdt-fake", **sc.over
        )
        (work / "ssdrep.conf").write_text(conf)
        sudo("cp", work / "ssdrep.conf", root / "etc/ssdrep.conf")
        sudo("mount", "-t", "proc", "proc", root / "proc")
        sudo("mount", "-t", "sysfs", "sysfs", root / "sys")
        sudo("mount", "-t", "tmpfs", "-o", "mode=0755", "tmpfs", root / "dev")
        mm = (
            Path(f"/sys/class/block/{Path(loop).name}/dev")
            .read_text()
            .strip()
            .split(":")
        )
        sudo("mknod", root / "dev" / Path(loop).name, "b", *mm)
        sudo("mknod", root / "dev/null", "c", "1", "3")
        t = time.monotonic()
        run = sudo(
            "env",
            "-i",
            shutil.which("chroot"),
            root,
            "/init",
            check=False,
            timeout=REHEARSE_TIMEOUT_S,
        )
        res["init_s"] = round(time.monotonic() - t, 1)
        res["init_rc"] = run.returncode
        boxlog = (
            (root / "run/ssdrep.log").read_text()
            if (root / "run/ssdrep.log").exists()
            else ""
        )
        wdt = root / "run/wdt-fake"
        res["wdt_feeds"] = wdt.stat().st_size if wdt.exists() else 0
        (work / "init.stderr").write_text(run.stderr)
        (work / "ssdrep.log").write_text(boxlog)
        for mp in _mounts_under(root):
            sudo("umount", mp)

        post = parse_dump(sudo("sfdisk", "-d", loop).stdout)
        errs = []
        resized = sc.expect_ok if sc.resized is None else sc.resized
        if sc.expect_ok:
            errs += check_table(post, g, after=True)
        elif resized:
            if 3 in post["parts"] or post["parts"][2]["size"] != g.p2_new_sectors:
                errs.append("late failure: want p2 at new size and no p3")
        elif post != pre:
            errs.append("table changed on an aborted run")
        if post["parts"][1] != pre["parts"][1] or post.get("label_id") != pre.get(
            "label_id"
        ):
            errs.append("p1 or label-id changed")
        res["p2_sectors_after"] = post["parts"][2]["size"]
        res["p3"] = post["parts"].get(3)
        fsb = fs_bytes(sudo("dumpe2fs", "-h", p2).stdout)
        res["fs_bytes_after"] = fsb
        want_fs = g.fs_target_bytes if resized else res["fs_bytes_before"]
        if fsb != want_fs:
            errs.append(f"fs {fsb} B != {want_fs} B")
        if fsb > post["parts"][2]["size"] * SECTOR:
            errs.append("fs larger than p2")
        fck = sudo("e2fsck", "-f", "-n", p2, check=False)
        res["e2fsck_n_rc"] = fck.returncode
        if fck.returncode:
            errs.append(f"host e2fsck -fn rc={fck.returncode}")
        sudo("mount", "-o", "ro", p2, dmnt)
        bad = [
            p
            for p, h in hashes.items()
            if p != "boot/uImage" and sha256((dmnt / p).read_bytes()) != h
        ]
        if sha256((dmnt / "boot/uImage").read_bytes()) != hashes["boot/uImage.normal"]:
            bad.append("boot/uImage != uImage.normal (restore)")
        saved = dmnt / "root/ssd-repartition-20261113-000000.log"
        res["saved_log"] = saved.exists()
        saved_txt = saved.read_text() if saved.exists() else ""
        sudo("umount", dmnt)
        res["files_checked"] = len(hashes)
        res["files_bad"] = bad
        errs += [f"hash mismatch: {p}" for p in bad]
        want_line = "RESULT: OK" if sc.expect_ok else "RESULT: FAIL"
        if want_line not in saved_txt:
            errs.append(f"saved log lacks {want_line!r}")
        for s in sc.expect_log:
            if s not in boxlog:
                errs.append(f"log lacks {s!r}")
        if sc.name == "ok" and "stop feeding" in boxlog:
            errs.append("feeder stopped on the ok run")
        if bool(res["wdt_feeds"]) != sc.expect_fed:
            errs.append(
                f"watchdog fed {res['wdt_feeds']} times, expected fed={sc.expect_fed}"
            )
        if (run.returncode == 0) != sc.expect_ok:
            errs.append(f"init rc={run.returncode}")
        res["errors"] = errs
        res["pass"] = not errs
        return res
    finally:
        for mp in _mounts_under(work):
            sudo("umount", mp, check=False)
        sudo("losetup", "-d", loop, check=False)


# chroot /init on the 4 GiB image: ok scenario ~16 s (rehearsal.json init_s); ~3.8x.
# Expiry: TimeoutExpired propagates, cleanup unmounts + detaches the loop.
REHEARSE_TIMEOUT_S = 60


def cmd_rehearse(a) -> int:
    if not CPIO.exists():
        log("FATAL: no cpio - `./dev.py ssd-repartition build --cpio-only` first")
        return 1
    if sudo("true", check=False).returncode:
        log("FATAL: rehearsal needs `sudo -n` (losetup, mount, chroot)")
        return 1
    sc_r = subprocess.run(
        ["uvx", "--from", "shellcheck-py", "shellcheck", "-s", "bash", str(INIT_SRC)],
        capture_output=True,
        text=True,
        timeout=300,  # first run downloads shellcheck-py
        check=False,
    )
    log(f"rehearse: shellcheck rc={sc_r.returncode} {sc_r.stdout.strip()[:500]}")
    cpio = CPIO.read_bytes()
    members = read_cpio(cpio)
    if members["init"][1] != INIT_SRC.read_bytes():
        log("FATAL: cpio /init differs from the repo - rebuild")
        return 1
    if "MODE=real" not in members["etc/ssdrep.conf"][1].decode():
        log("FATAL: cpio conf is not MODE=real")
        return 1
    results = {}
    for name in a.scenario or list(SCENARIOS):
        log(f"rehearse: scenario {name}")
        r = rehearse_one(SCENARIOS[name], cpio)
        results[name] = r
        log(f"rehearse[{name}]: {'PASS' if r['pass'] else 'FAIL'} {json.dumps(r)}")
    ok = all(r["pass"] for r in results.values()) and sc_r.returncode == 0
    REHEARSAL_JSON.write_text(
        json.dumps(
            {
                "pass": ok,
                "cpio_sha256": sha256(cpio),
                "init_sha256": sha256(INIT_SRC.read_bytes()),
                "shellcheck_rc": sc_r.returncode,
                "scenarios": results,
            },
            indent=1,
        )
    )
    log(f"rehearse: {'PASS' if ok else 'FAIL'} -> {REHEARSAL_JSON}")
    return 0 if ok else 1


# --- preflight / run / verify ------------------------------------------
# Resize estimate (2026-11-13 dumpe2fs): 125.7 GiB used in groups past 800 GiB,
# ~2 GiB of it their own inode tables. Copy = read 557 MB/s (dd direct, measured)
# + write ~300 MB/s (850 EVO sustained, assumed) serial -> ~11 min; + inode
# relocation of ~1.02 M inodes + dir rewrite ~5 min -> 20 min expected worst.
READ_BPS, WRITE_BPS = 557e6, 300e6
RESIZE_FIXED_S = 300


def resize_estimate_s(tail_used_bytes: int) -> int:
    return int(tail_used_bytes * (1 / READ_BPS + 1 / WRITE_BPS)) + RESIZE_FIXED_S


# Runner limit: reboot -> maint /init ~60 s, disk 30, fsck 120, resize 1200,
# fsck -n 120, misc 30, reset 17, normal boot -> sshd 80 (#352) = 1657 s;
# x1.5 = 2486 s. The /init ceilings sum to 2460 s + 157 s of boots, so 2700 s
# covers both: past it, init can no longer be running under its own watchdog.
RUN_LIMIT_S = 2700
POLL_S = 20


def init_probe(ip: str) -> tuple[list[str], str]:
    """Source the real /init's functions on the live box and run its read-only
    checks (find_disk, identity, geometry) against the real SSD. Private mount
    namespace + tmpfs /run: nothing persists, nothing touches the disk."""
    conf = base64.b64encode(render_conf(REAL, "real").encode()).decode()
    init = base64.b64encode(INIT_SRC.read_bytes()).decode()
    script = f"""set -u
mount -t tmpfs tmpfs /run || exit 90
echo {conf} | base64 -d >/run/c.conf
echo {init} | base64 -d >/run/init
SSDREP_CONF=/run/c.conf SSDREP_LIB=1 . /run/init
MODE=probe
LOG=/run/probe.log
find_disk && echo PROBE find_disk ok
identity_ok && echo PROBE identity ok P2=$P2
geometry_ok; echo PROBE geometry rc=$?
echo PROBE fs_bytes=$(fs_bytes)
cat /run/probe.log
"""
    out = ssh(ip, "unshare -m --propagation private bash -s", stdin=script)
    errs = []
    for want in ("PROBE find_disk ok", "PROBE identity ok P2=/dev/", "is mounted"):
        if want not in out:
            errs.append(f"init probe: no {want!r}")
    # On the live box geometry_ok must fail ONLY because p2 is mounted.
    bad = [
        ln
        for ln in out.splitlines()
        if re.search(r"!=|neither|already exists|start .* !=", ln)
    ]
    errs += [f"init probe: {ln}" for ln in bad]
    if f"PROBE fs_bytes={REAL.disk_sectors}" in out:
        errs.append("init probe: fs_bytes nonsense")
    return errs, out


def preflight(ip: str) -> dict:
    f: dict = {"errors": []}
    e = f["errors"]
    f["kver"] = ssh(ip, "uname -r").strip()
    if "-perf-" not in f["kver"]:
        e.append(f"box kernel {f['kver']} is not a perf build")
    disks = []
    for line in ssh(
        ip,
        "for b in /sys/block/sd*; do echo ${b##*/} $(cat $b/size) $(cat $b/device/model); done",
    ).splitlines():
        dn, size, model = line.split(" ", 2)
        if model.startswith(REAL.model):
            disks.append((dn, int(size)))
    if len(disks) != 1:
        e.append(f"want one {REAL.model} disk, found {disks}")
        return f
    dn, size = disks[0]
    f["disk"] = f"/dev/{dn}"
    if size != REAL.disk_sectors:
        e.append(f"{dn} is {size} sectors, want {REAL.disk_sectors}")
    d = parse_dump(ssh(ip, f"sfdisk -d /dev/{dn}"))
    f["table"] = d
    e += check_table(d, REAL, after=False)
    p2 = f"/dev/{dn}2"
    root_src = ssh(ip, "findmnt -n -o SOURCE /").strip()
    if root_src != p2:
        e.append(f"/ is {root_src}, want {p2}")
    used = int(ssh(ip, "df -B1 --output=used / | tail -n1").strip())
    f["used_bytes"] = used
    if used >= 780 * GiB:
        e.append(f"/ uses {used / GiB:.1f} GiB >= 780 GiB")
    h = ssh(ip, f"dumpe2fs -h {p2} 2>/dev/null")
    f["fs_bytes"] = fs_bytes(h)
    if "Block size:               4096" not in h:
        e.append("fs block size is not 4096")
    if f["fs_bytes"] <= REAL.fs_target_bytes:
        e.append("fs is already <= target")
    m = re.search(
        r"minimum size of the filesystem: (\d+)", ssh(ip, f"resize2fs -P {p2} 2>&1")
    )
    f["min_bytes"] = int(m.group(1)) * 4096 if m else None
    if not m or f["min_bytes"] >= REAL.fs_target_bytes:
        e.append(f"resize2fs -P minimum {f['min_bytes']} >= target")
    cut = REAL.fs_target_bytes // 4096 // 32768
    n, ub, ui = map(int, ssh(ip, tail_usage_cmd(p2, cut)).split())
    f["tail"] = {"groups": n, "used_bytes": ub * 4096, "used_inodes": ui}
    f["resize_estimate_s"] = resize_estimate_s(ub * 4096)
    if f["resize_estimate_s"] > TIMEOUTS["T_RESIZE"] / 1.5:
        e.append(
            f"resize estimate {f['resize_estimate_s']} s exceeds the T_RESIZE budget"
        )
    boot = ssh(
        ip,
        "for p in /boot/uImage /boot/unvr.dtb /boot/uImage.normal /boot/ssd-repartition.id; do test -e $p && echo $p; done; true",
    )
    present = set(boot.split())
    for p in ("/boot/uImage", "/boot/unvr.dtb"):
        if p not in present:
            e.append(f"{p} missing")
    for p in ("/boot/uImage.normal", "/boot/ssd-repartition.id"):
        if p in present:
            e.append(f"{p} already exists (stale run?)")
    perrs, f["init_probe"] = init_probe(ip)
    e += perrs
    m = re.search(r"PROBE fs_bytes=(\d+)", f["init_probe"])
    if not m or int(m.group(1)) != f["fs_bytes"]:
        e.append("init probe fs_bytes disagrees with dumpe2fs")
    return f


def local_checks() -> list[str]:
    e = []
    if not BUILD_JSON.exists() or not REHEARSAL_JSON.exists():
        return ["build and rehearse first"]
    b, r = json.loads(BUILD_JSON.read_text()), json.loads(REHEARSAL_JSON.read_text())
    ub = UIMAGE.read_bytes()
    if sha256(ub) != b["uimage_sha256"]:
        e.append("uImage changed since build")
    try:
        nm, _ = uimage_check(ub)
        if "maint" not in nm:
            e.append(f"uImage name {nm!r} is not a maint build")
    except ValueError as x:
        e.append(str(x))
    if not r.get("pass"):
        e.append("last rehearsal did not pass")
    if (
        r.get("cpio_sha256") != b["cpio_sha256"]
        or sha256(CPIO.read_bytes()) != b["cpio_sha256"]
    ):
        e.append("rehearsed cpio != the cpio in the uImage build")
    if b["init_sha256"] != sha256(INIT_SRC.read_bytes()):
        e.append("init changed since build")
    if "-maint-" not in b["kver"]:
        e.append(f"kver {b['kver']} lacks -maint-")
    return e


def cmd_preflight(_a) -> int:
    ip = _box.require()
    f = preflight(ip)
    errs = f["errors"] + local_checks()
    log(f"preflight: {json.dumps({k: v for k, v in f.items() if k != 'table'})}")
    for x in errs:
        log(f"preflight FAIL: {x}")
    log("preflight: " + ("OK" if not errs else f"{len(errs)} problem(s)"))
    return 1 if errs else 0


def verify_after(ip: str, run_id: str) -> int:
    errs = []
    name = f"/root/ssd-repartition-{run_id}.log"
    boxlog = ssh(ip, f"cat {name} 2>/dev/null || true")
    dest = TMP / "logs" / f"ssd-repartition-{run_id}-box.log"
    dest.write_text(boxlog)
    if boxlog:
        log(f"--- {name} (copy: {dest}) ---")
        for line in boxlog.splitlines():
            print(line)
    else:
        errs.append(f"{name} missing")
        log(
            "no box log; pstore:\n"
            + ssh(
                ip,
                "ls -l /sys/fs/pstore /var/lib/systemd/pstore; tail -n 60 /sys/fs/pstore/console-ramoops-0 2>/dev/null; true",
            )
        )
    if "RESULT: OK" not in boxlog:
        errs.append("box log has no RESULT: OK")
    dn = None
    for line in ssh(
        ip, "for b in /sys/block/sd*; do echo ${b##*/} $(cat $b/device/model); done"
    ).splitlines():
        if line.split(" ", 1)[1].startswith(REAL.model):
            dn = line.split()[0]
    if not dn:
        errs.append(f"no {REAL.model} disk")
        for x in errs:
            log(f"verify FAIL: {x}")
        return 1
    d = parse_dump(ssh(ip, f"sfdisk -d /dev/{dn}"))
    errs += check_table(d, REAL, after=True)
    fsb = fs_bytes(ssh(ip, f"dumpe2fs -h /dev/{dn}2 2>/dev/null"))
    if fsb != REAL.fs_target_bytes:
        errs.append(f"fs is {fsb} B, want {REAL.fs_target_bytes}")
    log("df: " + ssh(ip, "df -h /").strip().replace("\n", " | "))
    sums = boot_sums(ip)
    restored = len(sums) == 2 and len(set(sums.values())) == 1
    if not restored:
        errs.append(f"/boot/uImage != /boot/uImage.normal: {sums}")
    else:
        ssh(ip, "rm -f /boot/uImage.normal /boot/ssd-repartition.id && sync")
        log(
            "cleanup: removed /boot/uImage.normal and the run id (uImage verified == normal)"
        )
    for x in errs:
        log(f"verify FAIL: {x}")
    log("verify: " + ("OK" if not errs else f"{len(errs)} problem(s)"))
    return 1 if errs else 0


def install(ip: str, run_id: str) -> str:
    """normal -> uImage.normal, maint -> uImage; every copy sha256-verified."""
    ub = UIMAGE.read_bytes()
    maint = sha256(ub)
    out = ssh(
        ip,
        "cp -p /boot/uImage /boot/uImage.normal && sync && sha256sum /boot/uImage /boot/uImage.normal",
    )
    hs = re.findall(r"^([0-9a-f]{64}) ", out, re.M)
    if len(hs) != 2 or hs[0] != hs[1]:
        raise RuntimeError(f"uImage.normal copy mismatch: {out}")
    normal = hs[0]
    # scp ~25 MB over 1G: < 1 s; 60 s backstop.
    subprocess.run(
        ["scp", *_box.SSH_OPTS_BATCH, str(UIMAGE), f"root@{ip}:/boot/uImage.maint"],
        check=True,
        timeout=60,
    )
    epoch = int(time.time())
    out = ssh(
        ip,
        f"printf 'ID={run_id}\\nEPOCH={epoch}\\n' > /boot/ssd-repartition.id"
        " && sha256sum /boot/uImage.maint | grep -q '^" + maint + " '"
        " && mv -f /boot/uImage.maint /boot/uImage && sync"
        " && sha256sum /boot/uImage /boot/uImage.normal",
    )
    hs = re.findall(r"^([0-9a-f]{64}) ", out, re.M)
    if hs != [maint, normal]:
        raise RuntimeError(f"install verify failed: {out}")
    log(f"install: /boot/uImage = maint {maint[:16]}, uImage.normal = {normal[:16]}")
    return normal


def boot_sums(ip: str) -> dict[str, str]:
    out = ssh(ip, "sha256sum /boot/uImage /boot/uImage.normal 2>/dev/null; true")
    return {p: h for h, p in re.findall(r"^([0-9a-f]{64})  (\S+)$", out, re.M)}


def rollback(ip: str, orig: str) -> None:
    """Put the pre-run uImage back, only from a copy proven to be it."""
    s = boot_sums(ip)
    if s.get("/boot/uImage") == orig:
        log("rollback: /boot/uImage is still the original")
        return
    if s.get("/boot/uImage.normal") != orig:
        log(f"ROLLBACK IMPOSSIBLE: no verified copy of the original uImage: {s}")
        return
    ssh(ip, "cp -p /boot/uImage.normal /boot/uImage && sync")
    ok = boot_sums(ip).get("/boot/uImage") == orig
    log(f"rollback: {'OK' if ok else 'FAILED - /boot/uImage is not the original'}")


def wait_back(old_boot_id: str, kver: str, t0: float, expected_s: int) -> str | None:
    last_err = ""
    while (el := time.monotonic() - t0) < RUN_LIMIT_S:
        ip = _box.locate()
        if ip:
            try:
                k, bid = _box.boot_state(ip)
                if bid != old_boot_id:
                    log(f"back after {el:.0f}s on {k} at {ip}")
                    if k != kver:
                        log(f"WARN: kernel {k} != pre-run {kver}")
                    return ip
            except RuntimeError as e:
                last_err = str(e)
        log(f"waiting: {el:.0f}/{RUN_LIMIT_S}s (expected ~{expected_s}s) {last_err}")
        time.sleep(POLL_S)
    log(f"TIMEOUT: no new boot after {RUN_LIMIT_S}s; last ssh error: {last_err}")
    return None


GUIDANCE = """\
NOT power-cycling automatically. What is known:
- /init restores /boot/uImage BEFORE touching the filesystem, but there is no
  network in the initramfs: whether it got that far is unknown until the box returns.
- /init's own step ceilings (incl. T_RESIZE {t_resize}s) have all expired by now, and an
  armed SP805 would have reset the box. A box still silent is hung outside that
  (kernel never reached /init, or U-Boot did not boot the maint uImage).
- Power-cycle (./dev.py power-cycle) is safe for the disk only if resize2fs is not running;
  past {limit}s it cannot be. If the maint uImage was never restored, a power-cycle
  re-runs /init from the top (restore first), which is the intended retry.
- Then: ./dev.py ssd-repartition verify --id {run_id}
- No progress at all after a power-cycle -> serial console needed.
"""


def cmd_run(a) -> int:
    if not a.go:
        log("refusing: pass --go (this resizes the root filesystem)")
        return 2
    errs = local_checks()
    ip = _box.require()
    f = preflight(ip)
    errs += f["errors"]
    if errs:
        for x in errs:
            log(f"preflight FAIL: {x}")
        return 1
    # Same terms as RUN_LIMIT_S, with this box's measured resize estimate.
    expected = 60 + 30 + 120 + f["resize_estimate_s"] + 120 + 30 + 17 + 80
    log(
        f"preflight OK: {f['tail']}, resize estimate {f['resize_estimate_s']}s, limit {RUN_LIMIT_S}s"
    )
    run_id = time.strftime("%Y%m%d-%H%M%S")
    kver, old_bid = _box.boot_state(ip)
    orig = boot_sums(ip)["/boot/uImage"]
    try:
        install(ip, run_id)
    except (RuntimeError, subprocess.SubprocessError) as e:
        log(f"FATAL: install: {e}")
        rollback(ip, orig)
        return 1
    log(f"run id {run_id}: rebooting into the maint kernel")
    try:
        # The box drops the session as it goes down; rc is meaningless. 30 s backstop.
        subprocess.run(
            _box.ssh_argv(ip, cmd=["systemctl reboot"], batch=True),
            check=False,
            timeout=30,
            capture_output=True,
        )
    except subprocess.TimeoutExpired:
        log("systemctl reboot: ssh did not return in 30 s (box going down?) - polling")
    t0 = time.monotonic()
    ip = wait_back(old_bid, kver, t0, expected)
    if not ip:
        print(
            GUIDANCE.format(
                t_resize=TIMEOUTS["T_RESIZE"], limit=RUN_LIMIT_S, run_id=run_id
            )
        )
        return 3
    return verify_after(ip, run_id)


def cmd_verify(a) -> int:
    return verify_after(_box.require(), a.id)


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("collect")
    b = sub.add_parser("build")
    b.add_argument("--cpio-only", action="store_true")
    r = sub.add_parser("rehearse")
    r.add_argument("--scenario", action="append", choices=list(SCENARIOS))
    sub.add_parser("preflight")
    rn = sub.add_parser("run")
    rn.add_argument("--go", action="store_true")
    v = sub.add_parser("verify")
    v.add_argument("--id", required=True)
    a = ap.parse_args()
    STAGE.mkdir(parents=True, exist_ok=True)
    fn = {
        "collect": cmd_collect,
        "build": cmd_build,
        "rehearse": cmd_rehearse,
        "preflight": cmd_preflight,
        "run": cmd_run,
        "verify": cmd_verify,
    }[a.cmd]
    return fn(a)


if __name__ == "__main__":
    sys.exit(main())
