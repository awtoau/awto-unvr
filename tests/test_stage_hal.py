# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""scripts/stage_hal.py - the staged HAL is derived, pristine, and uncommitted.

The point of #256 is that modules/al_eth/al_hal_* is the single source. These
pin the three properties that make a second copy impossible rather than merely
discouraged: byte-identical staging, a read-only staged tree (so an edit fails
at the edit), and no checked-in staged copy anywhere in the repo.

Run: pytest tests/test_stage_hal.py -q
"""

from __future__ import annotations

import filecmp
import os
import stat
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

import stage_hal  # noqa: E402


def test_stages_the_expected_file_set() -> None:
    """Shared HAL only: never the Linux driver beside it."""
    names = [p.name for p in stage_hal.hal_sources()]
    assert names, "no HAL sources found - the glob or the source dir moved"
    for n in names:
        assert n.startswith(("al_hal_", "al_init_", "al_serdes.")), n
    # al_serdes.h is shared HAL despite the name: al_init_eth_{kr,lm}.h include
    # it, and without it those two are the only files that fail to compile.
    assert "al_serdes.h" in names
    # The Linux driver and its kernel glue are host-specific, never shared.
    for host_only in (
        "al_eth_main.c",
        "al_eth_phylink.c",
        "kcompat.c",
        "kcompat.h",
        "Makefile",
    ):
        assert host_only not in names, f"{host_only} is host glue, not shared HAL"
    # Each host supplies its own platform shim, checked against the contract.
    assert "al_hal_plat_services.h" not in names


def test_staged_files_are_byte_identical(tmp_path: Path) -> None:
    """A staged file differing from the source means the copy was patched -
    which is the exact failure mode #256 exists to prevent."""
    dest = tmp_path / "staged"
    names = stage_hal.stage(dest)
    for n in names:
        assert filecmp.cmp(stage_hal.SOURCE / n, dest / n, shallow=False), (
            f"{n} differs from modules/al_eth/ - staged copies must be pristine"
        )


def test_staged_files_are_read_only(tmp_path: Path) -> None:
    """Read-only so an edit to the derived copy fails loudly at the edit,
    rather than silently vanishing at the next build."""
    dest = tmp_path / "staged"
    names = stage_hal.stage(dest)
    for n in names:
        mode = (dest / n).stat().st_mode
        assert not mode & stat.S_IWUSR, f"{n} is writable"


def test_restage_over_a_readonly_tree(tmp_path: Path) -> None:
    """Every build re-stages. Read-only files must not block the rewrite."""
    dest = tmp_path / "staged"
    first = stage_hal.stage(dest)
    second = stage_hal.stage(dest)
    assert first == second


def test_clean_removes_everything(tmp_path: Path) -> None:
    dest = tmp_path / "staged"
    n = len(stage_hal.stage(dest))
    assert stage_hal.clean(dest) == n
    assert not dest.exists()


def test_stale_staged_file_is_removed(tmp_path: Path) -> None:
    """A file dropped from the source must not linger in the staged tree -
    the bug unstage_stale() was added for, in the HAL's own staging path."""
    dest = tmp_path / "staged"
    stage_hal.stage(dest)
    stale = dest / "al_hal_gone_upstream.h"
    stale.write_text("/* removed from the source since the last build */\n")
    stage_hal.stage(dest)
    assert not stale.exists()


# Directories still holding a checked-in al_hal_* copy, each with the reason it
# is NOT staged from modules/al_eth/. Phase 1 (#256) covers the al_eth HAL only;
# an entry here is a scope statement, not a permanent exemption. Nothing may be
# added without a reason, and the al_eth HAL must never appear.
ALLOWLIST: dict[str, str] = {
    "uboot-port/board/annapurna/alpine/al_ddr": (
        "U-Boot-only: no modules/al_ddr exists. DDR training runs once in the "
        "bootloader; Linux never re-trains DRAM, so there is no Linux copy to "
        "be the single source. Not staged, not deleted."
    ),
    "uboot-port/drivers/phy/al_serdes": (
        "phase 2 (#256): modules/al_eth/ has al_hal_serdes_* but the copies "
        "have really diverged (al_hal_serdes_25g.c differs by 176 lines). "
        "Consolidating needs its own 10G/SerDes bring-up proof, not the 1G "
        "tftpboot gate this phase is measured by."
    ),
    "uboot-port/drivers/crypto/al_ssm": (
        "phase 2 (#256): modules/al_ssm/ holds the same basenames, diverged "
        "(al_hal_ssm_raid.c by 41 lines). Consolidating needs a crypto/RAID "
        "regression run of its own."
    ),
    "uboot-port/drivers/net/al_hal_shim": (
        "not a HAL copy: this is the U-Boot platform shim implementing "
        "hal/al_hal_plat_contract.h. Per-host by definition - the contract is "
        "what keeps the hosts honest, not a shared copy."
    ),
    "Platform/Ubiquiti/UNVR/Library/AlpineHalLib": (
        "UEFI phase (#256): EDK2's copy, taken from the U-Boot tree by "
        "df19434 because an EDK2 INF cannot reference sources outside its "
        "package. It gets re-based onto the staged Linux sources when the "
        "UEFI platform is rewritten; re-basing it against a tree that is "
        "itself being replaced would be done twice."
    ),
    "uboot-port/drivers/net/al_eth/hal": (
        "BLOCKED, not exempt (#256). This copy is meant to be deleted and is "
        "the point of phase 1 - but U-Boot's HAL is a NEWER vendor generation "
        "than Linux's, not the same code at another vintage: struct "
        "al_hal_eth_adapter has 30 members against Linux's 23, and 33 of its "
        "56 files have no Linux counterpart (7 of 12 compiled objects, incl. "
        "the whole al_hal_eth_mac* dispatch layer). Mixing them in one link is "
        "an ODR violation, not a build error. The existing glue does not "
        "compile against the Linux HAL - al_eth_dm.c wants dev_id, "
        "mac_common_regs, eth_common_regs_base, unit_adapter, common_mode. "
        "Deleting it therefore requires the alu_* glue rewrite, which is a "
        "later phase. Until then hal-drift-check.py keeps policing it."
    ),
}


def test_no_unallowlisted_hal_copy_is_checked_in() -> None:
    """No second checked-in copy of the shared HAL outside the allowlist.

    modules/al_eth/ is the source. A directory may only carry its own
    al_hal_* if ALLOWLIST states why - and the al_eth HAL never may."""
    tracked = subprocess.run(
        ["git", "ls-files", "uboot-port", "Platform"],
        cwd=REPO,
        capture_output=True,
        text=True,
        check=True,
        timeout=60,
    ).stdout.split()
    shared = {p.name for p in stage_hal.hal_sources()}
    offenders = [
        f
        for f in tracked
        if Path(f).name in shared and not any(f.startswith(a + "/") for a in ALLOWLIST)
    ]
    assert not offenders, (
        "checked-in copies of shared HAL files found - stage them instead, or "
        "add a justified ALLOWLIST entry:\n  " + "\n  ".join(sorted(offenders))
    )


def test_allowlist_has_no_dead_entries() -> None:
    """An allowlisted directory that no longer holds a HAL copy is stale -
    delete the entry so the list keeps meaning what it says."""
    tracked = subprocess.run(
        ["git", "ls-files", "uboot-port", "Platform"],
        cwd=REPO,
        capture_output=True,
        text=True,
        check=True,
        timeout=60,
    ).stdout.split()
    for a, reason in ALLOWLIST.items():
        assert reason.strip(), f"{a} allowlisted with no reason"
        assert any(f.startswith(a + "/") for f in tracked), (
            f"{a} is allowlisted but holds no checked-in HAL files - remove it"
        )


def test_al_eth_hal_entry_is_marked_blocked_not_exempt() -> None:
    """The al_eth HAL copy is the one this phase exists to delete.

    It is allowlisted only because the deletion is BLOCKED on the alu_* glue
    rewrite (U-Boot's HAL is a newer vendor generation; see the entry). That
    is a scope statement with an owner, not an exemption - so require the
    entry to keep saying so. A future edit that quietly downgrades it to a
    normal exemption fails here."""
    entries = [a for a in ALLOWLIST if "al_eth" in a]
    assert len(entries) == 1, "expected exactly one al_eth HAL allowlist entry"
    reason = ALLOWLIST[entries[0]]
    assert "BLOCKED" in reason, (
        "the al_eth HAL copy must stay marked BLOCKED - it is scheduled for "
        "deletion, not exempt from it"
    )
    assert "alu_" in reason, "the entry must name what unblocks it"
