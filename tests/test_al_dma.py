# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""al_dma on the host: the real driver + real HAL against a fake UDMA engine.

modules/al_dma/al_dma_drv.c is #included into tests/c/al_dma_host.c and linked
with the unmodified HAL .c files. Fake: the kernel (tests/c/kshim), the two
plat headers, and the register file - whose doorbell and queue-reset writes
drive a small engine model that writes S2M completion descriptors into the
ring memory the driver programmed. Each case in al_dma_host.c is one pytest
item and prints PASS/FAIL lines.

Build dir: tmp/test-al-dma/ (not system /tmp). Needs a host cc.
"""

from __future__ import annotations

import re
import shutil
import subprocess
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
MOD = REPO / "modules" / "al_dma"
CDIR = REPO / "tests" / "c"
BUILD = REPO / "tmp" / "test-al-dma"

pytestmark = pytest.mark.skipif(
    shutil.which("cc") is None, reason="needs a host C compiler"
)

HAL_C = [
    "al_hal_ssm.c",
    "al_hal_ssm_raid.c",
    "al_hal_m2m_udma.c",
    "al_hal_udma_main.c",
    "al_hal_udma_config.c",
    "al_hal_udma_iofic.c",
    "al_hal_iofic.c",
]

CASES = [
    "smoke",
    "cookie_order_stress",
]


@pytest.fixture(scope="module")
def harness() -> Path:
    if BUILD.exists():
        shutil.rmtree(BUILD)
    (BUILD / "kshim").mkdir(parents=True)
    for f in MOD.glob("*.[ch]"):
        shutil.copy(f, BUILD / f.name)
    # the two plat headers are the only host-specific part of the HAL
    shutil.copy(CDIR / "host_dma_plat_types.h", BUILD / "al_hal_plat_types.h")
    shutil.copy(CDIR / "host_dma_plat_services.h", BUILD / "al_hal_plat_services.h")
    shutil.copy(CDIR / "kshim" / "kshim.h", BUILD / "kshim" / "kshim.h")
    shutil.copy(CDIR / "al_dma_host.c", BUILD / "harness.c")
    # every <linux/x.h> the driver names forwards to the shim
    names = set()
    for f in [BUILD / "al_dma_drv.c", BUILD / "al_dma_drv.h"]:
        names |= set(re.findall(r"#include <(linux/[\w./-]+)>", f.read_text()))
    for n in names:
        p = BUILD / "kshim" / n
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text('#include "kshim.h"\n')
    exe = BUILD / "harness"
    r = subprocess.run(
        [
            "cc",
            "-std=gnu11",
            "-O1",
            "-g",
            "-w",
            f"-I{BUILD}",
            f"-I{BUILD / 'kshim'}",
            str(BUILD / "harness.c"),
            *[str(BUILD / c) for c in HAL_C],
            "-lpthread",
            "-o",
            str(exe),
        ],
        capture_output=True,
        text=True,
    )
    assert r.returncode == 0, r.stderr
    return exe


@pytest.mark.parametrize("case", CASES)
def test_al_dma_host(harness: Path, case: str):
    r = subprocess.run([str(harness), case], capture_output=True, text=True)
    out = r.stdout + r.stderr
    assert "FAIL" not in r.stdout, out
    assert r.returncode == 0, out
    assert r.stdout.count("PASS") >= 1, out
