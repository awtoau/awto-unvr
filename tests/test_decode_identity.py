# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""decode-identity.py --image / dump discovery.

The dump directory used to be a hardcoded path whose NAME carried the box's
MAC - the one allowlisted MAC literal outside _box.py. It is now the newest
images/mtd/UNVR-*-sysidea16-* by name, or --image.

Run: pytest tests/test_decode_identity.py -q
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


def _mod():
    spec = importlib.util.spec_from_file_location(
        "decode_identity", REPO / "scripts/decode-identity.py"
    )
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def test_newest_dump_by_name_wins(monkeypatch, tmp_path):
    m = _mod()
    for name in (
        "UNVR-aabbccddeeff-sysidea16-20260815-164103",
        "UNVR-aabbccddeeff-sysidea16-20260815-212945-post-5.1.25-final",
        "UNVR-aabbccddeeff-sysidea16-20260815-180449-pre-1.4.9",
        "UNVR-aabbccddeeff-sysidbeef-20260901-000000",  # other sysid: ignored
    ):
        (tmp_path / "images" / "mtd" / name).mkdir(parents=True)
    monkeypatch.setattr(m, "REPO", str(tmp_path))
    assert Path(m.resolve_dump(None)).name.endswith("212945-post-5.1.25-final")


def test_explicit_image_wins(monkeypatch, tmp_path):
    m = _mod()
    monkeypatch.setattr(m, "REPO", str(tmp_path))
    assert m.resolve_dump("/somewhere/else") == "/somewhere/else"


def test_no_dump_is_a_clear_exit(monkeypatch, tmp_path):
    m = _mod()
    monkeypatch.setattr(m, "REPO", str(tmp_path))
    with pytest.raises(SystemExit, match="--image"):
        m.resolve_dump(None)
