#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Prove al_nand's WRITE path, with a before/after check on every NAND partition (#208).

Reads are verified (scripts/verify-nand-reads.py). Writes silently no-oped:
`nandwrite` exited 0 and the readback was still 0xff, because
`ecc_write_page()` never issued SEQIN/PAGEPROG. This exercises the fix and
proves nothing else moved.

Three things a bare `nandwrite` does not establish, and this does:

  1. the bytes actually land     - write a random pattern, read back, sha256
  2. a FAILED write is DETECTED  - write to a non-erased block, expect non-zero
                                   exit. The pre-fix defect was exit 0 on a
                                   write that never happened, which is worse
                                   than failing loudly
  3. nothing else was touched    - sha256 every NAND partition before and
                                   after, and stock U-Boot must still boot

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
PATH = "export PATH=/usr/sbin:$PATH; "  # mtd-utils live in /usr/sbin

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

    log("--- BEFORE")
    before = checksums(host, parts)
    for k, v in before.items():
        log(f"  {k:14s} {v}")

    if not a.write:
        log("checksums only. Pass --write to run the write test (needs approval).")
        return 0

    log("--- WRITE TEST")
    rc, o = box_sh(host, f"flash_erase /dev/mtd{tgt} 0 1 2>&1 | tail -1")
    log(f"  erase: rc={rc} {o[-60:]}")

    box_sh(host, f"head -c {BLOCK} /dev/urandom > /tmp/pat.bin")
    _, want = box_sh(host, "sha256sum /tmp/pat.bin | cut -c1-24")

    rc, o = box_sh(host, f"nandwrite -p /dev/mtd{tgt} /tmp/pat.bin 2>&1 | tail -2")
    log(f"  write: rc={rc} {o[-70:]}")
    write_rc = rc

    box_sh(host, f"nanddump -l {BLOCK} -f /tmp/rb.bin /dev/mtd{tgt} >/dev/null 2>&1")
    _, got = box_sh(host, "sha256sum /tmp/rb.bin | cut -c1-24")
    log(f"  written  {want}")
    log(f"  readback {got}")
    crit1 = want == got and write_rc == 0
    log(f"  [1] bytes land, write exits 0: {'PASS' if crit1 else 'FAIL'}")

    # Induced failure: the block now holds data, so a second write to it must
    # be rejected. Pre-fix this exited 0 - the defect that made a dead write
    # indistinguishable from a live one.
    rc, o = box_sh(host, f"nandwrite -p /dev/mtd{tgt} /tmp/pat.bin 2>&1 | tail -1")
    crit2 = rc != 0
    log(f"  [2] write to non-erased block fails loudly: {'PASS' if crit2 else 'FAIL'}")
    log(f"      rc={rc} {o[-70:]}")

    _, ecc = box_sh(
        host,
        f"cat /sys/class/mtd/mtd{tgt}/ecc_failures /sys/class/mtd/mtd{tgt}/corrected_bits",
    )
    crit3 = ecc.split() == ["0", "0"]
    log(
        f"  [3] ECC clean after write/read: {'PASS' if crit3 else 'FAIL'} ({ecc.split()})"
    )

    # Leave the target as we found it.
    box_sh(host, f"flash_erase /dev/mtd{tgt} 0 1 >/dev/null 2>&1")
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
    ok = crit1 and crit2 and crit3 and not moved
    log(
        f"RESULT: {'PASS' if ok else 'FAIL'}"
        + (f" - collateral change in {moved}" if moved else "")
    )
    log("Next: reboot and confirm stock U-Boot still chainloads awto-uboot.")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
