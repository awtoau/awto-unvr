#!/usr/bin/env python3
"""Production kernel: Fedora 44 aarch64 config + our Alpine V2 patches, no UEFI.

Builds AWTO_KERNEL_SRC (our Alpine board support - DTS, pcie-al-internal.c,
the pcie-al.c DBI fix, unvr_defconfig - is committed directly into the
kernel tree's own git history, not applied by a script) with **Fedora's
own aarch64 kernel config** as the base, so every systemd/fs/net symbol is
correct by construction. Booted by U-Boot (no UEFI/GRUB) via root=PARTUUID,
no embedded initramfs.

Prereq: have tmp/fedora-kernel/fedora-aarch64.config extracted.

Out: build-out-fedora/ (uImage-unvr-ea16-<VER>-fedora, dtb, config, modroot/).
Deploy modroot/lib/modules/<kv> -> the Fedora rootfs /lib/modules; boot the uImage.

Variants (env; same env on publish-fedora/deploy-ssd picks the matching out dir):
- default: debug kernel (#214), build-out-fedora/
- AWTO_KASAN_BUILD=1: + KASAN/UBSAN/DMA_API_DEBUG, build-out-fedora-kasan/, -kasan
- AWTO_PERF_BUILD=1: #214 debug set off, for throughput (#353),
  build-out-fedora-perf/, -perf
- AWTO_MAINT_BUILD=1 + AWTO_INITRAMFS_SOURCE=<cpio>: perf + SP805/ramoops built in +
  that cpio embedded, Image only, build-out-fedora-maint/, -maint (ssd-repartition.py)
"""

import os
import pathlib
import shutil
import subprocess
import sys
import time

import _kbuild
from _kbuild import CC_ARG, DTS_NAME, ENV, check_dts_shared, kver, mkuimage

import _repo
from _repo import (
    NPROC,
    REPO,
    build_ident,
    check_kernel_patches,
    kernel_build_out,
    kernel_build_ver,
    kernel_src,
    kernel_variant,
)  # -j28 host build parallelism (#146); self-locating repo root

SRC = str(kernel_src())  # store worktree via tmp/refs (#350)
PORT = os.environ.get("AWTO_KERNEL_PORT", "/mnt/2tb/unvr-port-refs/linux-alpine-v2")
# _repo.py's REPO self-locates via git rev-parse, correct wherever this
# script runs from (dev host or natively on woomera).
# kernel_build_out() is the single source of truth shared with
# _fedora_deploy.py (the consumer) - see its own docstring for why this
# used to be two independently-hardcoded copies.
VARIANT = kernel_variant()  # "", "kasan", "perf" or "maint"; >1 env set = fatal
PERF_LIKE = VARIANT in ("perf", "maint")  # maint = perf config + MAINT_SET
# One-shot maintenance initramfs (maint only); nothing else may embed one.
INITRAMFS = os.environ.get("AWTO_INITRAMFS_SOURCE", "")
if (VARIANT == "maint") != bool(INITRAMFS):
    sys.exit(
        "FATAL: AWTO_INITRAMFS_SOURCE is required by, and only valid for, AWTO_MAINT_BUILD"
    )
if INITRAMFS and not os.path.isfile(INITRAMFS):
    sys.exit(f"FATAL: AWTO_INITRAMFS_SOURCE {INITRAMFS} not found")
# U-Boot gunzips the uImage into CONFIG_SYS_BOOTM_LEN (uboot-port defconfig);
# a bigger Image fails bootm and strands the box at U-Boot. 1 MiB headroom.
BOOTM_LEN = 0x4000000
IMAGE_MAX = BOOTM_LEN - 0x100000
OUT = str(kernel_build_out(VARIANT))
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
FEDORA_CONFIG = os.path.join(REPO, "tmp", "fedora-kernel", "fedora-aarch64.config")
# Baked into every output filename AND the uImage's own internal Image Name
# field (U-Boot displays this on boot) - must match AWTO_KERNEL_SRC's real
# version or the artifacts (and what U-Boot reports booting) lie about it.
VER = kernel_build_ver()


def log(m):
    print(m, flush=True)


def run(cmd, **kw):
    # stdin=DEVNULL: a kernel bump adds new Kconfig symbols, and localmodconfig
    # then PROMPTS for each one. With an inherited tty that blocks forever -
    # indistinguishable from a hang. On EOF Kconfig takes the default, which is
    # what olddefconfig below would pick anyway.
    kw.setdefault("stdin", subprocess.DEVNULL)
    _repo.run(cmd, log=log, env=ENV, **kw)


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


# #214 debug set: on in the default/kasan builds, --disabled in perf (#353).
DEBUG_SYMS = (
    "PROVE_LOCKING",
    "WQ_WATCHDOG",
    "DEBUG_ATOMIC_SLEEP",
    "DEBUG_OBJECTS",
    "DEBUG_OBJECTS_WORK",
    "DEBUG_OBJECTS_TIMERS",
    "FAULT_INJECTION",
    "FAULT_INJECTION_DEBUG_FS",
    "FAILSLAB",
    "FAIL_PAGE_ALLOC",
    "FAIL_MAKE_REQUEST",
)
# Perf-only extra --disables: lock debugging PROVE_LOCKING selected (prompted
# symbols survive olddefconfig on their own), and SW PAN (#353).
PERF_DISABLE = (
    "PROVE_RAW_LOCK_NESTING",
    "PROVE_RCU",
    "DEBUG_LOCK_ALLOC",
    "DEBUG_RT_MUTEXES",
    "DEBUG_SPINLOCK",
    "DEBUG_MUTEXES",
    "DEBUG_WW_MUTEX_SLOWPATH",
    "DEBUG_RWSEMS",
    "DEBUG_LOCKDEP",
    "LOCK_STAT",
    "DEBUG_KMEMLEAK",
    "KASAN",
    # Security hardening traded for uaccess cost, perf variant only (#353).
    "ARM64_SW_TTBR0_PAN",
    # Zeroes every TCP send page; boot `init_on_alloc=1` restores it (#353).
    "INIT_ON_ALLOC_DEFAULT_ON",
)
# Must be absent from a perf .config after olddefconfig.
PERF_MUST_BE_OFF = (
    *DEBUG_SYMS,
    *PERF_DISABLE,
    "LOCKDEP",
    "DEBUG_OBJECTS_FREE",
    "DEBUG_OBJECTS_RCU_HEAD",
)


def localversion() -> str:
    """CONFIG_LOCALVERSION for this build: "[-kasan|-perf]-<git describe>" (#258).

    Order matters. The variant must stay a *prefix* of the suffix so the
    builds keep landing in different /lib/modules/ dirs even when they share
    a SHA - swapping the order would make a plain and a KASAN build of the
    same commit collide again (the #131 incident)."""
    prefix = f"-{VARIANT}" if VARIANT else ""
    return f"{prefix}-{build_ident('kernel')}"


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
            # Drop BTF only (needs a matching pahole), module signing (our OOT
            # al_* are unsigned) and Fedora's key/cert requirements.
            # FULL DWARF IS KEPT DELIBERATELY - this is a debug/dev kernel and
            # KASAN/kprobe/kgdb work is worth far more than the seconds it
            # costs (measured: 11-16s incremental). Do not "fix" this by
            # disabling DEBUG_INFO. Owner decision, #214.
            "--disable",
            "DEBUG_INFO_BTF",
            "--disable",
            "DEBUG_INFO_BTF_MODULES",
            # Richer debuginfo + gdb helpers (lx-dmesg, lx-lsmod, ...).
            "--enable",
            "DEBUG_INFO_DWARF5",
            "--enable",
            "GDB_SCRIPTS",
            # kdb frontend, so a halted kernel is usable over the serial
            # console without a gdb host attached. KGDB/KGDB_SERIAL_CONSOLE
            # are already on.
            "--enable",
            "KGDB_KDB",
            # Deadlock and stall detection, each aimed at an open bug:
            #   PROVE_LOCKING (lockdep)  - would have caught #171's rtnl
            #                              self-deadlock automatically
            #   WQ_WATCHDOG              - workqueue stalls (#184, #182)
            #   DEBUG_ATOMIC_SLEEP       - sleeping in atomic context
            #   DEBUG_OBJECTS_*          - work/timer use-after-free (#186)
            # These cost runtime, which is the trade this kernel exists to make.
            # Fault injection, so a driver's error path can be PROVEN rather
            # than closed on code review. Each targets paths that cannot be
            # reached by normal load:
            #   FAILSLAB + FAIL_PAGE_ALLOC - #305 rx refill shortfall
            #   FAIL_MAKE_REQUEST          - block-layer error paths
            #   DEBUG_FS + FAULT_INJECTION_DEBUG_FS - the /sys/kernel/debug knobs
            # The perf variant --disables this whole set instead (#353).
            *(
                a
                for sym in DEBUG_SYMS
                for a in ("--disable" if PERF_LIKE else "--enable", sym)
            ),
            # NETCONSOLE gives netpoll, which is the only caller that hands
            # rx_poll a zero budget (#314).
            "--module",
            "NETCONSOLE",
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
            # RAID5/6 + the async_tx offload path. This is a NAS: MD_RAID456
            # was not built at all, so parity RAID was unavailable AND al_dma's
            # DMA_XOR/DMA_PQ had no consumer. RAID6_PQ_BENCHMARK so the kernel
            # measures rather than defaulting to neonx8 unmeasured (#236).
            "--module",
            "MD_RAID456",
            "--enable",
            "RAID6_PQ_BENCHMARK",
            # A57 Crypto Extensions. These were set only in build-linux-ea16.py,
            # which is NOT the deployed path - publish-fedora runs THIS script,
            # so the daily driver ran SHA in software. CRCT10DIF_CE accelerates
            # the T10-DIF CRC used by md/RAID and SCSI DIF.
            "--enable",
            "CRYPTO_SHA1_ARM64_CE",
            "--enable",
            "CRYPTO_SHA2_ARM64_CE",
            "--enable",
            "CRYPTO_SHA512_ARM64_CE",
            "--enable",
            "CRYPTO_CRCT10DIF_ARM64_CE",
            # WERROR on: a warning in our own OOT drivers is a defect, and the
            # repo rule is zero warnings. DEBUG_INFO_NONE is NOT set - full
            # DWARF is deliberate (#214), and enabling both would contradict.
            "--enable",
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
            # dmaengine self-test/benchmark module - used to verify al_dma
            # completion handling (#23) without needing a real RAID array.
            "--module",
            "DMATEST",
            # NIC-only throughput under al_eth MAC loopback, no TCP/copy (#353).
            "--module",
            "NET_PKTGEN",
            "--enable",
            "MDIO_I2C",
            "--enable",
            "I2C_MUX_PCA954x",
            # #157: USB-as-root never works on this board's no-initramfs boot
            # (root=PARTUUID, no UEFI/GRUB, no embedded initramfs - see this
            # file's own docstring). rootwait's prepare_namespace() just polls
            # msleep() forever waiting for the root device (confirmed live via
            # a sysrq-w blocked-task dump during the actual hang - the ONLY
            # blocked task is that poll loop itself, nothing stuck on I/O or a
            # lock) because usb_storage/uas were =m with nowhere to load a
            # module from this early - no rootfs mounted yet (that's what's
            # being waited for), no initramfs to stage them. xhci_hcd already
            # works fine because Fedora's own config has it =y; do the same
            # for the two drivers actually needed to attach a block device on
            # top of that link.
            "--enable",
            "USB_STORAGE",
            "--enable",
            "USB_UAS",
            # DisplayLink (17e9:6015 ThinkPad Hybrid dock). =m, not =y: it is
            # not needed to reach a rootfs. DRM and DRM_GEM_SHMEM_HELPER (its
            # dependency) are already =y from the Fedora base.
            "--module",
            "DRM_UDL",
            # RTC. Built, but INERT today: the DTS node is status="disabled"
            # to keep Linux off i2c ch0 (#86 SDA-hold wedge). No battery either,
            # so NTP is the real clock fix - this only readies the driver for
            # when ch0 recovery exists.
            "--enable",
            "RTC_DRV_S35390A",
            # FAT. Its absence made efi.automount fail, which killed dbus-broker
            # at step NAMESPACE and took ALL networking down (#231). Also needed
            # to read the SSD's ESP or any FAT stick. NLS_CODEPAGE_437/ASCII are
            # already =y.
            "--enable",
            "VFAT_FS",
            "--enable",
            "MSDOS_FS",
            # vfat's DEFAULT iocharset is iso8859-1. Without it every plain
            # `mount /dev/sdd1` fails "IO charset iso8859-1 not found" even
            # though VFAT_FS is on - only -o iocharset=ascii works (#250).
            "--enable",
            "NLS_ISO8859_1",
            # Everything the box physically has gets a driver, even on a
            # headless NAS: an unexercised subsystem is an unknown, and each one
            # that probes is a test of the port. Audit: scripts/audit-hw-coverage.py
            # Dock audio (3x UAC interfaces on 17e9:6015). CONFIG_SOUND was off
            # entirely, so ALSA had nothing to bind.
            "--module",
            "SOUND",
            "--module",
            "SND",
            "--module",
            "SND_USB_AUDIO",
            # EDAC on the DDR controller. Note the compatible mismatch: our DTS
            # says annapurna-labs,alpine-mc but al_mc_edac.c matches
            # amazon,al-mc-edac - the symbol alone will not bind (#208).
            "--module",
            "EDAC_AL_MC",
            # Raw-NAND core, for our OOT al_nand.ko (#208). An external module
            # cannot supply nand_scan()/nand_cleanup() - they live in
            # nand_base.o inside this symbol. localmodconfig drops it by
            # construction (the build host has no raw NAND). MTD_OF_PARTS is
            # already =m and is what turns the DT fixed-partitions into mtd8+.
            "--module",
            "MTD_RAW_NAND",
            # #92: per-port MSI-X for the two Alpine AHCI controllers instead
            # of board_ahci_al's one shared INTx. Needs
            # patches/ahci-alpine-per-port-msix.patch applied in the kernel
            # tree; the =y check below is what makes a missing patch loud.
            "--enable",
            "AHCI_ALPINE",
            # USB-attached ethernet (the dock's ports: RTL8153 0bda:8153 and
            # Lenovo ThinkPad Lan 17ef:a359, both r8152-family; CDC_ETHER/NCM
            # cover the standard-class ones). localmodconfig above drops these
            # by construction - it only keeps what the BUILD host's running
            # kernel had loaded, which never includes a driver for a device
            # attached to the target box. Confirmed live 2026-09-03: both dock
            # ethernet devices enumerated on USB but had no driver at all
            # ("modprobe: FATAL: Module r8152 not found"), so no interfaces.
            "--module",
            "USB_USBNET",
            "--module",
            "USB_RTL8152",
            "--module",
            "USB_NET_CDCETHER",
            "--module",
            "USB_NET_CDC_NCM",
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
                    "KASAN_VMALLOC",
                    "--enable",
                    "PROVE_LOCKING",
                    "--enable",
                    "DEBUG_LOCKDEP",
                    "--enable",
                    "DEBUG_ATOMIC_SLEEP",
                    "--enable",
                    "DEBUG_SPINLOCK",
                    "--enable",
                    "DEBUG_MUTEXES",
                    "--enable",
                    "FTRACE",
                    "--enable",
                    "FUNCTION_TRACER",
                    "--enable",
                    "FUNCTION_GRAPH_TRACER",
                    "--enable",
                    "DYNAMIC_FTRACE",
                    # #172 is a double-unmap - this is the tool built to catch
                    # exactly that class of bug directly, not just its symptom.
                    "--enable",
                    "DMA_API_DEBUG",
                    # Plain UBSAN, not UBSAN_TRAP - TRAP replaces the whole
                    # point of it (a descriptive report of exactly what
                    # overflowed/misaligned/etc.) with a bare crash and no
                    # explanation, actively working against finding the bug.
                    # No KCSAN here: lib/Kconfig.kcsan depends on !KASAN,
                    # mutually exclusive with the KASAN above, not just an
                    # unmet dependency - would need a separate build.
                    "--enable",
                    "UBSAN",
                    # A lockup gets logged by default (softlockup_panic=0) -
                    # promote it to a real panic so pstore/ramoops actually
                    # captures it instead of a console message nobody sees.
                    # SOFTLOCKUP_PANIC is int type ("panic after N*20s") -
                    # HARDLOCKUP_PANIC (despite the near-identical name) is
                    # plain bool - verified against lib/Kconfig.debug
                    # directly rather than assuming they match.
                    "--set-val",
                    "BOOTPARAM_SOFTLOCKUP_PANIC",
                    "1",
                    "--enable",
                    "BOOTPARAM_HARDLOCKUP_PANIC",
                    # panic_on_warn is a runtime sysctl/boot param, not a
                    # Kconfig symbol - set `sysctl kernel.panic_on_warn=1`
                    # after boot instead of trying to bake it in here.
                ]
                if VARIANT == "kasan"
                else []
            ),
            *(a for sym in PERF_DISABLE for a in ("--disable", sym) if PERF_LIKE),
        ]
    )
    if VARIANT == "maint":
        # No modules in the initramfs: watchdog + ramoops must be =y. XZ keeps
        # the Image under IMAGE_MAX.
        run(
            [cfg, "--file", os.path.join(KOUT, ".config")]
            + ["--enable", "ARM_SP805_WATCHDOG"]
            + ["--enable", "PSTORE_RAM", "--enable", "PSTORE_CONSOLE"]
            + ["--set-str", "INITRAMFS_SOURCE", os.path.abspath(INITRAMFS)]
            + ["--enable", "INITRAMFS_COMPRESSION_XZ"]
        )
    # #258: stamp the build's revision into `uname -r`, EVERY build. Without
    # it every build ever made reported the same bare version, so a stale
    # deploy was invisible (#105, #131, #161).
    # Composed, not clobbered: -kasan stays (see the load-bearing note above -
    # it keeps the two builds in separate /lib/modules/ dirs), with the SHA
    # appended after it.
    run(
        [
            cfg,
            "--file",
            os.path.join(KOUT, ".config"),
            "--set-str",
            "LOCALVERSION",
            localversion(),
        ]
    )
    # MD_RAID456 does not survive the big --enable/--disable chain above -
    # RAID6_PQ_BENCHMARK from the same block does, so it is --module in a mixed
    # chain that gets lost. Re-assert it on its own; olddefconfig preserves it.
    run([cfg, "--file", os.path.join(KOUT, ".config"), "--module", "MD_RAID456"])
    # al_ssm's only in-kernel consumer (#267: AF_ALG and fscrypt mask ASYNC
    # drivers out; dm-crypt does not). localmodconfig drops DM_CRYPT (the build
    # host has no dm-crypt volume loaded); `bench-all --ssm` needs it.
    # CRYPTO_TEST (tcrypt) re-runs testmgr on demand:
    # `modprobe tcrypt alg=xts-aes-al-ssm`.
    run(
        [cfg, "--file", os.path.join(KOUT, ".config")]
        + ["--module", "DM_CRYPT", "--module", "CRYPTO_TEST"]
        # Fuzzes every algorithm against the generic one at registration (zero
        # length, partial blocks, MAY_BACKLOG, in-place, misaligned sg): the
        # check #282/#291 call for. Slows boot, so opt-in.
        + (
            ["--enable", "CRYPTO_MANAGER_EXTRA_TESTS"]
            if os.environ.get("AWTO_CRYPTO_EXTRA_TESTS")
            else []
        )
    )
    run(["make", "-C", SRC, f"O={KOUT}", "olddefconfig"])
    dotcfg = pathlib.Path(os.path.join(KOUT, ".config")).read_text()
    for sym in (
        "CONFIG_ARCH_ALPINE=y",
        "CONFIG_PCIE_AL_INTERNAL=y",
        "CONFIG_PCIE_AL=y",
        # USB ethernet (dock ports) - localmodconfig drops these every run,
        # so verify the force-enable above actually survived rather than
        # finding out from a box with no USB network interfaces again.
        "CONFIG_USB_RTL8152=m",
        "CONFIG_USB_USBNET=m",
        # #212: al_ssm/al_dma devlink health reporters. NET_DEVLINK has no
        # Kconfig prompt (select-only) and an OOT module cannot select it -
        # ARCH_ALPINE does it instead (arch/arm64/Kconfig.platforms, in the
        # kernel tree's own history). Both .ko's fail to load without it.
        "CONFIG_NET_DEVLINK=y",
        # #236: parity RAID on a NAS, and the only consumer of al_dma's
        # DMA_XOR/DMA_PQ. localmodconfig drops it (nothing was using it),
        # so verify the force-enable survived.
        "CONFIG_MD_RAID456=m",
        # al_ssm's only in-kernel consumer (see the force-enable above).
        "CONFIG_DM_CRYPT=m",
        # #258: the build's own identity. olddefconfig has been seen to drop
        # a --set-str, and a silently-empty LOCALVERSION puts us straight
        # back to "every build looks identical".
        f'CONFIG_LOCALVERSION="{localversion()}"',
    ):
        if sym not in dotcfg:
            log(f"FATAL: {sym} not set after olddefconfig")
            sys.exit(1)
    if PERF_LIKE:
        for sym in PERF_MUST_BE_OFF:
            if f"CONFIG_{sym}=" in dotcfg:
                log(f"FATAL: CONFIG_{sym} still set in the {VARIANT} build (#353)")
                sys.exit(1)
    if VARIANT == "maint":
        for sym in (
            "CONFIG_ARM_SP805_WATCHDOG=y",
            "CONFIG_PSTORE_RAM=y",
            "CONFIG_PSTORE_CONSOLE=y",
            "CONFIG_INITRAMFS_COMPRESSION_XZ=y",
            f'CONFIG_INITRAMFS_SOURCE="{os.path.abspath(INITRAMFS)}"',
            "CONFIG_EXT4_FS=y",
            "CONFIG_BLK_DEV_SD=y",
            "CONFIG_DEVTMPFS=y",
            "CONFIG_MAGIC_SYSRQ=y",
        ):
            if sym not in dotcfg:
                log(f"FATAL: {sym} not set after olddefconfig (maint)")
                sys.exit(1)
    if VARIANT == "kasan":
        # scripts/config --enable silently no-ops on an unknown symbol name
        # instead of erroring - verify every debug flag actually landed
        # rather than assuming the invocation above worked.
        for sym in (
            "CONFIG_KASAN=y",
            "CONFIG_KASAN_GENERIC=y",
            "CONFIG_KASAN_VMALLOC=y",
            "CONFIG_PROVE_LOCKING=y",
            "CONFIG_DEBUG_LOCKDEP=y",
            "CONFIG_DEBUG_ATOMIC_SLEEP=y",
            "CONFIG_DEBUG_SPINLOCK=y",
            "CONFIG_DEBUG_MUTEXES=y",
            "CONFIG_DMA_API_DEBUG=y",
            "CONFIG_UBSAN=y",
            "CONFIG_BOOTPARAM_SOFTLOCKUP_PANIC=1",  # int type, not =y
            "CONFIG_BOOTPARAM_HARDLOCKUP_PANIC=y",
            "CONFIG_FTRACE=y",
            "CONFIG_FUNCTION_TRACER=y",
            "CONFIG_FUNCTION_GRAPH_TRACER=y",
            "CONFIG_DYNAMIC_FTRACE=y",
        ):
            if sym not in dotcfg:
                log(
                    f"FATAL: {sym} not set after olddefconfig - AWTO_KASAN_BUILD "
                    f"requested it but it didn't land (renamed symbol? "
                    f"unmet Kconfig dependency?)"
                )
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
        # #208: al_nand.ko links against nand_scan()/nand_cleanup() from the
        # raw-NAND core. Without it the module builds with unresolved symbols
        # and NAND stays invisible - exactly the gap it exists to close.
        "CONFIG_MTD_RAW_NAND",
    ):
        if f"{sym}=y" not in dotcfg and f"{sym}=m" not in dotcfg:
            log(f"FATAL: {sym} not set (y or m) after olddefconfig")
            sys.exit(1)
    # #157: unlike the above, these MUST be =y specifically (not =m) - the
    # whole fix is that they need to be active before any rootfs/initramfs
    # exists to load a module from. =m here would silently reintroduce the
    # exact bug this was added to fix.
    for sym in ("CONFIG_USB_STORAGE", "CONFIG_USB_UAS"):
        if f"{sym}=y" not in dotcfg:
            log(
                f"FATAL: {sym} not built in (=y) after olddefconfig - #157 fix regressed"
            )
            sys.exit(1)
    # Same no-initramfs reasoning as above: root is on SATA, so the driver that
    # owns the port must be =y. scripts/config --enable silently no-ops on an
    # unknown symbol, so an unapplied patch shows up here rather than as a
    # quietly-still-on-INTx kernel.
    if "CONFIG_AHCI_ALPINE=y" not in dotcfg:
        log(
            "FATAL: CONFIG_AHCI_ALPINE not built in (=y) - apply "
            "patches/ahci-alpine-per-port-msix.patch in AWTO_KERNEL_SRC (#92)"
        )
        sys.exit(1)
    # Same idea for the patches with no config symbol of their own - our DTS
    # sets snps,no-enable-abort, so a tree that ignores it wedges the pld bus
    # on the box (cold-power-cycle-only recovery) rather than failing here.
    check_kernel_patches(log)
    log(
        "configured: Fedora base + ARCH_ALPINE + PCIE_AL_INTERNAL + phylink/sfp confirmed"
    )


def stage_dts():
    _kbuild.stage_dts(SRC, log, PORT)


def build_maint():
    """Image only: the maint kernel never mounts a rootfs, so no modules."""
    run(["make", "-C", SRC, f"O={KOUT}", *CC_ARG, f"-j{NPROC}", "Image"])
    image = os.path.join(KOUT, "arch/arm64/boot/Image")
    size = os.path.getsize(image)
    if size > IMAGE_MAX:
        log(
            f"FATAL: Image {size} B > {IMAGE_MAX} B (BOOTM_LEN - 1 MiB): bootm would fail"
        )
        sys.exit(1)
    shutil.copy(image, os.path.join(OUT, "Image"))
    shutil.copy(
        os.path.join(KOUT, ".config"),
        os.path.join(OUT, f"unvr-ea16-{VER}-maint.config"),
    )
    log(f"KVER={kver(KOUT)}")
    log(f"DONE. Image: {OUT}/Image ({size} B, limit {IMAGE_MAX})")


def build():
    os.makedirs(OUT, exist_ok=True)
    check_dts_shared(log)
    configure()
    stage_dts()
    if VARIANT == "maint":
        build_maint()
        return
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
    kv = kver(KOUT)
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

    _kbuild.build_oot_modules(
        run, log, KOUT, OUT, os.path.join(modroot, f"lib/modules/{kv}/extra")
    )
    _repo.run(["depmod", "-b", modroot, kv], log=log, check=False)

    # collect boot artifacts
    image = os.path.join(KOUT, "arch/arm64/boot/Image")
    dtb = os.path.join(KOUT, f"arch/arm64/boot/dts/amazon/{DTS_NAME}.dtb")
    shutil.copy(image, os.path.join(OUT, "Image"))
    shutil.copy(dtb, os.path.join(OUT, f"{DTS_NAME}-{VER}.dtb"))
    shutil.copy(
        os.path.join(KOUT, ".config"),
        os.path.join(OUT, f"unvr-ea16-{VER}-fedora.config"),
    )
    mkuimage(
        image,
        os.path.join(OUT, f"uImage-unvr-ea16-{VER}-fedora"),
        f"unvr-ea16-{VER}-fedora",
        log,
    )
    log(f"DONE. modroot: {modroot} ; uImage: {OUT}/uImage-unvr-ea16-{VER}-fedora")


if __name__ == "__main__":
    t0 = time.time()
    build()
    log(f"TOTAL {int(time.time() - t0)}s")
