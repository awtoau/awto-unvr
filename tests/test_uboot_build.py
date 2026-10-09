# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""scripts/uboot-build.py - run() tees make output (#257); --clean rebuilds.

Run: pytest tests/test_uboot_build.py -q
"""

from __future__ import annotations

import importlib.util
import os
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")


def _load(tmp_path: Path):
    spec = importlib.util.spec_from_file_location(
        "uboot_build", REPO / "scripts" / "uboot-build.py"
    )
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    mod.TREE = str(tmp_path)
    mod.LOG = str(tmp_path / "build.log")
    return mod


def test_run_tees_output_and_relogs_tail_on_failure(tmp_path, capsys) -> None:
    ub = _load(tmp_path)
    rc = ub.run(["sh", "-c", "echo compiling foo.o; echo 'error: boom' >&2; exit 3"])
    assert rc == 3
    out = capsys.readouterr().out
    log = Path(ub.LOG).read_text()
    for text in (out, log):
        assert "compiling foo.o" in text
        assert "error: boom" in text
        assert "FAILED rc=3" in text


def test_run_kills_on_timeout(tmp_path) -> None:
    ub = _load(tmp_path)
    ub.BUILD_TIMEOUT_S = 0.3  # test-only: a silent hang must still expire
    assert ub.run(["sh", "-c", "exec sleep 5"]) == 124
    assert "TIMEOUT" in Path(ub.LOG).read_text()


def test_clean_flag_falls_through_to_build() -> None:
    src = (REPO / "scripts" / "uboot-build.py").read_text()
    main = src[src.index('if __name__ == "__main__":') :]
    assert "else:" not in main
    assert main.index("clean()") < main.index("build()")
