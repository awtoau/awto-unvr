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

Boots the FULL FEDORA ROOTFS on disk (via ram-boot-deploy.py), not an
initramfs. An earlier initramfs-based design (console+USB only, no
AHCI/disk dependency) was tried first for speed, but #190's own comment
thread shows why it doesn't work as a bisect oracle: the same commit that
crashes 6/6 times on the Fedora boot came back clean across 5 retries in
the lightweight initramfs boot, even with USB enumeration confirmed
genuine both times. That points to an environmental/timing difference
(a much heavier concurrent Fedora boot vs. a near-idle initramfs boot)
that the initramfs path can't reproduce - so it can't be trusted not to
report a false GOOD on an actually-bad commit, which would poison the
bisect. The Fedora-rootfs path costs a real disk boot per candidate but
has the only actual reproduction record (6/6).

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
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
# build-linux-fedora.py and _repo import each need the direct-invocation
# guard bypassed (see _repo.py's _enforce_via_devpy) - this script (driven
# by `git bisect run`, which needs a bare argv and can't go through dev.py's
# subcommand dispatch) is itself always a direct invocation, so it opts in
# for itself and every subprocess it spawns.
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
from _repo import kernel_build_out, kernel_build_ver  # noqa: E402
from _console import connect as console_connect  # noqa: E402
from _console import login as console_login  # noqa: E402
from _console import sh as console_sh  # noqa: E402

SRC = Path(os.environ.get("AWTO_KERNEL_SRC", "/mnt/2tb/unvr-port-refs/linux-v7.3-fresh"))
# MUST be the KASAN variant: all 6 original crash reproductions tonight were
# on AWTO_KASAN_BUILD=1 kernels. The bug is a torn/racy pointer read - without
# KASAN's shadow-memory instrumentation to catch it, a garbage-but-plausible
# pointer read can silently "succeed" instead of oopsing. Confirmed live:
# the plain (non-KASAN) build ran clean (0/3) on the exact commit that
# crashed 6/6 on KASAN builds.
FEDORA_OUT = kernel_build_out(kasan=True)
FEDORA_KOUT = FEDORA_OUT / "kbuild"
VER = kernel_build_ver()
LOG = REPO / "tmp/logs/bisect-r8152-workqueue.log"
ENV = dict(os.environ, ARCH="arm64", CROSS_COMPILE="aarch64-linux-gnu-",
           AWTO_ALLOW_DIRECT_SCRIPT="1", AWTO_VIA_DEVPY="1", AWTO_KASAN_BUILD="1")

BOARD_COMMITS = ["3a8d7a1aed24", "2c9316496bad", "ff8d1c237ea6", "ae48f7150404"]
DTS_NAME = "alpine-v2-ubnt-unvr-ea16"

# Exit codes git-bisect run understands: 0 = good, 1-124/126-127 = bad, 125 = skip.
GOOD, BAD, SKIP = 0, 1, 125

# Real measured build-linux-fedora.py TOTAL time with a warm ccache: 208s
# (tmp/logs/bisect-fedora-timing-test.log). That run benefited from
# tonight's extensive prior Fedora builds keeping ccache hot; a bisect
# candidate further from cache-warm state (a big refactor commit) will
# cost more, plus this script's own separate r8152/mii module build on
# top - budget ~6x that measured figure, not the measured figure itself.
BUILD_TIMEOUT_S = 1200
# ram-boot-deploy.py's own internal wait-for-boot bound is 310s (its own
# comment: ~39s bootm->"Starting kernel" for a similar-sized image, plus
# margin) - this wraps the WHOLE subprocess (power-cycle + catch-uboot +
# 2 tftp transfers + boot + wait-for-login), so budget generously above
# that rather than re-deriving each sub-step's cost here.
DEPLOY_TIMEOUT_S = 500

R8152_ATTEMPTS = 3  # the original crash reproduced on the FIRST insmod after
# a fresh boot, 6/6 times - this isn't chasing a missed race like the
# initramfs retry loop was, just cheap extra confidence once we've already
# paid for a full disk boot.

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
    """Read what kbuild itself wrote, not a separate `make kernelrelease`
    invocation - build-linux-fedora.py's own kver() docs a real, caught-live
    staleness bug in the latter (a KASAN run followed by a plain run can
    keep printing the previous run's release string)."""
    path = FEDORA_KOUT / "include/config/kernel.release"
    if not path.exists():
        raise FileNotFoundError(f"{path} missing - build before calling kver()")
    return path.read_text().strip()


def build_fedora_with_r8152() -> str | None:
    """Runs build-linux-fedora.py's own pipeline, then explicitly re-enables
    r8152/mii and builds them separately. build-linux-fedora.py's configure()
    runs `localmodconfig` against a real lsmod snapshot from the box
    (scripts/woomera-lsmod-known-good.txt) BEFORE r8152 was ever tested -
    confirmed live: the resulting .config has `# CONFIG_USB_RTL8152 is not
    set` despite the raw Fedora config shipping it as =m, so the normal
    modules_install step never installs it. Returns the kernel release, or
    None on failure (candidate -> SKIP)."""
    try:
        run([sys.executable, "scripts/build-linux-fedora.py"], cwd=REPO,
            env=dict(ENV, AWTO_KERNEL_SRC=str(SRC)), timeout=BUILD_TIMEOUT_S)
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
        log("build-linux-fedora.py failed/timed out - skipping this candidate")
        return None

    kv = kver()
    uimage = FEDORA_OUT / f"uImage-unvr-ea16-{VER}-fedora"
    dtb = FEDORA_OUT / f"{DTS_NAME}-{VER}.dtb"
    modroot = FEDORA_OUT / "modroot"
    if not (uimage.exists() and dtb.exists()):
        log(f"expected build outputs missing (uimage={uimage.exists()}, "
            f"dtb={dtb.exists()}) - skipping")
        return None

    cfg = FEDORA_KOUT / ".config"
    run([str(SRC / "scripts/config"), "--file", str(cfg),
         "--module", "USB_RTL8152", "--module", "MII"], timeout=30)
    run(["make", "-C", str(SRC), f"O={FEDORA_KOUT}", "olddefconfig"], env=ENV, timeout=60)
    try:
        run(["make", "-C", str(SRC), f"O={FEDORA_KOUT}", f"-j{os.cpu_count() or 4}", "modules"],
            env=ENV, timeout=BUILD_TIMEOUT_S)
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
        log("r8152 module build failed - skipping this candidate")
        return None

    r8152_ko = FEDORA_KOUT / "drivers/net/usb/r8152.ko"
    mii_ko = FEDORA_KOUT / "drivers/net/mii.ko"
    if not r8152_ko.exists() or not mii_ko.exists():
        log("r8152.ko/mii.ko missing after build - skipping")
        return None
    extra = modroot / f"lib/modules/{kv}/extra"
    extra.mkdir(parents=True, exist_ok=True)
    shutil.copy(r8152_ko, extra / "r8152.ko")
    shutil.copy(mii_ko, extra / "mii.ko")
    return kv


def deploy_and_test(kv: str) -> int:
    """Deploy via ram-boot-deploy.py (power-cycle, catch-uboot race, tftp,
    boot, wait for login - all handled internally, single deploy concept,
    not a second hand-rolled reimplementation), then probe r8152 over the
    console and check dmesg for the crash signature."""
    uimage = FEDORA_OUT / f"uImage-unvr-ea16-{VER}-fedora"
    dtb = FEDORA_OUT / f"{DTS_NAME}-{VER}.dtb"
    modroot = FEDORA_OUT / "modroot"
    try:
        run([sys.executable, "scripts/ram-boot-deploy.py",
             "--kernel", str(uimage), "--dtb", str(dtb),
             "--modules-dir", str(modroot)],
            cwd=REPO, env=ENV, timeout=DEPLOY_TIMEOUT_S)
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
        log("ram-boot-deploy.py failed/timed out - skipping this candidate")
        return SKIP

    s = console_connect()
    console_login(s)
    rc, out = console_sh(
        s,
        "rmmod r8152 2>/dev/null; rmmod mii 2>/dev/null; "
        f"for i in $(seq 1 {R8152_ATTEMPTS}); do "
        "modprobe r8152 2>&1; echo attempt $i insmod rc=$?; rmmod r8152 2>&1; "
        "done",
        timeout=60,
    )
    if rc is None:
        log(f"r8152 probe loop never returned (box likely crashed hard) -> BAD\n{out}")
        return BAD

    rc, out = console_sh(
        s, "dmesg | grep -Eic 'Internal error|Unable to handle kernel'", timeout=15,
    )
    if rc is None:
        log("dmesg check never returned (hung/crashed post-probe) -> BAD")
        return BAD
    crash_count = int(out.strip().splitlines()[-1]) if out.strip() else 0
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

    kv = build_fedora_with_r8152()
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
