#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Diff al_eth_regdump() output between Linux and U-Boot, register for register.

Both sides run the same HAL function (modules/al_eth/al_hal_eth_main.c,
staged into U-Boot), emitting one line per register:

    regdump <adapter> <stage> <block>.<reg>=<hex>

Linux writes to dmesg (fetched over SSH, or --linux-file); U-Boot to the
console log. Adapters are named differently per side (Linux al_eth_N, U-Boot
alu_eth_10g/1g), so both names are given. Stages compared by default:
after_adapter_init, after_mac_config, after_mac_start; U-Boot's tx_timeout and
Linux's ondemand are matched against each other (both = "steady state, link up").

Usage:
    ./scripts/regdump-diff.py --linux al_eth_1 --uboot alu_eth_10g
    ./scripts/regdump-diff.py --linux al_eth_1 --uboot alu_eth_10g --linux-file dmesg.txt
    ./scripts/regdump-diff.py ... --all      # print matching registers too

Log: tmp/logs/regdump-diff.log
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
from _repo import make_log  # noqa: E402

log = make_log("regdump-diff", stamped=False)

LINE = re.compile(r"regdump (\S+) (\S+) (\S+)=([0-9a-f]{8})")
CONSOLE = REPO / "tmp/logs/unvr-console.log"
# U-Boot's steady-state dump is tx_timeout; Linux's is ondemand (link up).
STAGE_ALIAS = {"tx_timeout": "steady", "ondemand": "steady"}
DEFAULT_STAGES = ["after_adapter_init", "after_mac_config", "after_mac_start", "steady"]


def parse(text: str, adapter: str) -> dict[str, dict[str, int]]:
    """{stage: {block.reg: value}}; the LAST dump of each stage wins."""
    out: dict[str, dict[str, int]] = defaultdict(dict)
    seen_begin: dict[str, int] = defaultdict(int)
    for m in LINE.finditer(text):
        name, stage, reg, val = m.groups()
        if name != adapter:
            continue
        stage = STAGE_ALIAS.get(stage, stage)
        out[stage][reg] = int(val, 16)
    for m in re.finditer(r"regdump (\S+) (\S+) begin", text):
        if m.group(1) == adapter:
            seen_begin[STAGE_ALIAS.get(m.group(2), m.group(2))] += 1
    for st, n in seen_begin.items():
        if n > 1:
            log(f"note: {adapter} stage {st} dumped {n}x - using the last")
    return out


def linux_text(path: str | None) -> str:
    if path:
        return Path(path).read_text(errors="replace")
    log("fetching Linux dmesg over ssh")
    r = subprocess.run(
        [sys.executable, str(REPO / "scripts/ssh-woomera.py"), "--", "dmesg"],
        capture_output=True,
        text=True,
        check=False,
    )
    if r.returncode:
        sys.exit(f"dmesg fetch failed: {r.stderr.strip()[:200]}")
    return r.stdout


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--linux", required=True, help="Linux adapter name, e.g. al_eth_1")
    ap.add_argument(
        "--uboot", required=True, help="U-Boot adapter name, e.g. alu_eth_10g"
    )
    ap.add_argument("--linux-file", help="saved dmesg instead of ssh")
    ap.add_argument("--uboot-file", default=str(CONSOLE))
    ap.add_argument("--stages", default=",".join(DEFAULT_STAGES))
    ap.add_argument("--all", action="store_true", help="also print matching registers")
    a = ap.parse_args()

    lin = parse(linux_text(a.linux_file), a.linux)
    ub_text = (
        Path(a.uboot_file).read_bytes().replace(b"\x00", b"").decode("utf-8", "replace")
    )
    ub = parse(ub_text, a.uboot)

    if not lin:
        sys.exit(f"no regdump lines for Linux adapter {a.linux!r}")
    if not ub:
        sys.exit(f"no regdump lines for U-Boot adapter {a.uboot!r} in {a.uboot_file}")

    total_diff = 0
    for stage in a.stages.split(","):
        L, U = lin.get(stage, {}), ub.get(stage, {})
        if not L and not U:
            log(f"[{stage}] absent on both sides")
            continue
        if not L or not U:
            log(f"[{stage}] only on {'Linux' if L else 'U-Boot'} ({len(L or U)} regs)")
            continue
        regs = sorted(set(L) | set(U))
        diffs = [(r, L.get(r), U.get(r)) for r in regs if L.get(r) != U.get(r)]
        log(f"[{stage}] {len(regs)} regs, {len(diffs)} differ")
        for r, lv, uv in diffs:
            ls = f"{lv:08x}" if lv is not None else "   -    "
            us = f"{uv:08x}" if uv is not None else "   -    "
            log(f"  {r:34s} linux={ls}  uboot={us}  xor={(lv or 0) ^ (uv or 0):08x}")
        if a.all:
            for r in regs:
                if L.get(r) == U.get(r):
                    log(f"  {r:34s} both={L[r]:08x}")
        total_diff += len(diffs)
    log(f"total differing registers across stages: {total_diff}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
