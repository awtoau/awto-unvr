#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Extract whole files out of a large vendor `diff -puNrb` patch.

Why: vendor GPL drops often arrive as one enormous patch against a stock kernel
(MikroTik's RouterOS drop is a 24 MB `linux-5.6.3.patch` adding 500+ Annapurna
HAL files). To compare their HAL against ours we need the resulting files, not
the diff, and applying the whole patch to a 5.6 tree just to read a few files is
not worth it.

Only handles files the patch ADDS in full (every hunk line is context-free `+`),
which is the normal case for vendored driver drops. A file the patch merely
modifies is reported as such rather than silently emitted half-formed.

Usage:
    ./scripts/extract-from-patch.py <patch> --match HAL/services/eth/ --out tmp/mt-hal
    ./scripts/extract-from-patch.py <patch> --list --match alpine
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# `+++ linux/drivers/soc/alpine/...` with an optional timestamp column.
PLUS_RE = re.compile(rb"^\+\+\+ ([^\t\n]+)")
MINUS_RE = re.compile(rb"^--- ([^\t\n]+)")


def iter_files(patch: bytes):
    """Yield (path, body_lines, is_pure_add) for each file in the patch."""
    lines = patch.split(b"\n")
    i = 0
    n = len(lines)
    while i < n:
        m = MINUS_RE.match(lines[i])
        if not m or i + 1 >= n:
            i += 1
            continue
        pm = PLUS_RE.match(lines[i + 1])
        if not pm:
            i += 1
            continue
        old = m.group(1)
        new = pm.group(1)
        i += 2
        body = []
        pure_add = True
        while i < n:
            ln = lines[i]
            if MINUS_RE.match(ln) and i + 1 < n and PLUS_RE.match(lines[i + 1]):
                break
            if ln.startswith(b"diff "):
                break
            if ln.startswith(b"@@"):
                i += 1
                continue
            if ln.startswith(b"+"):
                body.append(ln[1:])
            elif ln.startswith(b"-") or (ln and not ln.startswith(b"\\")):
                # A context or removal line means this is a modification, not a
                # clean add - the extracted text would be incomplete.
                if ln.startswith(b"-") or ln.startswith(b" "):
                    pure_add = False
            i += 1
        yield (
            new.decode("utf-8", "replace"),
            body,
            pure_add,
            old.decode("utf-8", "replace"),
        )


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("patch")
    ap.add_argument("--match", default="", help="only files whose path contains this")
    ap.add_argument("--out", default="", help="directory to write extracted files into")
    ap.add_argument("--list", action="store_true", help="list matches, extract nothing")
    args = ap.parse_args()

    data = Path(args.patch).read_bytes()
    written = skipped = 0
    for path, body, pure_add, old in iter_files(data):
        if args.match and args.match not in path:
            continue
        if args.list:
            kind = "add" if pure_add else "MODIFY"
            print(f"{kind:>7}  {len(body):>6} lines  {path}")
            continue
        if not pure_add:
            print(f"SKIP (modification, not a full add): {path}")
            skipped += 1
            continue
        if not args.out:
            print("--out is required unless --list is given")
            return 2
        # Strip the leading tree component ("linux/") so the layout is usable.
        rel = path.split("/", 1)[1] if "/" in path else path
        dest = Path(args.out) / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(b"\n".join(body) + b"\n")
        written += 1

    if not args.list:
        print(
            f"wrote {written} files to {args.out} ({skipped} skipped as modifications)"
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
