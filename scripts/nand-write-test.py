#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Prove al_nand's WRITE path, with a before/after check on every NAND partition (#208).

Reads are verified (scripts/verify-nand-reads.py). Writes silently no-oped:
`nandwrite` exited 0 and the readback was still 0xff, because
`ecc_write_page()` never issued SEQIN/PAGEPROG. This exercises the fix and
proves nothing else moved.

What a bare `nandwrite` does not establish, and this does:

  1. the bytes actually land     - write a random pattern, read back, sha256
  2. (informational) re-programming a programmed block. NAND reports no FAIL
                                 for identical data (NOP=4, pass status), so
                                 rc 0 is the CORRECT outcome; this cannot show
                                 that a failed write is detected (docs/nand-study.md s7)
  3. nothing else was touched    - sha256 every NAND partition before and
                                   after, and stock U-Boot must still boot
  4. the ECC bytes are right     - scripts/nand-survey.py ecc re-derives the
                                   BCH parity independently of the controller;
                                   sha256 + 0 corrected_bits only shows the
                                   controller agrees with itself

Write target is mtd1 "device_tree" ONLY: verified all-0xff, and U-Boot takes
the DTB from the NOR TOC, not from here. mtd3 is awto-uboot (the live
bootloader, #216) - its checksum is the one that must not change.

Per CLAUDE.md a flash write needs the user's explicit approval. --write is
that approval; without it this only takes checksums.

    ./scripts/nand-write-test.py                # checksums only, no writes
    ./scripts/nand-write-test.py --write        # run the write test
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

from _box import require, ssh_argv  # noqa: E402
from _repo import make_log  # noqa: E402

log = make_log("nand-write-test")

TARGET_LABEL = "device_tree"
PROTECTED_LABEL = "awto-uboot"
BLOCK = 262144  # 256 KiB erase block (Micron MT29F8G08ABBCAH4)
# mtd-utils live in /usr/sbin. pipefail: every command below that ends in
# `| tail` would otherwise report tail's exit status, so `nandwrite` failing
# looked like rc=0 and criteria [1]/[2] could not observe the one thing they test.
PATH = "set -o pipefail; export PATH=/usr/sbin:$PATH; "

# Bounds: a 256 KiB erase is ~2 ms and a block program ~tens of ms on this
# chip, so these only catch a hang. sha256 of a 1 GiB partition dominates
# everything else at ~60 s, hence the wider read bound. On expiry: the step
# is logged with its limit and the run aborts before any write.
SH_TIMEOUT_S = 180
READ_TIMEOUT_S = 600


def box_sh(host: str, cmd: str, timeout: int = SH_TIMEOUT_S) -> tuple[int, str]:
    try:
        r = subprocess.run(
            # One arg, not ["sh","-c",...]: ssh space-joins its command words,
            # so the remote shell would take the next word as $0 and drop it.
            ssh_argv(host, cmd=[PATH + cmd]),
            capture_output=True,
            text=True,
            check=False,
            timeout=timeout,
        )
        return r.returncode, (r.stdout + r.stderr).strip()
    except subprocess.TimeoutExpired:
        log(f"TIMEOUT after {timeout}s: {cmd[:70]}")
        return 124, ""


def partitions(host: str) -> dict[str, int]:
    """label -> mtd index, from the live /proc/mtd. Addressed by LABEL because
    the numbers moved when al_nand landed (NOR went mtd0-7 -> mtd5-12)."""
    _, raw = box_sh(host, "cat /proc/mtd")
    out = {}
    for line in raw.splitlines():
        if not line.startswith("mtd"):
            continue
        idx = line.split(":")[0].removeprefix("mtd")
        if '"' in line:
            out[line.split('"')[1]] = int(idx)
    return out


def checksums(host: str, parts: dict[str, int]) -> dict[str, str]:
    """sha256 of every NAND partition (256 KiB erasesize), via the ro node."""
    out = {}
    _, raw = box_sh(host, "cat /proc/mtd")
    nand = {
        line.split('"')[1]
        for line in raw.splitlines()
        if line.startswith("mtd") and "00040000" in line and '"' in line
    }
    for label in sorted(nand):
        idx = parts[label]
        rc, o = box_sh(
            host,
            f"dd if=/dev/mtd{idx}ro bs=1M 2>/dev/null | sha256sum | cut -c1-24",
            timeout=READ_TIMEOUT_S,
        )
        out[label] = o if rc == 0 else "READ-FAILED"
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--write", action="store_true", help="USER APPROVAL for the flash write"
    )
    a = ap.parse_args()

    host = require(hint="is woomera up?")
    parts = partitions(host)
    if TARGET_LABEL not in parts:
        log(f"no '{TARGET_LABEL}' partition in /proc/mtd - is al_nand bound?")
        return 1
    tgt = parts[TARGET_LABEL]
    log(f"box {host}; target mtd{tgt} '{TARGET_LABEL}'; partitions: {parts}")

    # The target must be erased before we touch it: writing over existing
    # content would destroy whatever is there and muddy the result.
    rc, nonff = box_sh(
        host, f"dd if=/dev/mtd{tgt}ro bs=1M 2>/dev/null | tr -d '\\377' | wc -c"
    )
    if rc != 0 or nonff.strip() != "0":
        log(f"ABORT: mtd{tgt} '{TARGET_LABEL}' is NOT erased ({nonff} non-0xff bytes)")
        return 1
    log(f"mtd{tgt} confirmed erased (0 non-0xff bytes)")

    # The check above goes through the ECC engine, which reports a page that
    # was programmed WITHOUT parity as 0xff (docs/nand-study.md s5). Raw check.
    raw = subprocess.run(
        [
            str(REPO / "scripts" / "nand-survey.py"),
            "ecc",
            "--label",
            TARGET_LABEL,
            "--pages",
            "all",
        ],
        capture_output=True,
        text=True,
        check=False,
        timeout=60,
    )
    if raw.returncode != 0 or "erased 256," not in raw.stdout:
        log(
            f"ABORT: raw check of '{TARGET_LABEL}' is not all-0xff: {raw.stdout[-200:]}"
        )
        return 1
    log(f"mtd{tgt} raw (no-ECC) check: all 256 pages erased")

    log("--- BEFORE")
    before = checksums(host, parts)
    for k, v in before.items():
        log(f"  {k:14s} {v}")

    if not a.write:
        log("checksums only. Pass --write to run the write test (needs approval).")
        return 0

    log("--- WRITE TEST")
    # No `| tail`: a pipeline returns the LAST command's status, so a failing
    # flash_erase read as success (#302).
    erase_rc, o = box_sh(host, f"flash_erase /dev/mtd{tgt} 0 1 2>&1")
    log(f"  erase: rc={erase_rc} {o.splitlines()[-1][-60:] if o else ''}")
    erase1_ok = erase_rc == 0
    log(f"  [0] erase exits 0: {'PASS' if erase1_ok else 'FAIL'}")

    box_sh(host, f"head -c {BLOCK} /dev/urandom > /tmp/pat.bin")
    _, want = box_sh(host, "sha256sum /tmp/pat.bin | cut -c1-24")

    rc, o = box_sh(host, f"nandwrite -p /dev/mtd{tgt} /tmp/pat.bin 2>&1")
    log(f"  write: rc={rc} {o[-70:]}")
    write_rc = rc

    dump_rc, _ = box_sh(
        host, f"nanddump -l {BLOCK} -f /tmp/rb.bin /dev/mtd{tgt} >/dev/null 2>&1"
    )
    if dump_rc != 0:
        log(f"  nanddump failed: rc={dump_rc}")
    _, got = box_sh(host, "sha256sum /tmp/rb.bin | cut -c1-24")
    log(f"  written  {want}")
    log(f"  readback {got}")
    crit1 = want == got and write_rc == 0 and dump_rc == 0
    log(f"  [1] bytes land, write exits 0: {'PASS' if crit1 else 'FAIL'}")

    # Identical data over a programmed page: AND of equal bits, same parity, and
    # NAND returns pass status, so rc 0 is expected. NOT an induced failure.
    rc, o = box_sh(host, f"nandwrite -p /dev/mtd{tgt} /tmp/pat.bin 2>&1")
    log(
        f"  [2] re-program identical data (info only, rc 0 expected): rc={rc} {o[-70:]}"
    )

    _, ecc = box_sh(
        host,
        f"cat /sys/class/mtd/mtd{tgt}/ecc_failures /sys/class/mtd/mtd{tgt}/corrected_bits",
    )
    crit3 = ecc.split() == ["0", "0"]
    log(
        f"  [3] ECC clean after write/read: {'PASS' if crit3 else 'FAIL'} ({ecc.split()})"
    )

    # Independent of the controller: 64 pages = the 256 KiB just written.
    surv = subprocess.run(
        [
            str(REPO / "scripts" / "nand-survey.py"),
            "ecc",
            "--label",
            TARGET_LABEL,
            "--pages",
            str(BLOCK // 4096),
        ],
        capture_output=True,
        text=True,
        check=False,
        timeout=60,
    )
    crit4 = surv.returncode == 0 and f"parity ok {BLOCK // 4096}," in surv.stdout
    log(
        f"  [4] parity matches the independent BCH model: {'PASS' if crit4 else 'FAIL'}"
    )
    log(
        f"      {surv.stdout.strip().splitlines()[-1] if surv.stdout.strip() else surv.stderr[-200:]}"
    )

    # Leave the target as we found it.
    restore_rc, o = box_sh(host, f"flash_erase /dev/mtd{tgt} 0 1 2>&1")
    log(f"  restore erase: rc={restore_rc} {o.splitlines()[-1][-60:] if o else ''}")
    box_sh(host, "rm -f /tmp/pat.bin /tmp/rb.bin")

    log("--- AFTER")
    after = checksums(host, parts)
    moved = []
    for k, v in sorted(after.items()):
        same = v == before.get(k)
        log(f"  {k:14s} {v} {'unchanged' if same else 'CHANGED'}")
        if not same and k != TARGET_LABEL:
            moved.append(k)

    if PROTECTED_LABEL in moved:
        log(
            f"*** {PROTECTED_LABEL} CHANGED - the bootloader was modified. Do not reboot."
        )
    # The erase above is the first erase of a PROGRAMMED block (the earlier
    # device_tree erase was of an already-erased one): the target must be back to
    # its pre-test checksum or erase is unproven.
    erased_back = after.get(TARGET_LABEL) == before.get(TARGET_LABEL)
    log(f"  [5] erase restored {TARGET_LABEL}: {'PASS' if erased_back else 'FAIL'}")
    erases_ok = erase1_ok and restore_rc == 0
    log(f"  [6] both erases exited 0: {'PASS' if erases_ok else 'FAIL'}")
    ok = crit1 and crit3 and crit4 and erased_back and erases_ok and not moved
    log(
        f"RESULT: {'PASS' if ok else 'FAIL'}"
        + (f" - collateral change in {moved}" if moved else "")
    )
    log("Next: reboot and confirm stock U-Boot still chainloads awto-uboot.")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
