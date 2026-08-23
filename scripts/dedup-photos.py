#!/usr/bin/env python3
"""Find + remove duplicate / near-duplicate board photos in docs/photos/.

Groups images by (a) exact sha256 and (b) perceptual dHash (near-identical frames,
Hamming distance <= --thresh). Within each group keeps the LARGEST file (proxy for
sharpness/detail) and deletes the rest. Dry-run by default; --apply does `git rm`.

  ./scripts/dedup-photos.py                 # report only
  ./scripts/dedup-photos.py --apply         # git rm the redundant copies
  ./scripts/dedup-photos.py --thresh 4      # tighter/looser near-dup threshold
"""

from __future__ import annotations

import argparse
import hashlib
import subprocess
import sys
from pathlib import Path

from PIL import Image

DIR = Path(__file__).resolve().parent.parent / "docs" / "photos"


def dhash(path: Path, n: int = 8) -> int:
    im = Image.open(path).convert("L").resize((n + 1, n))
    bits = 0
    for r in range(n):
        for c in range(n):
            bits = (bits << 1) | (im.getpixel((c, r)) > im.getpixel((c + 1, r)))
    return bits


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--apply", action="store_true")
    ap.add_argument(
        "--thresh", type=int, default=5, help="max Hamming dist for near-dup"
    )
    a = ap.parse_args()

    jpgs = sorted(DIR.glob("*.jpg"))
    info = {
        p: (p.stat().st_size, hashlib.sha256(p.read_bytes()).hexdigest(), dhash(p))
        for p in jpgs
    }

    # union-find groups: exact sha256 OR dHash within thresh
    parent = {p: p for p in jpgs}

    def find(x):
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    def union(a_, b_):
        parent[find(a_)] = find(b_)

    for i, p in enumerate(jpgs):
        for q in jpgs[i + 1 :]:
            same = info[p][1] == info[q][1]
            near = (info[p][2] ^ info[q][2]).bit_count() <= a.thresh
            if same or near:
                union(p, q)

    groups: dict = {}
    for p in jpgs:
        groups.setdefault(find(p), []).append(p)

    to_delete, kept = [], 0
    for g in groups.values():
        if len(g) == 1:
            continue
        g.sort(key=lambda p: info[p][0], reverse=True)  # largest first
        keep, drop = g[0], g[1:]
        kept += 1
        exact = all(info[d][1] == info[keep][1] for d in drop)
        print(
            f"\ngroup ({'EXACT' if exact else 'near-dup'}): keep {keep.name} "
            f"({info[keep][0] // 1024}K)"
        )
        for d in drop:
            hd = (info[d][2] ^ info[keep][2]).bit_count()
            print(f"  drop {d.name} ({info[d][0] // 1024}K, hd={hd})")
            to_delete.append(d)

    print(
        f"\n=== {len(jpgs)} photos, {kept} dup-groups, {len(to_delete)} to delete ==="
    )
    if not to_delete:
        return 0
    if a.apply:
        subprocess.run(["git", "rm", "-q", *[str(d) for d in to_delete]], check=True)
        print("git rm done — commit to finalize.")
    else:
        print("dry-run; re-run with --apply to git rm the above.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
