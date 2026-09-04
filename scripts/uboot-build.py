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
from pathlib import Path

from _repo import NPROC  # -j28 host build parallelism (#146)

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
    # #140: `cpuon` PSCI CPU_ON test command (does powering cluster1 change
    # the xHCI Enable-Slot timeout?).
    "board/annapurna/alpine/al_cpuon.c": "board/annapurna/alpine/al_cpuon.c",
    "include/configs/alpine.h": "include/configs/alpine.h",
    "configs/alpine_v2_unvr_defconfig": "configs/alpine_v2_unvr_defconfig",
    "arch/dts/awto-alpine-v2-unvr-uboot.dts": "arch/arm/dts/awto-alpine-v2-unvr-uboot.dts",
    # DW i2c: patched to honor explicit raw SCL hcnt/lcnt from DT so we can pin
    # stock's proven s35390a-safe pld-bus timing (docs/rtc-s35390a-fault.md).
    "drivers/i2c/designware_i2c.c": "drivers/i2c/designware_i2c.c",
    "drivers/i2c/designware_i2c.h": "drivers/i2c/designware_i2c.h",
    # AHCI: #92 raise MAX_SATA_BLOCKS_READ_WRITE 0x80->0x800 (62MB/s->faster);
    # #94 spin-up-aware link-up wait so the first `scsi scan` finds cold HDDs.
    "drivers/ata/ahci.c": "drivers/ata/ahci.c",
    # DW spi: #91 bound EEPROM-read transfers to the Rx FIFO depth so a slow
    # polled consumer can't overrun the Rx FIFO (env load fails otherwise).
    "drivers/spi/designware_spi.c": "drivers/spi/designware_spi.c",
    # #140: external PCIe aliases its one root-port device across every devfn.
    # addr_valid() rejects devfn != 0 on the port's own bus, so `pci enum`
    # binds one xhci driver instead of 32 clobbering one register block.
    "drivers/pci/pcie_ecam_generic.c": "drivers/pci/pcie_ecam_generic.c",
}

# Whole subtrees copied verbatim: scaffold rel-dir -> tree rel-dir.
# al_hal_shim is include-only (no Makefile) — it is not descended into by kbuild;
# each driver's ccflags -I resolves the shared plat shim + common HAL headers.
DIRS = {
    "drivers/net/al_hal_shim": "drivers/net/al_hal_shim",
    "board/annapurna/alpine/al_ddr": "board/annapurna/alpine/al_ddr",
    "drivers/net/al_eth": "drivers/net/al_eth",
    "drivers/phy/al_serdes": "drivers/phy/al_serdes",
    "drivers/crypto/al_ssm": "drivers/crypto/al_ssm",
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
# Sanity marker: pristine drivers/Makefile always has this (pulls in the DM
# core). Its absence means the file is truncated/corrupt, not just unpatched
# (see #102: a prior manual edit once overwrote the whole file down to a
# single appended line and neither stage() nor unstage() noticed).
DRIVERS_MK_SANITY = "obj-$(CONFIG_$(PHASE_)DM) += core/\n"
PHY_KCONFIG = os.path.join(TREE, "drivers/phy/Kconfig")
PHY_KCONFIG_LINE = 'source "drivers/phy/al_serdes/Kconfig"\n'

# al_ssm wiring: drivers/crypto/{Makefile,Kconfig}. drivers/crypto/ is always
# descended (drivers/Makefile: unconditional `obj-y += crypto/`), so a subdir
# obj line + a sourced Kconfig suffice - same pattern as al_eth under net/.
CRYPTO_MAKEFILE = os.path.join(TREE, "drivers/crypto/Makefile")
CRYPTO_MK_LINE = "obj-$(CONFIG_AL_SSM) += al_ssm/\n"
CRYPTO_KCONFIG = os.path.join(TREE, "drivers/crypto/Kconfig")
CRYPTO_KC_LINE = 'source "drivers/crypto/al_ssm/Kconfig"\n'
CRYPTO_KC_ANCHOR = "endmenu\n"


def log(msg):
    line = f"[{time.strftime('%Y-%m-%dT%H:%M:%S%z')}] {msg}"
    print(line, flush=True)
    with open(LOG, "a") as f:
        f.write(line + "\n")


def unstage_stale():
    """Reset TREE to pristine (git checkout --) before staging, so a file
    removed from FILES/DIRS (e.g. a reverted diagnostic/candidate-fix
    overlay) actually goes away instead of silently lingering, modified,
    forever - stage() only ever copies files IN, nothing previously
    unstaged them. Found live (2026-08-27): a #140 CRS-handling patch was
    reverted in this repo (commit 73f4701) but the shared TREE still had
    the modified file - `git status` showed 153 lines still there, weeks
    of future builds would have silently kept using it. TREE is a plain
    git checkout of mainline that only ever gets modified by this script,
    so a full reset here is safe - nothing else should have uncommitted
    changes in it."""
    rc = subprocess.run(
        ["git", "checkout", "--", "."], cwd=TREE, check=False
    ).returncode
    if rc != 0:
        log(
            f"WARNING: git checkout -- . in {TREE} failed (rc={rc}) - stale files may remain"
        )
    else:
        log(f"reset {TREE} to pristine before staging")


def stage():
    unstage_stale()
    for src, dst in FILES.items():
        s = os.path.join(SCAFFOLD, src)
        d = os.path.join(TREE, dst)
        os.makedirs(os.path.dirname(d), exist_ok=True)
        shutil.copy2(s, d)
        log(f"staged {dst}")

    # Shared internal-PCIe register-constants header (docs/uefi.md P1.5,
    # docs/audits/audit-edk2-pcie-glue.md) - canonical copy lives in our
    # Linux fork's drivers/pci/controller/pcie-al-internal.c; this repo's
    # hal/ is the byte-identical mirror both alpine.c and the EDK2 port
    # actually #include, not a hand-retyped copy. Staged into the same
    # directory as alpine.c so its plain `#include "pcie-al-alpine-regs.h"`
    # resolves without needing a build-time -I flag.
    shutil.copy2(
        os.path.join(REPO, "hal/pcie-al-alpine-regs.h"),
        os.path.join(TREE, "board/annapurna/alpine/pcie-al-alpine-regs.h"),
    )
    log("staged board/annapurna/alpine/pcie-al-alpine-regs.h (shared, from hal/)")

    for src, dst in DIRS.items():
        s = os.path.join(SCAFFOLD, src)
        d = os.path.join(TREE, dst)
        if os.path.isdir(d):
            shutil.rmtree(d)
        shutil.copytree(s, d)
        log(f"staged {dst}/ ({sum(len(f) for _, _, f in os.walk(s))} files)")

    # arch/arm/Kconfig — board TARGET + board Kconfig source
    txt = Path(KCONFIG).read_text()
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
        Path(KCONFIG).write_text(txt)
        log("patched arch/arm/Kconfig (TARGET + board source)")

    # drivers/net Makefile + Kconfig hooks for al_eth
    mk = Path(NET_MAKEFILE).read_text()
    if NET_MK_LINE not in mk:
        with open(NET_MAKEFILE, "a") as f:
            f.write(NET_MK_LINE)
        log("patched drivers/net/Makefile (al_eth obj)")
    kc = Path(NET_KCONFIG).read_text()
    if NET_KC_LINE not in kc:
        kc = kc.replace(NET_KC_ANCHOR, NET_KC_LINE + "\n" + NET_KC_ANCHOR, 1)
        Path(NET_KCONFIG).write_text(kc)
        log("patched drivers/net/Kconfig (al_eth source)")

    # drivers/Makefile + drivers/phy/Kconfig hooks for al_serdes
    dm = Path(DRIVERS_MAKEFILE).read_text()
    if DRIVERS_MAKE_LINE not in dm:
        if DRIVERS_MK_SANITY not in dm:
            log(
                f"ABORT: {DRIVERS_MAKEFILE} missing expected content "
                f"({len(dm)} bytes) - refusing to patch a truncated/corrupt "
                "file (would silently cement the damage)"
            )
            sys.exit(1)
        Path(DRIVERS_MAKEFILE).write_text(dm.rstrip("\n") + "\n" + DRIVERS_MAKE_LINE)
        log("patched drivers/Makefile (phy/al_serdes/)")
    pk = Path(PHY_KCONFIG).read_text()
    if PHY_KCONFIG_LINE not in pk:
        idx = pk.rfind("endmenu")
        if idx < 0:
            log(
                f"ABORT: {PHY_KCONFIG} has no 'endmenu' ({len(pk)} bytes) - "
                "refusing to patch a truncated/corrupt file (would silently "
                "cement the damage)"
            )
            sys.exit(1)
        pk = pk[:idx] + PHY_KCONFIG_LINE + "\n" + pk[idx:]
        Path(PHY_KCONFIG).write_text(pk)
        log("patched drivers/phy/Kconfig (source al_serdes/Kconfig)")

    # drivers/crypto Makefile + Kconfig hooks for al_ssm
    cm = open(CRYPTO_MAKEFILE).read()
    if CRYPTO_MK_LINE not in cm:
        open(CRYPTO_MAKEFILE, "a").write(CRYPTO_MK_LINE)
        log("patched drivers/crypto/Makefile (al_ssm obj)")
    ck = open(CRYPTO_KCONFIG).read()
    if CRYPTO_KC_LINE not in ck:
        idx = ck.rfind(CRYPTO_KC_ANCHOR)
        if idx < 0:
            log(
                f"ABORT: {CRYPTO_KCONFIG} has no 'endmenu' ({len(ck)} bytes) - "
                "refusing to patch a truncated/corrupt file"
            )
            sys.exit(1)
        ck = ck[:idx] + CRYPTO_KC_LINE + "\n" + ck[idx:]
        open(CRYPTO_KCONFIG, "w").write(ck)
        log("patched drivers/crypto/Kconfig (source al_ssm/Kconfig)")


# Post-stage verification: tree rel-path -> (marker that must be present,
# patch under patches/uboot/ that supplies it, what breaks without it).
# stage() is idempotent-by-`if X not in txt`, which drifts as upstream moves;
# this re-reads the tree afterwards so a hook that silently failed to land is
# loud here instead of producing a quietly-wrong u-boot.bin (#224).
PATCH_MARKERS = {
    "arch/arm/Kconfig": (
        "TARGET_ALPINE_V2_UNVR",
        "0001-board-alpine-v2-unvr-target.patch",
        "no board target - defconfig will not resolve",
    ),
    "drivers/net/Makefile": (
        "obj-$(CONFIG_AL_ETH)",
        "0002-net-al_eth-wiring.patch",
        "al_eth never compiled - no 1G ethernet",
    ),
    "drivers/net/Kconfig": (
        "drivers/net/al_eth/Kconfig",
        "0002-net-al_eth-wiring.patch",
        "CONFIG_AL_ETH unknown - obj line silently no-ops",
    ),
    "drivers/Makefile": (
        "obj-$(CONFIG_AL_SERDES)",
        "0003-phy-al_serdes-wiring.patch",
        "al_serdes never compiled - no 10G/25G SFP+",
    ),
    "drivers/phy/Kconfig": (
        "drivers/phy/al_serdes/Kconfig",
        "0003-phy-al_serdes-wiring.patch",
        "CONFIG_AL_SERDES unknown - obj line silently no-ops",
    ),
    "drivers/crypto/Makefile": (
        "obj-$(CONFIG_AL_SSM)",
        "0004-crypto-al_ssm-wiring.patch",
        "al_ssm never compiled",
    ),
    "drivers/crypto/Kconfig": (
        "drivers/crypto/al_ssm/Kconfig",
        "0004-crypto-al_ssm-wiring.patch",
        "CONFIG_AL_SSM unknown - obj line silently no-ops",
    ),
    # Whole-file copies from uboot-port/. Their diffs are invisible in the
    # copy, so the marker is the one line the fix turns on.
    "drivers/i2c/designware_i2c.c": (
        "i2c-ss-scl-hcnt-raw",
        "0005-i2c-designware-raw-scl-hcnt-lcnt.patch",
        "computed i2c timing wedges the pld bus (#86)",
    ),
    "drivers/pci/pcie_ecam_generic.c": (
        "awto,single-devfn",
        "0008-pci-ecam-reject-aliased-devfn.patch",
        "aliased devfn binds 32 xhci drivers on one register block (#140)",
    ),
    "drivers/ata/ahci.c": (
        "WAIT_MS_LINKUP_SPINUP",
        "0006-ahci-block-size-and-spinup-wait.patch",
        "cold HDDs missed by the first scsi scan; SATA capped ~62 MB/s (#92, #94)",
    ),
    "drivers/spi/designware_spi.c": (
        "SPI_MEM_DATA_IN && priv->fifo_len",
        "0007-spi-designware-bound-rx-to-fifo-depth.patch",
        "Rx FIFO overrun corrupts the env read (#91)",
    ),
}


def verify_patch_state():
    """Re-read TREE after stage() and confirm every upstream hook is present.
    Equivalent to the kernel's CONFIG_AHCI_ALPINE=y FATAL (#92): a missing or
    half-applied patch must be loud, not a silently-wrong binary."""
    missing = []
    for relpath, (marker, patch, consequence) in PATCH_MARKERS.items():
        p = Path(TREE) / relpath
        if not p.exists() or marker not in p.read_text():
            missing.append(f"  {relpath}: no {marker!r} - {consequence}")
            missing.append(f"    supplied by patches/uboot/{patch}")
    if missing:
        log("ABORT: UBOOT_TREE is not at the expected patch state:")
        for line in missing:
            log(line)
        sys.exit(1)
    log(f"verified {len(PATCH_MARKERS)} upstream patch markers in {TREE}")


def unstage():
    txt = Path(KCONFIG).read_text()
    txt = txt.replace(TARGET_BLOCK.lstrip("\n") + "\n", "")
    txt = txt.replace(SRC_LINE, "")
    Path(KCONFIG).write_text(txt)
    log("reverted arch/arm/Kconfig")

    # revert al_eth wiring
    mk = Path(NET_MAKEFILE).read_text()
    if NET_MK_LINE in mk:
        Path(NET_MAKEFILE).write_text(mk.replace(NET_MK_LINE, ""))
        log("reverted drivers/net/Makefile")
    kc = Path(NET_KCONFIG).read_text()
    if NET_KC_LINE in kc:
        Path(NET_KCONFIG).write_text(kc.replace(NET_KC_LINE + "\n", ""))
        log("reverted drivers/net/Kconfig")

    # revert al_serdes + al_ssm wiring
    for f, line in (
        (DRIVERS_MAKEFILE, DRIVERS_MAKE_LINE),
        (PHY_KCONFIG, PHY_KCONFIG_LINE),
        (CRYPTO_MAKEFILE, CRYPTO_MK_LINE),
        (CRYPTO_KCONFIG, CRYPTO_KC_LINE + "\n"),
    ):
        if os.path.exists(f):
            content = Path(f).read_text()
            if line in content:
                Path(f).write_text(content.replace(line, ""))
                log(f"reverted {os.path.relpath(f, TREE)}")

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
                check=False,
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
    rc = run(["make", o, f"-j{NPROC}"])
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
    if subprocess.run([sys.executable, chk], check=False).returncode:
        log("ABORT: DTS shared-fact check failed")
        sys.exit(1)


if __name__ == "__main__":
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    if "--clean" in sys.argv:
        clean()
    else:
        check_dts_shared()
        stage()
        verify_patch_state()
        sys.exit(build())
