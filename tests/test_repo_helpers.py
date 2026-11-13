# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""_repo.make_log() and _repo.run() - the one logger and one subprocess wrapper.

24 scripts each defined a log() that printed a line and appended it to
tmp/logs/<script>.log (18 with an ISO-8601 stamp, 6 without); three build
scripts each defined a run() that logged "+ cmd" and called subprocess.run
with check=True. These pin the shared versions' contract.

Run: pytest tests/test_repo_helpers.py -q
"""

from __future__ import annotations

import os
import re
import subprocess
import sys
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

import _repo  # noqa: E402

STAMP = re.compile(r"^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}[+-]\d{2}:\d{2}  ")


def test_make_log_stamped_prints_and_appends(monkeypatch, tmp_path, capsys):
    monkeypatch.setattr(_repo, "LOGS", tmp_path)
    log = _repo.make_log("unit")
    log("hello")
    log("again")
    lines = (tmp_path / "unit.log").read_text().splitlines()
    assert len(lines) == 2 and lines[0].endswith("  hello")
    assert STAMP.match(lines[0]), "ISO 8601 with offset, the repo rule"
    assert capsys.readouterr().out.splitlines() == lines


def test_make_log_unstamped_is_verbatim(monkeypatch, tmp_path, capsys):
    monkeypatch.setattr(_repo, "LOGS", tmp_path)
    log = _repo.make_log("plain", stamped=False)
    log("DONE - kernel on the SSD")
    assert (tmp_path / "plain.log").read_text() == "DONE - kernel on the SSD\n"
    assert capsys.readouterr().out == "DONE - kernel on the SSD\n"


def test_run_logs_the_command_and_checks():
    seen = []
    r = _repo.run(["true"], log=seen.append)
    assert seen == ["+ true"] and r.returncode == 0
    with pytest.raises(subprocess.CalledProcessError):
        _repo.run(["false"], log=seen.append)
    r = _repo.run(["false"], log=seen.append, check=False)
    assert r.returncode == 1


def test_run_passes_kwargs_through():
    r = _repo.run(
        ["sh", "-c", "echo $AWTO_UNIT"],
        log=lambda m: None,
        env={"AWTO_UNIT": "ok", "PATH": os.environ["PATH"]},
        capture_output=True,
        text=True,
    )
    assert r.stdout.strip() == "ok"


@pytest.mark.parametrize(
    ("env", "want"),
    [
        ({}, ""),
        ({"AWTO_PERF_BUILD": "1"}, "perf"),
        ({"AWTO_MAINT_BUILD": "1"}, "maint"),
        ({"AWTO_KASAN_BUILD": "1"}, "kasan"),
    ],
)
def test_kernel_variant(monkeypatch, env, want):
    for k in ("AWTO_PERF_BUILD", "AWTO_MAINT_BUILD", "AWTO_KASAN_BUILD"):
        monkeypatch.delenv(k, raising=False)
    for k, v in env.items():
        monkeypatch.setenv(k, v)
    assert _repo.kernel_variant() == want
    monkeypatch.delenv("AWTO_KERNEL_OUT", raising=False)
    assert _repo.kernel_build_out().name.endswith(f"-{want}" if want else "fedora")


def test_kernel_variant_exclusive(monkeypatch):
    monkeypatch.setenv("AWTO_PERF_BUILD", "1")
    monkeypatch.setenv("AWTO_MAINT_BUILD", "1")
    with pytest.raises(SystemExit):
        _repo.kernel_variant()
