#!/usr/bin/env python3
"""
Stage the awto-unvr modern-U-Boot board scaffold + the three integrated driver
ports (DDR, al_eth 1G, al_serdes 25G) into a U-Boot source tree, patch the
tree's Kconfig/Makefile hooks, and build the Stage-1 (chainload) console image.

Scaffold source of truth: uboot-port/  (tracked in this repo).
Default target tree: /mnt/2tb/unvr-port-refs/u-boot-v2026.07 (shared ref).

Single-source layout (no per-driver plat_api duplication):
  drivers/net/al_hal_shim/       ONE shared plat_api shim + common HAL headers
                                 (include-only, no Makefile; each driver's
                                 ccflags adds -I to it).
  board/annapurna/alpine/al_ddr/ DDR HAL subset + `ddr` cmd (board Makefile).
  drivers/net/al_eth/            1G RGMII DM_ETH driver + curated HAL subset.
  drivers/phy/al_serdes/         25G/10GBASE-R SerDes HAL + `serdes` cmd.

Usage:
  scripts/uboot-build.py            # stage + configure + build, log to tmp/logs
  scripts/uboot-build.py --clean    # distclean build + un-stage + revert hooks

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

# scaffold rel-path -> tree rel-path (individual files)
FILES = {
    "board/annapurna/alpine/Kconfig": "board/annapurna/alpine/Kconfig",
    "board/annapurna/alpine/Makefile": "board/annapurna/alpine/Makefile",
    "board/annapurna/alpine/MAINTAINERS": "board/annapurna/alpine/MAINTAINERS",
    "board/annapurna/alpine/alpine.c": "board/annapurna/alpine/alpine.c",
    "board/annapurna/alpine/pl061.c": "board/annapurna/alpine/pl061.c",
    "include/configs/alpine.h": "include/configs/alpine.h",
    "configs/alpine_v2_unvr_defconfig": "configs/alpine_v2_unvr_defconfig",
    "arch/dts/awto-alpine-v2-unvr-uboot.dts": "arch/arm/dts/awto-alpine-v2-unvr-uboot.dts",
    # DW i2c: patched to honor explicit raw SCL hcnt/lcnt from DT so we can pin
    # stock's proven s35390a-safe pld-bus timing (docs/rtc-s35390a-fault.md).
    "drivers/i2c/designware_i2c.c": "drivers/i2c/designware_i2c.c",
    "drivers/i2c/designware_i2c.h": "drivers/i2c/designware_i2c.h",
}

# Whole subtrees copied verbatim: scaffold rel-dir -> tree rel-dir.
# al_hal_shim is include-only (no Makefile) — it is not descended into by kbuild;
# each driver's ccflags -I resolves the shared plat shim + common HAL headers.
DIRS = {
    "drivers/net/al_hal_shim": "drivers/net/al_hal_shim",
    "board/annapurna/alpine/al_ddr": "board/annapurna/alpine/al_ddr",
    "drivers/net/al_eth": "drivers/net/al_eth",
    "drivers/phy/al_serdes": "drivers/phy/al_serdes",
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

# al_eth wiring: drivers/net/{Makefile,Kconfig}.
NET_MAKEFILE = os.path.join(TREE, "drivers/net/Makefile")
NET_MK_LINE = "obj-$(CONFIG_AL_ETH) += al_eth/\n"
NET_KCONFIG = os.path.join(TREE, "drivers/net/Kconfig")
NET_KC_LINE = 'source "drivers/net/al_eth/Kconfig"\n'
NET_KC_ANCHOR = "endif # NETDEVICES\n"

# al_serdes wiring: built via drivers/Makefile (NOT drivers/phy/Makefile, which
# is only descended into when CONFIG_PHY=y — we don't need the PHY uclass). The
# Kconfig is sourced from drivers/phy/Kconfig (always sourced from drivers/Kconfig).
DRIVERS_MAKEFILE = os.path.join(TREE, "drivers/Makefile")
DRIVERS_MAKE_LINE = "obj-$(CONFIG_AL_SERDES) += phy/al_serdes/\n"
PHY_KCONFIG = os.path.join(TREE, "drivers/phy/Kconfig")
PHY_KCONFIG_LINE = 'source "drivers/phy/al_serdes/Kconfig"\n'


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
    for src, dst in DIRS.items():
        s = os.path.join(SCAFFOLD, src)
        d = os.path.join(TREE, dst)
        if os.path.isdir(d):
            shutil.rmtree(d)
        shutil.copytree(s, d)
        log(f"staged {dst}/ ({sum(len(f) for _, _, f in os.walk(s))} files)")

    # arch/arm/Kconfig — board TARGET + board Kconfig source
    txt = open(KCONFIG).read()
    changed = False
    if "TARGET_ALPINE_V2_UNVR" not in txt:
        txt = txt.replace(
            TARGET_ANCHOR, TARGET_BLOCK.lstrip("\n") + "\n" + TARGET_ANCHOR, 1
        )
        changed = True
    if SRC_LINE not in txt:
        txt = txt.replace(SRC_ANCHOR, SRC_ANCHOR + SRC_LINE, 1)
        changed = True
    if changed:
        open(KCONFIG, "w").write(txt)
        log("patched arch/arm/Kconfig (TARGET + board source)")

    # drivers/net Makefile + Kconfig hooks for al_eth
    mk = open(NET_MAKEFILE).read()
    if NET_MK_LINE not in mk:
        open(NET_MAKEFILE, "a").write(NET_MK_LINE)
        log("patched drivers/net/Makefile (al_eth obj)")
    kc = open(NET_KCONFIG).read()
    if NET_KC_LINE not in kc:
        kc = kc.replace(NET_KC_ANCHOR, NET_KC_LINE + "\n" + NET_KC_ANCHOR, 1)
        open(NET_KCONFIG, "w").write(kc)
        log("patched drivers/net/Kconfig (al_eth source)")

    # drivers/Makefile + drivers/phy/Kconfig hooks for al_serdes
    dm = open(DRIVERS_MAKEFILE).read()
    if DRIVERS_MAKE_LINE not in dm:
        open(DRIVERS_MAKEFILE, "w").write(dm.rstrip("\n") + "\n" + DRIVERS_MAKE_LINE)
        log("patched drivers/Makefile (phy/al_serdes/)")
    pk = open(PHY_KCONFIG).read()
    if PHY_KCONFIG_LINE not in pk:
        idx = pk.rfind("endmenu")
        pk = (
            (pk + "\n" + PHY_KCONFIG_LINE)
            if idx < 0
            else pk[:idx] + PHY_KCONFIG_LINE + "\n" + pk[idx:]
        )
        open(PHY_KCONFIG, "w").write(pk)
        log("patched drivers/phy/Kconfig (source al_serdes/Kconfig)")


def unstage():
    txt = open(KCONFIG).read()
    txt = txt.replace(TARGET_BLOCK.lstrip("\n") + "\n", "")
    txt = txt.replace(SRC_LINE, "")
    open(KCONFIG, "w").write(txt)
    log("reverted arch/arm/Kconfig")

    # revert al_eth wiring
    mk = open(NET_MAKEFILE).read()
    if NET_MK_LINE in mk:
        open(NET_MAKEFILE, "w").write(mk.replace(NET_MK_LINE, ""))
        log("reverted drivers/net/Makefile")
    kc = open(NET_KCONFIG).read()
    if NET_KC_LINE in kc:
        open(NET_KCONFIG, "w").write(kc.replace(NET_KC_LINE + "\n", ""))
        log("reverted drivers/net/Kconfig")

    # revert al_serdes wiring
    for f, line in (
        (DRIVERS_MAKEFILE, DRIVERS_MAKE_LINE),
        (PHY_KCONFIG, PHY_KCONFIG_LINE),
    ):
        if os.path.exists(f):
            open(f, "w").write(open(f).read().replace(line, ""))

    # staged directories (board/annapurna covers al_ddr)
    for dst in DIRS.values():
        d = os.path.join(TREE, dst)
        if os.path.isdir(d):
            shutil.rmtree(d)
    board = os.path.join(TREE, "board/annapurna")
    if os.path.isdir(board):
        shutil.rmtree(board)
    for dst in (
        "include/configs/alpine.h",
        "configs/alpine_v2_unvr_defconfig",
        "arch/arm/dts/awto-alpine-v2-unvr-uboot.dts",
    ):
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
            p = subprocess.run(
                cmd,
                cwd=TREE,
                env=env,
                stdout=f,
                stderr=subprocess.STDOUT,
                timeout=BUILD_TIMEOUT_S,
            )
        except subprocess.TimeoutExpired:
            log(f"TIMEOUT after {BUILD_TIMEOUT_S}s: {' '.join(cmd)}")
            return 124
    log(f"exit {p.returncode} ({time.time() - t0:.0f}s)")
    return p.returncode


def build():
    os.makedirs(BUILDDIR, exist_ok=True)
    o = f"O={BUILDDIR}"
    if run(["make", o, "alpine_v2_unvr_defconfig"]) != 0:
        log("defconfig FAILED")
        return 1
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


def check_dts_shared():
    """Fail the build if the shared i2c timing facts drift between the two DTS
    trees (docs/rtc-s35390a-fault.md). Guard until they are unified (#75)."""
    chk = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "check-dts-shared.py"
    )
    if subprocess.run([sys.executable, chk]).returncode:
        log("ABORT: DTS shared-fact check failed")
        sys.exit(1)


if __name__ == "__main__":
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    if "--clean" in sys.argv:
        clean()
    else:
        check_dts_shared()
        stage()
        sys.exit(build())
