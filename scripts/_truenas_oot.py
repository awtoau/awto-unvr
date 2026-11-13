# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""OpenZFS + SCST kernel modules for the TrueNAS 6.18 kernel (#338 phase 2).

- Sources: REFS/truenas-zfs @ ZFS_PIN, REFS/truenas-scst @ SCST_PIN (_truenas.py). The
  checkouts stay pristine; each build runs in a fresh copy under OUT/oot-src/.
- Recipes follow TrueNAS scale-build's `openzfs` and `scst` steps (conf/build.manifest):
  ZFS `configure --with-config=kernel` + `make -C module`; SCST `make 2perf`, then the
  debian/rules SUBDIRS with BUILD_2X_MODULE / CONFIG_SCSI_QLA_* set.
- Installed to the paths the 26.04 image's dpkg lists use (EXPECTED), stripped, signed.
"""

from __future__ import annotations

import os
import pathlib
import shutil
import subprocess
import sys

import _truenas
from _kbuild import CROSS

from _repo import NPROC, REFS, REPO

# relpath under lib/modules/<kver>/extra/, from the image's openzfs-zfs-modules-* and scst .list
ZFS_KOS = ("zcommon/spl.ko", "zcommon/zfs.ko")
SCST_KOS = (
    "scst.ko",
    *(
        f"dev_handlers/scst_{h}.ko"
        for h in (
            "cdrom", "changer", "disk", "modisk", "processor", "raid", "tape", "user", "vdisk",
        )
    ),
    "fcst.ko",
    "ib_srpt.ko",
    "iscsi-scst.ko",
    "isert-scst.ko",
    "qla2x00tgt.ko",
    "qla2xxx_scst.ko",
    "scst_local.ko",
)  # fmt: skip
EXPECTED = ZFS_KOS + SCST_KOS
# debian/rules SUBDIRS minus scstadmin (userspace, from the image); fcst because LIBFC=m.
SCST_SUBDIRS = (
    "scst", "fcst", "iscsi-scst", "qla2x00t-32gbit/qla2x00-target", "scst_local", "srpt",
)  # fmt: skip
# Local clone/fetch of a small repo; network fetch of one pin is the long pole.
CLONE_TIMEOUT_S = 600


def ensure_src(name: str, url: str, pin: str, log) -> pathlib.Path:
    """REFS/<name> at `pin`, detached. Clones if missing; FATAL on a dirty tree."""
    src = REFS / name
    if not src.exists():
        _git(["clone", "-q", "--filter=blob:none", url, str(src)], REFS, log)
    if _git(["status", "--porcelain"], src, log).strip():
        log(f"FATAL: {src} has local changes; it must stay pristine")
        sys.exit(1)
    if subprocess.run(
        ["git", "-C", str(src), "cat-file", "-e", f"{pin}^{{commit}}"]
    ).returncode:
        _git(["fetch", "-q", "origin", pin], src, log)
    _git(["checkout", "-q", "--detach", pin], src, log)
    return src


def _git(args, cwd, log) -> str:
    try:
        r = subprocess.run(
            ["git", *args],
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=CLONE_TIMEOUT_S,
        )
    except subprocess.TimeoutExpired:
        log(f"FATAL: git {' '.join(args)}: no result after {CLONE_TIMEOUT_S} s")
        sys.exit(1)
    if r.returncode:
        log(f"FATAL: git {' '.join(args)} in {cwd}: {r.stderr.strip()}")
        sys.exit(1)
    return r.stdout


def stage(run, log, src: pathlib.Path, work: str, name: str) -> str:
    """Fresh copy of `src` (no .git) at `work`, then patches/<name>/*.patch (reasons in
    each patch's header). Builds write into the tree, so never into the checkout."""
    if os.path.exists(work):
        shutil.rmtree(work)
    shutil.copytree(src, work, ignore=shutil.ignore_patterns(".git"), symlinks=True)
    for p in sorted((REPO / "patches" / name).glob("*.patch")):
        run(["git", "apply", "-p1", str(p)], cwd=work)
        log(f"patched {name}: {p.name}")
    return work


def build_zfs(run, log, ksrc: str, kout: str, oot: str) -> str:
    src = ensure_src("truenas-zfs", _truenas.ZFS_REPO, _truenas.ZFS_PIN, log)
    work = stage(run, log, src, os.path.join(oot, "zfs"), "truenas-zfs")
    run(["sh", "autogen.sh"], cwd=work)
    # --host=aarch64 so host_cpu gates out x86 SIMD defines (else zfs.ko wants fletcher_4_sse2);
    # CC=host gcc: the Fedora cross GCC has no libc for configure's link probes. Kernel probes
    # and the module build go through KERNEL_* (arm64 kbuild).
    run(
        [
            "./configure", "--with-config=kernel", "--host=aarch64-linux-gnu", "CC=gcc",
            f"--with-linux={ksrc}", f"--with-linux-obj={kout}",
            # no KERNEL_CC: configure word-splits it; kbuild uses $(CROSS_COMPILE)gcc
            "KERNEL_ARCH=arm64", f"KERNEL_CROSS_COMPILE={CROSS}",
        ],
        cwd=work,
    )  # fmt: skip
    run(["make", "gitrev"], cwd=work)  # include/zfs_gitrev.h; top-level `all` makes it
    run(["make", "-C", "module", f"-j{NPROC}", "modules"], cwd=work)
    return work


def build_scst(run, log, kout: str, kv: str, oot: str) -> str:
    src = ensure_src("truenas-scst", _truenas.SCST_REPO, _truenas.SCST_PIN, log)
    work = stage(run, log, src, os.path.join(oot, "scst"), "truenas-scst")
    mk = [
        f"KDIR={kout}", f"KVER={kv}",
        # modinfo stamps; the Makefile would take uname -m (host) and git (absent in the copy)
        "ARCH_TYPE=aarch64", f"GIT_COMMIT={_truenas.SCST_PIN[:12]}", "BUILD_NUMBER=1",
        "BUILD_2X_MODULE=y", "CONFIG_SCSI_QLA_FC=y", "CONFIG_SCSI_QLA2XXX_TARGET=y",
        # no CC=: SCST's Makefiles word-split it; kbuild uses $(CROSS_COMPILE)gcc
    ]  # fmt: skip
    run(["make", *mk, "2perf"], cwd=work)
    for d in SCST_SUBDIRS:
        # iscsi-scst: `mods` only; iscsi-scstd/-adm come from the image.
        target = "mods" if d == "iscsi-scst" else "all"
        run(["make", "-C", d, *mk, f"-j{NPROC}", target], cwd=work)
    return work


def find_ko(tree: str, name: str, log) -> str:
    hits = [
        str(p) for p in pathlib.Path(tree).rglob(name) if ".tmp_versions" not in p.parts
    ]
    if len(hits) != 1:
        log(
            f"FATAL: {name}: expected 1 built copy under {tree}, found {hits or 'none'}"
        )
        sys.exit(1)
    return hits[0]


def install(run, log, kos, tree: str, extra: str, sign) -> list[str]:
    """Strip debug info (as dh_strip), sign, copy to extra/<relpath>."""
    out = []
    for rel in kos:
        ko = find_ko(tree, os.path.basename(rel), log)
        run([f"{CROSS}strip", "--strip-debug", ko])
        sign(ko)
        dst = os.path.join(extra, rel)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copy(ko, dst)
        out.append(dst)
    return out


def verify(log, installed: list[str], kv: str) -> None:
    """FATAL unless every EXPECTED .ko is present with vermagic for this KVER."""
    bad = []
    for ko in installed:
        if not os.path.exists(ko):
            bad.append(f"{ko}: missing")
            continue
        vm = subprocess.run(
            ["modinfo", "-F", "vermagic", ko], capture_output=True, text=True
        ).stdout.strip()
        if vm.split(" ", 1)[0] != kv:
            bad.append(f"{ko}: vermagic '{vm}' != {kv}")
        else:
            log(f"ok  {os.path.basename(ko)}  vermagic={vm}")
    for b in bad:
        log(f"FATAL: {b}")
    if bad or len(installed) != len(EXPECTED):
        sys.exit(1)


def build_all(
    run, log, ksrc: str, kout: str, out: str, modroot: str, kv: str, sign
) -> None:
    oot = os.path.join(out, "oot-src")
    extra = os.path.join(modroot, f"lib/modules/{kv}/extra")
    zfs = build_zfs(run, log, ksrc, kout, oot)
    scst = build_scst(run, log, kout, kv, oot)
    installed = install(run, log, ZFS_KOS, zfs, extra, sign)
    installed += install(run, log, SCST_KOS, scst, extra, sign)
    verify(log, installed, kv)
    log(
        f"OpenZFS {_truenas.ZFS_PIN[:12]} + SCST {_truenas.SCST_PIN[:12]}: {len(installed)} .ko in {extra}"
    )
