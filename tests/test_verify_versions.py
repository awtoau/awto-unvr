"""verify-versions banner parsing (#258).

The regexes are the fragile part: each anchors on text a stage prints
unconditionally, with the revision in an OPTIONAL group so an unstamped
binary still matches the banner (and reports UNSTAMPED) instead of looking
like it never booted.

The bug these exist to prevent, caught live while writing this: the kernel
pattern left `-rc1` to the optional revision group, so an unstamped
"7.3.0-rc1+" reported its revision as "rc1" and compared unequal - a STALE
that hid the real answer, and a false accusation of a stale deploy.

Every banner string below is real console output from this box, not
invented: see docs and tmp/logs/unvr-console.log.*.

Run: pytest tests/test_verify_versions.py -q
"""

from __future__ import annotations

import importlib.util
import os
import sys
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

_spec = importlib.util.spec_from_file_location(
    "verify_versions", REPO / "scripts" / "verify-versions.py"
)
vv = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(vv)


# (stage, console line, expected revision or None for "banner but no rev")
STAMPED = [
    (
        "uboot",
        "U-Boot 2026.07-dirty (Sep 05 2026 - 08:54:47 +1000) awto-5f56952-dirty",
        "5f56952-dirty",
    ),
    (
        "uefi",
        "UEFI firmware (version UNVR EDK2 P1 awto-5f56952-dirty "
        "built at 00:38:19 on Sep  4 2026)",
        "5f56952-dirty",
    ),
    (
        "kernel",
        "[    0.000000] Linux version 7.3.0-rc1-5f56952-dirty+ (dan@selwyn) (gcc)",
        "5f56952-dirty",
    ),
    # A clean (non-dirty) tree, and a tag-shaped describe.
    (
        "uboot",
        "U-Boot 2026.07 (Sep 05 2026 - 08:54:47 +1000) awto-v2026.10-rc3",
        "v2026.10-rc3",
    ),
    (
        "kernel",
        "[    0.000000] Linux version 7.3.0-rc1-abc1234+ (dan@selwyn) (gcc)",
        "abc1234",
    ),
    # KASAN: -kasan precedes the sha and must not be reported as part of it,
    # or a KASAN build of the right commit reads as STALE.
    (
        "kernel",
        "[    0.000000] Linux version 7.3.0-rc1-kasan-abc1234-dirty+ (dan@x) (gcc)",
        "abc1234-dirty",
    ),
    # A released kernel, no -rcN at all.
    (
        "kernel",
        "[    0.000000] Linux version 7.3.0-abc1234+ (dan@selwyn) (gcc)",
        "abc1234",
    ),
]

# Real pre-#258 banners: the stage booted, but carries no revision.
UNSTAMPED = [
    ("uboot", "U-Boot 2026.07-dirty (Sep 04 2026 - 23:03:56 +1000)"),
    ("uefi", "UEFI firmware (version UNVR EDK2 P1 built at 00:38:19 on Sep  4 2026)"),
    ("kernel", "[    0.000000] Linux version 7.3.0-rc1+ (dan@selwyn) (gcc)"),
]


@pytest.mark.parametrize(("stage", "line", "want"), STAMPED)
def test_revision_extracted(stage: str, line: str, want: str) -> None:
    rev, banner = vv.scan(line + "\n", stage)
    assert rev == want, f"{stage}: parsed {rev!r} from {line!r}"
    assert banner is not None


@pytest.mark.parametrize(("stage", "line"), UNSTAMPED)
def test_unstamped_matches_banner_but_no_revision(stage: str, line: str) -> None:
    """The banner must still match - an unstamped stage is UNSTAMPED, not
    MISSING, and must never be reported as a revision mismatch."""
    rev, banner = vv.scan(line + "\n", stage)
    assert banner is not None, f"{stage}: banner not matched in {line!r}"
    assert rev is None, f"{stage}: invented revision {rev!r} from an unstamped banner"


def test_absent_stage_is_missing() -> None:
    for stage in vv.STAGES:
        assert vv.scan("nothing relevant here\n", stage) == (None, None)


def test_last_occurrence_wins() -> None:
    """The tail spans several boots; the newest is the box's current state."""
    text = (
        "U-Boot 2026.07 (Sep 01 2026 - 01:01:01 +1000) awto-old1234\n"
        "U-Boot 2026.07 (Sep 05 2026 - 08:54:47 +1000) awto-new5678\n"
    )
    assert vv.scan(text, "uboot")[0] == "new5678"


def test_stock_uboot_banner_is_not_ours() -> None:
    """Stock's 2015 U-Boot prints on every boot before ours. It has a
    ", Build: ..." suffix and no awto- ident; it must not be mistaken for a
    stamped revision."""
    line = (
        "U-Boot 2015.07-alpine_db-2.21-HAL (Dec 16 2020 - 05:54:51 +0800), "
        "Build: jenkins-amaz-alpinev2-boot-master-161"
    )
    assert vv.scan(line + "\n", "uboot")[0] is None


def test_every_stage_has_a_rev_group() -> None:
    for stage, (pattern, _) in vv.STAGES.items():
        assert "rev" in pattern.groupindex, f"{stage} pattern has no `rev` group"
