# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""scripts/hal-drift-check.py - the single-source rule fails when it is broken (#264).

Each test builds a throwaway module tree and requires the specific violation to
be named. The last test runs the rule over the real repo.

Run: pytest tests/test_hal_drift.py -q
"""

from __future__ import annotations

import importlib.util
import os
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

_spec = importlib.util.spec_from_file_location(
    "hal_drift_check", REPO / "scripts" / "hal-drift-check.py"
)
assert _spec and _spec.loader
hal = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(hal)


@pytest.fixture
def tree(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    """An empty repo with the five Linux module dirs and modules/al_hal."""
    monkeypatch.setattr(hal, "REPO", tmp_path)
    monkeypatch.setattr(hal, "REGISTRY", tmp_path / "scripts" / "hal-forks.json")
    (tmp_path / "scripts").mkdir()
    (tmp_path / hal.SHARED_DIR).mkdir(parents=True)
    for root in hal.LINUX_TREES.values():
        (tmp_path / root).mkdir(parents=True)
    return tmp_path


def put(tree: Path, rel: str, text: str) -> None:
    p = tree / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text)


def link(tree: Path, module: str, name: str) -> None:
    (tree / "modules" / module / name).symlink_to(f"../al_hal/{name}")


def problems(tree: Path, registry: dict | None = None) -> list[str]:
    reg = registry or {}
    return hal.check_shared_tree() + hal.check_forks(hal.collect_forks(), reg)


def test_linked_shared_file_is_clean(tree: Path) -> None:
    put(tree, "modules/al_hal/al_hal_a.h", "x\n")
    link(tree, "al_eth", "al_hal_a.h")
    link(tree, "al_dma", "al_hal_a.h")
    assert problems(tree) == []


def test_copy_of_a_shared_file_is_an_unregistered_fork(tree: Path) -> None:
    put(tree, "modules/al_hal/al_hal_a.h", "x\n")
    link(tree, "al_eth", "al_hal_a.h")
    put(tree, "modules/al_dma/al_hal_a.h", "x\n")
    out = "\n".join(problems(tree))
    assert "UNREGISTERED FORK al_hal_a.h" in out


def test_two_regular_copies_are_an_unregistered_fork(tree: Path) -> None:
    put(tree, "modules/al_eth/al_hal_b.c", "one\n")
    put(tree, "modules/al_dma/al_hal_b.c", "two\n")
    out = "\n".join(problems(tree))
    assert "UNREGISTERED FORK al_hal_b.c" in out
    assert "al_dma" in out and "al_eth" in out


def test_registered_fork_with_a_reason_is_clean(tree: Path) -> None:
    put(tree, "modules/al_eth/al_hal_b.c", "one\n")
    put(tree, "modules/al_dma/al_hal_b.c", "two\n")
    cur = hal.collect_forks()
    reg = {"al_hal_b.c": {"reason": "different generation", "trees": cur["al_hal_b.c"]}}
    assert hal.check_forks(cur, reg) == []


def test_fork_without_a_reason_fails(tree: Path) -> None:
    put(tree, "modules/al_eth/al_hal_b.c", "one\n")
    put(tree, "modules/al_dma/al_hal_b.c", "two\n")
    cur = hal.collect_forks()
    reg = {"al_hal_b.c": {"reason": "  ", "trees": cur["al_hal_b.c"]}}
    assert any("FORK WITHOUT REASON" in p for p in hal.check_forks(cur, reg))


def test_identical_registered_fork_must_be_merged(tree: Path) -> None:
    put(tree, "modules/al_eth/al_hal_b.c", "same\n")
    put(tree, "modules/al_dma/al_hal_b.c", "same\n")
    cur = hal.collect_forks()
    reg = {"al_hal_b.c": {"reason": "r", "trees": cur["al_hal_b.c"]}}
    assert any("IDENTICAL FORK" in p for p in hal.check_forks(cur, reg))


def test_identical_uboot_carrier_is_allowed(tree: Path) -> None:
    """uboot-port cannot symlink into modules/, so it may equal a module's copy."""
    put(tree, "modules/al_ssm/al_hal_b.c", "same\n")
    put(tree, "uboot-port/drivers/crypto/al_ssm/al_hal_b.c", "same\n")
    cur = hal.collect_forks()
    assert set(cur["al_hal_b.c"]) == {"al_ssm", "uboot"}
    reg = {"al_hal_b.c": {"reason": "r", "trees": cur["al_hal_b.c"]}}
    assert hal.check_forks(cur, reg) == []


def test_stale_fork_entry_fails(tree: Path) -> None:
    put(tree, "modules/al_eth/al_hal_b.c", "only one copy\n")
    reg = {"al_hal_b.c": {"reason": "r", "trees": {"al_eth": "x", "al_dma": "y"}}}
    assert any(
        "STALE FORK ENTRY" in p for p in hal.check_forks(hal.collect_forks(), reg)
    )


def test_editing_one_fork_copy_names_the_untouched_trees(tree: Path) -> None:
    put(tree, "modules/al_eth/al_hal_b.c", "one\n")
    put(tree, "modules/al_dma/al_hal_b.c", "two\n")
    cur = hal.collect_forks()
    reg = {"al_hal_b.c": {"reason": "r", "trees": cur["al_hal_b.c"]}}
    put(tree, "modules/al_eth/al_hal_b.c", "one, fixed\n")
    out = "\n".join(hal.check_forks(hal.collect_forks(), reg))
    assert "FORK EDITED in al_eth" in out
    assert "NOT changed in: al_dma" in out


def test_symlink_to_the_wrong_place_fails(tree: Path) -> None:
    put(tree, "modules/al_hal/al_hal_a.h", "x\n")
    put(tree, "modules/al_dma/other.h", "y\n")
    (tree / "modules/al_eth/al_hal_a.h").symlink_to("../al_dma/other.h")
    out = "\n".join(hal.check_shared_tree())
    assert "BAD SYMLINK modules/al_eth/al_hal_a.h" in out


def test_dangling_symlink_fails(tree: Path) -> None:
    link(tree, "al_eth", "al_hal_gone.h")
    assert any("dangles" in p for p in hal.check_shared_tree())


def test_orphan_shared_file_fails(tree: Path) -> None:
    put(tree, "modules/al_hal/al_hal_a.h", "x\n")
    assert any("ORPHAN SHARED FILE" in p for p in hal.check_shared_tree())


def test_the_real_repo_obeys_the_rule() -> None:
    """Same check as ./dev.py hal-drift, minus the git-based host checks."""
    reg = hal.load_registry()
    assert reg, "scripts/hal-forks.json is empty or missing"
    found = hal.check_shared_tree() + hal.check_forks(hal.collect_forks(), reg)
    assert found == [], "\n".join(found)
    for name, entry in reg.items():
        assert entry["reason"].strip(), f"{name}: fork with no reason"
