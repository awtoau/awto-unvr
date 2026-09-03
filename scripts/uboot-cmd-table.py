#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Recover U-Boot's command table from a stripped U-Boot binary.

Why: stock U-Boot exposes SerDes/link controls we have no source for -
`eth_freeze_serdes_settings`, `eth_link_training_enable`, `eth_retimer_config`,
`eth_mac_mode_set`, `eth_1g_params_set`, `eth_lm_debug_enable` (#197). Ubiquiti's
GPL drop is not in hand, so the implementations have to come out of the binary.
This finds each command's handler address so only those functions need
decompiling, instead of the whole image.

U-Boot keeps commands in a linker list of `struct cmd_tbl`:

    char *name;                                    +0
    int   maxargs;                                 +ptr
    int   repeatable;                              +ptr+4
    int (*cmd)(struct cmd_tbl *, int, int, char *const []);   +2*ptr
    char *usage;                                   +3*ptr
    char *help;        (CONFIG_SYS_LONGHELP)       +4*ptr
    int (*complete)(); (CONFIG_AUTO_COMPLETE)      +5*ptr

The link base is unknown for a raw image, so it is SOLVED rather than guessed:
find the file offsets of several command-name strings, then look for pointer-sized
values whose (value - offset) is the SAME constant across several of them. That
constant is the load base. A single match would be coincidence; agreement across
many commands is not.

Usage:
    ./scripts/uboot-cmd-table.py <uboot.bin> [--only eth_ ...]
    ./scripts/uboot-cmd-table.py tmp/sections/01-uboot.bin --only eth_
"""

from __future__ import annotations

import argparse
import re
import struct
import sys
from collections import Counter
from pathlib import Path

# Commands used to solve for the base. Common, and present in every U-Boot.
ANCHORS = [b"bootm", b"tftpboot", b"printenv", b"setenv", b"version", b"help"]


def cstring_offsets(buf: bytes, name: bytes) -> list[int]:
    """Offsets where `name` appears as a complete NUL-terminated C string."""
    out = []
    for m in re.finditer(re.escape(name) + b"\x00", buf):
        i = m.start()
        # Must start a string: preceded by NUL or padding, not mid-identifier.
        if (
            i == 0
            or buf[i - 1] in (0, 0xFF)
            or not (
                48 <= buf[i - 1] <= 57
                or 65 <= buf[i - 1] <= 90
                or 97 <= buf[i - 1] <= 122
                or buf[i - 1] in (95, 45)
            )
        ):
            out.append(i)
    return out


def solve_base(buf: bytes, ptr_size: int) -> tuple[int, int]:
    """Return (base, n_names) - the load address agreed on by the most DISTINCT
    command names.

    Counting raw pointer matches lets one string with many coincidental hits win.
    A base is only credible if several different command names independently
    imply it, so each name votes at most once per candidate.
    """
    fmt = "<Q" if ptr_size == 8 else "<I"
    step = 4
    values = set()
    for p in range(0, len(buf) - ptr_size, step):
        (val,) = struct.unpack_from(fmt, buf, p)
        if val:
            values.add(val)

    votes: Counter = Counter()
    for name in ANCHORS:
        cands = set()
        for off in cstring_offsets(buf, name):
            for val in values:
                if val <= off:
                    continue
                cand = val - off
                if cand & 0x3:  # a load base is at least word-aligned
                    continue
                if ptr_size == 8 and cand > (1 << 40):
                    continue
                cands.add(cand)
        for c in cands:  # each NAME votes once per candidate
            votes[c] += 1

    if not votes:
        return (0, 0)
    base, n = votes.most_common(1)[0]
    return (base, n)


def read_ptr(buf: bytes, off: int, ptr_size: int) -> int:
    fmt = "<Q" if ptr_size == 8 else "<I"
    if off + ptr_size > len(buf):
        return 0
    return struct.unpack_from(fmt, buf, off)[0]


def cstr_at(buf: bytes, addr: int, base: int, limit: int = 400) -> str:
    off = addr - base
    if off < 0 or off >= len(buf):
        return ""
    end = buf.find(b"\x00", off, off + limit)
    if end < 0:
        return ""
    return buf[off:end].decode("utf-8", "replace")


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("binary")
    ap.add_argument(
        "--only",
        default="",
        help="only report commands whose name contains this substring",
    )
    ap.add_argument(
        "--ptr-size",
        type=int,
        choices=(4, 8),
        default=0,
        help="pointer size; default: try 8 then 4",
    )
    ap.add_argument(
        "--base", default="", help="skip base solving and use this hex load address"
    )
    ap.add_argument(
        "--str",
        dest="strs",
        action="append",
        default=[],
        help="print the C string at this hex ADDRESS (repeatable) - "
        "for resolving adrp/add pairs seen in a disassembly",
    )
    args = ap.parse_args()

    buf = Path(args.binary).read_bytes()

    if args.base:
        base, ptr_size, votes = int(args.base, 16), (args.ptr_size or 8), -1
        print(f"using supplied base 0x{base:x}, ptr_size={ptr_size}\n")
    else:
        sizes = [args.ptr_size] if args.ptr_size else [8, 4]
        base = votes = ptr_size = 0
        for ps in sizes:
            b, v = solve_base(buf, ps)
            print(f"ptr_size={ps}: base candidate 0x{b:x}, {v} distinct names agree")
            if v > votes:
                base, votes, ptr_size = b, v, ps

        if votes < 2:
            print("FAILED: no load base agreed on by >=2 distinct command names")
            return 1
        print(f"\nusing base 0x{base:x}, ptr_size={ptr_size} ({votes} names)\n")

    if args.strs:
        for a in args.strs:
            addr = int(a, 16)
            print(f"0x{addr:x} (+0x{addr - base:x}): {cstr_at(buf, addr, base)!r}")
        return 0

    # Walk every string that looks like a command name and see whether a
    # cmd_tbl entry points at it.
    seen = set()
    rows = []
    for m in re.finditer(rb"[a-z][a-z0-9_]{2,40}\x00", buf):
        name = m.group()[:-1]
        if args.only and args.only.encode() not in name:
            continue
        if name in seen:
            continue
        off = m.start()
        target = base + off
        fmt = "<Q" if ptr_size == 8 else "<I"
        for p in range(0, len(buf) - ptr_size, 4):
            if struct.unpack_from(fmt, buf, p)[0] != target:
                continue
            cmd = read_ptr(buf, p + 2 * ptr_size, ptr_size)
            usage = read_ptr(buf, p + 3 * ptr_size, ptr_size)
            maxargs = struct.unpack_from("<I", buf, p + ptr_size)[0]
            # A real entry: sane maxargs and a code pointer inside the image.
            if not (0 < maxargs < 32):
                continue
            if not (base <= cmd < base + len(buf)):
                continue
            seen.add(name)
            rows.append(
                {
                    "name": name.decode(),
                    "entry_off": p,
                    "maxargs": maxargs,
                    "cmd": cmd,
                    "cmd_off": cmd - base,
                    "usage": cstr_at(buf, usage, base).split("\n")[0][:60],
                }
            )
            break

    if not rows:
        print("no command-table entries matched")
        return 1

    rows.sort(key=lambda r: r["name"])
    print(f"{'command':<32} {'handler':>12} {'file off':>10} {'args':>4}  usage")
    for r in rows:
        print(
            f"{r['name']:<32} 0x{r['cmd']:010x} 0x{r['cmd_off']:08x} "
            f"{r['maxargs']:>4}  {r['usage']}"
        )
    print(f"\n{len(rows)} commands. Disassemble one with:")
    print(
        "  aarch64-linux-gnu-objdump -D -b binary -m aarch64 "
        f"--adjust-vma=0x{base:x} --start-address=<handler> "
        f"--stop-address=<handler+0x400> {args.binary}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
