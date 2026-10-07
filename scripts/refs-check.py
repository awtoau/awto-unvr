#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Report how far each external reference tree is behind its upstream.

20 git trees under /mnt/2tb/unvr-port-refs/ supply our kernel base, U-Boot,
EDK2, vendor HALs and comparison sources (docs/reference-sources.md). Nothing
tracked whether any of them had moved, so "are we on the latest kernel" was
answered by hand each time.

Reports, per tree: current HEAD, its describe, the configured upstream, and
the commit delta each way. Fetches only with --fetch (network, minutes).

`ours` is what we carry on top of the upstream base - the port itself. A
rising `ours` on the kernel is expected; a rising `theirs` is the thing to
act on.

    ./scripts/refs-check.py                  # report from local refs
    ./scripts/refs-check.py --fetch          # fetch first, then report
    ./scripts/refs-check.py --fetch --only linux-v7.3-fresh
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

from _repo import make_log  # noqa: E402

log = make_log("refs-check")

REFS = Path("/mnt/2tb/unvr-port-refs")

# tree -> upstream ref to compare against. None = vendor snapshot, nothing to
# be behind. A "tag:<glob>" value means the tree was cloned at a tag and fetches
# no branches (edk2, u-boot): the question is "is there a newer tag", not a
# branch delta, so report newest matching tag instead of a commit count.
UPSTREAM = {
    "linux-v7.3-fresh": "torvalds/master",
    "edk2": "tag:edk2-stable*",
    "u-boot-v2026.07": "tag:v20*",
    "linux-alpine-v2": "origin/main",
    "imbushuo-ccr2004-uefi": "origin/master",
    "truenas-linux": "origin/master",
    # Old kernel bases kept for bisect/comparison. Pinned on purpose - being
    # "behind" is their job, so no upstream.
    "linux-6.12": None,
    "linux-v6.18": None,
    "linux-v6.18.44": None,
    "linux-v7.1.8": None,
    "linux-qnap-tsx32x": None,
    "delroth-alpine_hal": None,
    "delroth-al_eth-standalone": None,
    "mornepousse-al_eth-standalone": None,
    "mornepousse-al_thermal-standalone": None,
    "UBNT-source-code": None,
    "urnvr-kernel-4.19.152": None,
    "urnvr": None,
    "unifi-unvr-emu": None,
    "UNVR-NAS": None,
    "UNVR-diy-os": None,
}

# Fetch bound: a full kernel fetch over a home link is minutes, and a stall
# is indistinguishable from slow progress without one. 900s ~= 3x the longest
# observed (torvalds, ~5 min cold). On expiry: log the tree and carry on to
# the next, rather than failing the whole sweep.
FETCH_TIMEOUT_S = 900


def git(tree: Path, *args: str, timeout: int = 60) -> str:
    try:
        r = subprocess.run(
            ["git", "-C", str(tree), *args],
            capture_output=True,
            text=True,
            check=False,
            timeout=timeout,
        )
        return r.stdout.strip()
    except subprocess.TimeoutExpired:
        return ""


def count(tree: Path, rng: str) -> str:
    out = git(tree, "rev-list", "--count", rng)
    return out if out else "?"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--fetch", action="store_true", help="fetch upstreams first (slow)")
    ap.add_argument("--only", help="one tree name")
    a = ap.parse_args()

    # .git is a FILE in a worktree, a dir in a normal clone. Testing is_dir()
    # silently skipped linux-v7.3-fresh - the tree we actually build from.
    trees = sorted(p for p in REFS.iterdir() if (p / ".git").exists())
    if a.only:
        trees = [p for p in trees if p.name == a.only]
        if not trees:
            log(f"no such tree: {a.only}")
            return 2

    behind = []
    log(f"{'tree':30s} {'describe':26s} {'ours':>6s} {'theirs':>7s}  upstream")
    for t in trees:
        up = UPSTREAM.get(t.name, "unset")
        if up is None:
            log(
                f"{t.name:30s} {git(t, 'log', '-1', '--format=%h'):26s} {'-':>6s} {'-':>7s}  (vendor snapshot)"
            )
            continue
        if up == "unset":
            log(f"{t.name:30s} {'?':26s} {'?':>6s} {'?':>7s}  NOT IN UPSTREAM TABLE")
            continue

        if a.fetch:
            remote = "origin" if up.startswith("tag:") else up.split("/")[0]
            log(f"  fetching {t.name} {remote} ...")
            git(t, "fetch", "--tags", remote, timeout=FETCH_TIMEOUT_S)

        desc = git(t, "describe", "--tags", "HEAD") or git(
            t, "log", "-1", "--format=%h"
        )

        if up.startswith("tag:"):
            newest = git(t, "tag", "--list", up[4:], "--sort=-version:refname")
            newest = newest.splitlines()[0] if newest else "?"
            cur = git(t, "describe", "--tags", "--abbrev=0", "HEAD") or "?"
            state = "OK" if newest in (cur, "?") else f"NEWER TAG: {newest}"
            log(f"{t.name:30s} {desc[:26]:26s} {'tag':>6s} {'':>7s}  {state}")
            if state != "OK":
                behind.append((t.name, newest, cur))
            continue

        ours = count(t, f"{up}..HEAD")
        theirs = count(t, f"HEAD..{up}")
        log(f"{t.name:30s} {desc[:26]:26s} {ours:>6s} {theirs:>7s}  {up}")
        if theirs not in ("0", "?"):
            behind.append((t.name, theirs, desc))

    if behind:
        log("")
        log("BEHIND upstream:")
        for name, n, desc in behind:
            what = f"newer tag {n}" if not n.isdigit() else f"{n} commits"
            log(f"  {name}: {what} (on {desc})")
        log("")
        log("Kernel rebase: docs/kernel-rebase.md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
