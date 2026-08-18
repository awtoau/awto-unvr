#!/usr/bin/env python3
"""
Stage the awto-unvr modern-U-Boot board scaffold into a U-Boot source tree,
patch arch/arm/Kconfig, and build the Stage-1 (chainload) console image.

Scaffold source of truth: uboot-port/  (tracked in this repo).
Default target tree: /mnt/2tb/unvr-port-refs/u-boot-v2026.07 (shared ref).

Usage:
  scripts/uboot-build.py            # stage + configure + build, log to tmp/logs
  scripts/uboot-build.py --clean    # distclean build + un-stage + revert Kconfig

Disk is tight (~40 GB free). Build goes to an out-of-tree O= dir under tmp/ so a
single rm reclaims it; --clean also runs distclean and removes staged sources.
Timeout: build wall-clock cap = 900 s (~1.25x an observed worst-case full arm64
U-Boot build of ~12 min on this host); on expiry the make is killed and logged.
"""
import os
import shutil
import subprocess
import sys
import time

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SCAFFOLD = os.path.join(REPO, "uboot-port")
TREE = os.environ.get("UBOOT_TREE", "/mnt/2tb/unvr-port-refs/u-boot-v2026.07")
BUILDDIR = os.path.join(REPO, "tmp", "uboot-build")
LOG = os.path.join(REPO, "tmp", "logs", "uboot-port.log")
CROSS = "aarch64-linux-gnu-"
BUILD_TIMEOUT_S = 900  # see module docstring

# scaffold rel-path -> tree rel-path
FILES = {
    "board/annapurna/alpine/Kconfig":      "board/annapurna/alpine/Kconfig",
    "board/annapurna/alpine/Makefile":     "board/annapurna/alpine/Makefile",
    "board/annapurna/alpine/MAINTAINERS":  "board/annapurna/alpine/MAINTAINERS",
    "board/annapurna/alpine/alpine.c":     "board/annapurna/alpine/alpine.c",
    "board/annapurna/alpine/pl061.c":      "board/annapurna/alpine/pl061.c",
    "include/configs/alpine.h":            "include/configs/alpine.h",
    "configs/alpine_v2_unvr_defconfig":    "configs/alpine_v2_unvr_defconfig",
    "arch/dts/awto-alpine-v2-unvr-uboot.dts": "arch/arm/dts/awto-alpine-v2-unvr-uboot.dts",
}

KCONFIG = os.path.join(TREE, "arch/arm/Kconfig")
TARGET_BLOCK = """
config TARGET_ALPINE_V2_UNVR
	bool "Support Ubiquiti UNVR (Annapurna Labs Alpine V2 / AL-324)"
	select ARM64
	select DM
	select DM_SERIAL
	select OF_CONTROL
"""
TARGET_ANCHOR = "config TARGET_LS2080A_EMU"
SRC_LINE = 'source "board/annapurna/alpine/Kconfig"\n'
SRC_ANCHOR = 'source "board/armltd/total_compute/Kconfig"\n'


def log(msg):
    line = f"[{time.strftime('%Y-%m-%dT%H:%M:%S%z')}] {msg}"
    print(line, flush=True)
    with open(LOG, "a") as f:
        f.write(line + "\n")


def stage():
    for src, dst in FILES.items():
        s = os.path.join(SCAFFOLD, src)
        d = os.path.join(TREE, dst)
        os.makedirs(os.path.dirname(d), exist_ok=True)
        shutil.copy2(s, d)
        log(f"staged {dst}")
    txt = open(KCONFIG).read()
    changed = False
    if "TARGET_ALPINE_V2_UNVR" not in txt:
        txt = txt.replace(TARGET_ANCHOR, TARGET_BLOCK.lstrip("\n") + "\n" + TARGET_ANCHOR, 1)
        changed = True
    if SRC_LINE not in txt:
        txt = txt.replace(SRC_ANCHOR, SRC_ANCHOR + SRC_LINE, 1)
        changed = True
    if changed:
        open(KCONFIG, "w").write(txt)
        log("patched arch/arm/Kconfig (TARGET + board source)")


def unstage():
    txt = open(KCONFIG).read()
    txt = txt.replace(TARGET_BLOCK.lstrip("\n") + "\n", "")
    txt = txt.replace(SRC_LINE, "")
    open(KCONFIG, "w").write(txt)
    log("reverted arch/arm/Kconfig")
    board = os.path.join(TREE, "board/annapurna")
    if os.path.isdir(board):
        shutil.rmtree(board)
    for dst in ("include/configs/alpine.h", "configs/alpine_v2_unvr_defconfig",
                "arch/arm/dts/awto-alpine-v2-unvr-uboot.dts"):
        p = os.path.join(TREE, dst)
        if os.path.exists(p):
            os.remove(p)
    log("removed staged source files")


def run(cmd):
    env = dict(os.environ, CROSS_COMPILE=CROSS, ARCH="arm")
    log("run: " + " ".join(cmd))
    with open(LOG, "a") as f:
        t0 = time.time()
        try:
            p = subprocess.run(cmd, cwd=TREE, env=env, stdout=f,
                               stderr=subprocess.STDOUT, timeout=BUILD_TIMEOUT_S)
        except subprocess.TimeoutExpired:
            log(f"TIMEOUT after {BUILD_TIMEOUT_S}s: {' '.join(cmd)}")
            return 124
    log(f"exit {p.returncode} ({time.time()-t0:.0f}s)")
    return p.returncode


def build():
    os.makedirs(BUILDDIR, exist_ok=True)
    o = f"O={BUILDDIR}"
    if run(["make", o, "alpine_v2_unvr_defconfig"]) != 0:
        log("defconfig FAILED"); return 1
    nproc = str(os.cpu_count() or 4)
    rc = run(["make", o, "-j" + nproc])
    binp = os.path.join(BUILDDIR, "u-boot.bin")
    if rc == 0 and os.path.exists(binp):
        log(f"BUILD OK: u-boot.bin = {os.path.getsize(binp)} bytes")
    else:
        log("BUILD FAILED (see log tail)")
    return rc


def clean():
    if os.path.isdir(BUILDDIR):
        run(["make", f"O={BUILDDIR}", "distclean"])
        shutil.rmtree(BUILDDIR, ignore_errors=True)
        log("removed build dir")
    unstage()


if __name__ == "__main__":
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    if "--clean" in sys.argv:
        clean()
    else:
        stage()
        sys.exit(build())
