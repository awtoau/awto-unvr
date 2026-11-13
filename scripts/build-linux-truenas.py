#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""TrueNAS arm64 6.18 kernel + UNVR DTB + al_* modules (#338, phase 1: build only).

- Source: tmp/refs/truenas-unvr = TRUENAS_PIN + our generated series (_truenas.apply()).
- Config, as TrueNAS's scale-build `kernel` step (conf/build.manifest) for arm64:
  defconfig -> merge debian_arm64.config -> truenas.config -> tn-production.config,
  then our patches/truenas-6.18/unvr.config + LOCALVERSION, olddefconfig, FATAL checks.
- Modules: TrueNAS's MODULE_SIG_ALL signs in-tree ones at install; OOT al_* signed with
  the same build key. WERROR comes from truenas.config.
- Out: truenas_build_out() (Image, uImage, DTB, config, modroot/).
- Log: tmp/logs/build-linux-truenas.log
"""

from __future__ import annotations

import os
import pathlib
import re
import shutil
import subprocess
import sys
import time

import _kbuild
import _truenas
from _kbuild import CC_ARG, DTS_NAME, ENV, check_dts_shared, kver, mkuimage

import _repo
from _repo import NPROC, build_ident, check_kernel_patches, kernel_build_ver, make_log

log = make_log("build-linux-truenas")
OUT = str(_repo.truenas_build_out())
KOUT = os.path.join(OUT, "kbuild")
TN_FRAGMENTS = ("debian_arm64.config", "truenas.config", "tn-production.config")
UNVR_FRAGMENT = _truenas.FIXUP_DIR / "unvr.config"
TN_CERT = (
    "truenas-uefi-certs.pem"  # truenas.config SYSTEM_TRUSTED_KEYS="debian/certs/<this>"
)


def run(cmd, **kw):
    kw.setdefault("stdin", subprocess.DEVNULL)  # a Kconfig prompt on a tty would hang
    _repo.run(cmd, log=log, env=ENV, **kw)


def localversion() -> str:
    return f"-truenas-{build_ident('kernel')}"


def required_syms() -> dict[str, str]:
    """CONFIG_X -> value, from unvr.config plus the TrueNAS choices our modules rely on."""
    req = {}
    for line in UNVR_FRAGMENT.read_text().splitlines():
        m = re.match(r"(CONFIG_\w+)=(.*)", line)
        if m:
            req[m[1]] = m[2]
    req["CONFIG_WERROR"] = "y"  # zero-warning OOT build, as on 7.x
    req["CONFIG_MODULE_SIG"] = "y"
    req["CONFIG_LOCALVERSION"] = f'"{localversion()}"'
    return req


def configure(src: str) -> None:
    os.makedirs(KOUT, exist_ok=True)
    dotcfg = os.path.join(KOUT, ".config")
    if os.path.exists(dotcfg):
        os.remove(dotcfg)
    run(["make", "-C", src, f"O={KOUT}", "defconfig"])
    merge = os.path.join(src, "scripts/kconfig/merge_config.sh")
    tn = os.path.join(src, "scripts/package/truenas")
    for frag in TN_FRAGMENTS:
        run([merge, "-O", KOUT, dotcfg, os.path.join(tn, frag)], cwd=src)
    # mkdebian copies the cert here before TrueNAS builds; O= resolves it under KOUT.
    certs = os.path.join(KOUT, "debian/certs")
    os.makedirs(certs, exist_ok=True)
    shutil.copy(os.path.join(tn, TN_CERT), certs)
    run([merge, "-O", KOUT, dotcfg, str(UNVR_FRAGMENT)], cwd=src)
    cfg = os.path.join(src, "scripts/config")
    run([cfg, "--file", dotcfg, "--set-str", "LOCALVERSION", localversion()])
    run(["make", "-C", src, f"O={KOUT}", "olddefconfig"])
    have = {}
    for line in pathlib.Path(dotcfg).read_text().splitlines():
        if line.startswith("CONFIG_"):
            k, _, v = line.partition("=")
            have[k] = v
    bad = [
        f"{k}={v} (have {have.get(k, 'unset')})"
        for k, v in required_syms().items()
        if have.get(k) != v
    ]
    if bad:
        for b in bad:
            log(f"FATAL: {b} after olddefconfig")
        sys.exit(1)
    check_kernel_patches(log, pathlib.Path(src))
    log(
        f"configured: TrueNAS production + {UNVR_FRAGMENT.name}, {len(required_syms())} symbols verified"
    )


def sign_with_build_key(ko: str) -> None:
    hash_ = "sha256"
    m = re.search(
        r'^CONFIG_MODULE_SIG_HASH="(\w+)"',
        pathlib.Path(KOUT, ".config").read_text(),
        re.M,
    )
    if m:
        hash_ = m[1]
    run(
        [
            os.path.join(KOUT, "scripts/sign-file"),
            hash_,
            os.path.join(KOUT, "certs/signing_key.pem"),
            os.path.join(KOUT, "certs/signing_key.x509"),
            ko,
        ]
    )


def build() -> None:
    try:
        src = str(_truenas.apply(log).resolve())
    except _truenas.SeriesError as e:
        log(f"FATAL: {e}")
        sys.exit(1)
    ver = kernel_build_ver(pathlib.Path(src))
    os.makedirs(OUT, exist_ok=True)
    check_dts_shared(log)
    configure(src)
    _kbuild.stage_dts(src, log)
    dtb_rel = f"amazon/{DTS_NAME}.dtb"
    run(
        [
            "make",
            "-C",
            src,
            f"O={KOUT}",
            *CC_ARG,
            # TrueNAS builds with Debian's GCC 14; 6.18 in-tree code is not GCC 16 warning-clean.
            # Warnings stay in the log; OOT modules (no KCFLAGS) keep CONFIG_WERROR's -Werror.
            "KCFLAGS=-Wno-error",
            f"-j{NPROC}",
            "Image",
            dtb_rel,
            "modules",
        ]
    )
    kv = kver(KOUT)
    log(f"KVER={kv}")

    modroot = os.path.join(OUT, "modroot")
    if os.path.exists(modroot):
        shutil.rmtree(modroot)
    os.makedirs(modroot)
    run(
        [
            "make", "-C", src, f"O={KOUT}", f"INSTALL_MOD_PATH={modroot}",
            "INSTALL_MOD_STRIP=1", "modules_install",
        ]
    )  # fmt: skip
    extra = os.path.join(modroot, f"lib/modules/{kv}/extra")
    _kbuild.build_oot_modules(run, log, KOUT, OUT, extra, sign=sign_with_build_key)
    run(["depmod", "-b", modroot, kv], check=False)

    name = f"unvr-ea16-{ver}-truenas"
    image = os.path.join(KOUT, "arch/arm64/boot/Image")
    shutil.copy(image, os.path.join(OUT, "Image"))
    shutil.copy(
        os.path.join(KOUT, "arch/arm64/boot/dts", dtb_rel),
        os.path.join(OUT, f"{DTS_NAME}-{ver}-truenas.dtb"),
    )
    shutil.copy(os.path.join(KOUT, ".config"), os.path.join(OUT, f"{name}.config"))
    mkuimage(image, os.path.join(OUT, f"uImage-{name}"), name, log)
    log(f"DONE. KVER={kv} modroot: {modroot} ; Image/uImage/dtb in {OUT}")


if __name__ == "__main__":
    t0 = time.time()
    build()
    log(f"TOTAL {int(time.time() - t0)}s")
