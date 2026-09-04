#!/usr/bin/env python3
"""Headless Ghidra decompile of the stock rpsd AArch64 ELF (RPS interface RE).

Unlike scripts/ghidra-decompile.py (raw blob + BinaryLoader), rpsd is a normal
PIE ELF: let Ghidra's ELF loader place it so section vaddrs / relocations /
PLT thunks resolve, which is what makes json_pack()/json_object_set() call
sites readable.

Output: <out>/decompiled.c + <out>/disassembly.asm ; log tmp/logs/rps-decompile.log
"""

from __future__ import annotations

import argparse
import logging
import os
import subprocess
import sys
from pathlib import Path

log = logging.getLogger(__name__)

REPO = Path(__file__).resolve().parent.parent
# Override with AWTO_GHIDRA for a different install.
GHIDRA = Path(
    os.environ.get("AWTO_GHIDRA", "~/tools/ghidra_12.2_DEV_20260816")
).expanduser()
HEADLESS = GHIDRA / "support" / "analyzeHeadless"
SCRIPTPATH = REPO / "tmp" / "ghidra-scripts"
LOG = REPO / "tmp" / "logs" / "rps-decompile.log"

# 181 KB AArch64 ELF, ~700 functions. Observed worst case on this box ~8 min
# for a comparable image; 1.25x rounded up -> 900 s. On expiry Ghidra is killed
# and the operation + elapsed are logged (nothing is left half-imported: the
# project dir is throwaway and -deleteProject is requested).
TIMEOUT_S = 900


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("blob", type=Path)
    ap.add_argument("--name", default="rpsd")
    ap.add_argument("--proc", default="AARCH64:LE:64:v8A")
    ap.add_argument("--out", type=Path, default=REPO / "tmp" / "ghidra-out-rpsd")
    ap.add_argument("--proj", type=Path, default=REPO / "tmp" / "ghidra-proj-rpsd")
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
        a.proc,
        "-scriptPath",
        str(SCRIPTPATH),
        "-postScript",
        "ExportAll.java",
        str(a.out),
        "-deleteProject",
    ]
    log.info("RUN: %s", " ".join(cmd))
    try:
        r = subprocess.run(
            cmd, capture_output=True, text=True, timeout=TIMEOUT_S, check=False
        )
    except subprocess.TimeoutExpired as e:
        log.error(
            "TIMEOUT: ghidra headless import+decompile of %s exceeded %d s",
            a.blob,
            TIMEOUT_S,
        )
        log.error("partial stdout tail:\n%s", (e.stdout or b"")[-2000:])
        return 2
    log.info(r.stdout[-8000:] if r.stdout else "(no stdout)")
    if r.stderr:
        log.info("STDERR tail:\n%s", r.stderr[-4000:])
    log.info("exit %d", r.returncode)
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
