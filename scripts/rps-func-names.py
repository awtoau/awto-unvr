#!/usr/bin/env python3
"""Recover rpsd function names from its own log calls (the binary is stripped).

rpsd logs via `libubnt_log(level, tag, __func__, fmt, ...)`, so argument 3 of
every call is the *enclosing* function's real name. Ghidra renders those as
string literals in tmp/ghidra-out-rpsd/decompiled.c, giving a near-complete
symbol table for free.

Outputs (under tmp/rps-spec/):
  rpsd-names.tsv  ghidra_func, ghidra_vaddr, recovered_name(s), #log-calls
  rpsd-names.sym  name<TAB>0xVADDR  (Ghidra ApplyAlRegs .sym format)
  rpsd-named.c    decompiled.c with FUN_xxxx replaced by the recovered names

`--show NAME` prints the decompiled body of the named function.

Ghidra image base for this PIE import is 0x100000; the ELF file offset of a
function is (vaddr - 0x100000).
"""

from __future__ import annotations

import argparse
import logging
import re
import sys
from collections import Counter
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DEC = REPO / "tmp" / "ghidra-out-rpsd" / "decompiled.c"
OUTDIR = REPO / "tmp" / "rps-spec"
LOG = REPO / "tmp" / "logs" / "rps-func-names.log"
IMAGE_BASE = 0x100000

log = logging.getLogger(__name__)

BANNER = re.compile(r"^/\* @ 0x([0-9a-f]+) \*/\s*$")
SIG = re.compile(r"(FUN_[0-9a-f]+|[A-Za-z_][A-Za-z0-9_]*)\s*\(")
LOGCALL = re.compile(r'libubnt_log\s*\([^,]+,[^,]+,\s*"([A-Za-z_][A-Za-z0-9_]*)"')


def chunk(src: str):
    """Yield (vaddr:int|None, header_line:str, body:str) per exported function."""
    cur_addr, buf = None, []
    pend_addr = None
    for ln in src.splitlines():
        m = BANNER.match(ln)
        if m:
            if buf:
                yield cur_addr, "\n".join(buf)
            cur_addr = int(m.group(1), 16)
            buf = []
            del pend_addr
            pend_addr = None
            continue
        buf.append(ln)
    if buf:
        yield cur_addr, "\n".join(buf)


def func_name(body: str) -> str | None:
    for ln in body.splitlines():
        ln = ln.strip()
        if not ln or ln.startswith("//") or ln.startswith("/*"):
            continue
        m = SIG.search(ln)
        if m:
            return m.group(1)
    return None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--show", help="print the decompiled body of this function")
    a = ap.parse_args()
    LOG.parent.mkdir(parents=True, exist_ok=True)
    OUTDIR.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(
        level=logging.INFO,
        format="%(message)s",
        handlers=[logging.FileHandler(LOG), logging.StreamHandler(sys.stdout)],
    )

    src = DEC.read_text(errors="replace")
    rows = []
    rename: dict[str, str] = {}
    bodies: dict[str, str] = {}
    for vaddr, body in chunk(src):
        gname = func_name(body)
        if gname is None:
            continue
        bodies[gname] = body
        hits = Counter(LOGCALL.findall(body))
        if not hits:
            continue
        best, n = hits.most_common(1)[0]
        rows.append((gname, vaddr, sorted(hits), n))
        if gname.startswith("FUN_") and len(hits) == 1:
            rename[gname] = best

    tsv = OUTDIR / "rpsd-names.tsv"
    sym = OUTDIR / "rpsd-names.sym"
    with tsv.open("w") as ft, sym.open("w") as fs:
        ft.write("ghidra_func\tvaddr\tfile_off\trecovered\tn_logcalls\n")
        for gname, vaddr, cands, n in sorted(rows, key=lambda r: r[1] or 0):
            off = "" if vaddr is None else f"0x{vaddr - IMAGE_BASE:x}"
            va = "" if vaddr is None else f"0x{vaddr:x}"
            ft.write(f"{gname}\t{va}\t{off}\t{','.join(cands)}\t{n}\n")
            if gname in rename and vaddr is not None:
                fs.write(f"{rename[gname]}\t0x{vaddr:x}\n")
    log.info(
        "%d functions carry __func__ log calls; %d unambiguous renames -> %s",
        len(rows),
        len(rename),
        sym,
    )

    # emit a renamed copy of the decompiled C for reading
    out = src
    for old, new in sorted(rename.items(), key=lambda kv: -len(kv[0])):
        out = out.replace(old, new)
    (OUTDIR / "rpsd-named.c").write_text(out)
    log.info("wrote %s", OUTDIR / "rpsd-named.c")

    if a.show:
        target = a.show
        inv = {v: k for k, v in rename.items()}
        gname = inv.get(target, target)
        if gname in bodies:
            print(bodies[gname])
        else:
            log.error("no function %s (try a name from rpsd-names.tsv)", target)
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
