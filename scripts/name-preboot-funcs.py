#!/usr/bin/env python3
"""Map preboot FUN_<va> -> FULL vendor name via referenced __func__ strings.

The Annapurna/contractor preboot leaves __func__ in .rodata; log calls
(FUN_0100ceb0 / FUN_01010c00) pass the enclosing function's __func__ as an arg,
rendered by Ghidra as `s_<name>_<rodata_va>` (+optional offset). The Ghidra
label truncates the name at ~32 chars, so we do NOT trust the label text: we
take the rodata VA out of the label, read the *actual* NUL-terminated string
from the carved payload (VA base 0x01000000), and keep only clean C identifiers
(a real __func__; message strings contain spaces/caps/punctuation).

Per function body: the __func__ is the identifier string that recurs across its
log sites (highest hit count). Shared format strings ("vectors","status") and
message fragments are rejected by the identifier filter.

Static analysis only; no device access.
  in : docs/nor-reference/preboot-alboot-decompiled.c + tmp/alboot-payload.bin
  out: tmp/logs/name-preboot-funcs.log (full per-fn detail) + TSV to stdout

tmp/alboot-payload.bin (regenerable, gitignored) = the al_boot payload carved as
container[0x21000 : 0x21000+0x6a6b4] (435,892 B, VA base 0x01000000) — see
docs/preboot-decompile.md §Targets. The durable result is the curated map in that
doc's "Function name map" section, not this tmp log.
"""
import re
import sys
from collections import Counter
from pathlib import Path

SRC = Path("docs/nor-reference/preboot-alboot-decompiled.c")
BIN = Path("tmp/alboot-payload.bin")
VA_BASE = 0x01000000
LOG = Path("tmp/logs/name-preboot-funcs.log")

DEF_LINE = re.compile(r"^[A-Za-z_].*\bFUN_([0-9a-f]{8})\s*\(")
# s_<label>_<8hex-va> ; label is truncated, va is the string address (authoritative)
FUNC_STR = re.compile(r"\bs_[A-Za-z0-9_]*?_([0-9a-f]{8})\b")
IDENT = re.compile(r"^[a-z_][a-z0-9_]*$")  # a real __func__ reads as a C identifier
# shared .rodata format strings that also read as identifiers — never a func name
FMT_STRINGS = {"vectors", "status"}


def read_cstr(data: bytes, va: int, maxlen: int = 128) -> str:
    """NUL-terminated ASCII at a VA; skip a short leading run of prefix bytes
    (Ghidra sometimes anchors the label a few bytes before the text)."""
    off = va - VA_BASE
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
    return s.strip()


def main() -> int:
    for p in (SRC, BIN):
        if not p.exists():
            print(f"missing {p}", file=sys.stderr)
            return 1
    data = BIN.read_bytes()
    lines = SRC.read_text(errors="replace").splitlines()

    defs = []  # (start_idx, va)
    for i, ln in enumerate(lines):
        m = DEF_LINE.match(ln)
        if m:
            defs.append((i, m.group(1)))
    defs.append((len(lines), None))

    out = []
    for k in range(len(defs) - 1):
        start, va = defs[k]
        end = defs[k + 1][0]
        body = "\n".join(lines[start:end])
        names = Counter()
        for str_va_hex in FUNC_STR.findall(body):
            s = read_cstr(data, int(str_va_hex, 16))
            if s and IDENT.match(s) and s not in FMT_STRINGS:
                names[s] += 1
        if not names:
            continue
        name, hits = names.most_common(1)[0]
        out.append((va, name, hits, dict(names)))

    LOG.parent.mkdir(parents=True, exist_ok=True)
    with LOG.open("w") as f:
        f.write(f"scanned {len(defs)-1} function defs in {SRC}\n")
        f.write(f"named {len(out)} via full __func__ strings (from {BIN})\n\n")
        for va, name, hits, names in sorted(out):
            f.write(f"FUN_{va}\t{name}\t{hits}\t{names}\n")

    print(f"# named {len(out)} of {len(defs)-1} functions via full __func__")
    for va, name, hits, _ in sorted(out):
        print(f"0x{va}\t{name}\t{hits}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
