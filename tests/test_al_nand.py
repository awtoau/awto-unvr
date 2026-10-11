# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""al_nand on the host: the real driver + real HAL against a controller model.

modules/al_nand/al_nand_main.c is #included into tests/c/al_nand_host.c and
linked with the unmodified al_hal_nand.c. Fake: the kernel
(tests/c/kshim/kshim_nand.h), the rawnand core (legacy paths copied from v7.3,
source lines quoted in the harness), and the registers, via a model that
retires command-FIFO entries on a virtual microsecond clock and holds the
device busy for tBERS / tPROG after ERASE2 / PAGEPROG. Each case in
al_nand_host.c is one pytest item and prints PASS/FAIL lines; each was checked
to FAIL on the commit before the fix it covers.

Build dir: tmp/test-al-nand/ (not system /tmp). Needs a host cc.
"""

from __future__ import annotations

import re
import shutil
import subprocess
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
MOD = REPO / "modules" / "al_nand"
CDIR = REPO / "tests" / "c"
BUILD = REPO / "tmp" / "test-al-nand"

pytestmark = pytest.mark.skipif(
    shutil.which("cc") is None, reason="needs a host C compiler"
)

CASES = [
    "probe_sanity",
    "erase_stall_status",
    "erase_stall_erase2",
    "erase_no_stall",
    "stuck_addr_keeps_short_bound",
    "waitfunc_runs_core_with_latched_error",
    "reread_failure",
    "reread_blank_page",
    "reread_parityless_page",
    "timing_mode_feature",
    "setup_interface_check_only",
    "setup_interface_programs_controller",
    "write_byte_is_a_data_cycle",
    "int_en_rmw_is_locked",
    "wait_leaves_nothing_armed",
]


def build_harness(src_dir: Path = MOD, build: Path = BUILD) -> Path:
    """Compile the harness around the al_nand sources in src_dir."""
    if build.exists():
        shutil.rmtree(build)
    (build / "kshim").mkdir(parents=True)
    for f in src_dir.glob("*.[ch]"):
        shutil.copy(f, build / f.name)
    # the two plat headers are the only host-specific part of the HAL
    shutil.copy(CDIR / "host_plat_types.h", build / "al_hal_plat_types.h")
    shutil.copy(CDIR / "host_nand_plat_services.h", build / "al_hal_plat_services.h")
    shutil.copy(CDIR / "kshim" / "kshim_nand.h", build / "kshim" / "kshim_nand.h")
    shutil.copy(CDIR / "al_nand_host.c", build / "harness.c")
    # every <linux/x.h> the driver names forwards to the shim
    names = set(
        re.findall(
            r"#include <(linux/[\w./-]+)>", (build / "al_nand_main.c").read_text()
        )
    )
    for n in names:
        p = build / "kshim" / n
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text('#include "kshim_nand.h"\n')
    exe = build / "harness"
    r = subprocess.run(
        [
            "cc",
            "-std=gnu11",
            "-O1",
            "-g",
            "-w",
            f"-I{build}",
            f"-I{build / 'kshim'}",
            str(build / "harness.c"),
            str(build / "al_hal_nand.c"),
            "-o",
            str(exe),
        ],
        capture_output=True,
        text=True,
    )
    assert r.returncode == 0, r.stderr
    return exe


@pytest.fixture(scope="module")
def harness() -> Path:
    return build_harness()


@pytest.mark.parametrize("case", CASES)
def test_al_nand_host(harness: Path, case: str):
    r = subprocess.run([str(harness), case], capture_output=True, text=True)
    out = r.stdout + r.stderr
    assert not re.search(r"^FAIL", r.stdout, re.M), out
    assert r.returncode == 0, out
    assert r.stdout.count("PASS") >= 1, out
