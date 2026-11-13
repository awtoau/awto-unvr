#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Self-built arm64 TrueNAS 27.0 userland via scale-build in an emulated arm64 container (#356).

- Base: joel0-truenas/scale-build release/26.0.0-BETA.1-aarch64 at PIN_SHA, plus
  truenas/scale-build/patches/scale-build/*.patch (27.0.0 bump).
- Per-source patches: truenas/scale-build/patches/sources/<manifest name>/*.patch, applied after
  `make checkout` (which resets each tree to its branch head).
- Build state: $AWTO_TRUENAS_BUILD (default /mnt/2tb/truenas-build), outside any repo.
- Steps: container | checkout | packages [--only a,b] | update | iso | status.
- Guards: free space on the build fs < MIN_FREE_GB -> kill; no growth in stdout or
  scale-build's logs/ for --stall s -> kill. Max silence seen is logged (tune --stall).
- Log: tmp/logs/truenas-build.log
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
import threading
import time
from pathlib import Path

from _repo import (
    LINUX_STORE,
    NPROC,
    REPO,
    build_ident,
    make_log,
    rel,
    truenas_build_out,
)

log = make_log("truenas-build")

WORK = Path(os.environ.get("AWTO_TRUENAS_BUILD") or "/mnt/2tb/truenas-build")
SB = WORK / "scale-build"
CONF = REPO / "truenas" / "scale-build"
FORK = "https://github.com/joel0-truenas/scale-build"
FORK_BRANCH = "release/26.0.0-BETA.1-aarch64"
PIN_SHA = "cd53605006f4bffb5a0e8e4f44368e904e5fa39c"
LOCAL = WORK / "local"
# stand-ins for TrueNAS's closed packages, one Debian source package per dir
STUBS = REPO / "truenas" / "stubs"
KWORK = WORK / "kernel"
KIMAGE = "localhost/awto-truenas-kbuild:trixie"
KERNEL_DEBS = (
    "linux-image-truenas-production-arm64",
    "linux-headers-truenas-production-arm64",
    "linux-truenas-production-libc-dev",
)
IMAGE = "localhost/awto-truenas-build:trixie"
CTR_NAME = "awto-truenas-build"
MIN_FREE_GB = 15  # Dan 2026-11-14: /mnt/2tb shared, ~82 GB free at start
TRAIN = "TrueNAS-27"  # unverified train name; only matters for online update checks
# Emulated single-file compiles can be quiet for minutes; measured max silence goes in #356.
DEFAULT_STALL_S = 900
# Overall backstop per step (s); provisional until a cold run is measured (#356).
BACKSTOP_S = {
    "container": 3600,
    "kernel": 3 * 3600,
    "checkout": 2 * 3600,
    "packages": 30 * 3600,
    "update": 6 * 3600,
    "iso": 3 * 3600,
}


def free_gb(path: Path) -> float:
    return shutil.disk_usage(path).free / 2**30


def log_df(tag: str) -> float:
    gb = free_gb(WORK)
    log(f"df {tag}: {gb:.1f} GB free on {WORK} (min {MIN_FREE_GB})")
    return gb


def check_free(tag: str) -> None:
    if log_df(tag) < MIN_FREE_GB:
        sys.exit(f"FATAL: under {MIN_FREE_GB} GB free on {WORK} - stopping ({tag})")


def git(*args, cwd: Path = SB, check: bool = True) -> subprocess.CompletedProcess:
    log("+ git " + " ".join(args))
    r = subprocess.run(["git", *args], cwd=cwd, text=True, capture_output=True)
    if check and r.returncode:
        fatal(f"git {' '.join(args)} rc={r.returncode}: {r.stderr.strip()}")
    return r


def tree_bytes(path: Path) -> int:
    total = 0
    for root, _dirs, files in os.walk(path):
        for f in files:
            try:
                total += os.lstat(os.path.join(root, f)).st_size
            except OSError:
                pass
    return total


def run_guarded(
    cmd: list[str], step: str, stall_s: int, kill_name: str = CTR_NAME
) -> int:
    """Run cmd, timestamp its output into the log, kill on stall / low disk / backstop."""
    check_free(f"{step} start")
    log("+ " + " ".join(cmd))
    t0 = time.monotonic()
    proc = subprocess.Popen(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
    )
    state = {
        "last": time.monotonic(),
        "logs": -1,
        "max_quiet": 0.0,
        "why": None,
        "line": "",
    }
    pkg_start: dict[str, float] = {}
    peak_used = [0.0]
    free0 = free_gb(WORK)

    def reader() -> None:
        assert proc.stdout
        for line in proc.stdout:
            line = line.rstrip("\n")
            now = time.monotonic()
            state["max_quiet"] = max(state["max_quiet"], now - state["last"])
            state["last"], state["line"] = now, line
            log(f"[{step}] {line}")
            if "Successfully built '" in line:
                name = line.split("'")[1]
                started = pkg_start.get(name)
                dur = (
                    f"{now - started:.0f} s (from first log write)" if started else "?"
                )
                log(f"PKG {name} done in {dur}")

    def watchdog() -> None:
        pkgs_dir = SB / "logs" / "packages"
        while proc.poll() is None:
            time.sleep(30)  # watchdog poll period, in-process; not a shell sleep
            now = time.monotonic()
            if pkgs_dir.is_dir():
                for f in pkgs_dir.glob("*.log"):
                    pkg_start.setdefault(f.stem, now)
            size = tree_bytes(SB / "logs") if (SB / "logs").is_dir() else 0
            if size != state["logs"]:
                state["max_quiet"] = max(state["max_quiet"], now - state["last"])
                state["logs"], state["last"] = size, now
            gb = free_gb(WORK)
            peak_used[0] = max(peak_used[0], free0 - gb)
            why = None
            if gb < MIN_FREE_GB:
                why = f"free space {gb:.1f} GB < {MIN_FREE_GB} GB"
            elif now - state["last"] > stall_s:
                why = f"stall: no stdout or logs/ growth for {now - state['last']:.0f} s (limit {stall_s})"
            elif now - t0 > BACKSTOP_S[step]:
                why = f"backstop: {now - t0:.0f} s > {BACKSTOP_S[step]} s"
            if why:
                state["why"] = why
                log(
                    f"KILL {step}: {why}; last line: {state['line']!r}; loadavg {os.getloadavg()}"
                )
                subprocess.run(["sudo", "podman", "kill", kill_name], check=False)
                proc.terminate()  # SIGTERM: sudo relays it, SIGKILL it cannot
                return

    threads = [
        threading.Thread(target=reader, daemon=True),
        threading.Thread(target=watchdog, daemon=True),
    ]
    for t in threads:
        t.start()
    rc = proc.wait()
    threads[0].join()
    dt = time.monotonic() - t0
    log(
        f"STEP {step} rc={rc} elapsed {dt:.0f} s ({dt / 3600:.2f} h); max silence {state['max_quiet']:.0f} s; "
        f"peak extra fs use {peak_used[0]:.1f} GB (free at start {free0:.1f} GB)"
    )
    log_df(f"{step} end")
    if state["why"]:
        log(f"FATAL: {step} killed - {state['why']}")
        return 124
    return rc


# --network host: rootful podman's bridge has no working DNS on this host.
def in_container(argv: list[str], step: str, stall_s: int) -> int:
    """argv runs as root in the arm64 container: /build = SB, /awto = CONF (ro), /local = LOCAL."""
    LOCAL.mkdir(parents=True, exist_ok=True)
    cmd = [
        "sudo", "podman", "run", "--rm", "--name", CTR_NAME, "--platform", "linux/arm64",
        "--privileged", "--network", "host", "-v", f"{SB}:/build", "-v", f"{CONF}:/awto:ro",
        "-v", f"{LOCAL}:/local:ro", "-w", "/build", "-e", f"TRUENAS_TRAIN={TRAIN}", IMAGE, *argv,
    ]  # fmt: skip
    return run_guarded(cmd, step, stall_s)


def step_container(a) -> int:
    """Both images: arm64 scale-build host, x86_64 kernel cross-build host."""
    WORK.mkdir(parents=True, exist_ok=True)
    for plat, tag, cf in (
        ("linux/arm64", IMAGE, "Containerfile"),
        ("linux/amd64", KIMAGE, "Containerfile.kbuild"),
    ):
        cmd = [
            "sudo", "podman", "build", "--network", "host", "--platform", plat, "-t", tag,
            "-f", str(CONF / cf), str(CONF),
        ]  # fmt: skip
        if rc := run_guarded(cmd, "container", a.stall):
            return rc
    return 0


def prepare_scale_build() -> None:
    """SB = fork at PIN_SHA + our scale-build patches, rebuilt every time (patches are the source)."""
    WORK.mkdir(parents=True, exist_ok=True)
    if not (SB / ".git").is_dir():
        git("clone", "--branch", FORK_BRANCH, FORK, str(SB), cwd=WORK)
    if git("cat-file", "-e", f"{PIN_SHA}^{{commit}}", check=False).returncode:
        git("fetch", "origin", FORK_BRANCH)
    git(
        "checkout", "-q", "-f", "-B", "awto-27", PIN_SHA
    )  # -f: last run edited the manifest
    patches = sorted((CONF / "patches" / "scale-build").glob("*.patch"))
    if patches:
        r = git("-c", "user.name=awto", "-c", "user.email=noreply@awto.au", "am", "-q",
                *map(str, patches), check=False)  # fmt: skip
        if r.returncode:
            git("am", "--abort", check=False)
            sys.exit(f"FATAL: scale-build patch failed:\n{r.stdout}{r.stderr}")
    log(
        f"scale-build at {git('rev-parse', '--short', 'HEAD').stdout.strip()} ({len(patches)} patches on {PIN_SHA[:7]})"
    )


def fatal(msg: str) -> None:
    log(f"FATAL: {msg}")
    sys.exit(1)


def apply_source_patches(stall_s: int) -> None:
    """sources/ is root-owned (checkout ran in the container), so patch there too."""
    script = ["set -e"]
    for d in sorted((CONF / "patches" / "sources").iterdir()):
        if not (SB / "sources" / d.name / ".git").exists():
            fatal(
                f"sources/{d.name} missing - manifest name {d.name!r} not checked out"
            )
        for p in sorted(d.glob("*.patch")):
            script.append(
                f"git -C sources/{d.name} apply --index /awto/patches/sources/{d.name}/{p.name}"
                f" && echo 'patched sources/{d.name}: {p.name}'"
            )
    if in_container(["sh", "-c", "\n".join(script)], "checkout", stall_s):
        fatal("a source patch does not apply (see the lines above)")
    invalidate_on_patch_change()


def invalidate_on_patch_change() -> None:
    """scale-build keys rebuilds on HEAD; a changed patch on the same HEAD must force one."""
    state = WORK / "patch-state"
    state.mkdir(exist_ok=True)
    for d in sorted((CONF / "patches" / "sources").iterdir()):
        digest = hashlib.sha256(
            b"".join(p.read_bytes() for p in sorted(d.glob("*.patch")))
        )
        rec = state / d.name
        if rec.exists() and rec.read_text() != digest.hexdigest():
            log(
                f"patches for {d.name} changed - dropping its pkghash to force a rebuild"
            )
            subprocess.run(
                ["sudo", "rm", "-f", str(SB / "tmp" / "pkghashes" / f"{d.name}.hash")],
                check=True,
            )
        rec.write_text(digest.hexdigest())


def stub_dirs() -> list[Path]:
    """Every truenas/stubs/<name>/ with a debian/control: one scale-build source each."""
    if not STUBS.is_dir():
        log(
            f"WARNING: {rel(STUBS)} absent - no stand-ins for TrueNAS's closed packages"
        )
        return []
    dirs = sorted(d for d in STUBS.iterdir() if (d / "debian" / "control").is_file())
    for d in sorted(set(STUBS.iterdir()) - set(dirs)):
        log(f"WARNING: {rel(d)} has no debian/control - skipped")
    return dirs


def snapshot_local_sources() -> None:
    """Each stub dir -> git repo LOCAL/<name> (branch main) + manifest source, middlewared
    explicit dep and base-packages entries for its binary packages. Rebuilt every run."""
    if LOCAL.exists():
        shutil.rmtree(LOCAL)
    LOCAL.mkdir(parents=True)
    sources, deps, base = [], [], []
    for src in stub_dirs():
        name, dst = src.name, LOCAL / src.name
        shutil.copytree(src, dst, ignore=shutil.ignore_patterns("__pycache__", "*.pyc"))
        for argv in (["init", "-q", "-b", "main"], ["add", "-A"],
                     ["-c", "user.name=awto", "-c", "user.email=noreply@awto.au", "commit", "-q",
                      "-m", f"snapshot of awto-unvr {rel(src)}"]):  # fmt: skip
            git(*argv, cwd=dst)
        pkgs = re.findall(
            r"^Package:\s*(\S+)", (src / "debian" / "control").read_text(), re.M
        )
        sha = git("rev-parse", "--short", "HEAD", cwd=dst).stdout.strip()
        log(f"local source {name}: {rel(src)} @ {sha} -> {', '.join(pkgs)}")
        sources.append(
            f"- name: {name}\n  repo: file:///local/{name}\n  branch: main\n"
        )
        deps.append(f"        - {name}\n")
        base += [f"- name: {p}\n  install_recommends: true\n" for p in pkgs]
    mf = SB / "conf" / "build.manifest"
    text = mf.read_text()
    anchor_dep = (
        "        - truenas_pydiscovery\n"  # in middlewared's explicit_deps (patch 0001)
    )
    for anchor in ("\nsources:\n", "\nbase-packages:\n", anchor_dep):
        if text.count(anchor) != 1:
            fatal(f"manifest anchor {anchor!r} not found exactly once")
    text = text.replace("\nsources:\n", "\nsources:\n" + "".join(sources))
    text = text.replace("\nbase-packages:\n", "\nbase-packages:\n" + "".join(base))
    text = text.replace(anchor_dep, anchor_dep + "".join(deps))
    mf.write_text(text)


def step_checkout(a) -> int:
    prepare_scale_build()
    snapshot_local_sources()
    rc = in_container(["make", "checkout"], "checkout", a.stall)
    if rc:
        return rc
    apply_source_patches(a.stall)
    log(f"sources: {tree_bytes(SB / 'sources') / 2**30:.1f} GB")
    return 0


def step_packages(a) -> int:
    args = ["packages"] + ([f"PACKAGES={a.only}"] if a.only else [])
    return in_container(["make", *args], "packages", a.stall)


def step_update(a) -> int:
    return in_container(["make", "update"], "update", a.stall)


def step_iso(a) -> int:
    return in_container(["make", "iso"], "iso", a.stall)


def kernel_release_suffix() -> str:
    return f"-truenas-{build_ident('kernel')}"


# Cross build on the host arch (no emulation): ~ build-truenas's own kernel time, not hours.
KBUILD_SH = r"""set -e
cd /kobj
M="make -C $KSRC O=/kobj ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-"
$M olddefconfig
$M archprepare  # writes include/config/kernel.release (mkdebian + kernelrelease read it)
KR=$($M -s kernelrelease)
echo "KERNELRELEASE=$KR"
# mkdebian dch's then copies scripts/package/truenas/* relative to the objtree: writable copy
rm -rf scripts/package/truenas && mkdir -p scripts/package && cp -r $KSRC/scripts/package/truenas scripts/package/
# KDEB_PKGVERSION: mkdebian's fallback runs ./scripts/* from the srctree, not the objtree
KDEB_PKGVERSION=$KR-1 KDEB_CHANGELOG_DIST=trixie $M run-command KBUILD_RUN_COMMAND='$(srctree)/scripts/package/mkdebian'
# no perf package: cross-building tools/perf needs the arm64 userland dev libs
sed -i '/^Package: linux-perf-truenas$/,/^$/d' debian/control
export DEB_BUILD_PROFILES="pkg.linux-$KR.nokerneldbg" DEB_BUILD_OPTIONS="terse"
export ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
dpkg-buildpackage --build=binary --no-pre-clean --unsigned-changes -R'make -f debian/rules'     -j$NPROC -a$(cat debian/arch) --no-check-builddeps  # -jN sets DEB_BUILD_OPTIONS parallel=N
ls -la /kout/*.deb
"""


def step_kernel(a) -> int:
    """Our 6.18 tree + build-truenas's .config (LOCALVERSION refreshed) -> TrueNAS-named .debs."""
    import _truenas  # noqa: PLC0415 - only this step needs the kernel tree helpers

    ksrc = _truenas.build_tree().resolve()
    cfg = truenas_build_out() / "kbuild" / ".config"
    if not cfg.exists():
        fatal(f"no {cfg} - run ./dev.py build-truenas first (it configures the tree)")
    store = LINUX_STORE.resolve()
    obj = KWORK / "obj"
    obj.mkdir(parents=True, exist_ok=True)
    for old in KWORK.glob("*.deb"):
        old.unlink()
    text = cfg.read_text()
    text = re.sub(
        r'^CONFIG_LOCALVERSION=".*"$',
        f'CONFIG_LOCALVERSION="{kernel_release_suffix()}"',
        text,
        flags=re.M,
    )
    (obj / ".config").write_text(text)
    # `make debian` is a directory target: a stale debian/ makes it a no-op
    shutil.rmtree(obj / "debian", ignore_errors=True)
    cmd = [
        "sudo", "podman", "run", "--rm", "--name", f"{CTR_NAME}-kbuild", "--network", "host",
        "--user", f"{os.getuid()}:{os.getgid()}", "-e", "HOME=/tmp",
        "-v", f"{ksrc}:{ksrc}:ro", "-v", f"{store}:{store}:ro", "-v", f"{obj}:/kobj",
        "-v", f"{KWORK}:/kout", "-e", f"KSRC={ksrc}", "-e", f"NPROC={NPROC}",
        KIMAGE, "sh", "-c", KBUILD_SH,
    ]  # fmt: skip
    rc = run_guarded(cmd, "kernel", a.stall, kill_name=f"{CTR_NAME}-kbuild")
    if rc:
        return rc
    debs = sorted(p.name for p in KWORK.glob("*.deb"))
    for want in KERNEL_DEBS:
        if not any(d.startswith(want + "_") for d in debs):
            fatal(f"kernel build made no {want} .deb (have {debs})")
    log(f"kernel debs: {debs}")
    return inject_kernel_debs(a.stall)


def inject_kernel_debs(stall_s: int) -> int:
    """Copy KWORK/*.deb into scale-build's local apt repo and re-index it."""
    sh = "set -e; mkdir -p tmp/pkgdir; rm -f tmp/pkgdir/linux-*truenas*.deb; cp /kout/*.deb tmp/pkgdir/; "
    sh += "cd tmp/pkgdir && dpkg-scanpackages --multiversion . /dev/null | gzip -9c > Packages.gz; ls -la linux-*"
    cmd = [
        "sudo", "podman", "run", "--rm", "--name", f"{CTR_NAME}-inject", "--platform", "linux/arm64",
        "-v", f"{SB}:/build", "-v", f"{KWORK}:/kout:ro", "-w", "/build", IMAGE, "sh", "-c", sh,
    ]  # fmt: skip
    return run_guarded(cmd, "kernel", stall_s, kill_name=f"{CTR_NAME}-inject")


def step_status(_a) -> int:
    log_df("status")
    for sub in ("sources", "tmp/pkgdir", "tmp/release", "tmp/cache"):
        p = SB / sub
        if p.is_dir():
            log(f"{sub}: {tree_bytes(p) / 2**30:.2f} GB")
    pkgdir = SB / "tmp" / "pkgdir"
    debs = sorted(pkgdir.glob("*.deb")) if pkgdir.is_dir() else []
    log(f"{len(debs)} .deb in tmp/pkgdir")
    for h in (
        sorted((SB / "tmp" / "pkghashes").glob("*.hash"))
        if (SB / "tmp" / "pkghashes").is_dir()
        else []
    ):
        log(f"built: {h.stem}")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "step",
        choices=[
            "container",
            "kernel",
            "checkout",
            "packages",
            "update",
            "iso",
            "status",
        ],
    )
    ap.add_argument("--only", help="packages: comma list of manifest source names")
    ap.add_argument("--stall", type=int, default=DEFAULT_STALL_S, help="stall limit, s")
    a = ap.parse_args()
    fn = {
        "container": step_container, "kernel": step_kernel, "checkout": step_checkout, "packages": step_packages,
        "update": step_update, "iso": step_iso, "status": step_status,
    }[a.step]  # fmt: skip
    return fn(a)


if __name__ == "__main__":
    sys.exit(main())
