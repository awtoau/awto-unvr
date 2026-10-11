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
            "-Wno-unused",
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


# ---- #305: rx_poll completes NAPI whatever the refill did --------------------


def _header_defines(*names: str) -> str:
    hdr = (MAIN_C.parent / "al_eth.h").read_text()
    out = []
    for n in names:
        m = re.search(rf"^#define\s+{n}\b.*$", hdr, re.M)
        assert m, f"{n} not defined in al_eth.h"
        out.append(m.group(0))
    return "\n".join(out) + "\n"


RX_CASES = [
    "short_refill_completes",
    "zero_refill_completes",
    "full_refill_completes",
    "budget_exhausted_stays_scheduled",
    "moderation_follows_the_flag",
    "watchdog_kicks_after_threshold",
    "watchdog_nonempty_resets_count",
    "watchdog_full_ring_never_kicks",
    "watchdog_queues_independent",
    "watchdog_rearms_only_while_up",
    "poll_error_frames_counted_as_dropped",
    "poll_clean_frames_not_dropped",
    "poll_zero_budget_does_nothing",
]


@pytest.fixture(scope="module")
def rx_harness() -> Path:
    src = main_c()
    funcs = "\n\n".join(
        extract_function(src, n)
        for n in (
            "al_eth_rx_poll_finish",
            "al_eth_check_for_empty_rx_ring",
            "al_eth_rx_watchdog",
        )
    )
    consts = _header_defines(
        "AL_ETH_DEFAULT_RX_DESCS",
        "AL_ETH_EMPTY_RX_REFILL",
        "AL_ETH_RX_WATCHDOG_PERIOD",
        "AL_ETH_RX_RING_IDX_ADD",
    )
    return build(
        "rx",
        "al_eth_rx_host.c",
        {
            "rx_funcs": funcs,
            "consts": consts,
            "rx_poll": extract_function(src, "al_eth_rx_poll"),
        },
    )


@pytest.mark.parametrize("case", RX_CASES)
def test_rx_ring_recovery(rx_harness, case):
    assert f"PASS {case}" in run(rx_harness, case)


# ---- #299: a failed probe leaves nothing behind -----------------------------

PROBE_STEPS = [
    "none",
    "board_params",
    "hal_init",
    "serdes",
    "phylink",
    "hw_init",
    "rings",
    "sysfs",
    "register",
]


@pytest.fixture(scope="module")
def probe_harness() -> Path:
    return build(
        "probe",
        "al_eth_probe_host.c",
        {"probe": extract_function(main_c(), "al_eth_probe")},
    )


@pytest.mark.parametrize("phylink", [False, True], ids=["1g-rgmii", "10g-phylink"])
@pytest.mark.parametrize("step", PROBE_STEPS)
def test_probe_failure_leaves_nothing(probe_harness, step, phylink):
    if not phylink and step in ("serdes", "phylink"):
        pytest.skip("only reached on the phylink port")
    env = dict(os.environ)
    if phylink:
        env["USE_PHYLINK"] = "1"
    else:
        env.pop("USE_PHYLINK", None)
    r = subprocess.run(
        [str(probe_harness), step], capture_output=True, text=True, env=env
    )
    assert r.returncode == 0 and f"PASS {step}" in r.stdout, r.stdout + r.stderr


# ---- #298: UDMA HAL errors reach the caller -----------------------------------

UDMA_CASES = [
    "reset_all_clean",
    "reset_all_reports_timeout_and_keeps_going",
    "reset_all_rx_failure",
    "reset_all_skips_unmapped_queue",
    "hal_init_handle_get_failure",
    "hal_init_udma_init_failure",
    "hal_init_clean",
    "init_rings_handle_get_failure",
    "init_rings_clean",
]


@pytest.fixture(scope="module")
def udma_harness() -> Path:
    src = main_c()
    funcs = "\n\n".join(
        extract_function(src, n)
        for n in (
            "al_eth_udma_queues_reset_all",
            "al_eth_hal_adapter_init",
            "al_eth_init_rings",
        )
    )
    return build("udma", "al_eth_udma_host.c", {"udma_funcs": funcs})


@pytest.mark.parametrize("case", UDMA_CASES)
def test_udma_errors_reach_the_caller(udma_harness, case):
    assert f"PASS {case}" in run(udma_harness, case)


def test_al_eth_udma_hal_header_marks_must_check():
    # the compiler flags a discarded result only if the prototype says so (#298)
    hdr = (MAIN_C.parent / "al_hal_udma.h").read_text()
    for fn in (
        "al_udma_init",
        "al_udma_q_init",
        "al_udma_q_reset",
        "al_udma_q_handle_get",
        "al_udma_state_set",
    ):
        assert re.search(rf"__must_check\s+int\s+{fn}\(", hdr), (
            f"{fn} lacks __must_check"
        )


# ---- #297: TX csum/TSO offload only for what the MAC can do -------------------

TXCSUM_CASES = [
    "v4_tcp_offloaded",
    "v4_udp_offloaded",
    "v6_tcp_offloaded",
    "v4_tso_offloaded",
    "v6_exthdr_falls_back_to_software",
    "unknown_l4_falls_back",
    "encapsulated_falls_back",
    "fallback_failure_is_reported",
    "gso_unsupported_is_dropped",
    "no_offload_wanted_is_untouched",
    "fc_keeps_features_for_supported",
    "fc_strips_csum_and_gso_for_unsupported",
]


@pytest.fixture(scope="module")
def txcsum_harness() -> Path:
    src = main_c()
    funcs = "\n\n".join(
        extract_function(src, n)
        for n in ("al_eth_tx_l4_proto", "al_eth_features_check", "al_eth_tx_csum")
    )
    return build("txcsum", "al_eth_txcsum_host.c", {"txcsum_funcs": funcs})


@pytest.mark.parametrize("case", TXCSUM_CASES)
def test_tx_offload_only_for_supported_protocols(txcsum_harness, case):
    assert f"PASS {case}" in run(txcsum_harness, case)


def test_features_check_is_wired_into_netdev_ops():
    ops = re.search(
        r"struct net_device_ops al_eth_netdev_ops = \{.*?\};", main_c(), re.S
    )
    assert ops and ".ndo_features_check" in ops.group(0)


# ---- #296: dropped packets are counted ----------------------------------------

XMIT_CASES = [
    "sends_ok",
    "pad_failure_counted",
    "csum_failure_counted",
    "head_map_failure_counted_and_slot_cleared",
    "frag_map_failure_unwinds_and_counts",
    "frag_overflow_counted_as_frag_err",
    "ring_full_is_busy_not_a_drop",
    "ring_drops_sums_every_queue",
]


@pytest.fixture(scope="module")
def xmit_harness() -> Path:
    src = main_c()
    funcs = "\n\n".join(
        extract_function(src, n)
        for n in ("al_eth_tx_drop", "al_eth_ring_drops", "al_eth_start_xmit")
    )
    return build("xmit", "al_eth_xmit_host.c", {"xmit_funcs": funcs})


@pytest.mark.parametrize("case", XMIT_CASES)
def test_tx_drops_are_counted(xmit_harness, case):
    assert f"PASS {case}" in run(xmit_harness, case)
