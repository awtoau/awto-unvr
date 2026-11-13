#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""al_* OOT modules for the self-built TrueNAS 27 kernel, as one arm64 .deb (#356 phase C).

- Builds against the kernel's own linux-headers .deb (the objtree is deleted after packaging),
  natively in a throwaway arm64 container (its host tools are aarch64). Own container name:
  never touches awto-truenas-build.
- Modules: _kbuild.OOT_MODULES (al_eth as _1g/_10g). OpenZFS/SCST come from the image's packages.
- Unsigned: the signing key dies with the objtree; CONFIG_MODULE_SIG_FORCE is unset, so they load.
- Out: KERNEL_DIR/awto-unvr-modules_<kver>-1_arm64.deb (dropped beside the kernel debs).
- Log: tmp/logs/truenas-oot-deb.log
"""

from __future__ import annotations

import argparse
import glob
import os
import shutil
import subprocess
import sys
import threading
import time

import _kbuild

import _repo
from _repo import REPO, TMP, make_log

log = make_log("truenas-oot-deb")
KERNEL_DIR = "/mnt/2tb/truenas-build/kernel"
IMAGE = "localhost/awto-truenas-build:trixie"
CTR = "awto-truenas-oot"
# gcc under qemu-user, 9 small modules: expected ~5 min; stall guard per repo rule (no output = hung)
STALL_S = 300

BUILD_SH = r"""set -e
H=/hdr/usr/src/linux-headers-truenas-production-arm64
KR=$(cat $H/include/config/kernel.release)
echo "KERNELRELEASE=$KR"
mkdir -p /work && cp -dR --preserve=links /modules/. /work/
D=/out/pkg/usr/lib/modules/$KR/extra
mkdir -p $D
one() { # dir ko [make vars]
  d=/work/$1; ko=$2; shift 2
  [ "$1" = "" ] || { rm -rf /work/$ko; cp -dR --preserve=links $d /work/$ko; d=/work/$ko; }
  make -C $H M=$d "$@" -j$NPROC modules
  strip --strip-debug $d/$ko.ko
  cp $d/$ko.ko $D/
}
%s
mkdir -p /out/pkg/DEBIAN
cat > /out/pkg/DEBIAN/control <<EOF
Package: awto-unvr-modules
Version: $KR-1
Architecture: arm64
Maintainer: Awto <noreply@awto.au>
Section: kernel
Priority: optional
Description: Annapurna Alpine V2 (UNVR) out-of-tree al_* modules for kernel $KR
EOF
printf '#!/bin/sh\nset -e\ndepmod -a %%s\n' "$KR" > /out/pkg/DEBIAN/postinst
chmod 755 /out/pkg/DEBIAN/postinst
cp /out/pkg/DEBIAN/postinst /out/pkg/DEBIAN/postrm
dpkg-deb --root-owner-group -Zxz --build /out/pkg /out/awto-unvr-modules_$KR-1_arm64.deb
"""


def build_lines() -> str:
    lines = []
    for m in _kbuild.OOT_MODULES:
        if m == "al_eth":
            lines += [
                f"one al_eth al_eth_{v} MODULE_VARIANT={v}" for v in ("1g", "10g")
            ]
        else:
            lines.append(f"one {m} {m}")
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--kernel-dir", default=KERNEL_DIR)
    a = ap.parse_args()
    hdebs = glob.glob(f"{a.kernel_dir}/linux-headers-truenas-production-arm64_*.deb")
    if len(hdebs) != 1:
        log(f"FATAL: want 1 headers deb in {a.kernel_dir}, found {hdebs}")
        return 1
    scratch = TMP / "truenas-oot"
    hdr, outd = scratch / "hdr", scratch / "out"
    for d in (hdr, outd):
        shutil.rmtree(d, ignore_errors=True)
        d.mkdir(parents=True)
    subprocess.run(["dpkg-deb", "-x", hdebs[0], str(hdr)], check=True)
    sh = BUILD_SH % build_lines()
    cmd = [
        "sudo", "podman", "run", "--rm", "--name", CTR, "--platform", "linux/arm64",
        "--user", f"{os.getuid()}:{os.getgid()}", "-e", "HOME=/tmp", "-e", f"NPROC={_repo.NPROC}",
        "-v", f"{hdr}:/hdr:ro", "-v", f"{REPO / 'modules'}:/modules:ro", "-v", f"{outd}:/out",
        "--tmpfs", "/work:exec,mode=1777", IMAGE, "sh", "-c", sh,
    ]  # fmt: skip
    log(f"building in {CTR} from {os.path.basename(hdebs[0])}")
    last = [time.time(), ""]
    p = subprocess.Popen(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
    )

    def wd() -> None:
        while p.poll() is None:
            time.sleep(5)
            if time.time() - last[0] > STALL_S:
                log(f"FATAL: no output for {STALL_S} s (last: {last[1]!r}); killing")
                subprocess.run(["sudo", "podman", "kill", CTR], check=False)
                return

    threading.Thread(target=wd, daemon=True).start()
    for line in p.stdout:
        last[:] = [time.time(), line.strip()]
        log(line.rstrip())
    if p.wait():
        log(f"FATAL: container rc={p.returncode}")
        return 1
    debs = glob.glob(f"{outd}/*.deb")
    if len(debs) != 1:
        log(f"FATAL: expected 1 deb in {outd}, got {debs}")
        return 1
    kr = (
        open(
            hdr
            / "usr/src/linux-headers-truenas-production-arm64/include/config/kernel.release"
        )
        .read()
        .strip()
    )
    bad = verify(debs[0], kr)
    if bad:
        return 1
    dst = os.path.join(a.kernel_dir, os.path.basename(debs[0]))
    shutil.copy(debs[0], dst)
    log(f"DONE {dst}")
    return 0


def verify(deb: str, kr: str) -> bool:
    """True on failure: every expected .ko present, vermagic starts with KERNELRELEASE."""
    x = TMP / "truenas-oot" / "verify"
    shutil.rmtree(x, ignore_errors=True)
    x.mkdir(parents=True)
    subprocess.run(["dpkg-deb", "-x", deb, str(x)], check=True)
    want = {"al_eth_1g.ko", "al_eth_10g.ko"} | {
        f"{m}.ko" for m in _kbuild.OOT_MODULES if m != "al_eth"
    }
    d = x / f"usr/lib/modules/{kr}/extra"
    have = {p.name for p in d.glob("*.ko")}
    bad = False
    if have != want:
        log(f"FATAL: module set {sorted(have)} != {sorted(want)}")
        bad = True
    for ko in sorted(d.glob("*.ko")):
        vm = subprocess.run(
            ["modinfo", "-F", "vermagic", str(ko)], capture_output=True, text=True
        ).stdout.strip()
        ok = vm.split(" ", 1)[0] == kr
        log(f"{'ok ' if ok else 'BAD'} {ko.name} vermagic={vm}")
        bad |= not ok
    return bad


if __name__ == "__main__":
    sys.exit(main())
