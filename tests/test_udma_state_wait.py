# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""al_udma_state_set_wait() source-shape invariants (#254).

Two copies exist and stay forked (gen A in al_eth, gen B in al_ssm).
al_udma_state_get() reads NORMAL while a sub-block is busy, not while the
engine is enabled, so NORMAL and DISABLE both wait for IDLE (quiescence).
Waiting for NORMAL would time out on an enabled, idle engine and break
bring-up. These checks stop a later edit "fixing" the remap, and pin the
timeout warning to name the state the engine was left in.
"""

from __future__ import annotations

import re
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
COPIES = {
    "al_eth": REPO / "modules/al_eth/al_hal_eth_main.c",
    "al_ssm": REPO / "modules/al_ssm/al_hal_udma_main.c",
}


def wait_body(path: Path) -> str:
    src = path.read_text()
    m = re.search(r"^(?:static )?int al_udma_state_set_wait\(.*\)\n\{", src, re.M)
    assert m, f"al_udma_state_set_wait not found in {path}"
    depth = 0
    for j in range(m.end() - 1, len(src)):
        depth += (src[j] == "{") - (src[j] == "}")
        if depth == 0:
            return src[m.start() : j + 1]
    raise AssertionError("unbalanced braces")


@pytest.mark.parametrize("name", COPIES)
def test_normal_and_disable_wait_for_quiescence(name):
    b = wait_body(COPIES[name])
    assert re.search(
        r"if \(\(new_state == UDMA_NORMAL\) \|\| \(new_state == UDMA_DISABLE\)\)\s*"
        r"expected_state = UDMA_IDLE;",
        b,
    )


@pytest.mark.parametrize("name", COPIES)
def test_timeout_warning_names_the_state_the_engine_is_in(name):
    b = wait_body(COPIES[name])
    assert "dma state didn't change to %s" in b  # the string #254 tells people to grep
    assert re.search(r"engine still %s", b)
    assert "al_udma_states_name[state]" in b
