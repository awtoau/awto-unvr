# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""al_thermal on the host: the real driver + real sensor HAL against a model.

modules/al_thermal/al_thermal_main.c is #included into
tests/c/al_thermal_host.c and linked with the unmodified
al_hal_thermal_sensor.c and al_hal_pbs_utils.c. Fake: the kernel
(tests/c/kshim/kshim_thermal.h) and the sensor, modelled from the register
description (T_VALID clear-on-read, RESULT holds the last conversion, stale
across a power cycle). Each case in al_thermal_host.c is one pytest item.

Build dir: tmp/test-al-thermal/ (not system /tmp). Needs a host cc.
"""

from __future__ import annotations

import re
import shutil
import subprocess
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
MOD = REPO / "modules" / "al_thermal"
CDIR = REPO / "tests" / "c"
BUILD = REPO / "tmp" / "test-al-thermal"

pytestmark = pytest.mark.skipif(
    shutil.which("cc") is None, reason="needs a host C compiler"
)

CASES = [
    "steady_state",
    "wait_sleeps_and_is_bounded",
    "remove_unregisters_once",
    "sensor_off_after_zone",
    "pbs_mapping_not_leaked",
    "hwmon_is_devm",
    "probe_failure_powers_down",
]


def build_harness(src_dir: Path = MOD, build: Path = BUILD) -> Path:
    """Compile the harness around the al_thermal sources in src_dir."""
    if build.exists():
        shutil.rmtree(build)
    (build / "kshim").mkdir(parents=True)
    for f in src_dir.glob("*.[ch]"):
        shutil.copy(f, build / f.name)
    # the two plat headers are the only host-specific part of the HAL
    shutil.copy(CDIR / "host_plat_types.h", build / "al_hal_plat_types.h")
    shutil.copy(CDIR / "host_thermal_plat_services.h", build / "al_hal_plat_services.h")
    shutil.copy(CDIR / "kshim" / "kshim_thermal.h", build / "kshim" / "kshim_thermal.h")
    shutil.copy(CDIR / "al_thermal_host.c", build / "harness.c")
    names = set(
        re.findall(
            r"#include <(linux/[\w./-]+)>", (build / "al_thermal_main.c").read_text()
        )
    )
    for n in names:
        p = build / "kshim" / n
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text('#include "kshim_thermal.h"\n')
    exe = build / "harness"
    r = subprocess.run(
        [
            "cc",
            "-std=gnu11",
            "-O1",
            "-g",
            "-w",
            "-DCONFIG_PM",
            f"-I{build}",
            f"-I{build / 'kshim'}",
            str(build / "harness.c"),
            str(build / "al_hal_thermal_sensor.c"),
            str(build / "al_hal_pbs_utils.c"),
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
def test_al_thermal_host(harness: Path, case: str):
    r = subprocess.run([str(harness), case], capture_output=True, text=True)
    out = r.stdout + r.stderr
    assert not re.search(r"^FAIL", r.stdout, re.M), out
    assert r.returncode == 0, out
    assert r.stdout.count("PASS") >= 1, out
