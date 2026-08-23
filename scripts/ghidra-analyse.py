#!/usr/bin/env python3
"""One-command headless Ghidra pipeline for UNVR / AL-324 targets.

Chains, in order (see docs/ghidra.md §7):
  preScript  SetupAlpineMemory.java  -> create RAM + MMIO blocks (volatile/non-exec)
  auto-analysis
  postScript ApplyAlRegs.java <syms>  -> label HW registers (+ optional equates)
  postScript ExportAll.java <out>     -> decompiled.c + disassembly.asm

Ghidra 12.2 requires JDK 21 (application.java.min=21); JDK 25 hangs the decompiler,
JDK 17 is rejected by 12.2. Fix lives out-of-tree in
<ghidra>/support/launch.properties: JAVA_HOME_OVERRIDE=/usr/lib/jvm/java-21-temurin-jdk.

Register symbols come from scripts/gen-al-reg-symbols.py (run first, per peripheral).
Pass their output dir with --sym-dir; all *.sym and *.equ.tsv in it are applied.

Usage:
  # 64-bit U-Boot / kernel:
  scripts/ghidra-analyse.py uboot.bin --name uboot --arch aarch64 --base 0x1100000
  # 32-bit preboot al_boot payload (adds SoC-fabric regions):
  scripts/ghidra-analyse.py alboot.bin --name alboot --arch arm32 --base 0x1000000 \
      --preboot --sym-dir tmp/ghidra-in
"""

from __future__ import annotations

import argparse
import glob
import logging
import subprocess
import sys
from pathlib import Path

log = logging.getLogger(__name__)

REPO = Path(__file__).resolve().parent.parent
GHIDRA = Path("/home/dan/tools/ghidra_12.2_DEV")
HEADLESS = GHIDRA / "support" / "analyzeHeadless"
SCRIPTPATH = REPO / "scripts" / "ghidra"  # committed, reusable
LOG = REPO / "tmp" / "logs" / "ghidra-analyse.log"

# Ghidra language IDs. v8A covers the AL-324 Cortex-A57 cluster; the 32-bit preboot
# runs A32 (v8 profile). Confirm ARM vs THUMB entry per blob (S2 is Thumb-2).
LANG = {"aarch64": "AARCH64:LE:64:v8A", "arm32": "ARM:LE:32:v8"}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("blob", type=Path)
    ap.add_argument("--name", required=True, help="ghidra program name")
    ap.add_argument("--arch", choices=LANG, required=True)
    ap.add_argument(
        "--base",
        type=lambda s: int(s, 0),
        required=True,
        help="load VA (derive from AL TOC via parse-al-toc.py; never guess)",
    )
    ap.add_argument(
        "--preboot",
        action="store_true",
        help="also map SoC-fabric regions (0xf00xxxxx, 0xfbffxxxx, S2 SRAM)",
    )
    ap.add_argument(
        "--entry",
        type=lambda s: int(s, 0),
        default=None,
        help="seed a reset entry VA so auto-analysis follows it (SeedEntry.java)",
    )
    ap.add_argument(
        "--entry-thumb",
        action="store_true",
        help="the --entry point is Thumb (T32); default A32",
    )
    ap.add_argument(
        "--disasm-gaps",
        type=Path,
        default=None,
        help="file of hex VAs (verified code) to disassemble pre-analysis (DisasmGaps.java)",
    )
    ap.add_argument(
        "--sym-dir",
        type=Path,
        default=None,
        help="dir of *.sym / *.equ.tsv from gen-al-reg-symbols.py",
    )
    ap.add_argument("--out", type=Path, default=REPO / "tmp" / "ghidra-out")
    ap.add_argument("--proj", type=Path, default=REPO / "tmp" / "ghidra-proj")
    # 1.25x the observed worst case: 435 KB ARM32 auto-analysis + decompile-all was
    # ~20 s (preboot-decompile.md); a full 64-bit U-Boot/kernel is larger, budget
    # 1200 s. On expiry Ghidra is killed and the operation + elapsed are logged.
    ap.add_argument("--timeout", type=int, default=1200)
    a = ap.parse_args()

    LOG.parent.mkdir(parents=True, exist_ok=True)
    a.out.mkdir(parents=True, exist_ok=True)
    a.proj.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(
        level=logging.INFO,
        format="%(message)s",
        handlers=[logging.FileHandler(LOG), logging.StreamHandler(sys.stdout)],
    )

    cmd = [
        str(HEADLESS),
        str(a.proj),
        a.name + "_proj",
        "-import",
        str(a.blob),
        "-processor",
        LANG[a.arch],
        "-loader",
        "BinaryLoader",
        "-loader-baseAddr",
        hex(a.base),
        "-scriptPath",
        str(SCRIPTPATH),
        "-preScript",
        "SetupAlpineMemory.java",
    ]
    if a.preboot:
        cmd += ["preboot"]
    if a.entry is not None:
        cmd += [
            "-preScript",
            "SeedEntry.java",
            hex(a.entry),
            "thumb" if a.entry_thumb else "arm",
        ]
    if a.disasm_gaps is not None:
        cmd += [
            "-preScript",
            "DisasmGaps.java",
            str(a.disasm_gaps),
            "thumb" if a.entry_thumb else "arm",
        ]
    if a.sym_dir:
        syms = sorted(glob.glob(str(a.sym_dir / "*.sym")))
        syms += sorted(glob.glob(str(a.sym_dir / "*.equ.tsv")))
        if syms:
            cmd += ["-postScript", "ApplyAlRegs.java"] + syms
    cmd += ["-postScript", "ExportAll.java", str(a.out)]
    cmd += ["-postScript", "CoverageReport.java", str(a.out), "-deleteProject"]

    log.info("RUN: %s", " ".join(cmd))
    try:
        r = subprocess.run(
            cmd, capture_output=True, text=True, timeout=a.timeout, check=False
        )
    except subprocess.TimeoutExpired:
        log.error("TIMEOUT after %d s analysing %s (killed)", a.timeout, a.blob)
        return 2
    log.info(r.stdout[-8000:] if r.stdout else "(no stdout)")
    if r.stderr:
        log.info("STDERR tail:\n%s", r.stderr[-4000:])
    log.info("exit %d -> %s", r.returncode, a.out)
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
