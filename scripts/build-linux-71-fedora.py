#!/usr/bin/env python3
"""Production kernel: Fedora 44 aarch64 config + our Alpine V2 patches, no UEFI.

"Take Fedora stock and patch like we did to 7.1" — Fedora 44 ships kernel 7.1.8,
the exact version we patched. This builds the SAME source tree (our Alpine
patches already integrated by build-linux-71-ea16.py) but with **Fedora's own
aarch64 kernel config** as the base, so every systemd/fs/net symbol is correct
by construction (no hand-curation). Booted by U-Boot (no UEFI/GRUB) via
root=PARTUUID, no embedded initramfs -> the kernel runs Fedora's systemd.

Prereq: run build-linux-71-ea16.py first (integrates the patches into the tree)
and have tmp/fedora-kernel/fedora-aarch64.config extracted.

Out: build-out-71-fedora/  (uImage-unvr-ea16-7.1-fedora, dtb, config, modroot/).
Deploy modroot/lib/modules/<kv> -> the Fedora rootfs /lib/modules; boot the uImage.
"""

import os
import pathlib
import shutil
import struct
import subprocess
import sys
import time
import zlib

from _repo import NPROC  # -j28 host build parallelism (#146)

SRC = os.environ.get("AWTO_KERNEL_SRC", "/mnt/2tb/unvr-port-refs/linux-v7.1.8")
PORT = "/mnt/2tb/unvr-port-refs/linux-alpine-v2"
REPO = "/mnt/2tb/git/awto-unvr"  # OOT al_* module sources imported here (modules/)
# KASAN and normal builds used to share this path - every build did an
# unconditional rmtree() of the whole modroot, so a normal rebuild silently
# wiped the KASAN environment (and vice versa) even though they're both
# meant to coexist for on-demand diagnostic use. Give KASAN builds their own
# OUT by default so neither clobbers the other; AWTO_KERNEL_OUT still wins
# if explicitly set (e.g. the v7.3-fresh experiment).
_OUT_DEFAULT = (
    "/mnt/2tb/unvr-port-refs/build-out-71-fedora-kasan"
    if os.environ.get("AWTO_KASAN_BUILD")
    else "/mnt/2tb/unvr-port-refs/build-out-71-fedora"
)
OUT = os.environ.get("AWTO_KERNEL_OUT", _OUT_DEFAULT)
# kbuild's own O= output dir (Documentation/kbuild/kbuild.rst) - separate from
# OUT itself (which holds the COLLECTED final artifacts: uImage, dtb, config
# copy, modroot/). Before this, every build ran `make -C SRC` with no O=, so
# .config/generated-headers/.o files all lived IN SRC, shared and clobbered
# across every variant (fedora/kasan/ea16/whatever AWTO_KERNEL_SRC points at)
# no matter what OUT was set to - the actual bug class behind #131's false
# "config drift" lead this session (a module rebuilt against SRC's CURRENT,
# since-mutated .config didn't match a kernel built from SRC hours earlier).
# O= gives each OUT its own real, persistent kbuild state - config drift
# becomes structurally impossible instead of a discipline problem.
KOUT = os.path.join(OUT, "kbuild")
FEDORA_CONFIG = "/mnt/2tb/git/awto-unvr/tmp/fedora-kernel/fedora-aarch64.config"
DTS_NAME = "alpine-v2-ubnt-unvr-ea16"
VER = "7.1"
CROSS = "aarch64-linux-gnu-"
# ccache: ~free incremental-rebuild speed across variants sharing mostly-
# identical source (KASAN vs plain differ in a handful of CONFIG_* symbols;
# most .o files compile identically either way, and ccache is keyed on
# preprocessed source+flags, not the O= path, so KASAN and fedora builds
# share cache hits for anything neither actually changes). Documented
# invocation per Documentation/kbuild/llvm.rst's ccache example
# (`CC="ccache clang"` there; `CC="ccache <CROSS_COMPILE>gcc"` here).
_CCACHE = shutil.which("ccache")
CC_ARG = [f"CC=ccache {CROSS}gcc"] if _CCACHE else []

IH_MAGIC, IH_OS_LINUX, IH_ARCH_ARM64, IH_TYPE_KERNEL, IH_COMP_NONE = (
    0x27051956,
    5,
    22,
    2,
    0,
)
LOAD_ADDR = ENTRY_ADDR = 0x08000000
ENV = dict(os.environ, ARCH="arm64", CROSS_COMPILE=CROSS)


def log(m):
    print(m, flush=True)


def run(cmd, **kw):
    log("+ " + (cmd if isinstance(cmd, str) else " ".join(cmd)))
    subprocess.run(cmd, check=True, env=ENV, **kw)


LSMOD_KNOWN_GOOD = os.path.join(REPO, "scripts", "woomera-lsmod-known-good.txt")


def trim_to_woomera_modules():
    """Trim Fedora's generic-hardware driver pile down to what woomera's own
    lsmod actually uses (localmodconfig) - debug/dev kernel, board is fixed
    hardware, not a generic PC. Only touches =m stock Fedora drivers; our own
    al_* OOT modules build via a separate M= invocation, unaffected either
    way.

    Used to SSH to the box live for this every build - fragile by
    construction (this board's network has been repeatedly flaky this
    project) and the failure mode was silent: SSH timeout -> WARN ->
    fall through to the full untrimmed Fedora config, with nothing in
    the final artifacts distinguishing a trimmed build from a bloated
    one (#149). The board's actual module set doesn't change on its
    own between boots - a live fetch was never buying anything a
    checked-in snapshot doesn't, just adding a network dependency to
    every build. LSMOD_KNOWN_GOOD is a real `lsmod` capture from the
    box (scripts/woomera-lsmod-known-good.txt); update it by hand
    (`lsmod > scripts/woomera-lsmod-known-good.txt` over the console,
    then commit) when the board's hardware/driver set actually
    changes, not on every build."""
    if not os.path.exists(LSMOD_KNOWN_GOOD):
        log(f"FATAL: {LSMOD_KNOWN_GOOD} missing", "ERROR")
        sys.exit(1)
    run(["make", "-C", SRC, f"O={KOUT}", f"LSMOD={LSMOD_KNOWN_GOOD}", "localmodconfig"])
    n = len(pathlib.Path(LSMOD_KNOWN_GOOD).read_text().splitlines()) - 1
    log(f"trimmed to {LSMOD_KNOWN_GOOD} ({n} modules)")


def configure():
    os.makedirs(KOUT, exist_ok=True)
    shutil.copy(FEDORA_CONFIG, os.path.join(KOUT, ".config"))
    cfg = os.path.join(SRC, "scripts/config")
    run(
        [
            cfg,
            "--file",
            os.path.join(KOUT, ".config"),
            # our platform + drivers (off in stock Fedora)
            "--enable",
            "ARCH_ALPINE",
            "--enable",
            "PCIE_AL",
            "--enable",
            "PCIE_AL_INTERNAL",
            "--enable",
            "EXPERT",
            "--enable",
            "GPIO_SYSFS",
            # no embedded initramfs -> kernel runs systemd from root=PARTUUID
            "--set-str",
            "INITRAMFS_SOURCE",
            "",
            # drop build-complexity we don't need: BTF (needs matching pahole),
            # module signing (our OOT al_* are unsigned), Fedora key/cert requirements,
            # and full debuginfo (huge/slow). None affect booting Fedora.
            "--disable",
            "DEBUG_INFO_BTF",
            "--disable",
            "DEBUG_INFO_BTF_MODULES",
            "--disable",
            "MODULE_SIG",
            "--disable",
            "MODULE_SIG_ALL",
            "--disable",
            "MODULE_SIG_FORCE",
            "--disable",
            "SYSTEM_REVOCATION_KEYS",
            "--disable",
            "SECURITY_LOCKDOWN_LSM",
            # unrestricted /dev/mem: read/write SoC MMIO from userspace (DDR
            # controller/PHY regs, live hardware RE). Our own box, owner directive.
            "--enable",
            "DEVMEM",
            "--disable",
            "STRICT_DEVMEM",
            "--disable",
            "IO_STRICT_DEVMEM",
            # 4K-sector erase: the stock NOR config/cksum partitions are 4K-aligned
            # (not 64K), so with 64K-only erase the kernel force-read-onlys them.
            "--enable",
            "MTD_SPI_NOR_USE_4K_SECTORS",
            "--enable",
            "DEBUG_INFO_NONE",
            "--disable",
            "WERROR",
            # bring-up/dev kernel: bias toward hang-recoverability over
            # performance (#97) - sysrq default-on for serial BREAK+key
            # recovery, panic_timeout so a panic reboots instead of halting.
            "--set-val",
            "MAGIC_SYSRQ_DEFAULT_ENABLE",
            "1",
            "--set-val",
            "PANIC_TIMEOUT",
            "10",
            # CONFIG_SOFTLOCKUP_DETECTOR (already on, stock Fedora default)
            # only catches a CPU spinning at 100% without yielding - it
            # caught #131's multi-CPU udev-worker lockup, but a task simply
            # BLOCKED indefinitely in an uninterruptible sleep (D-state, no
            # CPU spin) is invisible to it. CONFIG_DETECT_HUNG_TASK is the
            # separate detector for exactly that (default 120s), and was
            # off - confirmed live: a genuine-looking hang produced zero
            # kernel output after 5+ minutes of waiting, precisely because
            # this was disabled, not because nothing was actually stuck.
            "--enable",
            "DETECT_HUNG_TASK",
        ]
    )
    trim_to_woomera_modules()
    run(
        [
            cfg,
            "--file",
            os.path.join(KOUT, ".config"),
            # 10G SFP+ port: mainline phylink + sfp.c (#113). PHYLINK has no
            # Kconfig prompt (select-only) - PCS_XPCS selects it. localmodconfig
            # above cannot know about any of these by construction (it only sees
            # modules the currently-running kernel has loaded), so force them
            # after it runs, then FATAL-check below in case a future
            # olddefconfig silently drops one.
            "--enable",
            "PCS_XPCS",
            "--enable",
            "SFP",
            "--enable",
            "MDIO_I2C",
            "--enable",
            "I2C_MUX_PCA954x",
            # DIAGNOSTIC KASAN build for #131 - disabled by default so the
            # normal build (matching the flashed NAND kernel/modules) stays
            # the daily-driver. Re-enable via AWTO_KASAN_BUILD=1 to continue
            # that investigation. See #131.
            #
            # -kasan LOCALVERSION is load-bearing, not cosmetic: without a
            # distinct `uname -r`, this build and the normal one collide in
            # the same /lib/modules/7.1.8-dirty/ on the box - whichever gets
            # published last silently breaks module loading for the other
            # kernel (root cause of a real incident this session - #131
            # module sync kept clobbering the daily-driver's matching
            # modules and vice versa, repeatedly, until this was fixed).
            *(
                [
                    "--enable",
                    "KASAN",
                    "--enable",
                    "KASAN_GENERIC",
                    "--enable",
                    "PROVE_LOCKING",
                    "--enable",
                    "DEBUG_LOCKDEP",
                    "--enable",
                    "DEBUG_ATOMIC_SLEEP",
                    "--enable",
                    "FTRACE",
                    "--enable",
                    "FUNCTION_TRACER",
                    "--enable",
                    "FUNCTION_GRAPH_TRACER",
                    "--enable",
                    "DYNAMIC_FTRACE",
                    "--set-str",
                    "LOCALVERSION",
                    "-kasan",
                ]
                if os.environ.get("AWTO_KASAN_BUILD")
                else []
            ),
        ]
    )
    run(["make", "-C", SRC, f"O={KOUT}", "olddefconfig"])
    dotcfg = pathlib.Path(os.path.join(KOUT, ".config")).read_text()
    for sym in (
        "CONFIG_ARCH_ALPINE=y",
        "CONFIG_PCIE_AL_INTERNAL=y",
        "CONFIG_PCIE_AL=y",
    ):
        if sym not in dotcfg:
            log(f"FATAL: {sym} not set after olddefconfig")
            sys.exit(1)
    # al_eth.ko (OOT, loaded via modprobe post-boot) can depend on these as
    # modules just as well as built-in - only "enabled at all" matters, not
    # =y vs =m. I2C_MUX_PCA954x in particular lands as =m here because its
    # I2C_MUX parent is already =m (the box's RTC/adt7475 mux, unrelated to
    # SFP+) - forcing that to =y too is out of scope and unnecessary.
    for sym in (
        "CONFIG_PHYLINK",
        "CONFIG_SFP",
        "CONFIG_PCS_XPCS",
        "CONFIG_MDIO_I2C",
        "CONFIG_I2C_MUX_PCA954x",
    ):
        if f"{sym}=y" not in dotcfg and f"{sym}=m" not in dotcfg:
            log(f"FATAL: {sym} not set (y or m) after olddefconfig")
            sys.exit(1)
    log(
        "configured: Fedora base + ARCH_ALPINE + PCIE_AL_INTERNAL + phylink/sfp confirmed"
    )


def kver():
    """`make kernelrelease` as a standalone invocation is not reliable here -
    caught it live: right after an AWTO_KASAN_BUILD=1 run followed by a plain
    run (LOCALVERSION reset only happens inside that conditional, so nothing
    explicitly clears a stale value), `make kernelrelease` still printed the
    PREVIOUS run's "-kasan" release even though the real build that followed
    correctly produced a plain (non-kasan) include/config/kernel.release on
    disk - the OOT modules got built and copied under the wrong kernel
    version's extra/ entirely. Read the file kbuild itself writes as part of
    the real build instead of trusting a separate, apparently-stale-prone
    target."""
    path = os.path.join(KOUT, "include/config/kernel.release")
    if not os.path.exists(path):
        sys.exit(f"FATAL: {path} missing - build the kernel before calling kver()")
    return pathlib.Path(path).read_text().strip()


def adapt_sgpo(mpath):
    """7.1 gpio_chip.set returns int (same fix as build-linux-71-ea16.py)."""
    f = os.path.join(mpath, "al_sgpo.c")
    t = pathlib.Path(f).read_text()
    t = t.replace(
        "static void al_sgpo_set(struct gpio_chip *gc, unsigned int offset, int value)\n{",
        "static int al_sgpo_set(struct gpio_chip *gc, unsigned int offset, int value)\n{",
    )
    t = t.replace(
        "\tif (group >= sgpo->num_groups)\n\t\treturn;\n\n\tspin_lock_irqsave(&sgpo->lock, flags);\n\n\twritel(value ? (1 << bit) : 0,\n\t       al_sgpo_group_reg(sgpo, group, GRP_VEC(1 << bit)));\n\n\tspin_unlock_irqrestore(&sgpo->lock, flags);\n}",
        "\tif (group >= sgpo->num_groups)\n\t\treturn -EINVAL;\n\n\tspin_lock_irqsave(&sgpo->lock, flags);\n\n\twritel(value ? (1 << bit) : 0,\n\t       al_sgpo_group_reg(sgpo, group, GRP_VEC(1 << bit)));\n\n\tspin_unlock_irqrestore(&sgpo->lock, flags);\n\n\treturn 0;\n}",
    )
    pathlib.Path(f).write_text(t)


def stage_dts():
    """Copy the repo-tracked DTS + DTSI into the kernel tree so the build ALWAYS
    compiles the tracked source. Previously only build-linux-71-ea16.py staged the
    board DTS; running this script alone compiled whatever stale copy was last left
    in the kernel tree -> edits to dts/ silently didn't take ("bugs keep coming
    back"). Repo dts/ is now the single source of truth (dts + the alpine-v2.dtsi
    include). Reference boards still come from PORT."""
    amazon = os.path.join(SRC, "arch/arm64/boot/dts/amazon")
    staged = []
    for f in sorted(os.listdir(os.path.join(REPO, "dts"))):
        if f.endswith((".dts", ".dtsi")):
            shutil.copy(os.path.join(REPO, "dts", f), amazon)
            staged.append(f)
    for f in ("dts/alpine-v2-ubnt-unvr.dts", "dts/alpine-v2-ubnt-udmpro.dts"):
        s = os.path.join(PORT, f)
        if os.path.exists(s):
            shutil.copy(s, amazon)
    log(f"staged DTS/DTSI from repo dts/: {', '.join(staged)}")


def check_dts_shared():
    """Fail the build if the shared i2c timing facts drift between the two DTS
    trees (docs/rtc-s35390a-fault.md). Guard until they are unified (#75)."""
    chk = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "check-dts-shared.py"
    )
    if subprocess.run([sys.executable, chk], check=False).returncode:
        log("ABORT: DTS shared-fact check failed")
        sys.exit(1)


def build():
    os.makedirs(OUT, exist_ok=True)
    check_dts_shared()
    configure()
    stage_dts()
    run(
        [
            "make",
            "-C",
            SRC,
            f"O={KOUT}",
            *CC_ARG,
            f"-j{NPROC}",
            "Image",
            "dtbs",
            "modules",
        ]
    )
    # kver() reads include/config/kernel.release off disk - only trustworthy
    # AFTER this real build has actually run and written it fresh.
    kv = kver()
    log(f"KVER={kv}")

    # module tree for the Fedora rootfs
    modroot = os.path.join(OUT, "modroot")
    if os.path.exists(modroot):
        shutil.rmtree(modroot)
    os.makedirs(modroot)
    run(
        [
            "make",
            "-C",
            SRC,
            f"O={KOUT}",
            f"INSTALL_MOD_PATH={modroot}",
            "INSTALL_MOD_STRIP=1",
            "modules_install",
        ]
    )

    # out-of-tree al_* modules into the same tree. -C KOUT (not SRC): with an
    # O= kernel build, KOUT *is* "$KDIR" for external-module purposes
    # (Documentation/kbuild/modules.rst - "$KDIR refers to ... the kernel
    # output directory if the kernel was built in a separate build
    # directory"). mpath is already OUT-scoped (a fresh per-build copy), so
    # its own .o/.ko outputs never cross variants either.
    extra = os.path.join(modroot, f"lib/modules/{kv}/extra")
    os.makedirs(extra, exist_ok=True)

    def build_oot(m, mpath, ko_name, make_vars=()):
        # imported source-of-truth: repo modules/ (carries iofic + crypto fixes).
        shutil.copytree(os.path.join(REPO, "modules", m), mpath)
        if m == "al_sgpo":
            adapt_sgpo(mpath)
        run(
            [
                "make",
                "-C",
                KOUT,
                f"M={mpath}",
                *CC_ARG,
                *make_vars,
                f"-j{NPROC}",
                "modules",
            ]
        )
        shutil.copy(os.path.join(mpath, f"{ko_name}.ko"), extra)
        log(f"OOT {ko_name}: ok")
        # A prior run here silently produced a modroot with `kernel/` fully
        # populated but `extra/` empty - "OOT al_eth: ok" printed (this log
        # line ran) yet the .ko wasn't in the final tree. Root cause not
        # pinned down; this check turns a repeat into a loud failure instead
        # of a working-but-broken build discovered only after flashing.
        copied = os.path.join(extra, f"{ko_name}.ko")
        if not os.path.exists(copied):
            log(f"FATAL: {copied} missing right after copying it", "ERROR")
            sys.exit(1)

    for m in ("al_eth", "al_dma", "al_ssm", "al_sgpo"):
        mpath = os.path.join(OUT, m)
        if os.path.exists(mpath):
            shutil.rmtree(mpath)
        if m == "al_eth":
            # #131: split into 2 .ko's (Makefile MODULE_VARIANT), each bound
            # to one PCI ID, so udev's 2 independent per-device modprobes
            # can never race on the same module. Separate source copies per
            # variant since MODULE_VARIANT changes ccflags.
            for suffix in ("_1g", "_10g"):
                if os.path.exists(mpath + suffix):
                    shutil.rmtree(mpath + suffix)
            build_oot(
                "al_eth",
                mpath + "_1g",
                "al_eth_1g",
                make_vars=["MODULE_VARIANT=1g"],
            )
            build_oot(
                "al_eth",
                mpath + "_10g",
                "al_eth_10g",
                make_vars=["MODULE_VARIANT=10g"],
            )
            continue
        build_oot(m, mpath, m)
    subprocess.run(["depmod", "-b", modroot, kv], check=False)

    # collect boot artifacts
    image = os.path.join(KOUT, "arch/arm64/boot/Image")
    dtb = os.path.join(KOUT, f"arch/arm64/boot/dts/amazon/{DTS_NAME}.dtb")
    shutil.copy(image, os.path.join(OUT, "Image"))
    shutil.copy(dtb, os.path.join(OUT, f"{DTS_NAME}-{VER}.dtb"))
    shutil.copy(
        os.path.join(KOUT, ".config"),
        os.path.join(OUT, f"unvr-ea16-{VER}-fedora.config"),
    )
    mkuimage(image, os.path.join(OUT, f"uImage-unvr-ea16-{VER}-fedora"), kv)
    log(f"DONE. modroot: {modroot} ; uImage: {OUT}/uImage-unvr-ea16-{VER}-fedora")


def mkuimage(image_path, out_path, kv):
    data = pathlib.Path(image_path).read_bytes()
    dcrc = zlib.crc32(data) & 0xFFFFFFFF
    nm = f"unvr-ea16-{VER}-fedora".encode()[:31].ljust(32, b"\0")

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


if __name__ == "__main__":
    t0 = time.time()
    build()
    log(f"TOTAL {int(time.time() - t0)}s")
