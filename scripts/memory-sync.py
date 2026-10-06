#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Mirror ~/.claude agent memory into the repo it describes, per project.

Agent memory lives in ~/.claude/projects/<slug>/memory/*.md, outside any git
repo - hand-written, not regenerable, and lost with the machine. This copies
each project's memory into that project's own repo under docs/agent-memory/,
so it is versioned beside the code it describes and reviewed in the same PR.

Slug = the workspace abs path with '/' replaced by '-', which is ambiguous
when a directory name itself contains a hyphen (/mnt/2tb/git/awto-unvr and
/mnt/2tb/git/awto/unvr produce the same slug). Resolved by trying every
grouping and keeping the one that exists on disk, fewest path segments first.

    ./scripts/memory-sync.py                 # this repo only, dry run
    ./scripts/memory-sync.py --write         # this repo only, copy
    ./scripts/memory-sync.py --all           # report every project
    ./scripts/memory-sync.py --all --write   # copy into every project repo
"""

from __future__ import annotations

import argparse
import itertools
import os
import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

from _repo import make_log  # noqa: E402

log = make_log("memory-sync")

CLAUDE_PROJECTS = Path.home() / ".claude" / "projects"
DEST_SUBDIR = "docs/agent-memory"


def resolve_slug(slug: str) -> Path | None:
    """The workspace path a slug encodes, or None if it is not on disk."""
    parts = slug.strip("-").split("-")
    n = len(parts)
    for ndirs in range(1, n + 1):
        for cuts in itertools.combinations(range(1, n), ndirs - 1):
            segs, prev = [], 0
            for c in [*cuts, n]:
                segs.append("-".join(parts[prev:c]))
                prev = c
            cand = Path("/" + "/".join(segs))
            if cand.is_dir():
                return cand
    return None


def git_root(p: Path) -> Path | None:
    """The repo containing p, or None."""
    cur = p
    while cur != cur.parent:
        if (cur / ".git").is_dir():
            return cur
        cur = cur.parent
    return None


def sync(mem: Path, root: Path, write: bool) -> tuple[int, int]:
    """Copy *.md from mem into root/DEST_SUBDIR. Returns (copied, skipped)."""
    dest = root / DEST_SUBDIR
    copied = skipped = 0
    for src in sorted(mem.glob("*.md")):
        tgt = dest / src.name
        if tgt.exists() and tgt.read_bytes() == src.read_bytes():
            skipped += 1
            continue
        if write:
            dest.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, tgt)
        copied += 1
    return copied, skipped


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--all", action="store_true", help="every project, not just this repo"
    )
    ap.add_argument("--write", action="store_true", help="copy; default is a dry run")
    a = ap.parse_args()

    if not CLAUDE_PROJECTS.is_dir():
        log(f"no {CLAUDE_PROJECTS} - nothing to sync")
        return 0

    total_c = total_s = 0
    for mem in sorted(CLAUDE_PROJECTS.glob("*/memory")):
        if not list(mem.glob("*.md")):
            continue
        slug = mem.parent.name
        real = resolve_slug(slug)
        if real is None:
            if a.all:
                log(f"  SKIP {slug}: workspace no longer on disk")
            continue
        root = git_root(real)
        if root is None:
            if a.all:
                log(f"  SKIP {real}: not in a git repo")
            continue
        if not a.all and root != REPO:
            continue

        c, s = sync(mem, root, a.write)
        total_c += c
        total_s += s
        verb = "copied" if a.write else "would copy"
        log(f"  {root.name:24s} {verb} {c:3d}, unchanged {s:3d}  ({mem})")

    verb = "copied" if a.write else "would copy"
    log(f"{verb} {total_c}, unchanged {total_s}")
    if not a.write and total_c:
        log("dry run - pass --write to copy")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
