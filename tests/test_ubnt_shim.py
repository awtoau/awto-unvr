# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""modules/ubnt_shim stays in step with the build list in scripts/_kbuild.py (#383)."""

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))

import _kbuild  # noqa: E402

SHIM = REPO / "modules" / _kbuild.SHIM_DIR


def test_every_shim_ko_has_source_and_kbuild_entry():
    makefile = (SHIM / "Makefile").read_text()
    objs = set(
        re.findall(r"(\w+)\.o\b", re.search(r"obj-m \+=(.*)", makefile).group(1))
    )
    assert objs == set(_kbuild.SHIM_KOS)
    for ko in _kbuild.SHIM_KOS:
        assert (SHIM / f"{ko}.c").is_file(), ko


def test_shim_headers_resolve():
    for h in SHIM.glob("*.h"):
        assert h.resolve().is_file(), h


def test_shims_not_in_shared_oot_list():
    # truenas (6.18) installs every OOT_MODULES entry; shims are 7.x opt-in.
    assert _kbuild.SHIM_DIR not in _kbuild.OOT_MODULES


def test_inventory_doc_covers_every_proc_file():
    doc = (REPO / "docs" / "unifi-shim-interface.md").read_text()
    src = (SHIM / "ubnthal.c").read_text()
    names = re.findall(r'\.name = "(\w+)"', src) + ["system.info", "board", "factory"]
    for n in names:
        assert f"`{n}`" in doc or f"`status/{n}`" in doc, n
