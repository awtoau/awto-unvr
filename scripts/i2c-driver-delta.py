#!/usr/bin/env python3
"""Diff the i2c/RTC driver path between UBNT's working 4.1.37 kernel and mainline.

Why: the s35390a ch0 wedge (docs/rtc-s35390a-fault.md, #86) is a code delta —
stock Linux 5.1/4.1.37 drove this exact bus without wedging. This dumps the
three drivers on the path (DW i2c, pca954x mux, rtc-s35390a) side by side so the
delta is reviewable rather than re-derived each session.

Output: ./tmp/logs/i2c-driver-delta.log (also stdout).
Read-only: never touches the box.
"""

from __future__ import annotations

import logging
import subprocess
import sys
from pathlib import Path

REFS = Path("/mnt/2tb/unvr-port-refs")
UBNT = REFS / "UBNT-source-code/UNVR-1.3.35-GPL/linux-arm64-unvr-4.1.37-ubnt"
MAINLINE = REFS / "linux-v7.3-fresh"

REPO = Path(__file__).resolve().parent.parent
LOG_DIR = REPO / "tmp" / "logs"

# (label, path under UBNT, path under mainline). The DW driver was split into
# common/master/platdrv upstream, so one 4.1.37 file maps to several.
PAIRS: list[tuple[str, str, str]] = [
    ("rtc-s35390a", "drivers/rtc/rtc-s35390a.c", "drivers/rtc/rtc-s35390a.c"),
    ("i2c-mux-pca954x", "drivers/i2c/muxes/i2c-mux-pca954x.c",
     "drivers/i2c/muxes/i2c-mux-pca954x.c"),
    ("dw-core-vs-master", "drivers/i2c/busses/i2c-designware-core.c",
     "drivers/i2c/busses/i2c-designware-master.c"),
    ("dw-core-vs-common", "drivers/i2c/busses/i2c-designware-core.c",
     "drivers/i2c/busses/i2c-designware-common.c"),
    ("dw-platdrv", "drivers/i2c/busses/i2c-designware-platdrv.c",
     "drivers/i2c/busses/i2c-designware-platdrv.c"),
]

# Properties/symbols worth grepping for on both sides: each is a knob that
# changes what appears on the wire for the fussy s35390a.
MARKERS = [
    "hcnt-raw", "lcnt-raw", "sda-hold-time-ns", "scl-falling-time-ns",
    "DW_IC_SDA_HOLD_RX_MASK", "i2c_recover_bus", "i2c-mux-idle-disconnect",
    "MUX_IDLE_DISCONNECT",
]


def setup_logging() -> None:
    LOG_DIR.mkdir(parents=True, exist_ok=True)
    handlers: list[logging.Handler] = [
        logging.FileHandler(LOG_DIR / "i2c-driver-delta.log", mode="w"),
        logging.StreamHandler(sys.stdout),
    ]
    logging.basicConfig(level=logging.INFO, format="%(message)s", handlers=handlers)


def check_trees() -> bool:
    ok = True
    for tree in (UBNT, MAINLINE):
        if not tree.is_dir():
            logging.error("missing reference tree: %s", tree)
            ok = False
    return ok


def diff_pair(label: str, ubnt_rel: str, main_rel: str) -> None:
    a, b = UBNT / ubnt_rel, MAINLINE / main_rel
    logging.info("\n%s\n== %s\n%s", "=" * 78, label, "=" * 78)
    if not a.is_file() or not b.is_file():
        logging.warning("skip %s: missing %s", label,
                        a if not a.is_file() else b)
        return
    # diff exits 1 when files differ; that is the expected case, not an error.
    res = subprocess.run(["diff", "-u", str(a), str(b)],
                         capture_output=True, text=True, check=False)
    if res.returncode == 0:
        logging.info("identical")
    else:
        logging.info(res.stdout)


def scan_markers() -> None:
    logging.info("\n%s\n== marker presence (UBNT 4.1.37 vs mainline v7.3)\n%s",
                 "=" * 78, "=" * 78)
    for marker in MARKERS:
        for name, tree in (("ubnt", UBNT), ("main", MAINLINE)):
            res = subprocess.run(
                ["grep", "-rl", marker, str(tree / "drivers/i2c"),
                 str(tree / "drivers/rtc")],
                capture_output=True, text=True, check=False)
            hits = [h for h in res.stdout.split("\n") if h]
            rel = sorted(str(Path(h).relative_to(tree)) for h in hits)
            logging.info("%-26s %-5s %s", marker, name, ", ".join(rel) or "-")


def main() -> int:
    setup_logging()
    if not check_trees():
        return 1
    logging.info("UBNT 4.1.37: %s", UBNT)
    logging.info("mainline   : %s", MAINLINE)
    for label, a, b in PAIRS:
        diff_pair(label, a, b)
    scan_markers()
    logging.info("\nlog: %s", LOG_DIR / "i2c-driver-delta.log")
    return 0


if __name__ == "__main__":
    sys.exit(main())
