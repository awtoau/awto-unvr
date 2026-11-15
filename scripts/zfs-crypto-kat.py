#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Known-answer + differential test of the ICP AES-GCM / AES-CCM impls (#368).

- Builds the ICP sources of an OpenZFS tree (default: the scale-build checkout, i.e. with the
  patches applied) + tools/zfs-crypto-kat/kat.c in an arm64 podman container (qemu-user emulates
  aes/pmull), no configure: file list comes from lib/libicp/Makefile.am.
- Vectors: Project Wycheproof aes_gcm/aes_ccm (Apache-2.0, includes the NIST/McGrew-Viega cases)
  + seeded random cases from python-cryptography (OpenSSL) for AES-128/192/256, lengths
  0..80 and up to 128 KiB, AAD 0..64, GCM IV 8/12/16/60, CCM nonce 7..13.
- Each vector runs through every (aes, gcm) impl combination (raw buffers, plus multi-iovec uios split at odd sizes for 2 combos) via crypto_encrypt/decrypt (the zio_crypt
  call path); verdict must match the expected ct+tag, and every combination must give identical
  per-vector results. Vectors in ZFS's shape (12-byte IV, 16-byte tag) may not be UNSUP.
- Limits: build stall = no output for BUILD_STALL_S; one kat run (qemu, ~1000 vectors up to 128 KiB)
  stall = RUN_STALL_S; kat prints one line per vector so silence means hung.
- Log: tmp/logs/zfs-crypto-kat.log
"""

from __future__ import annotations

import argparse
import json
import random
import subprocess
import sys
import threading
import time
import urllib.request
from concurrent.futures import ThreadPoolExecutor

from _repo import REPO, TMP, make_log

log = make_log("zfs-crypto-kat")
SRC = REPO / "tools" / "zfs-crypto-kat"
DEFAULT_ZFS = "/mnt/2tb/truenas-build/scale-build/sources/openzfs"
WORK = TMP / "zfs-crypto-kat"
IMAGE = "localhost/awto-zfs-kat-cc"
BUILD_STALL_S = 300
RUN_STALL_S = 120
WYCHEPROOF = (
    "https://raw.githubusercontent.com/C2SP/wycheproof/main/testvectors_v1/{}_test.json"
)
UIO_COMBOS = [("generic", "generic"), ("armv8", "pmull"), ("fastest", "fastest")]
COMBOS = [
    ("generic", "generic"),
    ("armv8", "generic"),
    ("generic", "pmull"),
    ("armv8", "pmull"),
    ("fastest", "fastest"),  # what the module runs by default: a COPY of the armv8 ops
]

BUILD_SH = r"""set -e
cd /zfs
O=/work/obj
rm -rf $O; mkdir -p $O; : > /work/zfs_config.h
INC="-include /work/zfs_config.h -I/zfs/include -I/zfs/module/icp/include -I/zfs/lib/libspl/include -I/zfs/lib/libspl/include/os/linux"
CF="-std=gnu99 -O2 -Wall -Wextra -Wstrict-prototypes -Wmissing-prototypes -Wno-sign-compare -Wno-missing-field-initializers -fno-strict-aliasing -fPIC -Werror -Wno-unknown-pragmas -Wno-enum-conversion -D_GNU_SOURCE -D_REENTRANT -D_FILE_OFFSET_BITS=64 -DTEXT_DOMAIN=\"kat\""
FILES=$(sed -n '/^nodist_libicp_la_SOURCES = /,/^$/p;/^if TARGET_CPU_AARCH64/,/^endif/p' lib/libicp/Makefile.am \
  | grep -o 'module/[^ ]*\.\(c\|S\)' | grep -v 'x86-64\|aesni\|pclmulqdq')
for f in $FILES lib/libspl/atomic.c lib/libspl/assert.c lib/libspl/backtrace.c module/avl/avl.c; do
  echo CC $f
  gcc $INC $CF -c $f -o $O/lib_$(echo $f | tr / _).o
done
for c in kat stubs; do
  echo CC $c.c
  gcc $INC -O2 -std=gnu99 -D_GNU_SOURCE -Wall -c /kat/$c.c -o $O/$c.o
done
gcc -o /work/kat $O/kat.o $O/stubs.o $O/lib_*.o
echo BUILD-OK
"""


def stream(cmd: list[str], stall: int, name: str, sink: list[str] | None = None) -> int:
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
    assert p.stdout
    for line in p.stdout:
        last[:] = [time.time(), line.strip()]
        if sink is not None:
            sink.append(line.rstrip())
        else:
            log(line.rstrip())
    return p.wait()


def podman(extra: list[str], cmd: list[str]) -> list[str]:
    return ["podman", "run", "--rm", "--platform", "linux/arm64", *extra, IMAGE, *cmd]


def build(zfs: str) -> None:
    WORK.mkdir(parents=True, exist_ok=True)
    if subprocess.run(["podman", "image", "exists", IMAGE]).returncode:
        if stream(["podman", "build", "--platform", "linux/arm64", "-t", IMAGE, "-f",
                   str(SRC / "Containerfile"), str(SRC)], BUILD_STALL_S, "image build"):  # fmt: skip
            sys.exit("FATAL: image build failed")
    (WORK / "build.sh").write_text(BUILD_SH)
    rc = stream(
        podman(
            ["-v", f"{zfs}:/zfs:ro", "-v", f"{SRC}:/kat:ro", "-v", f"{WORK}:/work"],
            ["sh", "/work/build.sh"],
        ),
        BUILD_STALL_S,
        "kat build",
    )
    if rc or not (WORK / "kat").exists():
        sys.exit(f"FATAL: kat build failed rc={rc}")


def fetch(name: str) -> dict:
    f = WORK / f"{name}.json"
    if not f.exists():
        log(f"fetch {name}")
        f.write_bytes(
            urllib.request.urlopen(WYCHEPROOF.format(name), timeout=60).read()
        )
    return json.loads(f.read_text())


def hx(b: bytes) -> str:
    return b.hex() or "-"


def vec(mode, vid, key, iv, aad, pt, ct, tag, valid) -> str:
    h = lambda s: (s if isinstance(s, str) else s.hex()) or "-"  # noqa: E731
    shape = f"iv{len(bytes.fromhex(h(iv).replace('-', '')))}-t{len(bytes.fromhex(h(tag).replace('-', '')))}"
    return f"{mode} {vid}-{shape} {h(key)} {h(iv)} {h(aad)} {h(pt)} {h(ct)} {h(tag)} {int(valid)}"


def gen_vectors() -> list[str]:
    out: list[str] = []
    for mode, name in (("GCM", "aes_gcm"), ("CCM", "aes_ccm")):
        d = fetch(name)
        for g in d["testGroups"]:
            for t in g["tests"]:
                out.append(
                    vec(mode, f"{mode.lower()}-wp{t['tcId']}", t["key"], t["iv"], t["aad"], t["msg"], t["ct"],
                        t["tag"], t["result"] != "invalid")
                )  # fmt: skip
    from cryptography.hazmat.primitives.ciphers.aead import AESCCM, AESGCM

    rnd = random.Random(368)
    lens = [*range(0, 81), 127, 128, 129, 255, 256, 257, 511, 512, 513, 1000, 4095, 4096, 4097,
            16384, 65536, 131072]  # fmt: skip
    n = 0
    for klen in (16, 24, 32):
        for ln in lens:
            for alen in (0, 1, 13, 16, 20, 64) if ln < 40 or ln % 7 == 0 else (0, 20):
                key = rnd.randbytes(klen)
                iv = rnd.randbytes(12)
                aad, pt = rnd.randbytes(alen), rnd.randbytes(ln)
                ct = AESGCM(key).encrypt(iv, pt, aad)
                out.append(
                    vec(
                        "GCM", f"gcm-rnd{n}", key, iv, aad, pt, ct[:-16], ct[-16:], True
                    )
                )
                n += 1
                if ln < 65 and alen in (0, 20):
                    for ivl in (8, 16, 60):
                        iv = rnd.randbytes(ivl)
                        ct = AESGCM(key).encrypt(iv, pt, aad)
                        out.append(
                            vec(
                                "GCM",
                                f"gcm-rnd{n}",
                                key,
                                iv,
                                aad,
                                pt,
                                ct[:-16],
                                ct[-16:],
                                True,
                            )
                        )
                        n += 1
                nonce = rnd.randbytes(12)
                ct = AESCCM(key, 16).encrypt(nonce, pt, aad)
                out.append(
                    vec(
                        "CCM",
                        f"ccm-rnd{n}",
                        key,
                        nonce,
                        aad,
                        pt,
                        ct[:-16],
                        ct[-16:],
                        True,
                    )
                )
                n += 1
                if ln < 65 and alen in (0, 20):
                    for nl, tl in ((7, 4), (8, 8), (13, 16), (11, 12)):
                        nonce = rnd.randbytes(nl)
                        ct = AESCCM(key, tl).encrypt(nonce, pt, aad)
                        out.append(
                            vec(
                                "CCM",
                                f"ccm-rnd{n}",
                                key,
                                nonce,
                                aad,
                                pt,
                                ct[:-tl],
                                ct[-tl:],
                                True,
                            )
                        )
                        n += 1
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument(
        "--src",
        default=DEFAULT_ZFS,
        help="OpenZFS tree (default: scale-build checkout)",
    )
    ap.add_argument("--skip-build", action="store_true")
    a = ap.parse_args()
    if not a.skip_build:
        build(a.src)
    vecs = gen_vectors()
    (WORK / "vectors.txt").write_text("\n".join(vecs) + "\n")
    log(f"{len(vecs)} vectors")
    results: dict[tuple[str, str, str], dict[str, str]] = {}
    bad = False
    runs = [(a_, g_, "raw") for a_, g_ in COMBOS] + [
        (a_, g_, "uio") for a_, g_ in UIO_COMBOS
    ]

    def one(run: tuple[str, str, str]) -> tuple[tuple[str, str, str], int, list[str]]:
        aes, gcm, mode = run
        lines: list[str] = []
        rc = stream(
            podman(
                ["-v", f"{WORK}:/work:ro"],
                [
                    "/work/kat",
                    aes,
                    gcm,
                    "/work/vectors.txt",
                    *(["uio"] if mode == "uio" else []),
                ],
            ),
            RUN_STALL_S,
            f"kat {aes}/{gcm}/{mode}",
            sink=lines,
        )
        return run, rc, lines

    with ThreadPoolExecutor(max_workers=len(runs)) as ex:
        done = list(ex.map(one, runs))
    for (aes, gcm, mode), rc, lines in done:
        res = {}
        for ln in lines:
            p = ln.split(" ", 2)
            if len(p) >= 2 and p[1] in ("PASS", "FAIL", "UNSUP"):
                res[p[0]] = p[1]
                if p[1] == "FAIL":
                    log(f"  {aes}/{gcm}/{mode}: {ln}")
        info = next((ln for ln in lines if ln.startswith("INFO")), "")
        summ = next((ln for ln in lines if ln.startswith("SUMMARY")), "no summary")
        cover = next(
            (ln for ln in lines if ln.startswith("COVER")), "COVER fused_calls=?"
        )
        summ += " " + cover
        if aes in ("armv8", "fastest") and cover.endswith("=0"):
            bad = True
            log(f"FATAL: {aes}/{gcm}/{mode} never took the multi-block asm path")
        log(f"{aes}/{gcm}/{mode}: rc={rc} {info} {summ}")
        if rc or not res:
            bad = True
            for ln in lines[-5:]:
                log("  " + ln)
        if aes in ("armv8", "fastest") and "pmull_hwcap=1" not in info:
            bad = True
            log("FATAL: PMULL not visible to the armv8 run; fused paths untested")
        results[(aes, gcm, mode)] = res
    base = results[(*COMBOS[0], "raw")]
    for combo, res in results.items():
        diff = [k for k in base if res.get(k) != base[k]]
        if diff:
            bad = True
            log(
                f"MISMATCH {combo} vs {COMBOS[0]}: {len(diff)} vectors, first {diff[:5]}"
            )
    zfs_shape = [k for k, v in base.items() if v == "UNSUP" and "-iv12-t16" in k]
    if zfs_shape:
        bad = True
        log(f"UNSUP in ZFS shape (iv12/t16): {zfs_shape[:5]}")
    npass = sum(v == "PASS" for v in base.values())
    nuns = sum(v == "UNSUP" for v in base.values())
    log(f"RESULT {'FAIL' if bad else 'PASS'}: {len(base)} vectors x {len(runs)} runs (impl combos, raw + multi-iovec uio), "
        f"{npass} pass, {nuns} unsupported-shape (identical across impls), all combos identical")  # fmt: skip
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
