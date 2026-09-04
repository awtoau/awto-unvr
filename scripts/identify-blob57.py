#!/usr/bin/env python3
"""Identify al_boot's `blob57` (issue #246).

Extracts the opaque 57 KB region from the carved al_boot payload, extracts the
reference `al_serdes_25g_fw[]` byte array from the Annapurna HAL header, and
compares them byte-for-byte.

Also reports the loader's call sites in the decompiled preboot so the boot-path
question ("is it reached before U-Boot?") is answered from the call graph.

Inputs (paths overridable on the CLI):
  tmp/alboot-payload.bin                            carved al_boot payload, 304,816 B
  <refs>/delroth-alpine_hal/drivers/serdes/al_hal_serdes_25g_fw.h
  docs/nor-reference/preboot-alboot-decompiled.c

Output: tmp/blob57/*.bin + a report on stdout and tmp/logs/identify-blob57.log
"""

from __future__ import annotations

import argparse
import hashlib
import logging
import math
import re
import sys
from collections import Counter
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent

# al_boot payload VA base and the blob's extent, both proven from the loader
# FUN_01021b60: source pointer 0x010346a0, byte count 0xdffc.
AL_BOOT_VA_BASE = 0x01000000
BLOB_VA = 0x010346A0
BLOB_LEN = 0xDFFC


def setup_logging(log_path: Path) -> None:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    handlers = [
        logging.FileHandler(log_path, mode="w"),
        logging.StreamHandler(sys.stdout),
    ]
    logging.basicConfig(level=logging.INFO, format="%(message)s", handlers=handlers)


def entropy(data: bytes) -> float:
    if not data:
        return 0.0
    counts = Counter(data)
    n = len(data)
    return -sum((c / n) * math.log2(c / n) for c in counts.values())


def extract_hal_fw(header: Path) -> bytes:
    """Pull the `al_serdes_25g_fw[]` initialiser out of the HAL header."""
    text = header.read_text(errors="replace")
    m = re.search(r"al_serdes_25g_fw\[\]\s*=\s*\{(.*?)\}\s*;", text, re.S)
    if not m:
        raise SystemExit(f"no al_serdes_25g_fw[] array found in {header}")
    return bytes(int(x, 16) for x in re.findall(r"0x([0-9a-fA-F]{2})", m.group(1)))


def find_loader_callers(decomp: Path, loader_va: int) -> list[tuple[int, str]]:
    """Return (line_no, text) for every line calling FUN_<loader_va>."""
    name = f"FUN_{loader_va:08x}"
    hits = []
    for i, line in enumerate(decomp.read_text(errors="replace").splitlines(), 1):
        if name in line:
            hits.append((i, line.strip()))
    return hits


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--payload", type=Path, default=REPO / "tmp/alboot-payload.bin")
    ap.add_argument(
        "--hal-fw",
        type=Path,
        default=Path(
            "/mnt/2tb/unvr-port-refs/delroth-alpine_hal/drivers/serdes/al_hal_serdes_25g_fw.h"
        ),
    )
    ap.add_argument(
        "--decompiled",
        type=Path,
        default=REPO / "docs/nor-reference/preboot-alboot-decompiled.c",
    )
    ap.add_argument("--outdir", type=Path, default=REPO / "tmp/blob57")
    args = ap.parse_args()

    setup_logging(REPO / "tmp/logs/identify-blob57.log")
    args.outdir.mkdir(parents=True, exist_ok=True)

    payload = args.payload.read_bytes()
    logging.info(
        "payload %s: %d B sha256=%s",
        args.payload,
        len(payload),
        hashlib.sha256(payload).hexdigest(),
    )

    off = BLOB_VA - AL_BOOT_VA_BASE
    blob = payload[off : off + BLOB_LEN]
    (args.outdir / "blob57.bin").write_bytes(blob)
    logging.info(
        "blob57: VA 0x%08x..0x%08x  file 0x%05x..0x%05x  %d B  entropy %.3f  sha256=%s",
        BLOB_VA,
        BLOB_VA + BLOB_LEN,
        off,
        off + BLOB_LEN,
        len(blob),
        entropy(blob),
        hashlib.sha256(blob).hexdigest(),
    )

    ref = extract_hal_fw(args.hal_fw)
    (args.outdir / "hal_serdes_25g_fw.bin").write_bytes(ref)
    logging.info(
        "HAL al_serdes_25g_fw[]: %d B  entropy %.3f  sha256=%s",
        len(ref),
        entropy(ref),
        hashlib.sha256(ref).hexdigest(),
    )

    logging.info("")
    n = min(len(blob), len(ref))
    diffs = [i for i in range(n) if blob[i] != ref[i]]
    logging.info("common prefix %d B, mismatching bytes in it: %d", n, len(diffs))
    if len(blob) == len(ref) and not diffs:
        logging.info(
            "VERDICT: blob57 is BYTE-IDENTICAL to the HAL's al_serdes_25g_fw[]."
        )
    else:
        logging.info("length delta: blob %d vs HAL %d", len(blob), len(ref))
        for i in diffs[:32]:
            logging.info("  @0x%05x blob=%02x hal=%02x", i, blob[i], ref[i])
        if len(blob) > n:
            logging.info("  blob tail beyond HAL: %s", blob[n : n + 48].hex())
        if len(ref) > n:
            logging.info("  HAL tail beyond blob: %s", ref[n : n + 48].hex())

    logging.info("")
    logging.info("=== loader call sites (FUN_01021b60 = 25G FW download) ===")
    for lineno, text in find_loader_callers(args.decompiled, 0x01021B60):
        logging.info("  %s:%d  %s", args.decompiled.name, lineno, text)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
