#!/usr/bin/env python3
"""Build the ea16 netboot installer image against AWTO_KERNEL_SRC.

Cross-compiled on the HOST (no docker) with aarch64-linux-gnu-gcc.
Netboot-ready: legacy uImage (initramfs embedded) + ea16 DTB. No flashing.

Board support (DTS, pcie-al-internal.c, the pcie-al.c DBI fix, unvr_defconfig)
lives as real commits in AWTO_KERNEL_SRC's own git history now, not as
patches applied by this script - nothing here needs to stage or integrate
them. This script's only job is the netboot installer artifact:
deploy-fedora-rootfs uses it to bootstrap a completely bare disk (no
existing rootfs) before formatting and streaming the real Fedora rootfs.
ram-boot-deploy can't substitute for that - it still needs root=PARTUUID=...
pointing at something already on disk.

Toolchain: host aarch64-linux-gnu-gcc. WERROR disabled defensively.

Order: configure -> `make modules` (builds vmlinux + Module.symvers) ->
out-of-tree al_* -> install into initramfs -> `make Image dtbs` (embeds the
now-populated initramfs) -> pure-python uImage wrap.

Out: /mnt/2tb/unvr-port-refs/build-out-ea16/
"""

import os
import pathlib
import re
import shutil
import struct
import subprocess
import sys
import time
import zlib

import _repo
from _repo import (
    NPROC,
    REPO,
    check_kernel_patches,
    ea16_build_out,
    kernel_build_ver,
)  # -j28 host build parallelism (#146); self-locating repo root

SRC = os.environ.get("AWTO_KERNEL_SRC", "/mnt/2tb/unvr-port-refs/linux-v7.3-fresh")
# _repo.py's REPO self-locates via git rev-parse, correct wherever this
# script runs from.
OUT = str(ea16_build_out())
# kbuild's own O= output dir, independent of SRC's own in-tree state (a
# build-fedora run against the same SRC needs this to not collide).
KOUT = os.path.join(OUT, "kbuild")
INITRAMFS_BASE = "/mnt/2tb/unvr-port-refs/build-out"  # base busybox initramfs skeleton, kernel-version-agnostic
DTS_NAME = "alpine-v2-ubnt-unvr-ea16"
VER = kernel_build_ver()  # baked into output filenames/uImage Image Name
# Empty string for a native build (e.g. running this script ON woomera itself).
CROSS = os.environ.get("AWTO_CROSS_COMPILE", "aarch64-linux-gnu-")
MIN_FREE_GB = 15

IH_MAGIC, IH_OS_LINUX, IH_ARCH_ARM64, IH_TYPE_KERNEL, IH_COMP_NONE = (
    0x27051956,
    5,
    22,
    2,
    0,
)
LOAD_ADDR = ENTRY_ADDR = 0x08000000

# Production mode: no embedded initramfs, so the kernel runs /sbin/init (systemd)
# from the real root (root=PARTUUID=..., kernel's own rootwait waits for SATA).
# The bring-up initramfs would otherwise hijack the boot into a shell. Output is
# suffixed so it never clobbers the installer uImage. See docs/fedora-on-ssd.md.
# (Audit #43 kernel-config adds are a later rebuild; this is the minimal change
# to get Fedora booting.)
PROD = ("--production" in sys.argv) or os.environ.get("UNVR_PRODUCTION") == "1"
SUFFIX = "-prod" if PROD else ""

ENV = dict(os.environ, ARCH="arm64", CROSS_COMPILE=CROSS)


def log(msg):
    print(msg, flush=True)


def run(cmd, **kw):
    _repo.run(cmd, log=log, env=ENV, **kw)


def check_space():
    # Was hardcoded to /mnt/2tb (dev host's data mount) - check OUT's own
    # mount instead, correct wherever this actually runs.
    st = os.statvfs(OUT if os.path.isdir(OUT) else os.path.dirname(OUT) or ".")
    free_gb = st.f_bavail * st.f_frsize / 1e9
    log(f"free space at {OUT}: {free_gb:.1f} GB")
    if free_gb < MIN_FREE_GB:
        log(f"ABORT: <{MIN_FREE_GB} GB free")
        sys.exit(2)


def prep_initramfs():
    """Reuse the 6.12 initramfs content, minus its lib/modules; refresh banner."""
    dst = os.path.join(OUT, "initramfs-root")
    if os.path.exists(dst):
        shutil.rmtree(dst)
    # symlinks=True is REQUIRED: the alpine rootfs is hundreds of busybox
    # symlinks; dereferencing them balloons 11 MB -> 400+ MB (and the Image).
    shutil.copytree(
        os.path.join(INITRAMFS_BASE, "initramfs-root"),
        dst,
        symlinks=True,
        ignore=shutil.ignore_patterns("modules"),
    )
    # copytree above keeps lib/modules/<ver> dirs; drop the whole lib/modules.
    libmod = os.path.join(dst, "lib/modules")
    if os.path.exists(libmod):
        shutil.rmtree(libmod)
    # refresh the cosmetic banner (module loading itself is $(uname -r)-driven).
    # Matches whatever version the reused initramfs skeleton was last stamped
    # with (this was an enumerated list of stale hardcoded strings - drifted
    # every kernel migration since it was written) and replaces it with the
    # actual VER this build is for.
    init = os.path.join(dst, "init")
    it = pathlib.Path(init).read_text()
    it = re.sub(r"Linux \S+ first boot", f"Linux {VER} first boot", it)
    pathlib.Path(init).write_text(it)
    # devnode list (real /dev/console for PID1 controlling tty).
    shutil.copy(
        os.path.join(INITRAMFS_BASE, "initramfs-devnodes"),
        os.path.join(OUT, "initramfs-devnodes"),
    )
    # Fail fast on a container marker: this skeleton is copied into every ea16
    # image, and one left by the old podman build propagated for weeks (#164,
    # #217). PID1 reads it before /run's tmpfs mounts and caches "container"
    # for life, disabling .swap units and ConditionVirtualization=!container.
    for marker in (".dockerenv", ".containerenv", "run/.containerenv"):
        for base, label in ((INITRAMFS_BASE, "skeleton"), (OUT, "build")):
            m = os.path.join(base, "initramfs-root", marker)
            if os.path.exists(m):
                log(
                    f"FATAL: container marker in the initramfs {label}: {m} (#164)",
                    "ERROR",
                )
                sys.exit(1)
    log("prepped initramfs-root (reused 6.12, modules cleared)")


def configure():
    os.makedirs(KOUT, exist_ok=True)
    run(["make", "-C", SRC, f"O={KOUT}", "unvr_defconfig"])
    cfg = os.path.join(SRC, "scripts/config")
    args = [
        cfg,
        "--file",
        os.path.join(KOUT, ".config"),
        "--disable",
        "WERROR",
        "--disable",
        "TRIM_UNUSED_KSYMS",
        "--enable",
        "PCIE_AL",
        "--enable",
        "PCIE_AL_INTERNAL",
        "--enable",
        "EXPERT",
        "--enable",
        "GPIO_SYSFS",
        # unrestricted /dev/mem for SoC MMIO poking from userspace (DDR/reg RE)
        "--enable",
        "DEVMEM",
        "--disable",
        "STRICT_DEVMEM",
        "--disable",
        "IO_STRICT_DEVMEM",
        # 4K-sector erase so the 4K-aligned NOR config/cksum partitions aren't
        # force-read-only (they don't align to 64K erase blocks).
        "--enable",
        "MTD_SPI_NOR_USE_4K_SECTORS",
    ]
    if PROD:
        # Production: no embedded initramfs -> kernel runs systemd from
        # root=PARTUUID. Add the exact Fedora/systemd-required symbols the audit
        # found missing (docs/improvements-audit.md dim 2) + cheap wins, so stock
        # Fedora boots with nothing degraded.
        args += [
            "--set-str",
            "INITRAMFS_SOURCE",
            "",
            "--enable",
            "PSI",
            "--enable",
            "CGROUP_BPF",
            "--enable",
            "BPF_JIT",
            "--module",
            "BINFMT_MISC",
            "--enable",
            "HUGETLBFS",
            "--enable",
            "CRYPTO_SHA1_ARM64_CE",
            "--enable",
            "CRYPTO_SHA2_ARM64_CE",
            "--enable",
            "CRYPTO_SHA512_ARM64_CE",
            "--enable",
            "XFS_FS",
            "--enable",
            "BTRFS_FS_POSIX_ACL",
            "--enable",
            "ZRAM",
            "--enable",
            "SECURITY_YAMA",
            "--enable",
            "SLAB_FREELIST_HARDENED",
        ]
    else:
        initramfs = f"{OUT}/initramfs-root {OUT}/initramfs-devnodes"
        args += [
            "--enable",
            "BLK_DEV_INITRD",
            "--set-str",
            "INITRAMFS_SOURCE",
            initramfs,
            "--set-val",
            "INITRAMFS_ROOT_UID",
            "0",
            "--set-val",
            "INITRAMFS_ROOT_GID",
            "0",
            "--enable",
            "RD_GZIP",
            "--enable",
            "INITRAMFS_COMPRESSION_GZIP",
        ]
    run(args)
    run(["make", "-C", SRC, f"O={KOUT}", "olddefconfig"])
    dotcfg = pathlib.Path(os.path.join(KOUT, ".config")).read_text()
    # AHCI_ALPINE comes from unvr_defconfig; check it survived olddefconfig -
    # its absence means patches/ahci-alpine-per-port-msix.patch isn't applied
    # in AWTO_KERNEL_SRC and SATA is silently back on shared INTx (#92).
    for sym in (
        "CONFIG_PCIE_AL_INTERNAL=y",
        "CONFIG_PCIE_AL=y",
        "CONFIG_AHCI_ALPINE=y",
    ):
        if sym not in dotcfg:
            log(f"FATAL: {sym} not set after olddefconfig")
            sys.exit(1)
    # Patches with no config symbol of their own - our DTS sets
    # snps,no-enable-abort, so a tree that ignores it wedges the pld bus on
    # the box (cold-power-cycle-only recovery) rather than failing here.
    check_kernel_patches(log)
    log("configured: PCIE_AL + PCIE_AL_INTERNAL confirmed =y")


def kver():
    return (
        subprocess.check_output(
            ["make", "-s", "-C", SRC, f"O={KOUT}", "kernelrelease"], env=ENV
        )
        .decode()
        .strip()
    )


def build_modules_intree():
    run(["make", "-C", SRC, f"O={KOUT}", f"-j{NPROC}", "modules"])


def build_oot_modules():
    """Build al_eth/al_dma/al_ssm/al_sgpo/al_thermal against 7.1. Non-fatal per
    module so a module API break does not deny us a bootable kernel; failures
    are reported."""
    kv = kver()
    mdst = os.path.join(OUT, "modules")
    if os.path.exists(mdst):
        shutil.rmtree(mdst)
    os.makedirs(mdst)
    results = {}
    for m in ("al_eth", "al_dma", "al_ssm", "al_sgpo", "al_thermal"):
        mpath = os.path.join(mdst, m)
        # imported source-of-truth: repo modules/ (carries iofic + crypto fixes).
        shutil.copytree(os.path.join(REPO, "modules", m), mpath)
        log(f"=== out-of-tree {m} ===")
        try:
            subprocess.run(
                [
                    "make",
                    "-C",
                    KOUT,
                    f"M={mpath}",
                    f"-j{NPROC}",
                    "modules",
                ],
                check=True,
                env=ENV,
            )
            results[m] = os.path.exists(os.path.join(mpath, f"{m}.ko"))
        except subprocess.CalledProcessError as e:
            log(f"MODULE BUILD FAILED: {m} ({e})")
            results[m] = False
    # install the ones that built into the embedded initramfs.
    extra = os.path.join(OUT, f"initramfs-root/lib/modules/{kv}/extra")
    os.makedirs(extra, exist_ok=True)
    for m, ok in results.items():
        if ok:
            shutil.copy(os.path.join(mdst, m, f"{m}.ko"), extra)
    # module metadata for modprobe.
    mroot = os.path.join(OUT, f"initramfs-root/lib/modules/{kv}")
    for meta in ("modules.builtin", "modules.order", "modules.builtin.modinfo"):
        s = os.path.join(KOUT, meta)
        if os.path.exists(s):
            shutil.copy(s, mroot)
    subprocess.run(
        ["depmod", "-b", os.path.join(OUT, "initramfs-root"), kv], check=False
    )
    log(
        "out-of-tree module results: "
        + ", ".join(f"{m}={'ok' if ok else 'FAIL'}" for m, ok in results.items())
    )
    return results


def build_image():
    run(["make", "-C", SRC, f"O={KOUT}", f"-j{NPROC}", "Image", "dtbs"])


def mkuimage(image_path, out_path, name=f"unvr-ea16-{VER}"):
    data = pathlib.Path(image_path).read_bytes()
    dcrc = zlib.crc32(data) & 0xFFFFFFFF
    nm = name.encode()[:31].ljust(32, b"\0")

    def hdr(hc):
        return struct.pack(
            ">IIIIIIIBBBB32s",
            IH_MAGIC,
            hc,
            int(time.time()),
            len(data),
            LOAD_ADDR,
            ENTRY_ADDR,
            dcrc,
            IH_OS_LINUX,
            IH_ARCH_ARM64,
            IH_TYPE_KERNEL,
            IH_COMP_NONE,
            nm,
        )

    h = hdr(zlib.crc32(hdr(0)) & 0xFFFFFFFF)
    pathlib.Path(out_path).write_bytes(h + data)
    log(
        f"uImage: {out_path} ({len(h) + len(data)} bytes, "
        f"load/entry=0x{LOAD_ADDR:08x}, dcrc=0x{dcrc:08x})"
    )


def regen_initramfs_cpio():
    work = os.path.join(OUT, "initramfs-root")
    cpio_gz = os.path.join(OUT, f"initramfs-ea16-{VER}.cpio.gz")
    find = subprocess.Popen(["find", "."], cwd=work, stdout=subprocess.PIPE)
    cpio = subprocess.Popen(
        ["cpio", "-o", "-H", "newc", "--quiet"],
        cwd=work,
        stdin=find.stdout,
        stdout=subprocess.PIPE,
    )
    with open(cpio_gz, "wb") as f:
        gz = subprocess.Popen(["gzip", "-9"], stdin=cpio.stdout, stdout=f)
        gz.communicate()
    find.wait()
    cpio.wait()
    log(f"standalone initramfs: {cpio_gz} ({os.path.getsize(cpio_gz) // 1024} KiB)")


def collect():
    image = os.path.join(KOUT, "arch/arm64/boot/Image")
    dtb = os.path.join(KOUT, f"arch/arm64/boot/dts/amazon/{DTS_NAME}.dtb")
    shutil.copy(image, os.path.join(OUT, "Image"))
    shutil.copy(dtb, os.path.join(OUT, f"{DTS_NAME}-{VER}.dtb"))
    shutil.copy(
        os.path.join(KOUT, ".config"), os.path.join(OUT, f"unvr-ea16-{VER}.config")
    )
    mkuimage(image, os.path.join(OUT, f"uImage-unvr-ea16-{VER}"))
    regen_initramfs_cpio()
    for root, _, files in os.walk(os.path.join(OUT, "modules")):
        for f in files:
            if f.endswith(".ko"):
                p = os.path.join(root, f)
                log(f"  module {os.path.basename(p)}  ({os.path.getsize(p)} bytes)")


def main():
    t0 = time.time()
    check_space()
    prep_initramfs()
    configure()
    build_modules_intree()
    check_space()
    build_oot_modules()
    build_image()
    collect()
    check_space()
    log(f"DONE in {int(time.time() - t0)}s (KVER={kver()})")


if __name__ == "__main__":
    sys.exit(main())
