#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Install our self-built TrueNAS 27 arm64 image onto woomera's p3 (#356, #338, docs/truenas.md).

  ./dev.py truenas-install build        # maint initramfs: live-env userland + our modules + /init
  ./dev.py truenas-install rehearse     # host: userland smoke + installer chroot steps (qemu-user)
  ./dev.py truenas-install stage        # job files -> p2 /srv/truenas-install, profile truenas-maint
                                        #   [--extra-modules TAR]: al_* modules tar for the image kernel
  ./dev.py truenas-install run --go     # one maint boot: pool + install into a p3-sized image on p2
  ./dev.py truenas-install commit-p3 --go   # Fedora: image partition -> p3 (extents, read-back)
  ./dev.py truenas-install stage-boot   # profile truenas: uImage + DTB + the installed initrd
  ./dev.py truenas-install verify       # on TrueNAS: kernel, pool, middleware, web UI, links

- Image = scale-build's `.update` (tmp/release/*.update, or $TRUENAS_UPDATE); no ISO: the maint
  live env is the update's own rootfs.squashfs (nested loop mount in /init).
- Installed kernel = the image's own deb; the maint boot kernel is the build-linux-truenas output.
- p2 writes only under /srv/truenas-install and /boot/awto. p3 written only by commit-p3.
"""

from __future__ import annotations

import argparse
import contextlib
import gzip
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _bootsel as bs  # noqa: E402
from _kbuild import mkuimage  # noqa: E402

import _box  # noqa: E402
from _net import MAINT_1G_IPADDR  # noqa: E402
from _repo import REPO, TMP, make_log, truenas_build_out  # noqa: E402

log = make_log("truenas-install")

SCALE_BUILD = Path("/mnt/2tb/truenas-build/scale-build")


def update_image() -> Path:
    """$TRUENAS_UPDATE, else the newest scale-build tmp/release/*.update."""
    if os.environ.get("TRUENAS_UPDATE"):
        return Path(os.environ["TRUENAS_UPDATE"])
    ups = sorted(
        (SCALE_BUILD / "tmp/release").glob("*.update"), key=lambda p: p.stat().st_mtime
    )
    if not ups:
        raise RuntimeError(
            f"no .update in {SCALE_BUILD}/tmp/release: ./dev.py truenas-build update first"
        )
    return ups[-1]


EXTRA_TAR = "modules-extra.tar"
STAGE = TMP / "truenas-install"
TREE = STAGE / "initramfs"
CPIO = STAGE / "initramfs.cpio.gz"
INIT_SRC = REPO / "scripts/initramfs/truenas-maint-init"
JOB_SRC = REPO / "scripts/truenas-install-job.py"
ROOT_DIR = (
    TMP / "truenas-install/rootfs"
)  # sudo unsquashfs of the update rootfs: real ownership
ROOT_STAMP = ROOT_DIR.parent / "rootfs.stamp"
COPY_SRC = REPO / "scripts/truenas_p3_copy.py"
OUT = truenas_build_out()

JOB_DIR = "/srv/truenas-install"
P2_PARTUUID = "dcdc291e-9956-48cd-9d7c-48219877881a"
P3_PARTUUID = "a3344d0d-1708-4cb3-a38f-758d120f1106"
P3_NAME = "truenas-boot"
P3_BYTES = 140932808704  # sysfs size x 512 of p3 (sdd3), read 2026-11-13
IMG_P1_OFFSET = 1 << 20  # sgdisk -n 1:2048 in the job

# Userland of the maint /init, taken from the live env (same glibc as its chroot).
BINS = ("bash", "kmod", "blkid", "mount", "umount", "mkdir", "cat", "sync", "chroot",
        "sleep", "head", "uname", "touch", "dmesg", "tail", "ls", "grep", "tr", "readlink")  # fmt: skip
KMOD_LINKS = ("modprobe", "insmod", "lsmod", "depmod")
# ramoops first: with ramoops.max_reason=3 the maint log survives the reset into Fedora
# (/sys/fs/pstore) - the only record when /init dies before p2 is mounted. Then ahci_alpine (#92).
# al_eth_1g + netconsole next: kernel log to ./dev.py netconsole-listen (no serial console).
# i2c-designware + fixed before ahci_alpine: SATA's DT *-supply bay rails sit on i2c pca9575;
# without them fw_devlink defers the AHCI probe forever (bind -> EAGAIN, #338, #104).
MODULES = ("ramoops", "al_eth_1g", "netconsole", "i2c_designware_platform", "fixed", "ahci_alpine", "sd_mod", "ext4", "loop", "squashfs", "overlay", "sp805_wdt",
           "al_reboot", "zfs")  # fmt: skip
CONF = {
    "JOB_DIR": JOB_DIR,
    "P2_PARTUUID": P2_PARTUUID,
    # SSD probed by uptime 4.7-21.6 s (ssd-repartition.py T_DISK); x~5 because expiry = no
    # install and a wasted boot, not a hang. On expiry: logged to the kernel log, reboot.
    "T_DISK": 120,
    # No SSD I/O and no job CPU for this long = hung; SP805 then resets (17 s hw timeout).
    "STALL_S": 300,
    "FEED_EVERY_S": 5,
}
# Maint boot: Fedora reset -> /init done -> reset -> Fedora sshd. First run unmeasured:
# estimate = 2 boots (~90 s) + unsquashfs 7.6 GB xz on 4x A57 (~15 min) + 2 initrds (~10 min).
# Polling only - expiry stops waiting and prints guidance, nothing is killed.
RUN_LIMIT_S = 3600
POLL_S = 20
# TrueNAS boot -> sshd; first boot runs middleware first-boot setup. Unmeasured; record in #338.
TRUENAS_BOOT_LIMIT_S = 900
SQL = (
    "UPDATE services_services SET srv_enable = 1 WHERE srv_service = 'ssh';\n"
    "UPDATE system_advanced SET adv_serialconsole = 1, adv_serialport = 'ttyS0', adv_serialspeed = 115200;\n"
)
# scp of the 1.8 GB update over 1G: ~20 s at line rate, ~60 s at the box's ext4 write rate.
SCP_BIG_TIMEOUT_S = 300


def kver() -> str:
    mods = sorted((OUT / "modroot/lib/modules").iterdir())
    if len(mods) != 1:
        raise RuntimeError(f"want one kernel in {OUT}/modroot, found {mods}")
    return mods[0].name


def sh(cmd: list, **kw) -> subprocess.CompletedProcess:
    kw.setdefault("check", True)
    return subprocess.run([str(c) for c in cmd], **kw)


# --- build --------------------------------------------------------------
def _real(root: Path, p: str) -> Path:
    """Resolve p inside root (absolute symlinks are root-relative)."""
    cur = root
    for part in Path(p).relative_to("/").parts:
        cur = cur / part
        n = 0
        while cur.is_symlink():
            t = os.readlink(cur)
            cur = (root / t.lstrip("/")) if t.startswith("/") else (cur.parent / t)
            cur = Path(os.path.normpath(cur))
            n += 1
            if n > 20:
                raise RuntimeError(f"symlink loop at {p}")
    return cur


def _needed(elf: Path) -> list[str]:
    out = sh(["readelf", "-d", elf], capture_output=True, text=True).stdout
    return re.findall(r"\(NEEDED\).*\[(.+)\]", out)


def _interp(elf: Path) -> str | None:
    out = sh(["readelf", "-l", elf], capture_output=True, text=True).stdout
    m = re.search(r"Requesting program interpreter: (\S+)\]", out)
    return m.group(1) if m else None


def collect_userland() -> dict[str, Path]:
    """rel path in initramfs -> source file in ROOT_DIR, NEEDED-closure included."""
    libdirs = (
        "/usr/lib/aarch64-linux-gnu",
        "/lib/aarch64-linux-gnu",
        "/usr/lib",
        "/lib",
    )
    files: dict[str, Path] = {}
    todo = []
    for b in BINS:
        for d in ("/usr/bin", "/usr/sbin"):
            src = _real(ROOT_DIR, f"{d}/{b}")
            if src.is_file():
                files[f"usr/bin/{b}"] = src
                todo.append(src)
                break
        else:
            raise RuntimeError(f"{b} not in {ROOT_DIR}")
    seen = set()
    while todo:
        elf = todo.pop()
        interp = _interp(elf)
        deps = _needed(elf)
        if interp and interp not in seen:
            seen.add(interp)
            src = _real(ROOT_DIR, interp)
            files[interp.lstrip("/")] = src
        for lib in deps:
            if lib in seen:
                continue
            seen.add(lib)
            for d in libdirs:
                src = _real(ROOT_DIR, f"{d}/{lib}")
                if src.is_file():
                    files[f"usr/lib/aarch64-linux-gnu/{lib}"] = src
                    todo.append(src)
                    break
            else:
                raise RuntimeError(f"{lib} (needed by {elf}) not found")
    return files


def module_closure(kv: str) -> list[str]:
    mdir = OUT / "modroot/lib/modules" / kv
    dep = {}
    for line in (mdir / "modules.dep").read_text().splitlines():
        k, _, v = line.partition(":")
        dep[Path(k).name.removesuffix(".ko").replace("-", "_")] = (k, v.split())
    want, out = list(MODULES), set()
    for m in want:
        if m not in dep:
            raise RuntimeError(f"module {m} not in {mdir}/modules.dep")
        k, deps = dep[m]
        out.add(k)
        out.update(deps)
    return sorted(out)


BOX_1G_IP = MAINT_1G_IPADDR


def netconsole_arg() -> str:
    """<box 1G ip>/eth0 -> this host's address + next-hop MAC on the LAN."""
    r = sh(
        ["ip", "-o", "route", "get", BOX_1G_IP], capture_output=True, text=True
    ).stdout
    dev, src = re.search(r" dev (\S+) .*src (\S+)", r).groups()
    mac = Path(f"/sys/class/net/{dev}/address").read_text().strip()
    return f"6665@{BOX_1G_IP}/eth0,6666@{src}/{mac}"


def cmd_build(_a) -> int:
    kv = kver()
    ensure_root()
    if TREE.exists():
        shutil.rmtree(TREE)
    for d in (
        "dev",
        "proc",
        "sys",
        "run",
        "tmp",
        "mnt",
        "etc",
        "usr/bin",
        "usr/lib/aarch64-linux-gnu",
    ):
        (TREE / d).mkdir(parents=True, exist_ok=True)
    for link, target in (
        ("bin", "usr/bin"),
        ("sbin", "usr/bin"),
        ("lib", "usr/lib"),
        ("usr/sbin", "bin"),
    ):
        os.symlink(target, TREE / link)
    for rel, src in collect_userland().items():
        dst = TREE / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        if not dst.exists():
            shutil.copy2(src, dst)
    for k in KMOD_LINKS:
        os.symlink("kmod", TREE / "usr/bin" / k)
    mdir = OUT / "modroot/lib/modules" / kv
    tdir = TREE / "usr/lib/modules" / kv
    tdir.mkdir(parents=True)
    for f in mdir.glob("modules.*"):
        shutil.copy2(f, tdir / f.name)
    mods = module_closure(kv)
    for m in mods:
        (tdir / m).parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(mdir / m, tdir / m)
    shutil.copy2(INIT_SRC, TREE / "init")
    (TREE / "init").chmod(0o755)
    conf = {"KVER": kv, "MODULES": " ".join(MODULES), **CONF}
    (TREE / "etc/modprobe.d").mkdir(parents=True, exist_ok=True)
    (TREE / "etc/modprobe.d/netconsole.conf").write_text(
        f"options netconsole netconsole={netconsole_arg()}\n"
    )
    (TREE / "etc/tnmaint.conf").write_text(
        "# generated by scripts/truenas-install.py\n"
        + "".join(f"{k}={shlex.quote(str(v))}\n" for k, v in conf.items())
    )
    listing = sh(
        ["bash", "-c", "find . | LC_ALL=C sort"],
        cwd=TREE,
        capture_output=True,
        text=True,
    ).stdout
    raw = sh(["cpio", "-o", "-H", "newc", "--owner=0:0", "--quiet"], cwd=TREE, input=listing.encode(),
             capture_output=True).stdout  # fmt: skip
    CPIO.write_bytes(gzip.compress(raw, 9, mtime=0))
    log(
        f"build: {CPIO} {CPIO.stat().st_size} B (cpio {len(raw)} B), {len(mods)} modules for {kv}"
    )
    return 0


def ensure_root() -> None:
    """ROOT_DIR = the current update's rootfs (live-env userland + chroot rehearsal)."""
    upd = update_image()
    want = f"{upd}:{upd.stat().st_mtime_ns}:{upd.stat().st_size}"
    if ROOT_DIR.is_dir() and ROOT_STAMP.exists() and ROOT_STAMP.read_text() == want:
        return
    mnt = ROOT_DIR.parent / "upd-mnt"
    mnt.mkdir(parents=True, exist_ok=True)
    sudo("rm", "-rf", ROOT_DIR, check=False)
    log(f"unsquashfs {upd} rootfs.squashfs -> {ROOT_DIR} (sudo, ~8 GB, xz)")
    sudo("mount", "-t", "squashfs", "-o", "ro,loop", upd, mnt)
    try:
        # ~15 min for 7.6 GB xz on the box's 4x A57; a host is faster. 1800 s backstop.
        sudo(
            "unsquashfs",
            "-n",
            "-d",
            ROOT_DIR,
            mnt / "rootfs.squashfs",
            timeout=1800,
            capture_output=True,
        )
    finally:
        sudo("umount", mnt, check=False)
    ROOT_STAMP.write_text(want)


# --- rehearse -------------------------------------------------------------
def sudo(*cmd, timeout=600, **kw) -> subprocess.CompletedProcess:
    """Privileged host step; 600 s = update-initramfs under qemu-user (slowest step) backstop."""
    return sh(["sudo", "-n", *cmd], timeout=timeout, **kw)


def image_kver(root: Path) -> str:
    """Kernel the image ships: the one /boot/vmlinuz-<kver> in its rootfs."""
    vm = sorted((root / "boot").glob("vmlinuz-*"))
    if len(vm) != 1:
        raise RuntimeError(f"want one /boot/vmlinuz-* in {root}, found {vm}")
    return vm[0].name.removeprefix("vmlinuz-")


def job_cfg(job_dir: str, password: str) -> dict:
    return {
        "job_dir": job_dir,
        "target": "image",
        "p3_bytes": P3_BYTES,
        "p3_partuuid": P3_PARTUUID,
        "admin_password": password,
        "sql": SQL,
    }


def admin_password() -> str:
    import yaml

    return yaml.safe_load(open(os.path.expanduser("~/.config/awto/secrets.yaml")))[
        "truenas-woomera"
    ]["password"]


def stage_job_files(dst: Path, extra_modules: Path | None) -> None:
    """Job inputs beyond job.py/job.json (same set the box gets), locally."""
    dst.mkdir(parents=True, exist_ok=True)
    if extra_modules:  # hook: al_* modules for the image kernel, tar of <kver>/... under usr/lib/modules
        shutil.copy2(extra_modules, dst / EXTRA_TAR)


EXTRA_RH = (
    Path(os.environ["TRUENAS_EXTRA_MODULES"])
    if os.environ.get("TRUENAS_EXTRA_MODULES")
    else None
)


def cmd_rehearse(_a) -> int:
    kv = kver()
    sh(["bash", "-n", INIT_SRC])
    # 1. maint userland runs (qemu-user binfmt) and finds every library and module.
    smoke = ("set -e; bash --version | head -n1; modprobe --version; blkid -V; mount --version; "
             f"modprobe -n -v -S {kv} -a {' '.join(MODULES)}")  # fmt: skip
    r = sudo(
        "chroot",
        TREE,
        "/bin/bash",
        "-c",
        smoke,
        capture_output=True,
        text=True,
        check=False,
    )
    log(f"userland smoke rc={r.returncode}\n{r.stdout}{r.stderr}")
    if r.returncode:
        return 1
    # 2. installer chroot steps + the image's kernel + initrd on a real-ownership rootfs.
    ensure_root()
    jd = STAGE / "rehearse-job"
    stage_job_files(jd, EXTRA_RH)
    shutil.copy2(Path.home() / ".ssh/authorized_keys" if (Path.home() / ".ssh/authorized_keys").exists()
                 else next((Path.home() / ".ssh").glob("id_*.pub")), jd / "authorized_keys")  # fmt: skip
    cfgf = jd / "job.json"
    cfgf.write_text(json.dumps(job_cfg(str(jd), admin_password())))
    cfgf.chmod(0o600)
    # Fresh overlay per run: ROOT_DIR stays pristine (the installer steps are not idempotent).
    ovl = STAGE / "rehearse-ovl"
    sudo("rm", "-rf", ovl, check=False)
    for d in ("upper", "work", "root"):
        (ovl / d).mkdir(parents=True)
    root = ovl / "root"
    sudo("mount", "-t", "overlay", "overlay", "-o",
         f"lowerdir={ROOT_DIR},upperdir={ovl / 'upper'},workdir={ovl / 'work'}", root)  # fmt: skip
    try:
        for d in ("dev", "proc", "sys"):
            sudo("mount", "--bind", f"/{d}", root / d)
        r = sudo(sys.executable, "-I", JOB_SRC, "--rehearse-root", root, cfgf,
                 capture_output=True, text=True, check=False, timeout=1800)  # fmt: skip
    finally:
        for d in ("sys", "proc", "dev"):
            sudo("umount", "-l", root / d, check=False)
        cfgf.unlink()
    (STAGE / "rehearse.log").write_text(r.stdout + r.stderr)
    log(f"installer-steps rehearsal rc={r.returncode} (log {STAGE / 'rehearse.log'})")
    if r.returncode:
        log((r.stdout + r.stderr)[-3000:])
        return 1
    ikv = image_kver(ROOT_DIR)
    initrd = root / f"boot/initrd.img-{ikv}"
    try:
        size = int(
            sudo("stat", "-c", "%s", initrd, capture_output=True, text=True).stdout
        )
        lst = sh(["bash", "-c", f"sudo -n cat {shlex.quote(str(initrd))} | zstd -dc | cpio -t --quiet"],
                 capture_output=True, text=True).stdout.splitlines()  # fmt: skip
    finally:
        sudo("umount", root, check=False)
    must = [
        "ahci_alpine.ko",
        "i2c-designware-platform.ko",
        "fixed.ko",
        "zfs.ko",
        "spl.ko",
        "sd_mod.ko",
        "scripts/zfs",
        "sbin/zpool",
        "etc/hostid",
    ]
    missing = [m for m in must if not any(x.endswith(m) or f"/{m}" in x for x in lst)]
    log(f"rehearsal initrd {size} B, {len(lst)} entries; missing: {missing or 'none'}")
    return 1 if [m for m in missing if m != "etc/hostid"] else 0


# --- box steps ------------------------------------------------------------
def ssh(ip: str, cmd: str, timeout: float = bs.SSH_TIMEOUT_S) -> str:
    return bs.ssh(ip, cmd, timeout=timeout)


def scp(ip: str, src: Path, dst: str) -> None:
    sh(
        ["scp", "-q", *_box.SSH_OPTS_BATCH, src, f"root@{ip}:{dst}"],
        timeout=SCP_BIG_TIMEOUT_S,
    )


def p3_identity(ip: str) -> str:
    """Return /dev/sdXN of p3 after checking PARTUUID, PARTNAME, size and that nothing uses it."""
    dev = ssh(ip, f"readlink -f /dev/disk/by-partuuid/{P3_PARTUUID}").strip()
    n = dev.removeprefix("/dev/")
    info = ssh(ip, f"cat /sys/class/block/{n}/size /sys/class/block/{n}/uevent; ls /sys/class/block/{n}/holders;"
                   f" grep -c '^{dev} ' /proc/mounts; true")  # fmt: skip
    if int(info.split()[0]) * 512 != P3_BYTES or f"PARTNAME={P3_NAME}" not in info:
        raise RuntimeError(f"{dev} is not p3 ({P3_NAME}, {P3_BYTES} B):\n{info}")
    if info.strip().splitlines()[-1] != "0":
        raise RuntimeError(f"{dev} is mounted")
    return dev


def cmd_stage(a) -> int:
    ip = _box.require()
    if not CPIO.exists():
        raise RuntimeError("run `./dev.py truenas-install build` first")
    p3_identity(ip)
    jd = STAGE / "box-job"
    stage_job_files(jd, a.extra_modules)
    ssh(
        ip,
        f"mkdir -p {JOB_DIR}/logs && rm -f {JOB_DIR}/result.json {JOB_DIR}/{EXTRA_TAR}",
    )
    want = {"TrueNAS-SCALE.update": update_image(), "job.py": JOB_SRC}
    if a.extra_modules:
        want[EXTRA_TAR] = jd / EXTRA_TAR
    # Named files only: a * would also hash the job's ~131 GB p3.img.
    have = ssh(
        ip, f"cd {JOB_DIR} && sha256sum {' '.join(want)} 2>/dev/null; true", timeout=120
    )
    have_h = {p: h for h, p in re.findall(r"^([0-9a-f]{64})  (\S+)$", have, re.M)}
    for name, src in want.items():
        h = sh(["sha256sum", src], capture_output=True, text=True).stdout.split()[0]
        if have_h.get(name) == h:
            log(f"stage: {name} already there")
            continue
        log(f"stage: {name} ({src.stat().st_size} B)")
        scp(ip, src, f"{JOB_DIR}/{name}")
        got = ssh(ip, f"sha256sum {JOB_DIR}/{name}", timeout=120).split()[0]
        if got != h:
            raise RuntimeError(f"{name}: sha256 mismatch after copy")
    ssh(ip, f"cp /root/.ssh/authorized_keys {JOB_DIR}/authorized_keys")
    cfg = jd / "job.json"
    cfg.write_text(json.dumps(job_cfg(f"/mnt/p2{JOB_DIR}", admin_password())))
    try:
        scp(ip, cfg, f"{JOB_DIR}/job.json")
    finally:
        cfg.unlink()
    ssh(ip, f"chmod 600 {JOB_DIR}/job.json && sync")
    dtb = TMP / "dtb/unvr-truenas.dtb"
    bs.stage_profile(ip, "truenas-maint", uimage=next(OUT.glob("uImage-*-truenas")), dtb=dtb, initrd=CPIO,
                     bootargs="console=ttyS0,115200 rdinit=/init panic=15 reboot=cold ramoops.max_reason=3 printk.devkmsg=on netpoll.carrier_timeout=15 "
                              "ahci.mask_port_map=0x5 ahci_alpine.mask_port_map=0x5", log=log)  # fmt: skip
    log("stage: done")
    return 0


GUIDANCE = """\
Not back on Fedora after {limit}s. Nothing is killed; check again later:
- ./dev.py ssh -- uname -r     (answers = run finished; then: ./dev.py truenas-install fetch)
- /init has no time cap: only {stall}s of no SSD I/O and no job CPU stops the watchdog feed.
- Silent for hours: power-cycle (./dev.py power-cycle). bootonce is already cleared -> Fedora.
"""


def fetch(ip: str) -> dict | None:
    rid = time.strftime("%Y%m%d-%H%M%S")
    dst = STAGE / "runs" / rid
    dst.mkdir(parents=True, exist_ok=True)
    sh(
        ["scp", "-q", "-r", *_box.SSH_OPTS_BATCH, f"root@{ip}:{JOB_DIR}/logs", dst],
        timeout=60,
        check=False,
    )
    res = ssh(ip, f"cat {JOB_DIR}/result.json 2>/dev/null; true").strip()
    if res:
        (dst / "result.json").write_text(res)
    out = ssh(ip, f"cat {JOB_DIR}/out/info.json 2>/dev/null; true").strip()
    if out:
        (dst / "info.json").write_text(out)
    log(f"fetched -> {dst}")
    return json.loads(res) if res else None


def cmd_run(a) -> int:
    if not a.go:
        log("refusing: pass --go (boots the maint kernel once)")
        return 2
    ip = _box.require()
    p3_identity(ip)
    if not bs.profile_present(ip, "truenas-maint"):
        raise RuntimeError("profile truenas-maint not staged")
    if ssh(ip, f"test -s {JOB_DIR}/job.json && echo ok").strip() != "ok":
        raise RuntimeError(
            "job.json missing (consumed by the last run?) - run stage again"
        )
    k0 = ssh(ip, "uname -r").strip()
    sh(
        [sys.executable, REPO / "scripts/boot-next.py", "truenas-maint"],
        env={**os.environ, "AWTO_VIA_DEVPY": "1"},
    )
    old = bs.reboot(ip, log)
    got = bs.wait_new_boot(old, RUN_LIMIT_S, log, poll_s=POLL_S)
    if not got:
        print(GUIDANCE.format(limit=RUN_LIMIT_S, stall=CONF["STALL_S"]))
        return 3
    ip, k = got
    if k != k0:
        log(f"WARN: back on {k}, was {k0}")
    res = fetch(ip)
    if not res:
        log("no result.json: the job never finished - see the fetched logs")
        return 1
    log(f"job ok={res['ok']} in {res.get('elapsed_s')}s {res.get('error', '')}")
    return 0 if res["ok"] else 1


def cmd_fetch(_a) -> int:
    res = fetch(_box.require())
    log(json.dumps(res, indent=1)[:2000] if res else "no result.json")
    return 0


def cmd_commit_p3(a) -> int:
    if not a.go:
        log("refusing: pass --go (overwrites p3)")
        return 2
    ip = _box.require()
    if ssh(ip, "cat /proc/cmdline").find("awto.profile=") >= 0:
        raise RuntimeError("not on Fedora's default boot")
    dev = p3_identity(ip)
    res = json.loads(ssh(ip, f"cat {JOB_DIR}/result.json"))
    if not res.get("ok") or not str(res.get("vdev", "")).endswith("p1"):
        raise RuntimeError(
            f"last job did not succeed on the image target: {res.get('error')}"
        )
    cmd = (
        f"python3 - {JOB_DIR}/p3.img {IMG_P1_OFFSET} {P3_BYTES} {P3_PARTUUID} {P3_NAME}"
    )
    # ~6 GB of extents at the SSD's ~400 MB/s write + the same read back: ~40 s. 300 s backstop.
    r = subprocess.run(_box.ssh_argv(ip, cmd=[cmd], batch=True), input=COPY_SRC.read_bytes(),
                       capture_output=True, timeout=300, check=False)  # fmt: skip
    out = r.stdout.decode() + r.stderr.decode()
    log(f"commit-p3 -> {dev}: rc={r.returncode}\n{out.strip()}")
    return r.returncode


def cmd_stage_boot(_a) -> int:
    ip = _box.require()
    info = json.loads(ssh(ip, f"cat {JOB_DIR}/out/info.json"))
    kv = info["kver"]
    local = STAGE / f"initrd.img-{kv}"
    vmz = STAGE / f"vmlinuz-{kv}"
    for name, dst in ((info["initrd"], local), (info["vmlinuz"], vmz)):
        sh(
            ["scp", "-q", *_box.SSH_OPTS_BATCH, f"root@{ip}:{JOB_DIR}/out/{name}", dst],
            timeout=60,
        )
    for f, key in ((local, "initrd_sha256"), (vmz, "vmlinuz_sha256")):
        if (
            sh(["sha256sum", f], capture_output=True, text=True).stdout.split()[0]
            != info[key]
        ):
            raise RuntimeError(f"{f.name} sha256 != info.json")
    raw = vmz.read_bytes()
    if raw[:2] == b"\x1f\x8b":  # Image.gz: mkuimage wants the raw Image
        raw = gzip.decompress(raw)
    if raw[56:60] != b"ARM\x64":
        raise RuntimeError("vmlinuz is not an arm64 Image")
    img = STAGE / f"Image-{kv}"
    img.write_bytes(raw)
    uimg = STAGE / f"uImage-{kv}"
    mkuimage(str(img), str(uimg), f"truenas-{kv}"[:31], log)
    args = (
        f"console=ttyS0,115200 root=ZFS={info['bootfs']} ro zfsforce=1 libata.allow_tpm=1 "
        "nvme_core.multipath=N panic=15 reboot=cold ahci.mask_port_map=0x5 ahci_alpine.mask_port_map=0x5"
    )
    bs.stage_profile(ip, "truenas", uimage=uimg, dtb=TMP / "dtb/unvr-truenas.dtb",
                     initrd=local, bootargs=args, log=log)  # fmt: skip
    return 0


def cmd_verify(_a) -> int:
    ip = _box.require()
    checks = {
        "kernel": "uname -r",
        "cmdline": "cat /proc/cmdline",
        "pool": "zpool status boot-pool",
        "bootfs": "zpool get -H -o value bootfs boot-pool",
        "vermagic": "modinfo -F vermagic zfs ahci_alpine al_eth_1g 2>&1; true",
        "middleware": "systemctl is-active middlewared; midclt call system.ready 2>&1; true",
        "links": "ip -br link; ip -br addr",
        "disks": "lsblk -o NAME,SIZE,PARTLABEL,FSTYPE,MOUNTPOINTS",
        "failed": "systemctl --failed --no-legend | head -20",
        "http": "curl -s -o /dev/null -w '%{http_code}' http://127.0.0.1/ ; echo",
    }
    bad = 0
    for k, c in checks.items():
        try:
            log(f"== {k}\n{ssh(ip, c, timeout=60).rstrip()}")
        except RuntimeError as e:
            bad += 1
            log(f"== {k}: FAILED {e}")
    return 1 if bad else 0


@contextlib.contextmanager
def api_session():
    """Logged-in JSON-RPC caller on ws://<box>/api/current (26.04 has no REST), or None."""
    from websockets.sync.client import connect

    # LAN round trip + one middleware call: well under 1 s; 20 s per call = a hung middleware.
    with connect(f"ws://{_box.require()}/api/current", open_timeout=20) as ws:
        n = 0

        def call(method: str, params: list):
            nonlocal n
            n += 1
            ws.send(
                json.dumps(
                    {"jsonrpc": "2.0", "id": n, "method": method, "params": params}
                )
            )
            r = json.loads(ws.recv(timeout=20))
            if "error" in r:
                raise RuntimeError(f"{method}: {r['error']}")
            return r["result"]

        r = call("auth.login_ex", [{"mechanism": "PASSWORD_PLAIN", "username": "truenas_admin",
                                    "password": admin_password()}])  # fmt: skip
        if r.get("response_type") != "SUCCESS":
            log(f"FATAL: auth.login_ex: {r.get('response_type')}")
            yield None
            return
        yield call


def cmd_reboot_api(_a) -> int:
    """TrueNAS -> reboot over the API (bootonce already cleared -> Fedora). For when sshd is down."""
    with api_session() as call:
        if call is None:
            return 1
        call("system.reboot", ["awto-unvr: back to Fedora"])
    log("system.reboot sent")
    return 0


def cmd_ssh_api(_a) -> int:
    """Start TrueNAS sshd over the API and report the job's outcome (enable is set by the install SQL)."""
    with api_session() as call:
        if call is None:
            return 1
        svc = call("service.query", [[["service", "=", "ssh"]]])
        log(f"ssh service: {svc}")
        jid = call("service.control", ["START", "ssh", {"silent": False}])
        # sshd start = keygen + config render: seconds. 60 s of polling = stuck job.
        end = time.monotonic() + 60
        while time.monotonic() < end:
            job = call("core.get_jobs", [[["id", "=", jid]]])[0]
            if job["state"] in ("SUCCESS", "FAILED", "ABORTED"):
                break
            time.sleep(2)
        log(
            f"service.control START ssh: {job['state']} result={job.get('result')} error={job.get('error')}"
        )
        if job.get("exception"):
            log(job["exception"][-3000:])
        return 0 if job["state"] == "SUCCESS" else 1


def cmd_verify_api(_a) -> int:
    """Same checks over the middleware JSON-RPC websocket (26.04 has no REST) - works before sshd."""
    calls = {
        "system": ("system.info", []),
        "boot": ("boot.get_disks", []),  # get_state: 26.04 nightly schema bug (all_sed)
        "services": ("service.query", []),
        "interfaces": ("interface.query", []),
        "disks": ("disk.query", []),
    }
    bad = 0
    with api_session() as call:
        if call is None:
            return 1
        for k, (method, params) in calls.items():
            try:
                data = call(method, params)
            except (RuntimeError, TimeoutError) as e:
                bad += 1
                log(f"== {k}: FAILED {e}")
                continue
            if k == "system":
                data = {
                    x: data.get(x)
                    for x in (
                        "version",
                        "hostname",
                        "uptime_seconds",
                        "physmem",
                        "cores",
                        "model",
                    )
                }
            elif k == "services":
                data = {s["service"]: s["state"] for s in data}
            elif k == "interfaces":
                data = {i["name"]: [a["address"] for a in i.get("state", {}).get("aliases", [])
                                    if a.get("type") == "INET"] for i in data}  # fmt: skip
            elif k == "disks":
                data = [
                    (d["name"], d.get("model"), d.get("serial"), d.get("size"))
                    for d in data
                ]
            log(f"== {k}\n{json.dumps(data, indent=1)[:3000]}")
    return 1 if bad else 0


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "step",
        choices=(
            "build",
            "rehearse",
            "stage",
            "run",
            "fetch",
            "commit-p3",
            "stage-boot",
            "verify",
            "verify-api",
            "reboot-api",
            "ssh-api",
        ),
    )
    ap.add_argument("--go", action="store_true")
    ap.add_argument(
        "--extra-modules",
        type=Path,
        help="stage: tar of <kver>/... al_* modules for the image kernel",
    )
    a = ap.parse_args()
    fn = {"build": cmd_build, "rehearse": cmd_rehearse, "stage": cmd_stage, "run": cmd_run, "fetch": cmd_fetch,
          "commit-p3": cmd_commit_p3, "stage-boot": cmd_stage_boot, "verify": cmd_verify, "verify-api": cmd_verify_api, "reboot-api": cmd_reboot_api, "ssh-api": cmd_ssh_api}[a.step]  # fmt: skip
    try:
        return fn(a)
    except (RuntimeError, ValueError, subprocess.SubprocessError, OSError) as e:
        log(f"FATAL: {e}")
        return 1


if __name__ == "__main__":
    sys.exit(main())
