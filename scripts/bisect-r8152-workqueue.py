#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""git-bisect driver for the r8152 __queue_work() wild-pointer crash (#190).

Confirmed live, on a fully clean (`make mrproper`, ccache disabled) build:
loading r8152.ko crashes deterministically in __queue_work()'s
per_cpu_ptr(wq->cpu_pwq, cpu) dereference - KASAN "probably
user-memory-access" (a torn/wild pointer, not NULL). The known fix for
exactly this class of bug (1d125f0e6cbd, "use RCU accessors when
populating wq->cpu_pwq") is already in our tree, so this is most likely a
DIFFERENT, still-unfixed bug in the same pwq-lifetime rework series
(merged 2026-07-31 to 2026-08-13) - a missing memory barrier that ARM64's
weak ordering exposes and x86 would silently mask (also why nothing
matching this has turned up in upstream bug trackers - see #190).

Bisect range: dbff1ec23f68 (2026-07-09, before the rework series started)
to cee9395acd80 (Linux 7.3-rc1, our current HEAD's raw upstream base).

Each bisect candidate is a RAW upstream commit with none of our board
support - those are 4 commits layered on top of our actual HEAD, not part
of upstream history - so each step cherry-picks them on before building.

Boots an INITRAMFS-embedded kernel (busybox shell over the serial console),
not the Fedora rootfs on disk: this test only needs console + USB/xHCI
working (both on the "external" PCIe controller - the pcie-al.c DBI fix),
not AHCI/SATA (the "internal" bus - pcie-al-internal.c). Avoiding a
disk-rootfs dependency means a candidate that happens to break AHCI
unrelated to this bug can't produce a false SKIP. No SSH/network needed -
entirely over the console, matching build-linux-ea16.py's own pattern
(reused here as a subprocess, then r8152.ko/mii.ko are added into the
built initramfs before the final image/cpio regeneration).

Usage (run once to set up, then let git bisect drive this script):
    cd /mnt/2tb/unvr-port-refs/linux-v7.3-fresh
    git bisect start cee9395acd80 dbff1ec23f68
    git bisect run python3 /mnt/2tb/git/awto-unvr/scripts/bisect-r8152-workqueue.py

Or run a single step directly (for testing this script itself, against
whatever commit SRC is currently checked out to):
    python3 scripts/bisect-r8152-workqueue.py
"""

from __future__ import annotations

import argparse
import importlib.util
import os
import shutil
import struct
import subprocess
import sys
import time
import zlib
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
from _net import detect_server_ip  # noqa: E402
from _power import power_cycle_verified  # noqa: E402

# ram-boot-deploy.py already has the proven tftpd-embed + tftp_and_verify +
# catch-uboot-race logic (see docs/build.md: single deploy concept, no
# duplicate reimplementations) - reused directly via importlib since its
# filename has dashes and can't be a plain `import` target. It imports
# _repo, which refuses direct `python3 scripts/foo.py` invocation unless
# AWTO_VIA_DEVPY or AWTO_ALLOW_DIRECT_SCRIPT is set - this script (driven by
# `git bisect run`, which needs a bare argv and can't go through dev.py's
# subcommand dispatch) is itself always a direct invocation, so it opts in
# for this in-process reuse rather than spawning ram-boot-deploy.py as a
# naive subprocess.
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
_rbd_spec = importlib.util.spec_from_file_location(
    "_ram_boot_deploy", REPO / "scripts/ram-boot-deploy.py")
_rbd = importlib.util.module_from_spec(_rbd_spec)
_rbd_spec.loader.exec_module(_rbd)
SRC = Path(os.environ.get("AWTO_KERNEL_SRC", "/mnt/2tb/unvr-port-refs/linux-v7.3-fresh"))
EA16_OUT = Path("/mnt/2tb/unvr-port-refs/build-out-ea16")
KOUT = EA16_OUT / "kbuild"
LOG = REPO / "tmp/logs/bisect-r8152-workqueue.log"
NPROC = os.cpu_count() or 4
CROSS = "aarch64-linux-gnu-"
ENV = dict(os.environ, ARCH="arm64", CROSS_COMPILE=CROSS)

BOARD_COMMITS = ["3a8d7a1aed24", "2c9316496bad", "ff8d1c237ea6", "ae48f7150404"]
DTS_NAME = "alpine-v2-ubnt-unvr-ea16"

# Exit codes git-bisect run understands: 0 = good, 1-124/126-127 = bad, 125 = skip.
GOOD, BAD, SKIP = 0, 1, 125

BUILD_TIMEOUT_S = 600  # see bisect-r8152-workqueue.py history: full-tree build ~200-260s tonight, 2x margin
DEPLOY_TIMEOUT_S = 280  # bootm -> shell banner: ~39s bootm->"Starting kernel" (ram-boot-deploy.py's
# own measured figure for a similar-sized image) + initramfs unpack/init - 2x+ margin;
# tftp itself is NOT covered by this (tftp_and_verify has its own internal 90s bound)

_log_lines: list[str] = []


def log(msg: str) -> None:
    line = f"{time.strftime('%Y-%m-%dT%H:%M:%S%z')}  {msg}"
    print(line, flush=True)
    _log_lines.append(line)


def flush_log() -> None:
    LOG.parent.mkdir(parents=True, exist_ok=True)
    with LOG.open("a") as f:
        f.write("\n".join(_log_lines) + "\n")
    _log_lines.clear()


def run(cmd: list[str], cwd: Path | None = None, timeout: float = 60,
        env: dict | None = None, check: bool = True) -> subprocess.CompletedProcess:
    log("+ " + " ".join(str(c) for c in cmd))
    p = subprocess.run(cmd, cwd=cwd, env=env, capture_output=True, text=True,
                        timeout=timeout)
    if p.stdout:
        log("  " + p.stdout.strip()[-2000:])
    if p.stderr:
        log("  stderr: " + p.stderr.strip()[-2000:])
    if check and p.returncode != 0:
        raise subprocess.CalledProcessError(p.returncode, cmd, p.stdout, p.stderr)
    return p


def console_send(expect: str, cmd: str, timeout: float) -> tuple[bool, str]:
    try:
        p = subprocess.run(
            [sys.executable, "./dev.py", "console-send", "--expect", expect,
             "--timeout", str(timeout), cmd],
            cwd=REPO, capture_output=True, text=True, timeout=timeout + 10,
        )
    except subprocess.TimeoutExpired:
        return False, "(console-send itself timed out)"
    out = p.stdout + p.stderr
    return ("<<MATCHED:" in out), out


def prep_board_commits(commit: str) -> bool:
    run(["git", "checkout", "-f", commit], cwd=SRC, timeout=30)
    run(["git", "clean", "-fdx", "--", "arch/arm64/boot/dts/amazon",
         "drivers/pci/controller"], cwd=SRC, timeout=30, check=False)
    for c in BOARD_COMMITS:
        p = run(["git", "cherry-pick", "--allow-empty", c], cwd=SRC, timeout=30, check=False)
        if p.returncode != 0:
            log(f"cherry-pick {c} failed onto {commit} - not applicable, skipping candidate")
            run(["git", "cherry-pick", "--abort"], cwd=SRC, timeout=15, check=False)
            return False
    return True


def kver() -> str:
    return subprocess.check_output(
        ["make", "-s", "-C", str(SRC), f"O={KOUT}", "kernelrelease"], env=ENV,
    ).decode().strip()


def build_ea16_with_r8152() -> str | None:
    """Runs build-linux-ea16.py's own pipeline (proven initramfs-embedded
    netboot build), then injects r8152.ko/mii.ko into the initramfs and
    regenerates the image. Returns the kernel release, or None on failure
    (candidate should be SKIPped)."""
    try:
        run([sys.executable, "scripts/build-linux-ea16.py"], cwd=REPO,
            env=dict(ENV, AWTO_KERNEL_SRC=str(SRC)), timeout=BUILD_TIMEOUT_S)
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
        log("build-linux-ea16.py failed/timed out - skipping this candidate")
        return None

    kv = kver()
    cfg = KOUT / ".config"
    run([str(SRC / "scripts/config"), "--file", str(cfg),
         "--module", "USB_RTL8152", "--module", "CRYPTO_LIB_SHA256",
         "--module", "MII", "--enable", "PHYLIB"], timeout=30)
    run(["make", "-C", str(SRC), f"O={KOUT}", "olddefconfig"], env=ENV, timeout=60)
    try:
        run(["make", "-C", str(SRC), f"O={KOUT}", f"-j{NPROC}", "modules"],
            env=ENV, timeout=BUILD_TIMEOUT_S)
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
        log("r8152 module build failed - skipping this candidate")
        return None

    extra = EA16_OUT / f"initramfs-root/lib/modules/{kv}/extra"
    extra.mkdir(parents=True, exist_ok=True)
    r8152_ko = KOUT / "drivers/net/usb/r8152.ko"
    mii_ko = KOUT / "drivers/net/mii.ko"
    if not r8152_ko.exists() or not mii_ko.exists():
        log("r8152.ko/mii.ko missing after build - skipping")
        return None
    shutil.copy(r8152_ko, extra / "r8152.ko")
    shutil.copy(mii_ko, extra / "mii.ko")

    # Load r8152 during the init script's own modprobe loop, same as the
    # al_* modules - the crash (if this candidate has it) happens
    # automatically during boot, before the "serial shell" banner. That
    # banner is what we wait for below, regardless of whether r8152
    # crashed: the oops kills the modprobe subshell, not the whole init
    # script (confirmed pattern all night - "note: insmod[N] exited with
    # irqs disabled", not a full panic), so the banner is a reliable
    # "boot completed" signal either way; dmesg is the actual verdict.
    init_path = EA16_OUT / "initramfs-root/init"
    init_text = init_path.read_text()
    marker = 'for m in al_dma al_ssm al_eth al_sgpo; do'
    if marker not in init_text:
        log("init script's modprobe loop marker not found - skipping")
        return None
    init_text = init_text.replace(
        marker, 'for m in mii r8152 al_dma al_ssm al_eth al_sgpo; do', 1,
    )
    init_path.write_text(init_text)

    # Regenerate the embedded-initramfs Image (picks up the newly-added
    # modules dir) and re-wrap the uImage - same two steps
    # build-linux-ea16.py's own main() does after build_oot_modules().
    try:
        run(["make", "-C", str(SRC), f"O={KOUT}", f"-j{NPROC}", "Image", "dtbs"],
            env=ENV, timeout=BUILD_TIMEOUT_S)
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
        log("final Image rebuild failed - skipping")
        return None
    return kv


def mkuimage(image_path: Path, out_path: Path) -> None:
    IH_MAGIC, IH_OS_LINUX, IH_ARCH_ARM64, IH_TYPE_KERNEL, IH_COMP_NONE = (
        0x27051956, 5, 22, 2, 0)
    LOAD_ADDR = ENTRY_ADDR = 0x08000000
    data = image_path.read_bytes()
    dcrc = zlib.crc32(data) & 0xFFFFFFFF
    nm = b"bisect-r8152"[:31].ljust(32, b"\0")

    def hdr(hc: int) -> bytes:
        return struct.pack(">IIIIIIIBBBB32s", IH_MAGIC, hc, int(time.time()),
                            len(data), LOAD_ADDR, ENTRY_ADDR, dcrc, IH_OS_LINUX,
                            IH_ARCH_ARM64, IH_TYPE_KERNEL, IH_COMP_NONE, nm)

    h = hdr(zlib.crc32(hdr(0)) & 0xFFFFFFFF)
    out_path.write_bytes(h + data)


# The initramfs init script's own distinctive line, printed right before it
# execs the interactive shell (confirmed by reading build-out/initramfs-
# root/init in full) - the ONE reliable "boot reached the end" signal for
# this image, since there's no getty/login process to produce a login
# prompt. This lands whether or not r8152 crashed: an oops kills the
# insmod/modprobe subshell, not the whole init script (confirmed pattern:
# "note: insmod[N] exited with irqs disabled", not a full panic) - so
# reaching this banner means "finished booting", and dmesg is the actual
# pass/fail verdict, checked separately below.
SHELL_BANNER = "serial shell on ttyS0"


def deploy_and_test(kv: str) -> int:
    """Netboot the self-contained initramfs kernel directly (r8152/mii are
    already baked into its own modprobe loop - see build_ea16_with_r8152),
    reach the post-boot shell over the console, then check dmesg for the
    crash signature. Reuses ram-boot-deploy.py's proven tftpd-embed +
    catch-uboot-race + tftp_and_verify (single deploy concept, not a second
    hand-rolled reimplementation of the same delicate boot race)."""
    image = KOUT / "arch/arm64/boot/Image"
    dtb = KOUT / f"arch/arm64/boot/dts/amazon/{DTS_NAME}.dtb"
    if not image.exists() or not dtb.exists():
        log(f"Image or DTB missing ({image}, {dtb}) - skipping")
        return SKIP
    uimage = EA16_OUT / "uImage-bisect"
    mkuimage(image, uimage)

    _rbd.ensure_tftpd()
    server_ip = detect_server_ip()
    log(f"tftp server IP (auto-detected): {server_ip}")

    log("starting catch-uboot.py to win the autoboot race")
    catch = subprocess.Popen(
        [sys.executable, "scripts/catch-uboot.py", "--seconds", "60"], cwd=REPO)
    try:
        power_cycle_verified(log=log)
    except Exception:
        catch.kill()
        log("power-cycle failed - skipping")
        return SKIP
    try:
        rc = catch.wait(timeout=70)
    except subprocess.TimeoutExpired:
        catch.kill()
        log("catch-uboot.py hung waiting for the U-Boot prompt - skipping")
        return SKIP
    if rc != 0:
        log("catch-uboot.py did not reach the U-Boot prompt (autoboot won) - skipping")
        return SKIP
    log("U-Boot prompt reached, autoboot stopped")

    try:
        _rbd.run_devpy("--expect", "ALPINE_UBNT_NAS_ALL>", "--timeout", "8",
                        f"setenv ipaddr {_rbd.IPADDR}")
        _rbd.run_devpy("--expect", "ALPINE_UBNT_NAS_ALL>", "--timeout", "8",
                        f"setenv serverip {server_ip}")
        _rbd.tftp_and_verify(uimage, _rbd.KERNEL_ADDR)
        _rbd.tftp_and_verify(dtb, _rbd.DTB_ADDR)
    except (RuntimeError, subprocess.TimeoutExpired) as e:
        log(f"tftp staging failed: {e} - skipping")
        return SKIP

    ok, out = console_send(f"{SHELL_BANNER}|Kernel panic",
                            f"bootm {_rbd.KERNEL_ADDR} - {_rbd.DTB_ADDR}",
                            DEPLOY_TIMEOUT_S)
    if "Kernel panic" in out:
        log("kernel itself panicked before reaching the shell - "
            "genuinely unbootable, not just unrelated-to-r8152 -> SKIP")
        return SKIP
    if not ok or SHELL_BANNER not in out:
        log("never reached the init script's shell banner -> SKIP")
        return SKIP

    matched, dmesg_out = console_send(
        "BISECT_DMESG_DONE",
        "dmesg | grep -Eic 'Internal error|Unable to handle kernel' ; "
        "echo BISECT_DMESG_DONE",
        15,
    )
    if not matched:
        log("shell reached but dmesg check never returned (hung/crashed post-boot) -> BAD")
        return BAD
    crash_count = 0
    for line in dmesg_out.splitlines():
        line = line.strip()
        if line.isdigit():
            crash_count = int(line)
            break
    if crash_count > 0:
        log(f"CRASH detected ({crash_count} matching dmesg lines) -> BAD")
        return BAD
    log("no crash signature in dmesg -> GOOD")
    return GOOD


def run_once() -> int:
    commit = subprocess.run(["git", "rev-parse", "HEAD"], cwd=SRC,
                             capture_output=True, text=True, timeout=10).stdout.strip()
    log(f"=== bisect step: testing {commit} ===")

    if not prep_board_commits(commit):
        flush_log()
        return SKIP

    kv = build_ea16_with_r8152()
    if kv is None:
        flush_log()
        return SKIP
    log(f"built kernel release: {kv}")

    result = deploy_and_test(kv)
    log(f"=== result for {commit}: "
        f"{'GOOD' if result == GOOD else 'BAD' if result == BAD else 'SKIP'} ===")
    flush_log()
    return result


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.parse_args()
    return run_once()


if __name__ == "__main__":
    sys.exit(main())
