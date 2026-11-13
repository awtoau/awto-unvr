#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Compile repo dts/ to a DTB against an existing kernel build, no kernel rebuild.

- A kernel rebuild re-stamps the version (build_ident) -> new modules needed; a DT-only
  change does not need that. Same cpp + dtc steps as kbuild's cmd_dtc.
- --tree fedora: kernel_src() + build-out-fedora-perf/kbuild; truenas: truenas-unvr + truenas_build_out().
- Prints the decompiled diff against that build's own DTB: review it, nothing else should move.

  ./dev.py build-dtb --tree fedora|truenas    # -> tmp/dtb/unvr-<tree>.dtb
"""

from __future__ import annotations

import argparse
import difflib
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _kbuild import CROSS, DTS_NAME  # noqa: E402

from _repo import (
    REFS,
    REPO,
    TMP,
    kernel_build_out,
    kernel_src,
    make_log,
    truenas_build_out,
)  # noqa: E402

log = make_log("build-dtb")
OUT = TMP / "dtb"
# cpp + dtc of one board DT: well under 1 s. 60 s backstop; expiry -> TimeoutExpired, rc != 0.
STEP_TIMEOUT_S = 60


def trees(name: str) -> tuple[Path, Path, Path]:
    """(kernel src, dtc binary, that build's DTB)."""
    if name == "fedora":
        out = kernel_build_out("perf")
        dtbs = sorted(out.glob(f"{DTS_NAME}-*.dtb"))
        return kernel_src(), out / "kbuild/scripts/dtc/dtc", dtbs[-1]
    out = truenas_build_out()
    dtbs = sorted(out.glob(f"{DTS_NAME}-*-truenas.dtb"))
    return (REFS / "truenas-unvr").resolve(), out / "kbuild/scripts/dtc/dtc", dtbs[-1]


def run(cmd: list[str]) -> str:
    r = subprocess.run(
        [str(c) for c in cmd],
        capture_output=True,
        text=True,
        timeout=STEP_TIMEOUT_S,
        check=False,
    )
    if r.returncode:
        raise RuntimeError(f"rc={r.returncode}: {' '.join(map(str, cmd))}\n{r.stderr}")
    return r.stdout


def build(tree: str) -> Path:
    src, dtc, ref = trees(tree)
    OUT.mkdir(parents=True, exist_ok=True)
    pp = OUT / f"{tree}.dts.pp"
    dtb = OUT / f"unvr-{tree}.dtb"
    inc = src / "scripts/dtc/include-prefixes"
    run(
        [
            f"{CROSS}gcc",
            "-E",
            "-nostdinc",
            "-I",
            REPO / "dts",
            "-I",
            inc,
            "-undef",
            "-D__DTS__",
            "-x",
            "assembler-with-cpp",
            "-o",
            pp,
            REPO / "dts" / f"{DTS_NAME}.dts",
        ]
    )
    run(
        [
            dtc,
            "-O",
            "dtb",
            "-o",
            dtb,
            "-b",
            "0",
            "-i",
            REPO / "dts",
            "-i",
            inc,
            "-Wno-unique_unit_address",
            "-Wno-interrupt_provider",
            pp,
        ]
    )
    a = run([dtc, "-I", "dtb", "-O", "dts", "-s", ref]).splitlines()
    b = run([dtc, "-I", "dtb", "-O", "dts", "-s", dtb]).splitlines()
    diff = list(difflib.unified_diff(a, b, str(ref), str(dtb), lineterm="", n=2))
    log(f"{tree}: {dtb} ({dtb.stat().st_size} B) from {src}")
    log(f"diff vs {ref.name}: {len(diff)} lines" + ("" if diff else " (identical)"))
    for line in diff:
        log(f"  {line}")
    return dtb


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--tree", choices=("fedora", "truenas"), required=True)
    a = ap.parse_args()
    try:
        build(a.tree)
    except (RuntimeError, subprocess.TimeoutExpired, IndexError) as e:
        log(f"FATAL: {e}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
