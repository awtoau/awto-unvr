#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Manifest of a woomera dataset tree (tank/home encryption migration): path, type, size, mode,
uid/gid, mtime, sha256, xattrs, ACL text. Dumped on the box, stored in ./tmp/home-encrypt/.

  home-manifest.py dump <label> [--root /mnt/tank/home]   # snapshot dirs work: <mnt>/.zfs/snapshot/<snap>
  home-manifest.py diff <labelA> <labelB>                 # exit 1 on any difference
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _box  # noqa: E402
from _repo import TMP, make_log  # noqa: E402

log = make_log("home-manifest")
OUT = TMP / "home-encrypt"
# Hashing ~10 MB plus a getfacl per node: <5 s expected; 120 s is the backstop for a wedged ssh.
DUMP_TIMEOUT_S = 120

REMOTE = r"""
import hashlib, json, os, subprocess, sys
root = sys.argv[1]
out = {}
def node(rel, p):
    st = os.lstat(p)
    e = {"mode": st.st_mode, "uid": st.st_uid, "gid": st.st_gid, "size": st.st_size,
         "mtime_ns": st.st_mtime_ns}
    if os.path.isfile(p) and not os.path.islink(p):
        h = hashlib.sha256()
        with open(p, "rb") as f:
            for b in iter(lambda: f.read(1 << 20), b""):
                h.update(b)
        e["sha256"] = h.hexdigest()
    elif os.path.islink(p):
        e["link"] = os.readlink(p)
    try:
        e["xattr"] = {k: os.getxattr(p, k, follow_symlinks=False).hex()
                      for k in os.listxattr(p, follow_symlinks=False)}
    except OSError as x:
        e["xattr_err"] = str(x)
    r = subprocess.run(["getfacl", "-p", "-n", "-c", p], capture_output=True, text=True)
    e["acl"] = r.stdout if r.returncode == 0 else "ERR " + r.stderr
    out[rel] = e
node(".", root)
for d, dirs, files in os.walk(root):
    for n in sorted(dirs + files):
        p = os.path.join(d, n)
        node(os.path.relpath(p, root), p)
json.dump(out, sys.stdout, sort_keys=True)
"""


def dump(label: str, root: str) -> dict:
    ip = _box.require()
    r = subprocess.run(
        _box.ssh_argv(ip, cmd=["python3", "-", root], batch=True),
        input=REMOTE,
        capture_output=True,
        text=True,
        timeout=DUMP_TIMEOUT_S,
    )
    if r.returncode:
        raise RuntimeError(f"dump {root}: {r.stderr[-300:]}")
    m = json.loads(r.stdout)
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT / f"{label}.json").write_text(json.dumps(m, indent=1, sort_keys=True))
    files = sum(1 for e in m.values() if "sha256" in e)
    log(f"manifest {label}: {len(m)} nodes, {files} files hashed, root {root}")
    return m


def diff(a: str, b: str) -> int:
    ma = json.loads((OUT / f"{a}.json").read_text())
    mb = json.loads((OUT / f"{b}.json").read_text())
    bad = 0
    for k in sorted(set(ma) | set(mb)):
        if k not in ma or k not in mb:
            log(f"DIFF {k}: only in {'B' if k not in ma else 'A'}")
            bad += 1
            continue
        # directory size/mtime change with dataset layout; files must match exactly
        skip = {"mtime_ns", "size"} if "sha256" not in ma[k] else set()
        for f in sorted(set(ma[k]) | set(mb[k]) - skip):
            if f not in skip and ma[k].get(f) != mb[k].get(f):
                log(f"DIFF {k}: {f}")
                bad += 1
    log(f"diff {a} vs {b}: {len(ma)} vs {len(mb)} nodes, {bad} differences")
    return 1 if bad else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("cmd", choices=["dump", "diff"])
    ap.add_argument("a")
    ap.add_argument("b", nargs="?")
    ap.add_argument("--root", default="/mnt/tank/home")
    a = ap.parse_args()
    if a.cmd == "diff":
        return diff(a.a, a.b)
    dump(a.a, a.root)
    return 0


if __name__ == "__main__":
    sys.exit(main())
