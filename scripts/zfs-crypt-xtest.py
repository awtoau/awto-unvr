#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Cross-implementation ZFS native-encryption test on woomera (#368).

- Per cipher (aes-256-gcm, aes-256-ccm) and direction (generic -> new, new -> generic):
  - set icp_aes_impl / icp_gcm_impl to the writer pair, create a throwaway encrypted dataset
    (passphrase on stdin), write known files, sync, `zfs unload-key` (drops decrypted state);
  - switch to the reader pair, load-key, read back, compare sha256 against the source files.
- Finally `zpool scrub -w POOL` must report 0 errors. Datasets are destroyed in every case.
- Only touches POOL/xtest-*; refuses if one exists. Leaves both params at `fastest`.
- Limits: each ssh call is bounded (SSH_S): 64 MiB written/read at >= 30 MB/s (generic gcm floor,
  #368) = 3 s, x3 slack for create/load; scrub bound = 3x a scrub of a few MB on an idle pool.
- Log: tmp/logs/zfs-crypt-xtest.log
"""

from __future__ import annotations

import argparse
import subprocess
import sys

import _box
from _repo import make_log

log = make_log("zfs-crypt-xtest")
PARAMS = "/sys/module/zfs/parameters"
PASS = "awto-xtest-passphrase"  # throwaway datasets only
SSH_S = 120
SCRUB_S = 300
SRC = "/var/tmp/xtest"
CIPHERS = ("aes-256-gcm", "aes-256-ccm")
GENERIC = ("generic", "generic")
NEW = ("armv8", "pmull")


def sh(ip: str, cmd: str, timeout: int = SSH_S) -> tuple[int, str]:
    try:
        r = subprocess.run(
            _box.ssh_argv(ip, "root", batch=True) + [cmd],
            capture_output=True,
            text=True,
            timeout=timeout,
            check=False,
        )
        return r.returncode, (r.stdout + r.stderr).strip()
    except subprocess.TimeoutExpired:
        log(f"FATAL: ssh timed out after {timeout} s: {cmd[:100]}")
        return 124, "timeout"


def set_impl(ip: str, pair: tuple[str, str]) -> None:
    rc, out = sh(
        ip,
        f"echo {pair[0]} > {PARAMS}/icp_aes_impl && echo {pair[1]} > {PARAMS}/icp_gcm_impl; "
        f"cat {PARAMS}/icp_aes_impl {PARAMS}/icp_gcm_impl",
    )
    log(f"impl {pair}: rc={rc} {out.replace(chr(10), ' | ')}")
    if rc or f"[{pair[0]}]" not in out or f"[{pair[1]}]" not in out:
        sys.exit(f"FATAL: could not select {pair}")


def one(ip: str, pool: str, cipher: str, wr: tuple, rd: tuple) -> bool:
    ds = f"{pool}/xtest-{cipher.split('-')[-1]}"
    tag = f"{cipher} write={wr[0]}/{wr[1]} read={rd[0]}/{rd[1]}"
    try:
        set_impl(ip, wr)
        rc, out = sh(
            ip,
            f"printf '%s\\n' '{PASS}' | zfs create -o encryption={cipher} -o keyformat=passphrase "
            f"-o keylocation=prompt -o compression=off {ds} && "
            f"cp {SRC}/a.bin {SRC}/b.bin /mnt/{ds}/ && sync && zfs unload-key {ds} && "
            f"zfs get -H -o value keystatus {ds}",
        )
        if rc or out.splitlines()[-1] != "unavailable":
            log(f"FAIL {tag}: write phase rc={rc} {out[-300:]}")
            return False
        set_impl(ip, rd)
        rc, out = sh(
            ip,
            f"printf '%s\\n' '{PASS}' | zfs load-key {ds} && zfs mount {ds} && "
            f"sha256sum /mnt/{ds}/a.bin /mnt/{ds}/b.bin",
        )
        want = sh(ip, f"sha256sum {SRC}/a.bin {SRC}/b.bin")[1]
        got = out
        ok = rc == 0 and [x.split()[0] for x in got.splitlines()] == [
            x.split()[0] for x in want.splitlines()
        ]
        log(
            f"{'PASS' if ok else 'FAIL'} {tag}: rc={rc}\n  want {want!r}\n  got  {got!r}"
        )
        return ok
    finally:
        rc, out = sh(ip, f"zfs destroy -rf {ds} 2>&1")
        log(f"cleanup {ds}: rc={rc} {out[-100:]}")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--pool", default="tank")
    a = ap.parse_args()
    ip = _box.require()
    rc, out = sh(
        ip,
        f"zfs list -H -o name -r {a.pool} | grep -c '/xtest-'; cat {PARAMS}/icp_aes_impl {PARAMS}/icp_gcm_impl",
    )
    log(f"box {ip}: {out.replace(chr(10), ' | ')}")
    if not out.startswith("0"):
        sys.exit("FATAL: leftover xtest datasets exist; refusing")
    if f"{NEW[0]}" not in out or f"{NEW[1]}" not in out:
        sys.exit(f"FATAL: {NEW} not offered by this build")
    rc, out = sh(
        ip,
        f"mkdir -p {SRC} && head -c 33554432 /dev/urandom > {SRC}/a.bin && head -c 1000003 /dev/urandom > {SRC}/b.bin",
    )
    if rc:
        sys.exit(f"FATAL: source files: {out}")
    results = []
    try:
        for cipher in CIPHERS:
            results.append(one(ip, a.pool, cipher, GENERIC, NEW))
            results.append(one(ip, a.pool, cipher, NEW, GENERIC))
    finally:
        set_impl(ip, ("fastest", "fastest"))
        sh(ip, f"rm -rf {SRC}")
    rc, out = sh(ip, f"zpool scrub -w {a.pool}; zpool status {a.pool}", SCRUB_S)
    clean = rc == 0 and "errors: No known data errors" in out and "with 0 errors" in out
    log(f"scrub: rc={rc} clean={clean}\n{out[-700:]}")
    ok = all(results) and clean
    log(
        f"RESULT {'PASS' if ok else 'FAIL'}: {sum(results)}/{len(results)} cross-impl reads, scrub {'clean' if clean else 'NOT clean'}"
    )
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
