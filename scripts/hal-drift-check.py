#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Enforce the single-source rule for the Annapurna HAL (#256, #264, #218).

Model:
  - modules/al_hal/ is the single source for every HAL file shared by two or
    more Linux modules (al_eth, al_dma, al_ssm, al_nand, al_thermal).
  - A module consumes a shared file through a relative symlink
    modules/<m>/<name> -> ../al_hal/<name>. Never a copy.
  - A basename that exists as a REGULAR file in two or more trees (modules/al_hal
    counts as a tree) is a fork. It is allowed only if registered in
    scripts/hal-forks.json WITH A REASON.

Checks (all fail the gate):
  1. BAD SYMLINK           a module symlink that is not ../al_hal/<same name>, or dangles.
  2. ORPHAN SHARED FILE    a modules/al_hal/ file no module links to.
  3. UNREGISTERED FORK     a basename that is a regular file in 2+ trees (al_hal counts as a
                           tree), not in the registry. A copy of a shared file is one.
  4. STALE FORK ENTRY      a registered fork that is now a regular file in fewer than 2 trees.
  5. IDENTICAL FORK        a registered fork whose Linux copies are byte-identical: merge it
                           (U-Boot carriers may equal a module's copy; they cannot be linked).
  6. FORK EDITED           a registered fork whose content hash changed in some trees and not
                           others (names the trees that did NOT change: "does the same fix
                           apply there?"). Record with --update after answering.
  7. checked-in staged HAL (al_hal_shared/), checked-in HAL in the EDK2 package,
     hal/pcie-al-alpine-regs.h vs the Linux fork's constants.

The registry holds the reason each fork exists. --update refuses to record a
fork that has no reason, so a fork cannot reappear silently.

Usage:
    ./scripts/hal-drift-check.py                       # check
    ./scripts/hal-drift-check.py --update              # re-record hashes after an INTENDED fork edit
    ./scripts/hal-drift-check.py --fork NAME --reason "why"   # register a new fork
    ./scripts/hal-drift-check.py --show <basename>     # who has it, and do they match
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
REGISTRY = REPO / "scripts" / "hal-forks.json"

SHARED_DIR = "modules/al_hal"

# Linux modules that consume the shared tree.
LINUX_TREES: dict[str, str] = {
    "al_eth": "modules/al_eth",
    "al_dma": "modules/al_dma",
    "al_ssm": "modules/al_ssm",
    "al_nand": "modules/al_nand",
    "al_thermal": "modules/al_thermal",
}

# U-Boot carries HAL files of its own for al_serdes and al_ssm (#256 phase 2).
# They are fork carriers, not consumers of modules/al_hal.
OTHER_TREES: dict[str, list[str]] = {
    "uboot": [
        "uboot-port/drivers/phy/al_serdes",
        "uboot-port/drivers/crypto/al_ssm",
    ],
}

EDK2_HAL_DIR = "Platform/Ubiquiti/UNVR/Library/AlpineHalLib"
EDK2_SHIM_FILES = {"al_hal_plat_services.h", "al_hal_plat_types.h"}

# hal/pcie-al-alpine-regs.h is one file #include'd by U-Boot and EDK2. Compared
# by #define value against the Linux fork's canonical copy (file-header prose
# legitimately differs, so a byte hash would fire forever).
SHARED_HEADER = "hal/pcie-al-alpine-regs.h"
SHARED_HEADER_CANONICAL = Path(
    "/mnt/2tb/unvr-port-refs/linux-v7.3-fresh/drivers/pci/controller/pcie-al-alpine-regs.h"
)
SHARED_HEADER_CONSUMERS = [
    "uboot-port/board/annapurna/alpine/alpine.c",
    "Platform/Ubiquiti/UNVR/Drivers/AlPcieSnoopFixDxe/AlPcieSnoopFixDxe.c",
]

# scripts/stage_hal.py output. A tracked copy means a derived tree was checked in.
STAGED_DIR_NAME = "al_hal_shared"

SUFFIXES = (".c", ".h")

DEFINE_RE = re.compile(
    r"^\s*#\s*define\s+(\w+)(?:\([^)]*\))?\s+(.*?)\s*(?:/\*.*)?$", re.M
)


def git_ls(*paths: str) -> list[str]:
    return subprocess.run(
        ["git", "ls-files", *paths],
        cwd=REPO,
        capture_output=True,
        text=True,
        check=True,
        timeout=60,
    ).stdout.split()


def sha(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()[:16]


def hal_files(root: Path) -> list[Path]:
    return sorted(p for p in root.iterdir() if p.suffix in SUFFIXES)


# ---- shared tree ----------------------------------------------------------


def shared_names() -> set[str]:
    base = REPO / SHARED_DIR
    if not base.is_dir():
        return set()
    return {p.name for p in hal_files(base)}


def check_shared_tree() -> list[str]:
    """Checks 2-3: every module link is ../al_hal/<same name>, no orphan."""
    problems: list[str] = []
    shared = shared_names()
    linked: set[str] = set()
    shared_dir = REPO / SHARED_DIR

    for p in hal_files(shared_dir) if shared_dir.is_dir() else []:
        if p.is_symlink():
            problems.append(
                f"  {SHARED_DIR}/{p.name} is itself a symlink - it must be the source"
            )

    for root in LINUX_TREES.values():
        base = REPO / root
        if not base.is_dir():
            continue
        for p in hal_files(base):
            if not p.is_symlink():
                continue
            want = f"../al_hal/{p.name}"
            got = os.readlink(p)
            if got != want:
                problems.append(
                    f"  BAD SYMLINK {root}/{p.name} -> {got} (must be {want})"
                )
            elif not p.exists():
                problems.append(
                    f"  BAD SYMLINK {root}/{p.name} dangles: no {SHARED_DIR}/{p.name}"
                )
            else:
                linked.add(p.name)

    for name in sorted(shared - linked):
        problems.append(
            f"  ORPHAN SHARED FILE {SHARED_DIR}/{name}: no module links to it"
        )
    return problems


# ---- forks ----------------------------------------------------------------


def collect_forks() -> dict[str, dict[str, str]]:
    """basename -> {tree: hash} for REGULAR files present in 2+ trees."""
    seen: dict[str, dict[str, str]] = {}
    trees = (
        {"al_hal": [SHARED_DIR]}
        | {t: [r] for t, r in LINUX_TREES.items()}
        | OTHER_TREES
    )
    for tree, roots in trees.items():
        for root in roots:
            base = REPO / root
            if not base.is_dir():
                continue
            for p in base.rglob("*"):
                if p.suffix not in SUFFIXES or p.is_symlink() or not p.is_file():
                    continue
                seen.setdefault(p.name, {})[tree] = sha(p)
    return {k: v for k, v in seen.items() if len(v) > 1}


def load_registry() -> dict[str, dict]:
    if not REGISTRY.exists():
        return {}
    return json.loads(REGISTRY.read_text())


def check_forks(cur: dict[str, dict[str, str]], reg: dict[str, dict]) -> list[str]:
    problems: list[str] = []
    for name in sorted(set(cur) | set(reg)):
        n, entry = cur.get(name), reg.get(name)
        if entry is None:
            problems.append(
                f"  UNREGISTERED FORK {name}: regular file in {', '.join(sorted(n))}\n"
                f"      merge it into {SHARED_DIR}/ (one source, symlinks), or register it:\n"
                f'      ./scripts/hal-drift-check.py --fork {name} --reason "why it cannot be merged"'
            )
            continue
        if not entry.get("reason", "").strip():
            problems.append(f"  FORK WITHOUT REASON {name}: every fork must say why")
        if n is None:
            problems.append(
                f"  STALE FORK ENTRY {name}: no longer a regular file in 2+ trees "
                f"- remove it from {REGISTRY.name}"
            )
            continue
        # U-Boot carriers cannot be symlinked, so they may equal a module's copy.
        linux = {t: h for t, h in n.items() if t not in OTHER_TREES}
        if len(linux) > 1 and len(set(linux.values())) == 1:
            problems.append(
                f"  IDENTICAL FORK {name}: {', '.join(sorted(linux))} are byte-identical "
                f"- move it to {SHARED_DIR}/ and link it"
            )
            continue
        old = entry.get("trees", {})
        if old != n:
            changed = sorted(t for t in n if old.get(t) != n[t])
            untouched = sorted(t for t in n if t not in changed)
            problems.append(
                f"  FORK EDITED in {', '.join(changed)}: {name}\n"
                f"      NOT changed in: {', '.join(untouched) or '(none)'}"
                f" - does the same fix apply there?"
            )
    return problems


def write_registry(cur: dict[str, dict[str, str]], reg: dict[str, dict]) -> None:
    """Record hashes for REGISTERED forks only; an unregistered fork stays a failure."""
    out = {
        name: {"reason": reg[name]["reason"], "trees": dict(sorted(cur[name].items()))}
        for name in sorted(reg)
        if name in cur
    }
    REGISTRY.write_text(json.dumps(out, indent=1, ensure_ascii=False) + "\n")


# ---- unrelated host checks (unchanged by the single-source rule) ------------


def check_no_staged_copy() -> list[str]:
    offenders = [f for f in git_ls() if f"/{STAGED_DIR_NAME}/" in f]
    return [
        f"  CHECKED-IN STAGED HAL: {f}\n"
        f"      {STAGED_DIR_NAME}/ is a build artifact - stage it, do not commit it"
        for f in sorted(offenders)
    ]


def check_no_edk2_hal_copy() -> list[str]:
    offenders = [
        f
        for f in git_ls(EDK2_HAL_DIR)
        if Path(f).name.startswith(("al_hal_", "al_init_", "al_serdes"))
        and Path(f).name not in EDK2_SHIM_FILES
    ]
    return [
        f"  CHECKED-IN EDK2 HAL: {f}\n"
        "      AlpineHalLib builds the STAGED shared HAL - stage it, do not commit it"
        for f in sorted(offenders)
    ]


def defines(text: str) -> dict[str, str]:
    return {
        m.group(1): " ".join(m.group(2).split())
        for m in DEFINE_RE.finditer(text)
        if m.group(2).strip()
    }


def check_shared_header() -> list[str]:
    problems = []
    local = REPO / SHARED_HEADER

    if not local.is_file():
        return [f"  MISSING {SHARED_HEADER} - both U-Boot and EDK2 #include it"]

    for consumer in SHARED_HEADER_CONSUMERS:
        p = REPO / consumer
        if not p.is_file():
            problems.append(f"  MISSING consumer {consumer}")
        elif "pcie-al-alpine-regs.h" not in p.read_text():
            problems.append(
                f"  {consumer} no longer #includes pcie-al-alpine-regs.h"
                " - it has gone back to hand-typed constants"
            )

    if not SHARED_HEADER_CANONICAL.is_file():
        print(
            f"note: canonical copy absent ({SHARED_HEADER_CANONICAL}), value check skipped"
        )
        return problems

    ours = defines(local.read_text())
    theirs = defines(SHARED_HEADER_CANONICAL.read_text())
    for name in sorted(set(ours) | set(theirs)):
        o, t = ours.get(name), theirs.get(name)
        if o != t:
            problems.append(
                f"  VALUE DIFFERS  {name}: {SHARED_HEADER}={o!r} vs Linux fork={t!r}"
            )
    return problems


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--update",
        action="store_true",
        help="re-record fork hashes after an intended edit",
    )
    ap.add_argument(
        "--fork", metavar="NAME", help="register NAME as a fork (needs --reason)"
    )
    ap.add_argument(
        "--reason", help="why the fork cannot be merged into modules/al_hal"
    )
    ap.add_argument("--show", metavar="BASENAME", help="which trees carry this file")
    args = ap.parse_args()

    cur = collect_forks()
    reg = load_registry()

    if args.show:
        found = {
            t: (REPO / r / args.show)
            for t, r in ({"al_hal": SHARED_DIR} | LINUX_TREES).items()
            if (REPO / r / args.show).exists()
        }
        if not found:
            print(f"{args.show}: not in any Linux module")
            return 1
        for t, p in found.items():
            kind = f"-> {os.readlink(p)}" if p.is_symlink() else f"regular {sha(p)}"
            print(f"  {t:8s} {kind}")
        return 0

    if args.fork:
        if not (args.reason or "").strip():
            print("--fork needs --reason: say why it cannot be merged")
            return 1
        if args.fork not in cur:
            print(f"{args.fork}: not a regular file in 2+ trees - nothing to register")
            return 1
        reg[args.fork] = {"reason": args.reason.strip(), "trees": {}}
        write_registry(cur, reg)
        print(f"registered fork {args.fork}")
        return 0

    if args.update:
        unregistered = [n for n in cur if n not in reg]
        if unregistered:
            print("unregistered forks (merge them, or --fork NAME --reason ...):")
            print("\n".join(f"  {n}" for n in unregistered))
            return 1
        dropped = sorted(n for n in reg if n not in cur)
        write_registry(cur, reg)
        print(f"fork registry updated: {len(cur)} forks -> {REGISTRY.name}")
        if dropped:
            print(f"dropped (no longer forks): {', '.join(dropped)}")
        return 0

    problems = check_shared_tree() + check_forks(cur, reg)
    other = []
    shared_hdr = check_shared_header()
    if shared_hdr:
        other.append(
            f"SHARED-HEADER DRIFT: {len(shared_hdr)} problem(s) in {SHARED_HEADER}"
        )
        other += shared_hdr
    staged = check_no_staged_copy() + check_no_edk2_hal_copy()
    if staged:
        other.append(f"STAGED HAL COMMITTED: {len(staged)} file(s)")
        other += staged

    if problems or other:
        if problems:
            print(f"HAL single-source violations: {len(problems)}\n")
            print("\n".join(problems))
            print()
        if other:
            print("\n".join(other))
        return 1

    print(
        f"HAL drift: OK ({len(shared_names())} shared files in {SHARED_DIR}, "
        f"all linked, no copies; {len(cur)} registered forks unchanged; "
        f"{SHARED_HEADER} matches the Linux fork; no checked-in {STAGED_DIR_NAME}/)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
