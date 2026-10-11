# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""scripts/bench-all.py bench_dma(): dmatest parsing and verdict logic, offline.

The subtest needs the box; these cover what it concludes from dmatest's
dmesg lines (format: drivers/dma/dmatest.c, pr_info at result() and summary).
"""

import importlib.util
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))


def _mod():
    spec = importlib.util.spec_from_file_location(
        "bench_all", REPO / "scripts" / "bench-all.py"
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
    assert m.judge_dmatest(m.parse_dmatest(CLEAN), 20000) == []


def test_short_run_fails():
    m = _mod()
    assert any(
        "ran 20000 of 40000" in f
        for f in m.judge_dmatest(m.parse_dmatest(CLEAN), 40000)
    )


def test_prep_errors_only_allowed_when_asked():
    m = _mod()
    p = m.parse_dmatest(PQ_PREP)
    assert m.judge_dmatest(p, 20000)
    assert m.judge_dmatest(p, 20000, {"prep error"}) == []


def test_data_error_is_never_allowed_by_prep_allowance():
    m = _mod()
    assert m.judge_dmatest(m.parse_dmatest(PQ_DATA), 20000, {"prep error"})


def test_queue_init_failure_in_kernel_log_fails_a_clean_summary():
    m = _mod()
    text = CLEAN + "[ 100.3] al_dma 0000:00:04.0: Failed to init DMA queue 0: -5\n"
    assert any(
        "Failed to init DMA queue" in f
        for f in m.judge_dmatest(m.parse_dmatest(text), 20000)
    )


def test_completion_error_in_kernel_log_fails_a_clean_summary():
    m = _mod()
    text = CLEAN + "[ 100.3] al_dma 0000:00:04.0: DMA chan 0 completion error: 0x10\n"
    assert any(
        "completion error" in f for f in m.judge_dmatest(m.parse_dmatest(text), 20000)
    )


def test_refusal_warning_is_expected_only_when_prep_errors_are():
    m = _mod()
    text = PQ_PREP + "[ 100.0] al_dma 0000:00:04.0: pq len 12032 rejected\n"
    p = m.parse_dmatest(text)
    assert m.judge_dmatest(p, 20000, {"prep error"}) == []
    assert any("rejected" in f for f in m.judge_dmatest(p, 20000))


def test_no_summary_fails():
    m = _mod()
    assert m.judge_dmatest(m.parse_dmatest("nothing\n"), 20000)


def test_run_limit_scales_with_iterations():
    m = _mod()
    assert m.dma_run_limit_s(20000) < m.dma_run_limit_s(200000)
    # 20000 iterations at the 2000 iops floor is 10 s; x1.25 plus connect+slack
    assert (
        m.dma_run_limit_s(20000) == int(10 * 1.25) + m.DMA_SSH_CONNECT_S + m.DMA_SLACK_S
    )


def test_refusals_extend_the_limit_by_dmatest_sleep():
    m = _mod()
    # 300 iterations, 90% refused, 0.1 s each, plus the 2000 iops floor
    base = m.dma_run_limit_s(300)
    assert (
        m.dma_run_limit_s(300, 0.9) == int((300 / 2000 + 300 * 0.9 * 0.1) * 1.25) + 13
    )
    assert m.dma_run_limit_s(300, 0.9) > base + 25


def test_bench_dma_skips_without_a_channel():
    m = _mod()
    m.run_remote = lambda *a, **k: (0, "modprobe: ok\nN\n")
    r = m.Report()
    m.bench_dma("h", "pw", 20000, r)
    assert r.checks[0][1] is True and "SKIPPED" in r.checks[0][2]
    assert not r.metrics


def test_bench_dma_skips_when_previous_run_cannot_be_stopped():
    m = _mod()

    def fake(host, pw, script, timeout=60):
        if "HAVE_CHAN" in script:
            return 0, "HAVE_CHAN\nRUN=Y\nINUSE=1\n"
        return 124, "ssh call timed out"

    m.run_remote = fake
    r = m.Report()
    m.bench_dma("h", "pw", 20000, r)
    assert len(r.checks) == 1
    assert r.checks[0][1] is True
    assert "SKIPPED" in r.checks[0][2] and "reboot" in r.checks[0][2]


def test_bench_dma_records_iops_metrics_and_fails_on_failures():
    m = _mod()

    def fake(host, pw, script, timeout=60):
        if "HAVE_CHAN" in script:
            return 0, "HAVE_CHAN\nRUN=N\nINUSE=0\n"
        return 0, CLEAN.replace("0 failures 12743.00", "3 failures 12743.00")

    m.run_remote = fake
    r = m.Report()
    m.bench_dma("h", "pw", 20000, r)
    assert r.metrics["dma dma0chan0 copy iops"] == 10050.25
    assert r.metrics["dma dma0chan0 xor iops"] == 12743.0
    assert r.checks[0][0] == "dma copy+xor" and r.checks[0][1] is False


def test_channel_state_parses_the_probe():
    m = _mod()
    st = m.dma_channel_state("HAVE_CHAN\nRUN=N\nINUSE=1\n")
    assert st == {"have": True, "run": "N", "in_use": 1}
    assert m.dma_channel_state("junk")["have"] is False


def test_leaked_client_is_a_clear_skip_not_a_hang():
    m = _mod()
    r = m.dma_skip_reason({"have": True, "run": "N", "in_use": 1}, True)
    assert "leaked client" in r and "reboot" in r
    assert m.dma_skip_reason({"have": True, "run": "N", "in_use": 0}, True) is None
    assert m.dma_skip_reason({"have": True, "run": "N", "in_use": None}, True) is None


def test_someone_elses_run_is_left_alone_when_not_taking_over():
    m = _mod()
    st = {"have": True, "run": "Y", "in_use": 1}
    assert "in progress" in m.dma_skip_reason(st, False)
    assert m.dma_skip_reason(st, True) is None  # one-shot stops it first


def test_bench_dma_does_not_stop_a_foreign_run_when_not_taking_over():
    m = _mod()
    calls = []

    def fake(host, pw, script, timeout=60):
        calls.append(script)
        return 0, "HAVE_CHAN\nRUN=Y\nINUSE=1\n"

    m.run_remote = fake
    r = m.Report(echo=False)
    m.bench_dma("h", "pw", 20000, r, take_over=False)
    assert "SKIPPED" in r.checks[0][2] and "in progress" in r.checks[0][2]
    assert not any("echo 0 >" in c for c in calls)  # never touched the run


def test_bench_dma_always_stops_dmatest_afterwards():
    m = _mod()
    calls = []

    def fake(host, pw, script, timeout=60):
        calls.append(script)
        if "HAVE_CHAN" in script:
            return 0, "HAVE_CHAN\nRUN=N\nINUSE=0\n"
        return 0, CLEAN

    m.run_remote = fake
    m.bench_dma("h", "pw", 20000, m.Report(echo=False))
    assert calls[-1].endswith("/run")  # the final `echo 0 > run`
    assert "echo 0 >" in calls[-1]


def test_a_stop_that_times_out_is_reported_as_a_wedge():
    m = _mod()

    def fake(host, pw, script, timeout=60):
        if "HAVE_CHAN" in script:
            return 0, "HAVE_CHAN\nRUN=N\nINUSE=0\n"
        if script.startswith("echo 0 >"):
            return 124, "ssh call timed out"
        return 0, CLEAN

    m.run_remote = fake
    r = m.Report(echo=False)
    m.bench_dma("h", "pw", 20000, r)
    stop = [c for c in r.checks if c[0] == "dma stop"]
    assert stop and stop[0][1] is False and "wedged" in stop[0][2]


def test_dmesg_marker_is_unique_per_run():
    m = _mod()
    seen = []

    def fake(host, pw, script, timeout=60):
        if "HAVE_CHAN" in script:
            return 0, "HAVE_CHAN\nRUN=N\nINUSE=0\n"
        if "/dev/kmsg" in script:
            seen.append([ln for ln in script.splitlines() if "/dev/kmsg" in ln][0])
        return 0, CLEAN

    m.run_remote = fake
    m.bench_dma("h", "pw", 20000, m.Report(echo=False))
    m.bench_dma("h", "pw", 20000, m.Report(echo=False))
    assert len(seen) == 4 and len(set(seen)) == 4  # no marker is ever reused
