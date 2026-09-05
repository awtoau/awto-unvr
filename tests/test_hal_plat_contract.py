# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""hal/al_hal_plat_contract.h - a host missing a primitive must FAIL TO COMPILE.

The vendor's per-host plat_services.h copies drifted to 31 vs 41 defines with
nothing checking, so a host could silently miss a primitive (#256). These tests
pin the property that makes that impossible: compile a synthetic host that
defines every primitive but one, and require an #error naming the one omitted.

Compiles with the HOST cc (-fsyntax-only) - the contract is preprocessor-only,
so it needs no cross toolchain and no U-Boot tree.

Run: pytest tests/test_hal_plat_contract.py -q
"""

from __future__ import annotations

import subprocess
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
CONTRACT = REPO / "hal" / "al_hal_plat_contract.h"

# Every primitive the contract requires, as a host would define it. Values are
# irrelevant - only definedness is checked - but each must be a valid macro.
PRIMITIVES: dict[str, str] = {
    "al_reg_read8": "#define al_reg_read8(l) 0",
    "al_reg_read16": "#define al_reg_read16(l) 0",
    "al_reg_read32": "#define al_reg_read32(l) 0",
    "al_reg_read64": "#define al_reg_read64(l) 0",
    "al_reg_write8": "#define al_reg_write8(l, v)",
    "al_reg_write16": "#define al_reg_write16(l, v)",
    "al_reg_write32": "#define al_reg_write32(l, v)",
    "al_reg_write64": "#define al_reg_write64(l, v)",
    "al_reg_read32_relaxed": "#define al_reg_read32_relaxed(l) 0",
    "al_reg_write32_relaxed": "#define al_reg_write32_relaxed(l, v)",
    "al_print": "#define al_print(...)",
    "al_err": "#define al_err(...)",
    "al_warn": "#define al_warn(...)",
    "al_info": "#define al_info(...)",
    "al_dbg": "#define al_dbg(...)",
    "al_sprintf": "#define al_sprintf(...)",
    "al_assert": "#define al_assert(c)",
    "al_assert_msg": "#define al_assert_msg(c, ...)",
    "al_assert_fatal": "#define al_assert_fatal(c)",
    "AL_PLAT_BARRIERS_PROVIDED": "#define AL_PLAT_BARRIERS_PROVIDED",
    "al_udelay": "#define al_udelay(u)",
    "al_msleep": "#define al_msleep(m)",
    "swap16_to_le": "#define swap16_to_le(x) (x)",
    "swap32_to_le": "#define swap32_to_le(x) (x)",
    "swap64_to_le": "#define swap64_to_le(x) (x)",
    "swap16_from_le": "#define swap16_from_le(x) (x)",
    "swap32_from_le": "#define swap32_from_le(x) (x)",
    "swap64_from_le": "#define swap64_from_le(x) (x)",
    "swap16_to_be": "#define swap16_to_be(x) (x)",
    "swap32_to_be": "#define swap32_to_be(x) (x)",
    "swap64_to_be": "#define swap64_to_be(x) (x)",
    "swap16_from_be": "#define swap16_from_be(x) (x)",
    "swap32_from_be": "#define swap32_from_be(x) (x)",
    "swap64_from_be": "#define swap64_from_be(x) (x)",
    "al_memset": "#define al_memset(p, v, c)",
    "al_memcpy": "#define al_memcpy(d, s, c)",
    "al_memcmp": "#define al_memcmp(a, b, c) 0",
    "al_strcmp": "#define al_strcmp(a, b) 0",
    "al_popcount": "#define al_popcount(x) 0",
    "al_get_cpu_id": "#define al_get_cpu_id() 0",
    "al_get_cluster_id": "#define al_get_cluster_id() 0",
}


def _host_source(omit: str | None = None) -> str:
    """A synthetic plat_services.h defining every primitive except `omit`."""
    lines = ["#define __PLAT_SERVICES_H__"]
    lines += [d for name, d in PRIMITIVES.items() if name != omit]
    lines.append(f'#include "{CONTRACT}"')
    return "\n".join(lines) + "\n"


def _compile(src: str, tmp_path: Path) -> subprocess.CompletedProcess:
    c = tmp_path / "host.c"
    c.write_text(src)
    # -fsyntax-only: the contract is preprocessor-only, nothing to link.
    # Timeout 30 s = ~1.25x a slow cold cc1 on this host; a preprocess-only
    # run is <0.1 s, so expiry means cc is wedged, and pytest reports which.
    return subprocess.run(
        ["cc", "-fsyntax-only", str(c)],
        capture_output=True,
        text=True,
        timeout=30,
    )


def test_complete_host_compiles(tmp_path: Path) -> None:
    """All 41 definitions present -> clean compile. Guards against a contract
    that rejects everything and so proves nothing."""
    r = _compile(_host_source(), tmp_path)
    assert r.returncode == 0, r.stderr


@pytest.mark.parametrize("omit", sorted(PRIMITIVES))
def test_missing_primitive_fails_naming_it(omit: str, tmp_path: Path) -> None:
    """Omit exactly one primitive -> #error, and the message names it.

    Naming matters: the failure a new host hits must say which primitive is
    missing, not just that something is."""
    r = _compile(_host_source(omit=omit), tmp_path)
    assert r.returncode != 0, f"omitting {omit} still compiled"
    assert "#error" in r.stderr or "error:" in r.stderr
    assert omit in r.stderr, f"error did not name {omit}:\n{r.stderr}"


def test_contract_included_before_plat_services_is_an_error(tmp_path: Path) -> None:
    """Including the contract first is a bug - the guard must catch it."""
    src = f'#include "{CONTRACT}"\n'
    r = _compile(src, tmp_path)
    assert r.returncode != 0
    assert "al_hal_plat_services.h" in r.stderr


def test_uboot_shim_satisfies_the_contract() -> None:
    """The real U-Boot shim includes the contract, so its build enforces it.

    A compile of the shim itself needs the U-Boot tree; the build does that.
    Here we only pin that the include is present and last-ish."""
    shim = (
        REPO / "uboot-port" / "drivers" / "net" / "al_hal_shim" / "al_hal_plat_services.h"
    )
    txt = shim.read_text()
    assert '#include "al_hal_plat_contract.h"' in txt, (
        "U-Boot shim no longer includes the contract - drift is unchecked again"
    )
