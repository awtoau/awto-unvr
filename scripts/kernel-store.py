#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""kernel-store - every kernel tree here is a worktree of the machine Linux store (#350).

- Store: env UNVR_LINUX_STORE, else gitignored symlink tmp/linux-store.
- Trees: worktrees of the store in the refs dir: env UNVR_REFS, else gitignored symlink tmp/refs.
  The refs dir also holds the non-kernel reference sources (docs/reference-sources.md).
- Upstreams are store remotes (truenas, qnap, urnvr, torvalds, ...). Never clone a kernel, never
  --depth, never `git gc`/`repack` in the store: it is shared and a machine timer maintains it.
- ./dev.py kernel-store [show | checkout [TREE ...] | setup --store PATH --refs DIR]
- Log: tmp/logs/kernel-store.log
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from _repo import LINUX_STORE, REFS, TMP, make_log, rel

log = make_log("kernel-store")

# tree -> (rev, store remote that has it, role). A full SHA is checked out detached; a name is
# a local store branch (our series, never fetched).
TREES = {
    "linux-v7.3-fresh": (
        "latest",
        "torvalds",
        "build tree (AWTO_KERNEL_SRC); branch = our port series",
    ),
    "truenas-linux": (
        "580caa3a792d20aa5ce7b94b4742d0e30fe13940",
        "truenas",
        "TrueNAS truenas/linux-6.18 (#163)",
    ),
    "linux-qnap-tsx32x": (
        "6864002c76982f3af47757db564a09f99f91425a",
        "qnap",
        "delroth qnap-tsx32x, Alpine V2",
    ),
    "urnvr-kernel-4.19.152": (
        "badfd862c3134eff05becd54d2a48e028c77bfab",
        "urnvr",
        "UBNT UNVR GPL 4.19.152",
    ),
}
# fetch: one remote, worst case = refs-check.py's cold torvalds fetch, x1.25.
# checkout: worktree add of the largest tree, cold page cache (measured in #350).
# Expiry: logged with the limit, exit 1.
FETCH_TIMEOUT_S, CHECKOUT_TIMEOUT_S = 375, 60


def git(*args: str, cwd=None, timeout: int = 60, check: bool = True) -> str:
    cwd = cwd or LINUX_STORE
    try:
        r = subprocess.run(
            ["git", "-C", str(cwd), *args],
            capture_output=True,
            text=True,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired:
        log(f"git {' '.join(args)} in {rel(cwd)}: no result after {timeout} s (limit)")
        sys.exit(1)
    if check and r.returncode:
        log(f"git {' '.join(args)} in {rel(cwd)} -> {r.returncode}: {r.stderr.strip()}")
        sys.exit(1)
    return r.stdout.strip()


def store_ok() -> None:
    if not (LINUX_STORE / "MAINTAINERS").exists():
        sys.exit(
            f"{rel(LINUX_STORE)}: no Linux store; ./dev.py kernel-store setup --store PATH --refs DIR"
        )


def is_store_worktree(path: Path) -> bool:
    if not (path / ".git").exists():
        return False
    common = git(
        "rev-parse", "--path-format=absolute", "--git-common-dir", cwd=path, check=False
    )
    return bool(common) and common == git(
        "rev-parse", "--path-format=absolute", "--git-common-dir"
    )


def checkout(name: str) -> bool:
    rev, remote, _ = TREES[name]
    wt = REFS / name
    if wt.exists():
        if not is_store_worktree(wt):
            log(
                f"{name}: {rel(wt)} exists but is not a store worktree - retire it first (#350)"
            )
            return False
        return True
    if len(rev) == 40 and not git(
        "rev-parse", "--verify", "--quiet", f"{rev}^{{commit}}", check=False
    ):
        log(f"{name}: fetch {remote} for {rev}")
        git("fetch", remote, timeout=FETCH_TIMEOUT_S)
    git("worktree", "prune")
    detach = ["--detach"] if len(rev) == 40 else []
    log(f"{name}: worktree add {rel(wt)} @ {rev}")
    git("worktree", "add", *detach, str(wt), rev, timeout=CHECKOUT_TIMEOUT_S)
    return True


def show() -> int:
    bad = 0
    for name, (rev, remote, role) in TREES.items():
        wt = REFS / name
        if not is_store_worktree(wt):
            why = (
                "NOT A STORE WORKTREE (a clone? retire it)"
                if wt.exists()
                else "MISSING"
            )
            log(f"{name:24s} {why} ({role}); ./dev.py kernel-store checkout {name}")
            bad += 1
            continue
        head = git("rev-parse", "HEAD", cwd=wt)
        want = git("rev-parse", f"{rev}^{{commit}}")
        dirty = " dirty" if git("status", "--porcelain", "-uno", cwd=wt) else ""
        state = "ok" if head == want else f"HEAD != {rev[:12]}"
        bad += head != want
        log(f"{name:24s} {head[:12]} {state}{dirty}  [{remote}] {role}")
    return 1 if bad else 0


def link(sym: Path, target: str) -> None:
    dest = Path(target).expanduser().resolve()
    if sym.exists() and not sym.is_symlink():
        sys.exit(
            f"tmp/{sym.name} is a real directory, not a symlink; move it aside first"
        )
    sym.unlink(missing_ok=True)
    sym.symlink_to(dest)
    log(f"tmp/{sym.name} -> {dest}")


def setup(store: str, refs: str) -> None:
    st = Path(store).expanduser().resolve()
    shallow = subprocess.run(
        ["git", "-C", str(st), "rev-parse", "--is-shallow-repository"],
        capture_output=True,
        text=True,
    ).stdout.strip()
    if shallow != "false" or not (st / "MAINTAINERS").exists():
        sys.exit(f"{st}: not a full (non-shallow) Linux checkout")
    if not Path(refs).expanduser().is_dir():
        sys.exit(f"{refs}: not a directory")
    TMP.mkdir(exist_ok=True)
    link(TMP / "linux-store", store)
    link(TMP / "refs", refs)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument(
        "action", nargs="?", default="show", choices=["show", "checkout", "setup"]
    )
    ap.add_argument("trees", nargs="*", help="checkout: these trees (default: all)")
    ap.add_argument("--store", help="setup: the machine Linux store")
    ap.add_argument("--refs", help="setup: directory the reference trees live in")
    a = ap.parse_args()
    if a.action == "setup":
        if not (a.store and a.refs):
            sys.exit("setup needs --store PATH --refs DIR")
        setup(a.store, a.refs)
        return 0
    store_ok()
    if a.action == "checkout":
        unknown = set(a.trees) - set(TREES)
        if unknown:
            sys.exit(
                f"unknown tree(s): {', '.join(sorted(unknown))}; known: {', '.join(TREES)}"
            )
        if not all([checkout(n) for n in a.trees or TREES]):
            return 1
    return show()


if __name__ == "__main__":
    sys.exit(main())
