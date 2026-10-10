# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""al_nand HAL data-buffer accounting, on the host (#208 defect 4).

al_nand_data_buff_write() once never stored cw_size_remaining/cw_count_remaining
back, so its "Not enough CW data" guard could not fire. This compiles the REAL
modules/al_nand/al_hal_nand.c with host stand-ins for the two plat headers and
drives it against fake registers.

Build dir: tmp/ (not system /tmp). Needs a host cc.
"""

from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
HAL = REPO / "modules" / "al_nand"
CDIR = REPO / "tests" / "c"
BUILD = REPO / "tmp" / "test-al-nand-hal"

pytestmark = pytest.mark.skipif(
    shutil.which("cc") is None, reason="needs a host C compiler"
)

GLOBALS_C = """
unsigned int host_fifo_writes, host_fifo_reads;
void *host_fifo_addr;
"""


def _build() -> Path:
    if BUILD.exists():
        shutil.rmtree(BUILD)
    BUILD.mkdir(parents=True)
    for f in HAL.glob("*.[ch]"):
        shutil.copy(f, BUILD / f.name)
    # the two plat headers are the only host-specific part of the HAL
    shutil.copy(CDIR / "host_plat_types.h", BUILD / "al_hal_plat_types.h")
    shutil.copy(CDIR / "host_plat_services.h", BUILD / "al_hal_plat_services.h")
    (BUILD / "globals.c").write_text(GLOBALS_C)
    shutil.copy(CDIR / "al_nand_hal_host.c", BUILD / "harness.c")
    exe = BUILD / "harness"
    r = subprocess.run(
        [
            "cc",
            "-std=gnu11",
            "-w",
            f"-I{BUILD}",
            str(BUILD / "harness.c"),
            str(BUILD / "globals.c"),
            str(BUILD / "al_hal_nand.c"),
            "-o",
            str(exe),
        ],
        capture_output=True,
        text=True,
    )
    assert r.returncode == 0, r.stderr
    return exe


def test_hal_cw_accounting():
    exe = _build()
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    assert "FAIL" not in r.stdout, r.stdout
    assert r.returncode == 0, r.stdout + r.stderr
    assert r.stdout.count("PASS") >= 8, r.stdout
