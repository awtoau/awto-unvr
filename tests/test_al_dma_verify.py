# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""scripts/al-dma-verify.py: the dmatest parsing and verdict logic, offline.

The script itself needs the box; these cover what it concludes from dmatest's
dmesg lines (format: drivers/dma/dmatest.c, pr_info at result() and summary).
"""

import importlib.util
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))


def _mod():
    spec = importlib.util.spec_from_file_location(
        "al_dma_verify", REPO / "scripts" / "al-dma-verify.py"
    )
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


CLEAN = (
    "[ 100.1] dmatest: dma0chan0-copy0: summary 20000 tests, 0 failures 10050.25 iops 72864 KB/s (0)\n"
    "[ 100.2] dmatest: dma0chan0-xor0: summary 20000 tests, 0 failures 12743.00 iops 99000 KB/s (0)\n"
)
PQ_PREP = (
    "[ 100.1] dmatest: dma0chan0-pq0: result #3: 'prep error' with src_off=0x2 dst_off=0x4 len=0x2f00 (-12)\n"
    "[ 100.1] dmatest: dma0chan0-pq0: result #4: 'prep error' with src_off=0x2 dst_off=0x4 len=0x3000 (-12)\n"
    "[ 100.2] dmatest: dma0chan0-pq0: summary 20000 tests, 2 failures 2000.00 iops 15000 KB/s (0)\n"
)
PQ_DATA = (
    "[ 100.1] dmatest: dma0chan0-pq0: result #3: 'data error' with src_off=0x2 dst_off=0x4 len=0x2f00 (7)\n"
    "[ 100.2] dmatest: dma0chan0-pq0: summary 20000 tests, 1 failures 2000.00 iops 15000 KB/s (0)\n"
)


def test_clean_run_passes():
    m = _mod()
    assert m.judge(m.parse_dmesg(CLEAN), 20000) == []


def test_short_run_fails():
    m = _mod()
    assert any("ran 20000 of 40000" in f for f in m.judge(m.parse_dmesg(CLEAN), 40000))


def test_prep_errors_only_allowed_when_asked():
    m = _mod()
    p = m.parse_dmesg(PQ_PREP)
    assert m.judge(p, 20000)
    assert m.judge(p, 20000, {"prep error"}) == []


def test_data_error_is_never_allowed_by_prep_allowance():
    m = _mod()
    assert m.judge(m.parse_dmesg(PQ_DATA), 20000, {"prep error"})


def test_queue_init_failure_in_kernel_log_fails_a_clean_summary():
    m = _mod()
    text = CLEAN + "[ 100.3] al_dma 0000:00:04.0: Failed to init DMA queue 0: -5\n"
    assert any(
        "Failed to init DMA queue" in f for f in m.judge(m.parse_dmesg(text), 20000)
    )


def test_no_summary_fails():
    m = _mod()
    assert m.judge(m.parse_dmesg("nothing\n"), 20000)


def test_run_limit_scales_with_iterations():
    m = _mod()
    assert m.run_limit_s(20000) < m.run_limit_s(200000)
    # 20000 iterations at the 2000 iops floor is 10 s; x1.25 plus connect+slack
    assert m.run_limit_s(20000) == int(10 * 1.25) + m.SSH_CONNECT_S + m.SLACK_S


def test_refusals_extend_the_limit_by_dmatest_sleep():
    m = _mod()
    # 300 iterations, 90% refused, 0.1 s each, plus the 2000 iops floor
    base = m.run_limit_s(300)
    assert m.run_limit_s(300, 0.9) == int((300 / 2000 + 300 * 0.9 * 0.1) * 1.25) + 13
    assert m.run_limit_s(300, 0.9) > base + 25


def test_find_bdf():
    m = _mod()
    show = (
        "pci/0000:00:04.0: reporter ssm   state healthy error 0 recover 0\n"
        "pci/0000:00:05.0: reporter dma   state healthy error 0 recover 0\n"
    )
    assert m.find_bdf(show) == "0000:00:05.0"
    assert m.find_bdf("pci/0000:00:04.0: reporter ssm") is None
