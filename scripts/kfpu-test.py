#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Stress the OpenZFS arm64 kfpu_begin/end save/restore on woomera (#364).

- Section code is extracted from the openzfs patch (truenas/scale-build/patches/sources/openzfs/0001-*),
  built into tools/kfpu-test/kfpu_test.ko against the TrueNAS kernel's headers deb.
- Build: native arm64 rootless podman container (qemu-user), image localhost/awto-kfpu-cc.
- Run: insmod, one kfpu_check per CPU (user regs + FPCR/FPSR verified across syscalls that run
  kfpu sections in the caller's context) while one kthread per CPU loops kfpu sections; rmmod.
- Any mismatch, oops or WARN = FAIL (exit 1).
- Limits: run = SECONDS + RUN_SLACK_S (checkers print only at the end; slack covers ssh, insmod,
  final report); build stall = no output for BUILD_STALL_S (qemu gcc, 1 small module).
- Log: tmp/logs/kfpu-test.log
"""

from __future__ import annotations

import argparse
import glob
import shutil
import subprocess
import sys
import threading
import time

import _box
from _repo import REPO, TMP, make_log

log = make_log("kfpu-test")
SRC = REPO / "tools" / "kfpu-test"
PATCH = (
    REPO
    / "truenas/scale-build/patches/sources/openzfs"
    / "0001-awto-arm64-kfpu-internal-neon-save-restore.patch"
)
KERNEL_DIR = "/mnt/2tb/truenas-build/kernel"
HDR = TMP / "truenas-oot" / "hdr"
HDR_REL = "usr/src/linux-headers-truenas-production-arm64"
BUILD = TMP / "kfpu-test" / "build"
IMAGE = "localhost/awto-kfpu-cc"
REMOTE = "/root/kfpu-test"  # TrueNAS /tmp is noexec
BUILD_STALL_S = 300
RUN_SLACK_S = 60

BUILD_SH = f"""set -e
H=/hdr/{HDR_REL}
cp /src/* /work/ && cd /work
make -C $H M=/work modules
gcc -O2 -mgeneral-regs-only -Wall -Wextra -static -o kfpu-check kfpu_check.c
strip --strip-debug kfpu_test.ko
cat $H/include/config/kernel.release
"""


def gen_header() -> None:
    """The HAVE_KERNEL_FPU_INTERNAL block of the patch, verbatim."""
    out, on = [], False
    for ln in PATCH.read_text().splitlines():
        if ln.startswith("+#elif defined(HAVE_KERNEL_FPU_INTERNAL)"):
            on = True
            continue
        if on and ln[1:].startswith("#else"):
            break
        if on:
            out.append(ln[1:])
    if not out:
        sys.exit(f"FATAL: no FPU_INTERNAL block in {PATCH}")
    (BUILD / "kfpu_arm64_gen.h").write_text("\n".join(out) + "\n")


def stream(cmd: list[str], stall: int, name: str) -> int:
    """Run cmd, log every line; kill on `stall` s of silence (repo stall-timeout rule)."""
    log("+ " + " ".join(cmd))
    p = subprocess.Popen(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
    )
    last = [time.time(), ""]

    def wd() -> None:
        while p.poll() is None:
            time.sleep(5)
            if time.time() - last[0] > stall:
                log(
                    f"FATAL: {name}: no output for {stall} s (last {last[1]!r}); killing"
                )
                p.kill()
                return

    threading.Thread(target=wd, daemon=True).start()
    for line in p.stdout:
        last[:] = [time.time(), line.strip()]
        log(line.rstrip())
    return p.wait()


def build() -> str:
    shutil.rmtree(BUILD, ignore_errors=True)
    BUILD.mkdir(parents=True)
    if not (HDR / HDR_REL).is_dir():
        deb = glob.glob(f"{KERNEL_DIR}/linux-headers-truenas-production-arm64_*.deb")
        if len(deb) != 1:
            sys.exit(f"FATAL: want 1 headers deb in {KERNEL_DIR}, got {deb}")
        HDR.mkdir(parents=True, exist_ok=True)
        subprocess.run(["dpkg-deb", "-x", deb[0], str(HDR)], check=True)
    gen_header()
    have = subprocess.run(["podman", "image", "exists", IMAGE]).returncode == 0
    if not have and stream(
        ["podman", "build", "--platform", "linux/arm64", "-t", IMAGE, "-f",
         str(SRC / "Containerfile"), str(SRC)], BUILD_STALL_S, "image build",
    ):  # fmt: skip
        sys.exit("FATAL: image build failed")
    rc = stream(
        ["podman", "run", "--rm", "--platform", "linux/arm64", "-v", f"{HDR}:/hdr:ro",
         "-v", f"{SRC}:/src:ro", "-v", f"{BUILD}:/work", IMAGE, "sh", "-c",
         BUILD_SH.replace("cp /src/* /work/", "cp /src/kfpu_test.c /src/kfpu_check.c /src/Kbuild /work/")],
        BUILD_STALL_S, "module build",
    )  # fmt: skip
    if (
        rc
        or not (BUILD / "kfpu_test.ko").exists()
        or not (BUILD / "kfpu-check").exists()
    ):
        sys.exit(f"FATAL: build failed rc={rc}")
    kr = (HDR / HDR_REL / "include/config/kernel.release").read_text().strip()
    log(f"built kfpu_test.ko + kfpu-check for {kr}")
    return kr


def ssh(ip: str, cmd: str, timeout: int) -> subprocess.CompletedProcess:
    argv = _box.ssh_argv(ip, cmd=[cmd], batch=True)
    try:
        return subprocess.run(
            argv, capture_output=True, text=True, timeout=timeout, check=False
        )
    except subprocess.TimeoutExpired as e:
        log(f"FATAL: ssh timed out after {timeout} s: {cmd[:80]}")
        return subprocess.CompletedProcess(argv, 124, e.stdout or "", "timeout")


def run(seconds: int, kr: str, sabotage: bool = False) -> int:
    ip = _box.require()
    r = ssh(ip, "uname -r; nproc", 20)
    box_kr, ncpu = r.stdout.split()
    log(f"box {ip} kernel {box_kr} cpus {ncpu}")
    if box_kr != kr:
        log(f"FATAL: box kernel {box_kr} != module kernel {kr} (vermagic)")
        return 2
    ssh(ip, f"rmmod kfpu_test 2>/dev/null; rm -rf {REMOTE}; mkdir -p {REMOTE}", 20)
    for f in ("kfpu_test.ko", "kfpu-check"):
        subprocess.run(
            ["scp", "-q", *_box.SSH_OPTS_BATCH, str(BUILD / f), f"root@{ip}:{REMOTE}/"],
            check=True, timeout=60,
        )  # fmt: skip
    ssh(ip, "dmesg -C >/dev/null 2>&1; true", 20)
    # module self-stops at the run limit, so a broken ssh can't leave it running (#365)
    stop_s = seconds + RUN_SLACK_S
    r = ssh(
        ip,
        f"insmod {REMOTE}/kfpu_test.ko sabotage={int(sabotage)} seconds={stop_s} && dmesg | tail -3",
        30,
    )
    log(f"insmod rc={r.returncode} {r.stdout.strip()} {r.stderr.strip()}")
    if r.returncode:
        return 2
    loop = (
        f"cd {REMOTE}; for c in $(seq 0 $(({ncpu}-1))); do "
        f"./kfpu-check $c {seconds} > out.$c 2>&1 & done; wait; cat out.*"
    )
    r = ssh(ip, loop, seconds + RUN_SLACK_S)
    for ln in r.stdout.splitlines():
        log("checker: " + ln)
    checker_bad = r.returncode != 0 or "mismatches 0" not in r.stdout
    checker_bad |= r.stdout.count("mismatches 0") != int(ncpu)
    r2 = ssh(
        ip,
        "rmmod kfpu_test; dmesg | grep -iE 'kfpu|BUG|Oops|WARNING|Call trace' | tail -20",
        30,
    )
    for ln in r2.stdout.splitlines():
        log("dmesg: " + ln)
    kern_bad = any(w in r2.stdout for w in ("BUG", "Oops", "WARNING", "Call trace"))
    ok = not (checker_bad or kern_bad)
    if sabotage:  # negative control: checker output must report mismatches; dead ssh proves nothing
        detected = (
            r.returncode == 0
            and "mismatches" in r.stdout
            and r.stdout.count("mismatches 0") < int(ncpu)
        )
        verdict = (
            "CONTROL-OK (mismatches detected)"
            if detected
            else "CONTROL-INCONCLUSIVE (no checker report)"
        )
        if r.returncode == 0 and r.stdout.count("mismatches 0") == int(ncpu):
            verdict = "CONTROL-FAIL (checker blind)"
        log(f"RESULT {verdict}")
        return 0 if detected else 1
    log(f"RESULT {'PASS' if ok else 'FAIL'} ({seconds} s x {ncpu} cpus)")
    return 0 if ok else 1


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawTextHelpFormatter
    )
    ap.add_argument("--seconds", type=int, default=240, help="checker duration per CPU")
    ap.add_argument(
        "--sabotage",
        action="store_true",
        help="negative control: no save/restore, must FAIL",
    )
    ap.add_argument("--build-only", action="store_true")
    ap.add_argument(
        "--skip-build", action="store_true", help="reuse tmp/kfpu-test/build"
    )
    a = ap.parse_args()
    if a.skip_build:
        kr = (HDR / HDR_REL / "include/config/kernel.release").read_text().strip()
    else:
        kr = build()
    return 0 if a.build_only else run(a.seconds, kr, a.sabotage)


if __name__ == "__main__":
    sys.exit(main())
