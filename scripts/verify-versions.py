#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Is the box running what we just built? (#258)

Answers it for the whole boot chain at once by scanning the serial console
log for each stage's own boot banner and comparing the revision baked into
it against `git describe` for that stage's source tree.

Why the console log rather than querying each stage live: every stage
already prints its banner to the same UART on the way past, so ONE boot
yields the whole chain's identity in one place. Querying live would need
three unrelated mechanisms (ssh for Linux, the console prompt for U-Boot, a
UEFI shell for EDK2) and each only works while that stage is the one
running.

What each stage emits (all three verified on real console output):
  U-Boot  U-Boot 2026.07-dirty (Sep 05 2026 - 08:54:47 +1000) awto-<sha>
          common/version.c appends CONFIG_IDENT_STRING to version_string[];
          lib/display_options.c prints it as the boot banner.
  UEFI    UEFI firmware (version UNVR EDK2 P1 awto-<sha> built at ...)
          PcdFirmwareVersionString, set by --pcd at build time.
  kernel  Linux version 7.3.0-rc1-<sha>-dirty+ (dan@host) ...
          CONFIG_LOCALVERSION, set for every build.

Four outcomes per stage. Only one of them is a defect:
  OK        banner present, revision matches the working tree.
  STALE     banner present, revision DIFFERS. The real signal. Exit 1.
  UNSTAMPED banner present, carries no revision: built before #258, so it
            cannot have one. Informational - rebuild that stage to enable
            the check. Never STALE: that would be a false accusation.
  MISSING   not seen in this log. The log only holds what scrolled past
            since the last roll, so a stage that has not booted since then
            is absent through nobody's fault.

UNSTAMPED and MISSING both mean "cannot answer", not "wrong answer", so
neither fails on its own - only --require makes them fail, for a stage the
caller knows must be present.

--gate is report-only and always exits 0, modelled on
check-box-container-markers.py: the state of the box is never a reason to
block a commit of unrelated code, and the gate is shared.

Run: ./dev.py verify-versions            # all three, newest boot
     ./dev.py verify-versions --stage kernel
     ./dev.py verify-versions --log tmp/logs/unvr-console.log.1
     ./dev.py verify-versions --require kernel,uboot   # MISSING also fails
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
from _repo import LOGS, build_ident, make_log, rel  # noqa: E402

CONSOLE_LOG = LOGS / "unvr-console.log"

# Only the tail is scanned: the console log is a rolling multi-MB capture of
# every boot ever, and the question is what the box is running NOW. 2 MB is
# ~15x the ~130 KB a single cold boot to a Fedora login produces, so it
# always spans the last boot and usually several - enough to catch a stage
# from a boot or two back (UEFI is chainloaded ad hoc, not every boot)
# without matching something from last week.
TAIL_BYTES = 2 * 1024 * 1024

# The revision is stamped as "awto-<describe>" (U-Boot/UEFI) or appended
# bare into the kernel release. `git describe --always --dirty` yields a
# tag, a tag-N-gSHA, or a bare short SHA, optionally "-dirty".
_REV = r"[0-9A-Za-z][0-9A-Za-z._-]*"

# stage -> (banner regex with a named `rev` group, human note).
# Each regex anchors on text the stage prints unconditionally, so a stage
# that booted but was built WITHOUT its stamp still matches the banner and
# reports its rev as absent, rather than looking like it never ran.
STAGES = {
    "uboot": (
        re.compile(
            r"U-Boot \d{4}\.\d{2}[^\s(]*\s\([^)]*\)(?:\s+awto-(?P<rev>" + _REV + r"))?"
        ),
        "CONFIG_IDENT_STRING (common/version.c)",
    ),
    "uefi": (
        re.compile(
            r"UEFI firmware \(version UNVR EDK2 [^\s]+"
            r"(?:\s+awto-(?P<rev>" + _REV + r"))?"
        ),
        "PcdFirmwareVersionString (--pcd at build)",
    ),
    "kernel": (
        # LOCALVERSION lands between the upstream version and kbuild's
        # trailing "+", e.g. 7.3.0-rc1-5f56952-dirty+. The optional -kasan
        # sits BEFORE the sha (build-linux-fedora.py:localversion()).
        # -rcN is matched explicitly and NOT as part of `rev`: left to the
        # optional-group fallback it is itself a valid _REV, so an
        # unstamped "7.3.0-rc1+" reported its revision as "rc1" and
        # compared unequal - a STALE that hid the real answer (UNSTAMPED).
        re.compile(
            r"Linux version \d+\.\d+\.\d+(?:-rc\d+)?"
            r"(?:(?:-kasan)?-(?P<rev>" + _REV + r"))?\+?\s"
        ),
        "CONFIG_LOCALVERSION",
    ),
}


log = make_log("verify-versions", stamped=False)


def scan(text: str, stage: str) -> tuple[str | None, str | None]:
    """(revision, whole banner line) for the LAST occurrence of `stage`'s
    banner in `text`, or (None, None) if it never appears.

    Last, not first: the tail spans several boots and the newest one is the
    box's current state."""
    pattern, _ = STAGES[stage]
    last = None
    for m in pattern.finditer(text):
        last = m
    if last is None:
        return None, None
    return last.group("rev"), last.group(0).strip()


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--log",
        type=Path,
        default=CONSOLE_LOG,
        help=f"console log to scan (default {rel(CONSOLE_LOG)})",
    )
    ap.add_argument(
        "--stage",
        action="append",
        choices=sorted(STAGES),
        help="check only this stage (repeatable; default: all three)",
    )
    ap.add_argument(
        "--require",
        default="",
        help="comma-separated stages where a MISSING banner is also a "
        "failure (default: only a wrong revision fails)",
    )
    ap.add_argument(
        "--gate",
        action="store_true",
        help="report-only mode for ./dev.py gate: print the table and always "
        "exit 0 (an absent console log SKIPs entirely). The box's state must "
        "never block a commit of unrelated code",
    )
    a = ap.parse_args()

    stages = a.stage or list(STAGES)
    required = {s.strip() for s in a.require.split(",") if s.strip()}
    bad = required - set(STAGES)
    if bad:
        sys.exit(f"--require: unknown stage(s) {sorted(bad)}")

    try:
        with a.log.open("rb") as f:
            f.seek(0, os.SEEK_END)
            f.seek(max(0, f.tell() - TAIL_BYTES))
            text = f.read().decode("utf-8", "replace")
    except OSError as e:
        if a.gate:
            log(f"verify-versions: SKIP (no console log: {e})")
            return 0
        log(f"verify-versions: cannot read {rel(a.log)}: {e}")
        return 1

    rows = []
    stale = 0
    missing = 0
    unstamped = 0
    for stage in stages:
        expected = build_ident(stage)
        running, banner = scan(text, stage)
        if running is None and banner is None:
            status = "MISSING"
            missing += 1
            shown = "-"
        elif running is None:
            # Banner present but carries NO revision: built before #258
            # existed, so it cannot possibly carry a SHA. That is its own
            # status, never STALE - calling it "a different build" would be
            # a false accusation, and it is the normal state of every stage
            # until that stage is next rebuilt AND redeployed.
            status = "UNSTAMPED"
            unstamped += 1
            shown = "(none)"
        elif running == expected:
            status = "OK"
            shown = running
        else:
            status = "STALE"
            stale += 1
            shown = running
        rows.append((stage, expected, shown, status, banner))

    w = max(len(r[1]) for r in rows) if rows else 8
    log(f"console log: {rel(a.log)} (last {TAIL_BYTES // 1024} KB)")
    log(f"{'stage':<7} {'expected':<{w}} {'running':<{w}} status")
    for stage, expected, shown, status, _ in rows:
        log(f"{stage:<7} {expected:<{w}} {shown:<{w}} {status}")
    for stage, _, _, status, banner in rows:
        if banner and status != "OK":
            log(f"  {stage} banner: {banner}")

    if stale:
        log(
            "\nSTALE: the box is running a DIFFERENT build than the working "
            "tree. Rebuild and redeploy that stage before trusting any "
            "result from it (#105, #131, #161 were all this)."
        )
    if unstamped:
        names = ", ".join(r[0] for r in rows if r[3] == "UNSTAMPED")
        log(
            f"\nUNSTAMPED ({names}): this stage predates version stamping "
            "(#258), so it carries no revision to check. Rebuild and "
            "redeploy it to enable the check. Not a stale deploy."
        )
    if missing:
        names = ", ".join(r[0] for r in rows if r[3] == "MISSING")
        log(
            f"\nMISSING ({names}): not seen in this log. The console log "
            "only holds what has scrolled past since the last roll, so a "
            "stage that has not booted since then is absent through nobody's "
            "fault - and UEFI is chainloaded on demand, not every boot."
        )

    # --gate is report-only, like check-box-container-markers.py: the state
    # of the BOX is never a reason to block a commit of unrelated code, and
    # the gate is shared by everyone working in this repo. Non-zero is
    # reserved for an explicit `./dev.py verify-versions`, where a human
    # asked the question and wants a scriptable answer.
    if a.gate:
        return 0
    if stale:
        return 1
    # UNSTAMPED and MISSING are both "cannot answer", not "wrong answer", so
    # neither fails on its own. --require turns "cannot answer" into a
    # failure for a stage the caller KNOWS must be there - deploy-ssd
    # --reboot passes --require kernel, having just written that kernel.
    unknown = {r[0] for r in rows if r[3] in ("MISSING", "UNSTAMPED")}
    if required & unknown:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
