# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""TrueNAS 6.18 port: our board support as a GENERATED patch series (#338).

- Series = `git format-patch SERIES_BASE..SERIES_TIP` from our 7.x tree (kernel_src()).
- Applied onto TRUENAS_PIN in the store worktree REFS/truenas-unvr, branch BUILD_BRANCH,
  reset to the pin on every run (idempotent).
- 6.18-only deltas: patches/truenas-6.18/series.json
  - `replace`: subject -> 6.18 version of a generated patch. Keyed to the 7.x patch-id it
    was made from; a changed 7.x patch FATALs until the replacement is redone.
  - `after`: fixups applied after the series (genuine API breaks only).
"""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import tempfile
from pathlib import Path

from _repo import LINUX_STORE, REFS, REPO, TMP, kernel_src

TRUENAS_PIN = "b97fad085a83735f9c644aa6bc1e89c970d5114b"  # truenas/linux-6.18, 6.18.60
# OOT module sources (#338 phase 2), matched to the 26.04.0-MASTER-20251221 arm64 image:
# ZFS = truenas/zfs-2.4-release head at image build (merge of tag zfs-2.4.0, pkg 2.4.0-2).
# SCST = pkg version 3.10.0.1.ae15cad9cfef: truenas-3.10.0-pre + Joel May's arm64 debian/control.
ZFS_REPO = "https://github.com/truenas/zfs"
ZFS_PIN = "c5ee957a6e813053455cff0fd8001ff08d2882a9"
SCST_REPO = "https://github.com/joel0-truenas/scst"
SCST_PIN = "ae15cad9cfef801c22832e5c20124513eb45af7a"
SERIES_BASE, SERIES_TIP = "torvalds/master", "latest"
BUILD_TREE = "truenas-unvr"
BUILD_BRANCH = "awto/unvr/truenas-6.18"
FIXUP_DIR = REPO / "patches" / "truenas-6.18"
SERIES_JSON = FIXUP_DIR / "series.json"
SERIES_OUT = TMP / "truenas-series"
# Fixed committer + author-date as commit date -> same input, same branch SHAs.
COMMITTER = {
    "GIT_COMMITTER_NAME": "awto-unvr truenas-series",
    "GIT_COMMITTER_EMAIL": "truenas-series@awto-unvr.invalid",
}
# git ops are local: format-patch/apply of ~10 patches is sub-second, worktree add of a full
# kernel checkout the long pole (kernel-store.py CHECKOUT_TIMEOUT_S, #350). Expiry -> SeriesError.
GIT_TIMEOUT_S, CHECKOUT_TIMEOUT_S = 30, 60


class SeriesError(RuntimeError):
    pass


def build_tree() -> Path:
    return REFS / BUILD_TREE


def git(*args: str, cwd: Path, env=None, timeout: int = GIT_TIMEOUT_S, stdin=None):
    try:
        return subprocess.run(
            ["git", *args],
            cwd=cwd,
            env={**os.environ, **(env or {})},
            capture_output=True,
            text=True,
            timeout=timeout,
            stdin=stdin,
        )
    except subprocess.TimeoutExpired as e:
        raise SeriesError(f"git {' '.join(args)}: no result after {timeout} s") from e


def must(r: subprocess.CompletedProcess, what: str) -> str:
    if r.returncode:
        raise SeriesError(
            f"{what}: rc={r.returncode}: {(r.stderr or r.stdout).strip()}"
        )
    return r.stdout.strip()


def patch_id(patch: Path) -> str:
    with patch.open() as fh:
        out = must(
            git("patch-id", "--stable", cwd=LINUX_STORE, stdin=fh),
            f"patch-id {patch.name}",
        )
    return out.split()[0] if out else ""


def subject(patch: Path) -> str:
    """Subject line, folded continuation lines joined, [PATCH n/m] stripped."""
    lines = patch.read_text().splitlines()
    for i, line in enumerate(lines):
        if line.startswith("Subject: "):
            s = line[len("Subject: ") :]
            for cont in lines[i + 1 :]:
                if not cont.startswith(" "):
                    break
                s += cont
            return s.split("] ", 1)[1] if s.startswith("[PATCH") else s
    raise SeriesError(f"{patch.name}: no Subject line")


def generate() -> list[Path]:
    """format-patch SERIES_BASE..SERIES_TIP from kernel_src() into a clean SERIES_OUT."""
    src = kernel_src()
    if SERIES_OUT.exists():
        shutil.rmtree(SERIES_OUT)
    SERIES_OUT.mkdir(parents=True)
    must(
        git(
            "format-patch", "-q", "--zero-commit", "--no-signature",
            "-o", str(SERIES_OUT), f"{SERIES_BASE}..{SERIES_TIP}",
            cwd=src,
        ),
        f"format-patch in {src}",
    )  # fmt: skip
    patches = sorted(SERIES_OUT.glob("*.patch"))
    if not patches:
        raise SeriesError(f"{SERIES_BASE}..{SERIES_TIP} in {src} is empty")
    return patches


def plan() -> list[tuple[Path, str]]:
    """Ordered (patch, note) to apply onto the pin: generated series with replacements
    swapped in, then the `after` fixups. FATAL on a stale or orphaned registry entry."""
    reg = json.loads(SERIES_JSON.read_text())
    replace, used = reg.get("replace", {}), set()
    out = []
    for p in generate():
        subj = subject(p)
        r = replace.get(subj)
        if not r:
            out.append((p, "generated"))
            continue
        used.add(subj)
        pid = patch_id(p)
        if pid != r["from_patch_id"]:
            raise SeriesError(
                f"'{subj}' changed on {SERIES_TIP} (patch-id {pid[:12]}, replacement made from "
                f"{r['from_patch_id'][:12]}): redo {rel_fix(r['file'])} from the new patch, "
                "update from_patch_id"
            )
        out.append((FIXUP_DIR / r["file"], f"6.18 replacement: {r['reason']}"))
    orphans = set(replace) - used
    if orphans:
        raise SeriesError(
            f"series.json replaces patches no longer in the series: {orphans}"
        )
    out += [
        (FIXUP_DIR / a["file"], f"6.18 fixup: {a['reason']}")
        for a in reg.get("after", [])
    ]
    for p, _ in out:
        if not p.exists():
            raise SeriesError(f"{p} missing")
    return out


def rel_fix(name: str) -> str:
    return str((FIXUP_DIR / name).relative_to(REPO))


def check(log) -> None:
    """Gate: apply the plan to a scratch index of the pin. No worktree, no checkout."""
    steps = plan()
    with tempfile.TemporaryDirectory(dir=TMP) as d:
        env = {"GIT_INDEX_FILE": str(Path(d) / "index")}
        must(git("read-tree", TRUENAS_PIN, cwd=LINUX_STORE, env=env), "read-tree pin")
        for p, note in steps:
            r = git("apply", "--cached", "--3way", str(p), cwd=LINUX_STORE, env=env)
            if r.returncode:
                raise SeriesError(
                    f"{p.name} does not apply to {TRUENAS_PIN[:12]} ({note}):\n"
                    f"{(r.stderr or r.stdout).strip()}"
                )
            log(f"ok  {p.name}  [{note}]")
    log(f"series applies cleanly to {TRUENAS_PIN[:12]}: {len(steps)} patches")


def apply(log) -> Path:
    """Reset BUILD_BRANCH in the build tree to the pin, `git am -3` the plan. Returns the tree."""
    steps = plan()
    wt = build_tree()
    if not wt.exists():
        log(f"worktree add {wt} @ {TRUENAS_PIN[:12]} ({BUILD_BRANCH})")
        must(
            git(
                "worktree", "add", "-B", BUILD_BRANCH, str(wt), TRUENAS_PIN,
                cwd=LINUX_STORE, timeout=CHECKOUT_TIMEOUT_S,
            ),
            "worktree add",
        )  # fmt: skip
    git("am", "--abort", cwd=wt)  # leftover from a failed run; rc ignored
    must(git("reset", "-q", "--hard", cwd=wt, timeout=CHECKOUT_TIMEOUT_S), "reset")
    must(git("clean", "-fdq", cwd=wt, timeout=CHECKOUT_TIMEOUT_S), "clean")
    must(
        git(
            "checkout",
            "-q",
            "-B",
            BUILD_BRANCH,
            TRUENAS_PIN,
            cwd=wt,
            timeout=CHECKOUT_TIMEOUT_S,
        ),
        "checkout pin",
    )
    for p, note in steps:
        r = git(
            "am", "-3", "--committer-date-is-author-date", str(p), cwd=wt, env=COMMITTER
        )
        if r.returncode:
            conflicts = git(
                "diff", "--name-only", "--diff-filter=U", cwd=wt
            ).stdout.split()
            git("am", "--abort", cwd=wt)
            raise SeriesError(
                f"{p.name} conflicts on {TRUENAS_PIN[:12]} ({note}): {conflicts or r.stderr.strip()}"
            )
        log(f"applied  {p.name}  [{note}]")
    head = must(git("rev-parse", "--short=12", "HEAD", cwd=wt), "rev-parse")
    log(f"{wt}: {BUILD_BRANCH} = {TRUENAS_PIN[:12]} + {len(steps)} patches -> {head}")
    return wt
