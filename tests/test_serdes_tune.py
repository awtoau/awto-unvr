# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Run the serdes-tune Rust unit tests (encoding, verify-mismatch, bounce outcome) in the gate."""

import shutil
import subprocess
from pathlib import Path

import pytest

CRATE = Path(__file__).resolve().parent.parent / "tools" / "serdes-tune"


@pytest.mark.skipif(shutil.which("cargo") is None, reason="cargo not installed")
def test_serdes_tune_unit_tests():
    # Offline: the crate has zero dependencies. 120 s ~= 10x a cold debug build.
    r = subprocess.run(
        ["cargo", "test", "--offline", "--quiet"],
        cwd=CRATE,
        capture_output=True,
        text=True,
        timeout=120,
    )
    assert r.returncode == 0, r.stdout + r.stderr
