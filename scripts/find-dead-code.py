#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Find unreferenced functions in modules/*/ by name reachability (#325).

Method (text-based, deliberately conservative: it can miss dead code, it must
not call live code dead):
  - ctags gives every function's span; an identifier inside a function body is
    a reference FROM that function.
  - an identifier in .c file scope (initialisers, `.probe = x`, EXPORT_SYMBOL,
    module_init) or in any #define body is a ROOT reference. Prototypes are
    excluded. Header file scope other than #define is ignored.
  - a name resolves to EVERY same-named definition in the module (static or
    not), so a name clash makes code look live, never dead.
  - liveness = reachable from the roots. Files absent from the Makefile are
    reported as orphans.

Reflection that a text scan cannot see: DT/PCI match tables are file-scope
initialisers (roots); sysfs/ethtool ops are initialisers (roots); anything
reached through a string or an asm label is not seen. Every "dead" result needs
a build after the move (build fails => it was live).

Usage:
    ./scripts/find-dead-code.py [module ...]    # default: every module
    ./scripts/find-dead-code.py --functions al_eth   # list dead functions
Log: tmp/logs/find-dead-code.log
"""

from __future__ import annotations

import argparse
import json
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

log = make_log("find-dead-code")

# an identifier after `.` or `->` is a field name, never a function/variable
IDENT = re.compile(r"(?<![\w.>])(?<!->)(?<!\.\s)(?<!->\s)[A-Za-z_]\w*")
COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)
STRING = re.compile(r'"(?:\\.|[^"\\])*"')


def strip(text: str) -> list[str]:
    """Blank comments and strings, keep line count."""

    def blank(m: re.Match) -> str:
        return re.sub(r"[^\n]", " ", m.group(0))

    return STRING.sub(blank, COMMENT.sub(blank, text)).split("\n")


def ctags(path: Path) -> list[dict]:
    out = subprocess.run(
        [
            "ctags",
            "--output-format=json",
            "--fields=+neSf",
            "--kinds-C=fpv",
            "-o",
            "-",
            str(path),
        ],
        capture_output=True,
        text=True,
        check=True,
    ).stdout
    return [json.loads(line) for line in out.splitlines() if line.startswith("{")]


def stmt_end(lines: list[str], start: int) -> int:
    """Last line of the file-scope statement starting at `start` (1-based):
    first ';' with no open (), {} or []. ctags gives no end for variables."""
    depth = 0
    for ln in range(start, len(lines) + 1):
        for ch in lines[ln - 1]:
            if ch in "({[":
                depth += 1
            elif ch in ")}]":
                depth -= 1
            elif ch == ";" and depth <= 0:
                return ln
    return start


def makefile_objs(mod: Path) -> set[str]:
    mk = (mod / "Makefile").read_text()
    return set(re.findall(r"([\w]+)\.o\b", mk))


def analyse(mod: Path):
    files = sorted(list(mod.glob("*.c")) + list(mod.glob("*.h")))
    funcs: dict[tuple[str, str], dict] = {}  # (file, name) -> info
    by_name: dict[str, list[tuple[str, str]]] = defaultdict(list)
    lines_of: dict[str, list[str]] = {}
    protos: dict[str, dict[int, str]] = defaultdict(dict)
    for f in files:
        lines = strip(f.read_text(errors="replace"))
        lines_of[f.name] = lines
        for t in ctags(f):
            if t["kind"] == "prototype":
                protos[f.name][t["line"]] = t["name"]
            if t["kind"] in ("function", "variable"):
                key = (f.name, t["name"])
                end = t.get("end") or stmt_end(lines, t["line"])
                funcs[key] = {
                    "start": t["line"],
                    "end": end,
                    "static": bool(t.get("file")),
                }
                by_name[t["name"]].append(key)

    # per file: line -> owning function, prototype lines
    owner: dict[str, dict[int, tuple[str, str]]] = defaultdict(dict)
    for (fn, name), info in funcs.items():
        for ln in range(info["start"], info["end"] + 1):
            owner[fn][ln] = (fn, name)

    def proto_name(fn: str, ln: int, lines: list[str]) -> str | None:
        """Name declared by the prototype statement covering line ln, if any."""
        for st, name in protos[fn].items():
            if st <= ln:
                j = st
                while j < len(lines) and ";" not in lines[j - 1]:
                    j += 1
                if ln <= j:
                    return name
        return None

    edges: dict[tuple[str, str], set[str]] = defaultdict(set)
    roots: set[str] = set()
    for f in files:
        lines = lines_of[f.name]
        in_define = False
        for i, line in enumerate(lines, 1):
            own = owner[f.name].get(i)
            is_define = in_define or bool(re.match(r"\s*#\s*define\b", line))
            in_define = is_define and line.rstrip().endswith("\\")
            idents = set(IDENT.findall(line))
            if own:
                edges[own] |= idents
            elif is_define:
                roots |= idents
            elif f.suffix == ".c":
                idents.discard(proto_name(f.name, i, lines))
                roots |= idents
    live: set[str] = set()
    work = [n for n in roots if n in by_name]
    live |= set(work)
    while work:
        n = work.pop()
        for key in by_name[n]:
            for ident in edges[key]:
                if ident in by_name and ident not in live:
                    live.add(ident)
                    work.append(ident)
    dead = {k: v for k, v in funcs.items() if k[1] not in live}
    return files, funcs, dead, lines_of


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("modules", nargs="*")
    ap.add_argument("--functions", action="store_true", help="list each dead function")
    args = ap.parse_args()

    mods = [REPO / "modules" / m for m in args.modules] or sorted(
        p for p in (REPO / "modules").iterdir() if p.is_dir()
    )
    for mod in mods:
        if not (mod / "Makefile").exists():
            continue  # shared-source dir, not a module
        objs = makefile_objs(mod)
        files, funcs, dead, lines_of = analyse(mod)
        log(f"== {mod.name}")
        for f in files:
            if f.suffix == ".c" and f.stem not in objs:
                log(
                    f"  ORPHAN (not in Makefile): {f.name} {len(lines_of[f.name])} lines"
                )
        per_file: dict[str, list] = defaultdict(list)
        for (fn, name), info in dead.items():
            per_file[fn].append((name, info))
        for fn in sorted(per_file):
            total = sum(1 for k in funcs if k[0] == fn)
            dl = sum(i["end"] - i["start"] + 1 for _, i in per_file[fn])
            log(
                f"  {fn}: {len(per_file[fn])}/{total} functions dead, "
                f"{dl} of {len(lines_of[fn])} lines"
            )
            if args.functions:
                for name, i in sorted(per_file[fn], key=lambda x: x[1]["start"]):
                    st = "static " if i["static"] else ""
                    log(f"      {i['start']}-{i['end']} {st}{name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
