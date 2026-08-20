#!/usr/bin/env python3
"""Annotate a preboot Ghidra export into named-function, string-resolved assembly.

The preboot's value is in the DISASSEMBLY (coproc/cache setup, DDR register pokes)
where Ghidra's C is opaque. This makes the .asm the primary, readable artifact:

  1. FUNCTION NAMES: FUN_<va> headers + `bl/blx 0x<va>` targets get real names,
     from (a) __func__ strings the code leaves in .rodata (auto, same method as
     name-preboot-funcs.py) merged with (b) a curated VA<TAB>name map (hand-RE +
     the DDR/TOC call graph) passed via --names.
  2. STRING RESOLUTION: `ldr rX,[0x<pool>]` literal-pool loads whose word points at
     printable ASCII get the text appended as a comment -> error/log strings sit
     next to the code that emits them.
  3. CALL TARGETS: `b/bl/blx 0x<va>` to a named function gets `; -> <name>`.

Writes <out>/decompiled_named.c and <out>/disassembly_named.asm (never edits the
raw export). Static only.

Usage:
  annotate-preboot.py <export_dir> <payload.bin> <base_hex> [--names curated.tsv]
"""

from __future__ import annotations

import argparse
import re
import sys
from collections import Counter
from pathlib import Path

FUNC_STR = re.compile(r"\bs_[A-Za-z0-9_]*?_([0-9a-f]{8})\b")
IDENT = re.compile(r"^[a-z_][a-z0-9_]*$")
FMT_STRINGS = {"vectors", "status", "off", "on"}
DEF_LINE = re.compile(r"^[A-Za-z_].*\bFUN_([0-9a-f]{6,8})\s*\(")
HDR = re.compile(r"^; ==== (FUN_[0-9a-f]+) @ ([0-9a-f]+) ====")
POOL = re.compile(r"\[0x([0-9a-f]{6,8})\]")
CALL = re.compile(r"\b(bl|blx|b|bx)\s+0x([0-9a-f]{6,8})\b")


def read_cstr(data: bytes, base: int, va: int, maxlen: int = 96) -> str:
    off = va - base
    if off < 0 or off >= len(data):
        return ""
    chunk = data[off : off + maxlen]
    end = chunk.find(b"\x00")
    if end != -1:
        chunk = chunk[:end]
    try:
        s = chunk.decode("ascii")
    except UnicodeDecodeError:
        return ""
    return s if all(32 <= ord(c) < 127 for c in s) else ""


def auto_names(cpath: Path, data: bytes, base: int) -> dict[str, str]:
    lines = cpath.read_text(errors="replace").splitlines()
    defs = []
    for i, ln in enumerate(lines):
        m = DEF_LINE.match(ln)
        if m:
            defs.append((i, m.group(1)))
    defs.append((len(lines), None))
    out = {}
    for k in range(len(defs) - 1):
        start, va = defs[k]
        body = "\n".join(lines[start : defs[k + 1][0]])
        names = Counter()
        for shex in FUNC_STR.findall(body):
            s = read_cstr(data, base, int(shex, 16))
            if s and IDENT.match(s) and s not in FMT_STRINGS:
                names[s] += 1
        if names:
            out[va.zfill(8)] = names.most_common(1)[0][0]
    return out


def word_le(data: bytes, base: int, va: int):
    off = va - base
    if 0 <= off <= len(data) - 4:
        return int.from_bytes(data[off : off + 4], "little")
    return None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("export_dir", type=Path)
    ap.add_argument("payload", type=Path)
    ap.add_argument("base", type=lambda s: int(s, 0))
    ap.add_argument("--names", type=Path, default=None, help="curated VA<TAB>name TSV")
    ap.add_argument(
        "--dump-sym",
        type=Path,
        default=None,
        help="also write the merged name map as name<TAB>0xADDR .sym",
    )
    a = ap.parse_args()
    data = a.payload.read_bytes()
    cpath = a.export_dir / "decompiled.c"
    apath = a.export_dir / "disassembly.asm"

    names = auto_names(cpath, data, a.base)  # va(8hex)->name
    n_auto = len(names)
    if a.names and a.names.exists():
        for ln in a.names.read_text().splitlines():
            ln = ln.strip()
            if not ln or ln.startswith("#"):
                continue
            c0, c1 = re.split(r"\s+", ln, maxsplit=1)
            # accept either "0xADDR<TAB>name" or "name<TAB>0xADDR"
            if re.fullmatch(r"0x[0-9a-fA-F]+", c0):
                va, nm = c0, c1
            else:
                va, nm = c1, c0
            names[va.lower().replace("0x", "").zfill(8)] = nm.strip()

    def name_of(vahex: str):
        return names.get(vahex.zfill(8))

    # --- rewrite disassembly ---
    out_asm = []
    for ln in apath.read_text(errors="replace").splitlines():
        m = HDR.match(ln)
        if m:
            va = m.group(2).zfill(8)
            nm = name_of(va)
            out_asm.append(
                f"; ==== {nm or m.group(1)} @ {m.group(2)} ===="
                + (f"   [{m.group(1)}]" if nm else "")
            )
            continue
        comment = []
        cm = CALL.search(ln)
        if cm:
            tn = name_of(cm.group(2))
            if tn:
                comment.append(f"-> {tn}")
        for ph in POOL.findall(ln):
            w = word_le(data, a.base, int(ph, 16))
            if w is None:
                continue
            s = read_cstr(data, a.base, w)
            if s and len(s) >= 3:
                comment.append(f'[0x{ph}]=0x{w:08x} "{s}"')
        out_asm.append(ln + ("   ; " + " ; ".join(comment) if comment else ""))
    (a.export_dir / "disassembly_named.asm").write_text("\n".join(out_asm) + "\n")

    # --- rewrite decompiled C: FUN_<va>( -> <name>( ---
    ctext = cpath.read_text(errors="replace")

    def repl(m):
        nm = name_of(m.group(1))
        return (nm + "(") if nm else m.group(0)

    ctext = re.sub(r"\bFUN_([0-9a-f]{6,8})\s*\(", repl, ctext)
    (a.export_dir / "decompiled_named.c").write_text(ctext)

    if a.dump_sym:
        with a.dump_sym.open("w") as f:
            f.write("# preboot function names -> ApplyAlRegs .sym (name<TAB>0xADDR)\n")
            for va in sorted(names):
                f.write(f"{names[va]}\t0x{va}\n")

    print(
        f"named {len(names)} functions ({n_auto} auto __func__, "
        f"{len(names) - n_auto} curated); wrote *_named.{{asm,c}} to {a.export_dir}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
