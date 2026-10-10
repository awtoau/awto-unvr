#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Rewrite positional struct initialisers as C99 designated initialisers (#189).

Field names come from each struct's definition in al_hal_eth.h, not from typing.
Each value token is copied verbatim, so only the field NAME is added. The count
of values per entry must equal the struct's member count, else the script aborts.

Verify the result by building the object before and after and comparing it
(see the #189 issue), not by reading this script's output.

Usage:
    scripts/designate-al-eth-tables.py            # rewrite in place
    scripts/designate-al-eth-tables.py --check    # report only, write nothing
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
SRC = REPO / "modules" / "al_eth" / "al_hal_eth_main.c"
HDR = REPO / "modules" / "al_eth" / "al_hal_eth.h"
LOG = REPO / "tmp" / "logs" / "designate-al-eth-tables.log"

# (table symbol, struct tag, entry-count macro)
TABLES = [
    (
        "al_eth_generic_tx_crc_gpd",
        "al_eth_tx_gpd_cam_entry",
        "AL_ETH_TX_GENERIC_CRC_ENTRIES_NUM",
    ),
    (
        "al_eth_tx_crc_chksum_replace_cmd",
        "al_eth_tx_crc_chksum_replace_cmd_for_protocol_num_entry",
        "AL_ETH_TX_GENERIC_CRC_ENTRIES_NUM",
    ),
    (
        "al_eth_generic_rx_crc_gpd",
        "al_eth_rx_gpd_cam_entry",
        "AL_ETH_RX_PROTOCOL_DETECT_ENTRIES_NUM",
    ),
]

COMMENT = re.compile(r"/\*.*?\*/", re.S)
MEMBER = re.compile(r"(\w+)\s*(?::\s*\d+)?\s*;\s*$")
ENTRY = re.compile(r"(?P<c>/\*.*?\*/\s*)?\{(?P<v>[^{}]*)\}", re.S)


def log(msg: str, fh) -> None:
    print(msg)
    fh.write(msg + "\n")


def struct_members(hdr: str, tag: str) -> list[str]:
    start = hdr.index(f"struct {tag} {{")
    body = hdr[start : hdr.index("\n};", start)]
    body = COMMENT.sub("", body)
    names = []
    for line in body.splitlines()[1:]:
        line = line.split("//")[0].strip()
        if not line:
            continue
        m = MEMBER.search(line)
        if m is None:
            raise SystemExit(f"cannot parse member line in struct {tag}: {line!r}")
        names.append(m.group(1))
    return names


def macro_value(src: str, name: str) -> int:
    m = re.search(rf"#define {name} (\d+)\b", src)
    if m is None:
        raise SystemExit(f"macro {name} not found")
    return int(m.group(1))


def convert_table(src: str, sym: str, tag: str, count_macro: str, hdr: str, fh) -> str:
    fields = struct_members(hdr, tag)
    expected = macro_value(src, count_macro)
    head = src.index(f"{sym}[")
    body_start = src.index("= {", head) + len("= {")
    body_end = src.index("\n};", body_start)
    raw = src[body_start:body_end]

    total_comments = raw.count("/*")
    out = []
    entries = 0
    matched_comments = 0
    for m in ENTRY.finditer(raw):
        entries += 1
        values = [v.strip() for v in m.group("v").split(",")]
        if values and values[-1] == "":
            values.pop()
        if len(values) != len(fields):
            raise SystemExit(
                f"{sym}: entry has {len(values)} values, struct {tag} has "
                f"{len(fields)} members (entry {len(out)})"
            )
        comment = m.group("c")
        if comment:
            matched_comments += 1
            out.append("\t" + comment.strip())
        out.append("\t{")
        for name, val in zip(fields, values):
            out.append(f"\t\t.{name} = {val},")
        out.append("\t},")

    if entries != expected:
        raise SystemExit(f"{sym}: parsed {entries} entries != {count_macro}={expected}")
    if matched_comments != total_comments:
        raise SystemExit(
            f"{sym}: {total_comments - matched_comments} comment(s) "
            "not attached to an entry; refusing to drop them"
        )

    log(f"{sym}: {expected} entries x {len(fields)} members -> designated", fh)
    new_body = "\n" + "\n".join(out) + "\n"
    return src[:body_start] + new_body + src[body_end + 1 :]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--check", action="store_true", help="report only")
    args = ap.parse_args()

    LOG.parent.mkdir(parents=True, exist_ok=True)
    with LOG.open("w") as fh:
        hdr = HDR.read_text()
        src = SRC.read_text()
        for sym, tag, count_macro in TABLES:
            src = convert_table(src, sym, tag, count_macro, hdr, fh)
        if args.check:
            log("check only: nothing written", fh)
            return 0
        SRC.write_text(src)
        log(f"wrote {SRC.relative_to(REPO)}", fh)
    return 0


if __name__ == "__main__":
    sys.exit(main())
