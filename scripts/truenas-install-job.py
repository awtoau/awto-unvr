#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""TrueNAS 27 (self-built image) install onto woomera, run INSIDE the TrueNAS live env (#338).

- Started by scripts/initramfs/truenas-maint-init; staged by ./dev.py truenas-install stage.
- Pool: the ISO installer's `zpool create` (truenas_installer/install.py, 26.04 copy) on
  target "image" (GPT image file on p2, partition 1 = p3's size, via loop -P) or "p3".
- Install: the update image's own truenas_install.main(), in-process, with run_command wrapped:
  - skip update-grub / grub-install (U-Boot boots us; no GRUB, no ESP),
  - before truenas-initrd.py: optional <job>/modules-extra.tar (al_* for the image kernel), depmod,
    initramfs module list, root's authorized_keys. The kernel itself is the image's own deb.
- Out: <job>/out/{initrd.img-<kver>,vmlinuz-<kver>,info.json}, <job>/result.json. Pool exported at the end.
- Host rehearsal (no zfs): --rehearse-root DIR runs the chroot steps on an extracted rootfs.
"""

from __future__ import annotations

import hashlib
import io
import json
import os
import shutil
import subprocess
import sys
import time
import traceback

POOL = "boot-pool"
ZPOOL_CREATE = [  # truenas_installer/install.py create_boot_pool(), 26.04
    "zpool", "create", "-f",
    "-o", "ashift=12", "-o", "cachefile=none", "-o", "compatibility=grub2",
    "-O", "acltype=off", "-O", "canmount=off", "-O", "compression=on", "-O", "devices=off",
    "-O", "mountpoint=none", "-O", "normalization=formD", "-O", "relatime=on", "-O", "xattr=sa",
    POOL,
]  # fmt: skip
SKIP = ("update-grub", "grub-install")
T0 = time.monotonic()


def log(msg: str) -> None:
    print(f"[{time.monotonic() - T0:7.1f}s] {msg}", flush=True)


def run(cmd: list[str], **kw) -> subprocess.CompletedProcess:
    log("+ " + " ".join(cmd))
    r = subprocess.run(cmd, capture_output=True, text=True, **kw)
    for line in (r.stdout + r.stderr).splitlines()[-40:]:
        log(f"  | {line}")
    if kw.get("check", True) is not False and r.returncode:
        raise RuntimeError(f"rc={r.returncode}: {' '.join(cmd)}")
    return r


def sha256(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while b := f.read(1 << 20):
            h.update(b)
    return h.hexdigest()


def set_clock(job_json: str) -> None:
    """No RTC driver in the maint boot -> 1970; the installer stamps password change dates with it
    (-> EXPIRED login, #338). job.json was written by stage under Fedora's synced clock."""
    t = os.stat(job_json).st_mtime
    if time.time() < t:
        time.clock_settime(time.CLOCK_REALTIME, t)
        log(f"clock set to job.json mtime {time.strftime('%Y-%m-%dT%H:%M:%S%z')}")


def image_kver(root: str) -> str:
    """The one kernel the image ships (/boot/vmlinuz-<kver>)."""
    vm = [f for f in os.listdir(os.path.join(root, "boot")) if f.startswith("vmlinuz-")]
    if len(vm) != 1:
        raise RuntimeError(f"want one /boot/vmlinuz-* in {root}, found {vm}")
    return vm[0].removeprefix("vmlinuz-")


def inject_kernel(cfg: dict, root: str, chroot=("chroot",)) -> str:
    """Install-time additions to the image's own kernel; returns its kver."""
    kver, job = image_kver(root), cfg["job_dir"]
    extra = os.path.join(job, "modules-extra.tar")
    if os.path.exists(extra):
        run(["tar", "-xf", extra, "-C", os.path.join(root, "usr/lib/modules")])
    run([*chroot, root, "depmod", "-a", kver])
    # SATA probe waits on the bay rails (i2c pca9575 -> regulator-fixed, #104): =m in TrueNAS's config.
    with open(os.path.join(root, "etc/initramfs-tools/modules"), "a") as f:
        f.write(
            "\n# UNVR (awto-unvr #338)\ni2c_designware_platform\nfixed\nahci_alpine\nsd_mod\n"
        )
    ssh = os.path.join(root, "root/.ssh")
    os.makedirs(ssh, mode=0o700, exist_ok=True)
    shutil.copyfile(
        os.path.join(job, "authorized_keys"), os.path.join(ssh, "authorized_keys")
    )
    os.chmod(os.path.join(ssh, "authorized_keys"), 0o600)
    log(f"kernel {kver} (image) + root authorized_keys prepared in {root}")
    return kver


def make_target(cfg: dict) -> tuple[str, str | None]:
    """(vdev path, loop device to detach or None)."""
    if cfg["target"] == "p3":
        dev = run(
            ["blkid", "-t", f"PARTUUID={cfg['p3_partuuid']}", "-o", "device"]
        ).stdout.split()
        if len(dev) != 1:
            raise RuntimeError(f"p3 PARTUUID {cfg['p3_partuuid']}: found {dev}")
        name = os.path.basename(dev[0])
        size = int(open(f"/sys/class/block/{name}/size").read()) * 512
        if size != cfg["p3_bytes"]:
            raise RuntimeError(f"{dev[0]} is {size} B, want {cfg['p3_bytes']}")
        return dev[0], None
    img = os.path.join(cfg["job_dir"], "p3.img")
    if os.path.exists(img):
        os.unlink(img)
    with open(img, "wb") as f:
        f.truncate(cfg["p3_bytes"] + (2 << 20))  # p1 at 1 MiB + backup GPT
    run(
        [
            "sgdisk",
            "-a",
            "2048",
            "-n",
            f"1:2048:{2048 + cfg['p3_bytes'] // 512 - 1}",  # absolute end: +N is a length
            "-t",
            "1:BF01",
            img,
        ]
    )
    loop = run(["losetup", "-f", "-P", "--show", img]).stdout.strip()
    part = loop + "p1"
    for _ in range(50):  # partition node from devtmpfs: appears within ms of losetup -P
        if os.path.exists(part):
            break
        time.sleep(0.1)
    size = int(open(f"/sys/class/block/{os.path.basename(part)}/size").read()) * 512
    if size != cfg["p3_bytes"]:
        raise RuntimeError(f"{part} is {size} B, want {cfg['p3_bytes']}")
    return part, loop


def install(cfg: dict, res: dict) -> None:
    job = cfg["job_dir"]
    if not os.path.exists("/dev/zfs"):
        raise RuntimeError("/dev/zfs missing: zfs module not loaded")
    run(["zpool", "version"])
    if not os.path.exists("/etc/hostid"):
        run(["zgenhostid"])
    vdev, loop = make_target(cfg)
    res.update(vdev=vdev, loop=loop)
    try:
        run([*ZPOOL_CREATE, vdev])
        run(["zfs", "create", "-o", "canmount=off", f"{POOL}/ROOT"])
        run(
            [
                "zfs",
                "create",
                "-o",
                "canmount=off",
                "-o",
                "mountpoint=legacy",
                f"{POOL}/grub",
            ]
        )
        upd = "/tmp/upd"
        os.makedirs(upd, exist_ok=True)
        run(
            [
                "mount",
                "-t",
                "squashfs",
                "-o",
                "ro,loop",
                os.path.join(job, "TrueNAS-SCALE.update"),
                upd,
            ]
        )
        try:
            run_truenas_install(cfg, upd, res)
        finally:
            run(["umount", upd], check=False)
        collect(cfg, res)
    finally:
        run(["zpool", "export", POOL], check=False)
        if loop:
            run(["losetup", "-d", loop], check=False)


def run_truenas_install(cfg: dict, upd: str, res: dict) -> None:
    sys.path.insert(0, upd)
    import truenas_install.__main__ as ti  # the update image's own installer

    orig = ti.run_command

    def wrapped(cmd, **kw):
        flat = " ".join(map(str, cmd))
        if any(s in flat for s in SKIP):
            log(f"SKIP (no GRUB on this box): {flat}")
            res.setdefault("skipped", []).append(flat)
            return subprocess.CompletedProcess(cmd, 0, "", "")
        if str(cmd[0]).endswith("truenas-initrd.py"):
            # As called it runs on the LIVE env's python, which lacks libzfs/pyudev: ImportError,
            # rc 1, which the installer accepts - no initrd rebuilt. Run it in the new root instead.
            root = cmd[-1]
            script = os.path.join(root, "usr/local/bin/truenas-initrd.py")
            if not os.path.exists(
                script
            ):  # rc 127 would otherwise be capped to the tolerated 1
                raise RuntimeError(
                    f"{script} missing from the image (package truenas-initrd, #362)"
                )
            kver = res["kver"] = inject_kernel(cfg, root)
            r = run(
                ["chroot", root, "/usr/local/bin/truenas-initrd.py", "-f", "/"],
                check=False,
            )
            if not os.path.exists(os.path.join(root, f"boot/initrd.img-{kver}")):
                run(["chroot", root, "update-initramfs", "-c", "-k", kver])
            res["truenas_initrd_rc"] = r.returncode
            if r.returncode == 127:
                raise RuntimeError(
                    "chroot truenas-initrd.py: rc 127, command not found (#362)"
                )
            return subprocess.CompletedProcess(
                cmd, min(r.returncode, 1), r.stdout, r.stderr
            )
        log("installer: " + flat)
        t = time.monotonic()
        r = orig(cmd, **kw)
        log(f"installer: rc={r.returncode} in {time.monotonic() - t:.0f}s")
        return r

    ti.run_command = wrapped
    params = {
        "authentication_method": {
            "username": "truenas_admin",
            "password": cfg["admin_password"],
        },
        "disks": [],
        "json": True,
        "pool_name": POOL,
        "post_install": None,
        "sql": cfg["sql"],
        "src": upd,
    }
    stdin, sys.stdin = sys.stdin, io.StringIO(json.dumps(params))
    cwd = os.getcwd()
    os.chdir(upd)
    try:
        ti.main()
    finally:
        sys.stdin = stdin
        os.chdir(cwd)


def collect(cfg: dict, res: dict) -> None:
    kver, out = res["kver"], os.path.join(cfg["job_dir"], "out")
    bootfs = run(["zpool", "get", "-H", "-o", "value", "bootfs", POOL]).stdout.strip()
    r = "/tmp/newroot"
    os.makedirs(r, exist_ok=True)
    run(["mount", "-t", "zfs", "-o", "ro", bootfs, r])  # root ds: mountpoint=legacy
    try:
        os.makedirs(out, exist_ok=True)
        src = os.path.join(r, f"boot/initrd.img-{kver}")
        shutil.copyfile(src, os.path.join(out, f"initrd.img-{kver}"))
        shutil.copyfile(
            os.path.join(r, f"boot/vmlinuz-{kver}"),
            os.path.join(out, f"vmlinuz-{kver}"),
        )
        boots = sorted(os.listdir(os.path.join(r, "boot")))
    finally:
        run(["umount", r], check=False)
    info = {
        "bootfs": bootfs,
        "kver": kver,
        "initrd": f"initrd.img-{kver}",
        "initrd_sha256": sha256(os.path.join(out, f"initrd.img-{kver}")),
        "vmlinuz": f"vmlinuz-{kver}",
        "vmlinuz_sha256": sha256(os.path.join(out, f"vmlinuz-{kver}")),
        "root_boot": boots,
        "datasets": run(
            ["zfs", "list", "-H", "-o", "name,mountpoint,readonly", "-r", POOL]
        ).stdout.splitlines(),
        "pool_guid": run(
            ["zpool", "get", "-H", "-o", "value", "guid", POOL]
        ).stdout.strip(),
    }
    with open(os.path.join(out, "info.json"), "w") as f:
        json.dump(info, f, indent=1)
    res["info"] = info
    log(f"collected: bootfs {bootfs}, initrd {info['initrd_sha256'][:16]}")


def rehearse(cfg: dict, root: str) -> int:
    """Host: the installer's chroot steps + ours on an extracted rootfs, no zfs."""
    ch = ("chroot",)
    run(
        [*ch, root, "/usr/local/bin/truenas-set-authentication-method.py"],
        input=json.dumps(
            {"username": "truenas_admin", "password": cfg["admin_password"]}
        ),
    )
    run([*ch, root, "sqlite3", "/data/freenas-v1.db"], input=cfg["sql"])
    run(
        [*ch, root, "/usr/local/bin/truenas-autotune.py", "--skip-unknown"], check=False
    )
    run([*ch, root, "/usr/local/bin/truenas-nvdimm.py"])
    run([*ch, root, "/usr/local/bin/truenas-grub.py"])
    kver = inject_kernel(cfg, root, ch)
    run([*ch, root, "update-initramfs", "-c", "-k", kver])
    run([*ch, root, "/usr/bin/configure_fips"])
    log("rehearsal OK")
    return 0


def main() -> int:
    if sys.argv[1] == "--rehearse-root":
        with open(sys.argv[3]) as f:
            return rehearse(json.load(f), sys.argv[2])
    with open(sys.argv[1]) as f:
        cfg = json.load(f)
    set_clock(sys.argv[1])
    res: dict = {"ok": False, "started": time.strftime("%Y-%m-%dT%H:%M:%S%z")}
    try:
        install(cfg, res)
        res["ok"] = True
    except BaseException as e:  # noqa: BLE001 - every failure must land in result.json
        res["error"] = f"{type(e).__name__}: {e}"
        res["traceback"] = traceback.format_exc()
        log(res["traceback"])
    finally:
        res["elapsed_s"] = round(time.monotonic() - T0)
        with open(os.path.join(cfg["job_dir"], "result.json"), "w") as f:
            json.dump(res, f, indent=1)
        if os.path.exists(sys.argv[1]):  # holds the admin password
            os.unlink(sys.argv[1])
        log(f"result ok={res['ok']} in {res['elapsed_s']}s")
    return 0 if res["ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
