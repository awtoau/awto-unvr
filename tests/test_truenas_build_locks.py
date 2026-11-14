# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""scripts/truenas-build.py stale git-lock cleanup (#356): only when no git runs in the container."""

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
    "truenas_build", REPO / "scripts" / "truenas-build.py"
)
tb = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(tb)


@pytest.fixture
def tree(tmp_path, monkeypatch):
    monkeypatch.setattr(tb, "SB", tmp_path)
    for src in ("middleware", "zfs"):
        g = tmp_path / "sources" / src / ".git"
        g.mkdir(parents=True)
        (g / "index.lock").write_text("")
        (g / "HEAD").write_text("ref")
    (tmp_path / "sources" / "zfs" / ".git" / "refs.lock.d").mkdir()  # not a *.lock file
    monkeypatch.setattr(tb, "log", lambda m: None)
    rm = []
    monkeypatch.setattr(
        tb.subprocess,
        "run",
        lambda argv, **kw: rm.append(argv) or type("R", (), {"returncode": 0})(),
    )
    return rm


def test_finds_only_git_dir_locks(tree):
    assert [p.parent.parent.name for p in tb.stale_git_locks()] == ["middleware", "zfs"]


@pytest.mark.parametrize("busy", [True, None])
def test_kept_when_git_running_or_unknown(tree, monkeypatch, busy):
    monkeypatch.setattr(tb, "container_git_running", lambda: busy)
    assert tb.clean_stale_git_locks() == 0
    assert tree == []


def test_removed_when_container_idle(tree, monkeypatch):
    monkeypatch.setattr(tb, "container_git_running", lambda: False)
    assert tb.clean_stale_git_locks() == 2
    assert tree[0][:3] == ["sudo", "rm", "-f"] and len(tree[0]) == 6


def test_no_locks_no_podman(tree, monkeypatch):
    for p in tb.stale_git_locks():
        p.unlink()
    monkeypatch.setattr(tb, "container_git_running", lambda: pytest.fail("called"))
    assert tb.clean_stale_git_locks() == 0
