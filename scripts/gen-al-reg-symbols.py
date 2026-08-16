#!/usr/bin/env python3
"""Turn Annapurna HAL `struct al_<periph>_regs {}` headers into Ghidra inputs.

For a given peripheral base address (from the AL-324 MMIO map, see docs/ghidra.md
§3) this emits, from the C struct that describes the register block layout:

  1. a flat SYMBOLS file  <name>\t0x<abs_addr>   (one label per register)
     -> applied by scripts/ghidra/ApplyAlRegs.java (creates a label at each reg).
  2. a Ghidra C STRUCT header  struct <periph>_regs { ... at :0x<off> }
     -> parse via Ghidra "Parse C Source" + apply as a data type at the base for
        struct-member decompile (al_i2c->con). Optional; labels alone read fine.

Field offsets are computed from field types + array counts; `/* 0xNN */` anchor
comments in the header are cross-checked and a mismatch is logged (catches a
mis-sized field). reserved*/rsrvd* fields advance the offset but get no label.

The bitfield `#define AL_X_Y_SHIFT/_MASK` constants are handled by --defines:
only *directly numeric* defines become equates (shift amounts, plain masks);
defines built from other macros (`(0x1 << AL_..._SHIFT)`) are skipped - noted in
docs/ghidra.md §4. Equates applied by ApplyAlRegs.java --equates.

Why headers not SVD: no CMSIS-SVD ships for the AL-324 (app-class SoC); the HAL
register structs ARE the machine-readable register description. See docs/ghidra.md §4.

Usage:
  scripts/gen-al-reg-symbols.py HEADER --struct al_i2c_regs --base 0xfd880000 \
      --prefix i2c0 --out tmp/ghidra-in
  scripts/gen-al-reg-symbols.py HEADER --defines --prefix AL_I2C --out tmp/ghidra-in
"""
from __future__ import annotations

import argparse
import logging
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
LOG = REPO / "tmp" / "logs" / "gen-al-reg-symbols.log"

# C scalar sizes seen in the AL HAL register structs.
SIZES = {"uint8_t": 1, "u8": 1, "int8_t": 1, "char": 1,
         "uint16_t": 2, "u16": 2, "int16_t": 2,
         "uint32_t": 4, "u32": 4, "int32_t": 4, "unsigned": 4, "int": 4,
         "uint64_t": 8, "u64": 8, "int64_t": 8}

FIELD = re.compile(
    r"^\s*(?P<type>[A-Za-z_]\w*)\s+(?P<name>\w+)"
    r"(?:\s*\[\s*(?P<arr>0x[0-9a-fA-F]+|\d+)\s*\])?\s*;")
ANCHOR = re.compile(r"/\*\s*0x([0-9a-fA-F]+)\s*\*/")
DEFINE = re.compile(r"^\s*#define\s+(?P<name>\w+)\s+(?P<val>0x[0-9a-fA-F]+|\d+)\s*(?:/\*.*)?$")


def parse_struct(text: str, struct: str) -> list[tuple[str, int, int]]:
    """Return [(field_name, offset, size), ...] for the named struct.

    Cross-checks running offset against `/* 0xNN */` anchor comments; logs a
    warning on any mismatch (a wrong field size desyncs everything after it).
    """
    m = re.search(r"struct\s+" + re.escape(struct) + r"\s*\{", text)
    if not m:
        logging.error("struct %s not found", struct)
        return []
    body = text[m.end():]
    depth = 1
    off = 0
    out: list[tuple[str, int, int]] = []
    for line in body.splitlines():
        if "}" in line:
            depth -= 1
            if depth == 0:
                break
        a = ANCHOR.search(line)
        if a:
            want = int(a.group(1), 16)
            if want != off:
                logging.warning("anchor mismatch: comment 0x%x but computed 0x%x "
                                "(check field sizes above)", want, off)
                off = want  # resync to the header's own anchor
        fm = FIELD.match(line)
        if not fm:
            continue
        typ = fm.group("type")
        if typ in ("struct", "union", "const", "volatile"):
            continue
        sz = SIZES.get(typ)
        if sz is None:
            logging.warning("unknown type %r at off 0x%x -- skipping line", typ, off)
            continue
        count = int(fm.group("arr"), 0) if fm.group("arr") else 1
        name = fm.group("name")
        total = sz * count
        if not re.match(r"(reserved|rsrvd)", name):
            out.append((name, off, total))
        off += total
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("header", type=Path)
    ap.add_argument("--struct", help="struct name, e.g. al_i2c_regs")
    ap.add_argument("--base", type=lambda s: int(s, 0), help="peripheral base VA")
    ap.add_argument("--prefix", required=True, help="label/equate prefix, e.g. i2c0")
    ap.add_argument("--defines", action="store_true",
                    help="emit numeric #define equates instead of register labels")
    ap.add_argument("--out", type=Path, default=REPO / "tmp" / "ghidra-in")
    a = ap.parse_args()

    LOG.parent.mkdir(parents=True, exist_ok=True)
    a.out.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(level=logging.INFO, format="%(message)s",
                        handlers=[logging.FileHandler(LOG),
                                  logging.StreamHandler(sys.stdout)])
    text = a.header.read_text(errors="replace")

    if a.defines:
        pref = a.prefix
        eq = a.out / f"{pref}.equ.tsv"
        n = 0
        with eq.open("w") as f:
            for line in text.splitlines():
                dm = DEFINE.match(line)
                if dm and dm.group("name").startswith(pref):
                    f.write(f"{dm.group('name')}\t{int(dm.group('val'), 0)}\n")
                    n += 1
        logging.info("wrote %d numeric equates -> %s", n, eq)
        return 0

    if not a.struct or a.base is None:
        logging.error("register mode needs --struct and --base")
        return 2
    fields = parse_struct(text, a.struct)
    if not fields:
        return 1
    sym = a.out / f"{a.prefix}.sym"
    hdr = a.out / f"{a.prefix}.h"
    with sym.open("w") as f:
        for name, off, _ in fields:
            f.write(f"{a.prefix}_{name}\t0x{a.base + off:x}\n")
    with hdr.open("w") as f:
        f.write(f"struct {a.prefix}_regs {{\n")
        cur = 0
        for name, off, size in fields:
            if off > cur:
                f.write(f"  unsigned char _pad_{cur:x}[0x{off - cur:x}];\n")
            ctype = {1: "unsigned char", 2: "unsigned short",
                     4: "unsigned int", 8: "unsigned long long"}.get(size, None)
            if ctype and size in (1, 2, 4, 8):
                f.write(f"  volatile {ctype} {name}; /* 0x{off:x} */\n")
            else:
                f.write(f"  volatile unsigned char {name}[0x{size:x}]; /* 0x{off:x} */\n")
            cur = off + size
        f.write("};\n")
    logging.info("wrote %d register labels -> %s  (+ struct %s)",
                 len(fields), sym, hdr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
