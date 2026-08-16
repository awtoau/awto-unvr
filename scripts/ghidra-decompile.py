#!/usr/bin/env python3
"""Headless Ghidra: import an ARM32 preboot blob, auto-analyse, export decompiled C + asm.

Ghidra 12.2_DEV master (>=2026-08 build) requires JDK 25 (application.java.min=25,
compiler=25); JDK 21 is now rejected by the launcher. JDK 25 does NOT hang the
decompiler on this build (verified: 103-func ELF, 0 failures). Set out-of-tree via
JAVA_HOME_OVERRIDE=/usr/lib/jvm/java-25-openjdk in <ghidra>/support/launch.properties.
See docs/preboot-decompile.md.

Usage:
  scripts/ghidra-decompile.py <blob> --base 0x1000000 --name alboot [--out DIR]
Exports <out>/decompiled.c, disassembly.asm via ExportAll.java.
"""
from __future__ import annotations

import argparse
import logging
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
GHIDRA = Path("/home/dan/tools/ghidra_12.2_DEV_20260816")
HEADLESS = GHIDRA / "support" / "analyzeHeadless"
SCRIPTPATH = REPO / "tmp" / "ghidra-scripts"
LOG = REPO / "tmp" / "logs" / "ghidra-decompile.log"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("blob", type=Path)
    ap.add_argument("--base", type=lambda s: int(s, 0), default=0x1000000)
    ap.add_argument("--name", required=True, help="ghidra program name")
    ap.add_argument("--proc", default="ARM:LE:32:v8", help="ghidra language id")
    ap.add_argument("--out", type=Path, default=REPO / "tmp" / "ghidra-out")
    ap.add_argument("--proj", type=Path, default=REPO / "tmp" / "ghidra-proj")
    a = ap.parse_args()

    LOG.parent.mkdir(parents=True, exist_ok=True)
    a.out.mkdir(parents=True, exist_ok=True)
    a.proj.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(level=logging.INFO, format="%(message)s",
                        handlers=[logging.FileHandler(LOG),
                                  logging.StreamHandler(sys.stdout)])

    cmd = [
        str(HEADLESS), str(a.proj), a.name + "_proj",
        "-import", str(a.blob),
        "-processor", a.proc,
        "-loader", "BinaryLoader",
        "-loader-baseAddr", hex(a.base),
        "-scriptPath", str(SCRIPTPATH),
        "-postScript", "ExportAll.java", str(a.out),
        "-deleteProject",
    ]
    logging.info("RUN: %s", " ".join(cmd))
    # Timeout: 435 KB ARM32 auto-analysis + decompile-all. Expected worst case
    # ~15 min on this box; 1.25x -> 1200 s. On expiry Ghidra is killed and we log.
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=1200)
    except subprocess.TimeoutExpired:
        logging.error("TIMEOUT after 1200 s analysing %s", a.blob)
        return 2
    logging.info(r.stdout[-8000:] if r.stdout else "(no stdout)")
    if r.stderr:
        logging.info("STDERR tail:\n%s", r.stderr[-4000:])
    logging.info("exit %d", r.returncode)
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
