#!/usr/bin/env python3
"""Decode the stage3 sysid->DTB-instance jump table in an Alpine preboot payload.

The contractor stage3 (multi_dt) reads the board id from the SPI-NOR EEPROM
partition (flash off 0x1F000C, big-endian), computes idx = boardid - BASE, and
dispatches through an ARM `ldrls pc,[pc,idx,lsl#2]` jump table. Each table slot
targets a `b common`; the `mov r2,#instance` sits at target+4. This finds the
table, reads each slot, and reports sysid -> DTB instance.

Input = the carved al_boot preboot payload (container[0x21000:], ARM32 base
0x01000000). Verified against 5.1.25 (adds ea30->4 / dt_hd) and the old build.
"""
from __future__ import annotations

import argparse
import logging
import re
import struct
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
LOG = REPO / "tmp" / "logs" / "decode-preboot-dtmap.log"
DT_NAMES = {0: "dt (one nas v5.0)", 1: "dt_pro (one nas pro)",
            2: "dt_ai (one nas ai)", 3: "dt_bt (one nas bt)",
            4: "dt_hd (one nas hd)", 0xFF: "INVALID (error path)"}


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("payload", type=Path, help="carved preboot payload (ARM32)")
    ap.add_argument("--base", type=lambda s: int(s, 0), default=0x01000000)
    a = ap.parse_args()
    LOG.parent.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(level=logging.INFO, format="%(message)s",
                        handlers=[logging.FileHandler(LOG),
                                  logging.StreamHandler(sys.stdout)])

    b = a.payload.read_bytes()
    dis = subprocess.run(
        ["arm-linux-gnu-objdump", "-D", "-b", "binary", "-m", "arm", "-EL",
         f"--adjust-vma=0x{a.base:x}", str(a.payload)],
        capture_output=True, text=True, check=True).stdout.splitlines()

    # find: sub rN,rN,#0xea00 ; sub rN,rN,#0x16 ; cmp rN,#count ; ldrls pc,[pc,...
    disp = None
    for i, ln in enumerate(dis):
        if "ldrls" in ln and "pc, [pc," in ln:
            ctx = " ".join(dis[max(0, i - 6):i])
            m = re.search(r"sub\s+\w+,\s*\w+,\s*#(\d+).*sub\s+\w+,\s*\w+,\s*#(\d+)"
                          r".*cmp\s+\w+,\s*#(\d+)", ctx)
            if m:
                base_sysid = int(m.group(1)) + int(m.group(2))
                count = int(m.group(3)) + 1
                ldr_va = int(re.match(r"\s*([0-9a-f]+):", ln).group(1), 16)
                disp = (base_sysid, count, ldr_va)
                break
    if not disp:
        logging.error("dispatch pattern not found")
        return 1
    base_sysid, count, ldr_va = disp
    tbl = ldr_va + 8                     # ARM: pc = insn+8
    logging.info("dispatch @0x%08x  base sysid 0x%04x  %d slots  table 0x%08x",
                 ldr_va, base_sysid, count, tbl)
    logging.info("boardid read from SPI-NOR EEPROM partition flash off 0x1F000C (BE)")
    logging.info("%-8s %-8s %s", "sysid", "instance", "DTB")
    fo = tbl - a.base
    for k in range(count):
        tgt = struct.unpack_from("<I", b, fo + k * 4)[0]
        mov = struct.unpack_from("<I", b, (tgt + 4) - a.base)[0]  # mov r2,#imm
        if (mov & 0x0FFFF000) == 0x03A02000:      # mov r2,#imm
            inst = mov & 0xFF
        elif (mov & 0x0FFFF000) == 0x03E02000:    # mvn r2,#0 -> 0xFF
            inst = 0xFF
        else:
            continue
        logging.info("0x%04x   %-8s %s", base_sysid + k, inst,
                     DT_NAMES.get(inst, "?"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
