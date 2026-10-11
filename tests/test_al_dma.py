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
    "cookie_zero_skipped",
    "len_limits",
    "pq_cap_gate",
    "slot_held_through_callback",
    "error_status",
    "realloc",
    "free_aborts_inflight",
    "alloc_failure",
    "remove_resets",
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
            "-Wl,--wrap=al_udma_q_init",
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
    assert not re.search(r"^FAIL", r.stdout, re.M), out
    assert r.returncode == 0, out
    assert r.stdout.count("PASS") >= 1, out


VENDOR = Path(
    "/mnt/2tb/unvr-port-refs/UBNT-source-code/UNVR-1.3.35-GPL/"
    "linux-arm64-unvr-4.1.37-ubnt/drivers/dma/al"
)


@pytest.mark.skipif(not VENDOR.exists(), reason="vendor GPL drop not on this host")
def test_limits_match_vendor():
    """The per-op length limits are the vendor driver's, not guesses (#290)."""
    logic = (MOD / "al_dma_logic.h").read_text()
    ours = {
        k: 1 << int(n)
        for k, n in re.findall(r"#define (AL_DMA_MAX_LEN_\w+)\s+\(1u << (\d+)\)", logic)
    }
    vh = (VENDOR / "al_dma.h").read_text()
    shift = {
        k: int(v) for k, v in re.findall(r"AL_DMA_MAX_SIZE_SHIFT_(\w+)\s+(\d+)", vh)
    }
    assert ours["AL_DMA_MAX_LEN_MEMCPY"] == 1 << shift["MEMCPY"]
    assert ours["AL_DMA_MAX_LEN_P"] == 1 << shift["XOR"]
    assert ours["AL_DMA_MAX_LEN_Q"] == 1 << shift["PQ"]
    vr = (VENDOR / "al_hal_ssm_raid.c").read_text()
    assert ours["AL_DMA_MAX_LEN_P"] == 1 << int(
        re.search(r"AL_RAID_OP_P_MAX_BLOCK_SIZE\s+\(1 << (\d+)\)", vr).group(1)
    )
    assert ours["AL_DMA_MAX_LEN_Q"] == 1 << int(
        re.search(r"AL_RAID_OP_Q_MAX_BLOCK_SIZE\s+\(1 << (\d+)\)", vr).group(1)
    )
