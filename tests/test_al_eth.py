# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""al_eth on the host: pieces of the REAL modules/al_eth/al_eth_main.c.

al_eth_main.c is 6k lines of netdev/phylink/page_pool and cannot be linked on
the host whole. Each case cuts the function (or preprocessor paragraph) under
test out of the real file by name and compiles it against a few stub types
in tests/c/al_eth_host.c. A function that starts touching a field the stub
lacks fails to compile: the stub tracks the code, not the other way round.

AL_ETH_MAIN_C=<path> points the cases at another copy of the file (used to
run them against the parent commit's source).
Build dir: tmp/test-al-eth/ (not system /tmp). Needs a host cc.
"""

from __future__ import annotations

import os
import re
import shutil
import subprocess
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
MAIN_C = Path(
    os.environ.get("AL_ETH_MAIN_C", REPO / "modules" / "al_eth" / "al_eth_main.c")
)
CDIR = REPO / "tests" / "c"
BUILD = REPO / "tmp" / "test-al-eth"

pytestmark = pytest.mark.skipif(
    shutil.which("cc") is None, reason="needs a host C compiler"
)


def main_c() -> str:
    return MAIN_C.read_text()


def extract_function(src: str, name: str) -> str:
    """Text of the function `name` (return type line through closing brace)."""
    m = re.search(rf"^[A-Za-z_][^;\n]*\b{re.escape(name)}\(", src, re.M)
    if not m:
        # return type on the previous line: "static int\nname(...)"
        m = re.search(rf"^{re.escape(name)}\(", src, re.M)
        assert m, f"function {name} not found in {MAIN_C}"
        start = src.rfind("\n", 0, src.rfind("\n", 0, m.start())) + 1
    else:
        start = m.start()
        prev = src.rfind("\n", 0, start - 1) + 1
        if re.fullmatch(
            r"(static\s+)?(inline\s+)?\w+(\s+\w+)*\s*\*?\s*\n?", src[prev:start]
        ):
            start = prev
    brace = src.index("{", m.end())
    depth, i = 0, brace
    while True:
        c = src[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return src[start : i + 1]
        i += 1


def build(
    tag: str, harness: str, parts: dict[str, str], flags: list[str] | None = None
) -> Path:
    """Compile tests/c/<harness> with `parts` written as <name>.inc beside it."""
    d = BUILD / tag
    shutil.rmtree(d, ignore_errors=True)
    d.mkdir(parents=True)
    for k, v in parts.items():
        (d / f"{k}.inc").write_text(v)
    exe = d / "t"
    r = subprocess.run(
        [
            "cc",
            "-std=gnu11",
            "-O1",
            "-Wall",
            "-Werror",
            "-I",
            str(d),
            "-I",
            str(CDIR),
            *(flags or []),
            str(CDIR / harness),
            "-o",
            str(exe),
        ],
        capture_output=True,
        text=True,
    )
    assert r.returncode == 0, r.stderr
    return exe


def run(exe: Path, *args: str) -> str:
    r = subprocess.run([str(exe), *args], capture_output=True, text=True)
    assert r.returncode == 0, r.stdout + r.stderr
    return r.stdout


# ---- #304: adaptive interrupt moderation default ----------------------------


def _moderation_paragraph(src: str) -> str:
    idx = src.index("#define AL_ETH_DEFAULT_ADAPTIVE_INT_MODERATION")
    start = src.rfind("\n\n", 0, idx) + 2
    end = src.index("\n", src.index("static int adaptive_int_moderation", idx)) + 1
    return src[start:end]


@pytest.mark.parametrize(
    "alpine", [True, False], ids=["CONFIG_ARCH_ALPINE", "no-ARCH_ALPINE"]
)
def test_adaptive_moderation_defaults_off(alpine):
    # vendor al_eth.c:274-278 picks AL_FALSE under CONFIG_ARCH_ALPINE; the port
    # kept only the AL_TRUE arm. Off must hold whether or not the symbol is set.
    exe = build(
        f"moder-{int(alpine)}",
        "al_eth_moderation_host.c",
        {"moderation": _moderation_paragraph(main_c())},
        ["-DCONFIG_ARCH_ALPINE=1"] if alpine else [],
    )
    assert run(exe).strip() == "0"
