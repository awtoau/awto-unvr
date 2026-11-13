# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Kernel-build pieces shared by build-linux-fedora.py (7.x) and build-linux-truenas.py (6.18).

Each caller passes its own run(), SRC (kernel tree), KOUT (kbuild O= dir) and OUT.
"""

from __future__ import annotations

import os
import pathlib
import shutil
import struct
import subprocess
import sys
import time
import zlib

from _repo import NPROC, REPO

DTS_NAME = "alpine-v2-ubnt-unvr-ea16"
CROSS = os.environ.get(
    "AWTO_CROSS_COMPILE", "aarch64-linux-gnu-"
)  # "" for a native build
CC_ARG = [f"CC=ccache {CROSS}gcc"] if shutil.which("ccache") else []
ENV = dict(os.environ, ARCH="arm64", CROSS_COMPILE=CROSS)

# al_eth is two .ko's (#131: one per PCI ID, so udev's per-device modprobes never race).
# al_reboot: the only working reset on AL-324 (#51). ubnt_hdd_pwrctl, al_nand: #208.
OOT_MODULES = (
    "al_eth",
    "al_dma",
    "al_ssm",
    "al_sgpo",
    "al_thermal",
    "al_reboot",
    "ubnt_hdd_pwrctl",
    "al_nand",
)

IH_MAGIC, IH_OS_LINUX, IH_ARCH_ARM64, IH_TYPE_KERNEL, IH_COMP_NONE = (
    0x27051956,
    5,
    22,
    2,
    0,
)
LOAD_ADDR = ENTRY_ADDR = 0x08000000


def kver(kout: str) -> str:
    """kbuild's own include/config/kernel.release; `make kernelrelease` has returned a
    previous variant's release here before."""
    path = os.path.join(kout, "include/config/kernel.release")
    if not os.path.exists(path):
        sys.exit(f"FATAL: {path} missing - build the kernel before calling kver()")
    return pathlib.Path(path).read_text().strip()


def check_dts_shared(log) -> None:
    """Shared i2c timing facts must agree between the two DTS trees (#75)."""
    chk = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "check-dts-shared.py"
    )
    if subprocess.run([sys.executable, chk], check=False).returncode:
        log("ABORT: DTS shared-fact check failed")
        sys.exit(1)


def stage_dts(src: str, log, port: str | None = None) -> None:
    """Repo dts/ (board DTS + alpine-v2.dtsi) over the tree: dts/ is the source of truth.
    Reference boards (unvr, udmpro) from `port` when given."""
    amazon = os.path.join(src, "arch/arm64/boot/dts/amazon")
    staged = []
    for f in sorted(os.listdir(os.path.join(REPO, "dts"))):
        if f.endswith((".dts", ".dtsi")):
            shutil.copy(os.path.join(REPO, "dts", f), amazon)
            staged.append(f)
    for f in ("dts/alpine-v2-ubnt-unvr.dts", "dts/alpine-v2-ubnt-udmpro.dts"):
        s = os.path.join(port, f) if port else ""
        if s and os.path.exists(s):
            shutil.copy(s, amazon)
    log(f"staged DTS/DTSI from repo dts/: {', '.join(staged)}")


def build_oot_modules(run, log, kout: str, out: str, extra: str, sign=None) -> None:
    """Build OOT_MODULES with M= against KOUT (the kbuild dir is $KDIR for an O= build),
    copy each .ko into `extra`. `sign(ko_path)` is called per .ko when given."""
    os.makedirs(extra, exist_ok=True)

    def one(m, mpath, ko_name, make_vars=()):
        if os.path.exists(mpath):
            shutil.rmtree(mpath)
        shutil.copytree(os.path.join(REPO, "modules", m), mpath)
        run(
            [
                "make",
                "-C",
                kout,
                f"M={mpath}",
                *CC_ARG,
                *make_vars,
                f"-j{NPROC}",
                "modules",
            ]
        )
        ko = os.path.join(mpath, f"{ko_name}.ko")
        if sign:
            sign(ko)
        shutil.copy(ko, extra)
        # A run once printed ok with extra/ empty; cause never found, so check.
        if not os.path.exists(os.path.join(extra, f"{ko_name}.ko")):
            log(f"FATAL: {extra}/{ko_name}.ko missing right after copying it")
            sys.exit(1)
        log(f"OOT {ko_name}: ok")

    for m in OOT_MODULES:
        if m == "al_eth":
            for v in ("1g", "10g"):
                one(
                    m,
                    os.path.join(out, f"al_eth_{v}"),
                    f"al_eth_{v}",
                    [f"MODULE_VARIANT={v}"],
                )
        else:
            one(m, os.path.join(out, m), m)


def mkuimage(image_path: str, out_path: str, name: str, log) -> None:
    """Legacy uImage (U-Boot `bootm`), uncompressed arm64 Image, 32-byte name field."""
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
        f"uImage: {out_path} ({len(h) + len(data)} bytes, load/entry=0x{LOAD_ADDR:08x})"
    )
