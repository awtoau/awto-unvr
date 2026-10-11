# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""scripts/bench-all.py pure logic, offline: health, baseline, idle, snapshots.

Each test feeds the function the text the box would have produced; nothing
here opens a socket.
"""

import functools
import importlib.util
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))


@functools.cache
def _mod():
    spec = importlib.util.spec_from_file_location(
        "bench_all", REPO / "scripts" / "bench-all.py"
    )
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


# ---- health ----------------------------------------------------------------

HEALTH_OK = """@@kernel
7.3.0-rc6-6bd513f+
@@vermagic
7.3.0-rc6-6bd513f+ SMP preempt mod_unload aarch64
@@link
enp0s1 speed=100 carrier=1 oper=up
enp0s2 speed=10000 carrier=1 oper=up
@@stats
enp0s1 rx_errors=0 rx_crc_errors=0 tx_errors=0
enp0s2 rx_errors=0 rx_crc_errors=0 tx_errors=0
@@coalesce
enp0s1 Adaptive RX: off  TX: n/a
enp0s2 Adaptive RX: off  TX: n/a
@@dmesg
[  10.0] al_eth 0000:00:01.0: of_irq_parse_pci: failed with rc=-22
[  11.0] EXT4-fs (sda2): mounted
"""


def _health(text, state=None):
    m = _mod()
    state = state or m.HealthState()
    checks, metrics = m.judge_health(m.parse_sections(text), state)
    return {n: (ok, d) for n, ok, d in checks}, metrics, state


def test_health_clean_box_passes_and_records_link_speed():
    checks, metrics, _ = _health(HEALTH_OK)
    assert all(ok for ok, _ in checks.values()), checks
    assert metrics == {
        "link enp0s1 speed Mbps": 100.0,
        "link enp0s2 speed Mbps": 10000.0,
    }


def test_health_vermagic_mismatch_fails():
    bad = HEALTH_OK.replace("7.3.0-rc6-6bd513f+ SMP", "7.3.0-rc5-aaaaaaa SMP")
    assert _health(bad)[0]["health al_eth module"][0] is False


def test_health_carrier_down_fails():
    bad = HEALTH_OK.replace(
        "speed=10000 carrier=1 oper=up", "speed= carrier=0 oper=down"
    )
    checks, metrics, _ = _health(bad)
    assert checks["health link enp0s2"][0] is False
    assert "link enp0s2 speed Mbps" not in metrics


def test_health_counters_one_shot_nonzero_fails():
    bad = HEALTH_OK.replace("enp0s2 rx_errors=0", "enp0s2 rx_errors=7")
    assert _health(bad)[0]["health MAC counters"][0] is False


def test_health_counters_judge_growth_not_history():
    m = _mod()
    state = m.HealthState()
    old = HEALTH_OK.replace("enp0s2 rx_errors=0", "enp0s2 rx_errors=7")
    m.judge_health(m.parse_sections(old), state)  # first look: 7 errors exist
    again = {n: ok for n, ok, _ in m.judge_health(m.parse_sections(old), state)[0]}
    assert again["health MAC counters"] is True  # unchanged: not a new finding
    grown = old.replace("rx_errors=7", "rx_errors=9")
    third = {n: ok for n, ok, _ in m.judge_health(m.parse_sections(grown), state)[0]}
    assert third["health MAC counters"] is False


def test_health_adaptive_moderation_on_fails():
    # #304: the port shipped with adaptive moderation ON; vendor default is OFF
    on = HEALTH_OK.replace("enp0s2 Adaptive RX: off", "enp0s2 Adaptive RX: on ")
    ok, detail = _health(on)[0]["health adaptive moderation off"]
    assert ok is False and "enp0s2" in detail
    assert _health(HEALTH_OK)[0]["health adaptive moderation off"][0] is True


def test_health_adaptive_moderation_missing_data_fails():
    gone = HEALTH_OK.replace("enp0s1 Adaptive RX: off  TX: n/a\n", "")
    assert _health(gone)[0]["health adaptive moderation off"][0] is False


def test_health_kernel_log_trouble_fails_but_benign_does_not():
    bad = (
        HEALTH_OK
        + "[  12.0] ------------[ cut here ]------------\n[ 12.1] WARNING: CPU: 1 PID: 4 at foo\n"
    )
    checks = _health(bad)[0]
    assert checks["health kernel log"][0] is False
    assert _health(HEALTH_OK)[0]["health kernel log"][0] is True


def test_health_script_scopes_dmesg_to_the_marker():
    m = _mod()
    assert "sed -n '/bench-all mark X/,$p'" in m._health_script("bench-all mark X")
    assert "dmesg | cat" in m._health_script(None)


def test_health_missing_sections_fail_instead_of_crashing():
    checks, _, _ = _health("@@kernel\n7.3.0\n")
    assert checks["health al_eth module"][0] is False
    assert checks["health link enp0s1"][0] is False
    assert checks["health MAC counters"][0] is False


# ---- baseline and regressions ----------------------------------------------


def test_regression_needs_a_drop_past_the_tolerance():
    m = _mod()
    base = {"eth 10G host->box": 8590.0, "dma dma0chan0 xor iops": 12743.0}
    ok = {"eth 10G host->box": 8000.0, "dma dma0chan0 xor iops": 10500.0}
    assert m.find_regressions(ok, base) == []  # -6.9% eth, -17.6% dma: inside
    bad = {"eth 10G host->box": 7000.0, "dma dma0chan0 xor iops": 9000.0}
    keys = {f.key for f in m.find_regressions(bad, base)}
    assert keys == {"regress:eth 10G host->box", "regress:dma dma0chan0 xor iops"}


def test_a_faster_run_or_a_missing_metric_is_never_a_regression():
    m = _mod()
    base = {"eth 1G host->box": 93.9, "sata sdb": 200.0}
    assert m.find_regressions({"eth 1G host->box": 940.0}, base) == []


def test_link_downshift_is_flagged_at_zero_tolerance():
    m = _mod()
    base = {"link enp0s2 speed Mbps": 10000.0, "usb sda link": 5000.0}
    cur = {"link enp0s2 speed Mbps": 1000.0, "usb sda link": 480.0}
    assert len(m.find_regressions(cur, base)) == 2
    assert m.find_regressions(base, base) == []


def test_tolerances_per_metric_family():
    m = _mod()
    assert m.tolerance_for("dma dma0chan0 copy iops") == m.DMA_TOLERANCE
    assert m.tolerance_for("eth 10G box->host") == m.REGRESSION_THRESHOLD
    assert m.tolerance_for("link enp0s1 speed Mbps") == 0.0


def test_failed_checks_become_hard_findings_and_skips_do_not():
    m = _mod()
    checks = [("dma copy+xor", False, "xor0: 3 failures"), ("dma", True, "SKIPPED: x")]
    f = m.failure_findings(checks)
    assert [x.key for x in f] == ["fail:dma copy+xor"] and f[0].hard


def test_wedge_signs():
    m = _mod()
    assert m.is_wedged([("crypto", False, "remote script failed (rc=124): x")])
    assert m.is_wedged([("dma stop", False, "engine presumed wedged, reboot")])
    assert not m.is_wedged([("eth 1G", False, "iperf3 client hung past 25s")])
    assert not m.is_wedged([("dma", True, "SKIPPED rc=124")])  # a skip is not a fail


def test_load_baseline_reads_snapshots_and_tolerates_absence(tmp_path):
    m = _mod()
    f = tmp_path / "b.json"
    f.write_text('{"metrics": {"a": 1, "b": 2.5}, "kernel": "x"}')
    assert m.load_baseline(f) == {"a": 1.0, "b": 2.5}
    assert m.load_baseline(tmp_path / "nope.json") == {}
    f.write_text("not json")
    assert m.load_baseline(f) == {}


def test_committed_baseline_names_real_metrics():
    m = _mod()
    base = m.load_baseline(REPO / "bench-history" / "baseline.json")
    assert base["dma dma0chan0 copy iops"] == 13716.0
    assert base["dma dma0chan0 xor iops"] == 12743.0
    assert base["eth 10G host->box"] > 8000 and base["eth 1G host->box"] < 100


# ---- report on change ------------------------------------------------------


def test_tracker_reports_a_failure_once_then_clears():
    m = _mod()
    t = m.ChangeTracker(confirm=2)
    f = m.Finding("fail:x", "x broke")
    assert t.update([f]) == ([f], [])
    assert t.update([f]) == ([], [])  # persists: silent
    assert t.update([f]) == ([], [])
    assert t.update([]) == ([], ["fail:x"])
    assert t.update([f]) == ([f], [])  # back again: new news


def test_tracker_needs_consecutive_cycles_to_report_a_regression():
    m = _mod()
    t = m.ChangeTracker(confirm=2)
    r = m.Finding("regress:eth", "eth slow", hard=False)
    assert t.update([r]) == ([], [])
    assert t.update([]) == ([], [])  # one noisy cycle: forgotten
    assert t.update([r]) == ([], [])
    assert t.update([r]) == ([r], [])
    assert t.update([r]) == ([], [])


# ---- snapshot diffing ------------------------------------------------------


def test_compare_metrics_flags_drops_past_threshold_only():
    m = _mod()
    rows = m.compare_metrics({"a": 100.0, "b": 100.0, "c": 5.0}, {"a": 89.0, "b": 95.0})
    by = {r[0]: r for r in rows}
    assert by["a"][4] is True and by["b"][4] is False
    assert by["c"][2] is None and by["c"][4] is False  # missing in B: not a regression


# ---- idle detection --------------------------------------------------------

PROBE_IDLE = """@@now
1000000
@@load
0.23 0.06 0.02 1/156 2489
@@dmatest
N
@@procs
systemd
sshd
kworker/R-dm_bufio_cache
ps
@@sessions
ttyS0 999000
pts/0 998000
"""


def _probe(text=PROBE_IDLE):
    m = _mod()
    return m, m.parse_probe(text)


def test_probe_parses_and_an_idle_box_has_no_blockers():
    m, p = _probe()
    assert p["load1"] == 0.23 and p["dmatest"] == "N"
    assert p["sessions"] == [("ttyS0", 1000), ("pts/0", 2000)]
    assert m.box_busy_reasons(p) == []


def test_a_garbled_probe_is_none_not_idle():
    m = _mod()
    assert m.parse_probe("") is None
    assert m.parse_probe("@@now\nxx\n@@load\n0.1 0 0\n") is None


def test_load_dmatest_and_busy_processes_each_block():
    m, p = _probe()
    p["load1"] = 1.3
    assert any("load 1.30" in r for r in m.box_busy_reasons(p))
    assert m.box_busy_reasons(p, check_load=False) == []  # our own load mid-cycle
    p["dmatest"] = "Y"
    assert any("dmatest run" in r for r in m.box_busy_reasons(p, check_load=False))
    _, p = _probe(
        PROBE_IDLE.replace("sshd", "iperf3\nfio").replace("ps\n", "dma0chan0-copy\n")
    )
    why = " ".join(m.box_busy_reasons(p))
    assert "fio, iperf3" in why and "dmatest threads" in why


def test_a_recently_active_login_blocks_but_a_stale_one_does_not():
    m, p = _probe()
    p["sessions"] = [("pts/1", 30), ("ttyS0", 7200)]
    why = m.box_busy_reasons(p)
    assert len(why) == 1 and "pts/1" in why[0]
    assert m.box_busy_reasons(p, allow_sessions=True) == []


def test_local_work_blocks_but_our_own_idle_run_and_the_idle_server_do_not():
    m = _mod()
    ps = (
        "100 python3 scripts/bench-all.py --idle\n"
        "101 /usr/bin/python3 /repo/scripts/ssh-woomera.py -- uptime\n"
        "102 python3 dev.py publish-fedora\n"
        "103 iperf3 -c 192.168.25.42 -t 30\n"
        "104 iperf3 -s -p 5701\n"
    )
    why = m.local_busy_reasons(ps, "", set())
    assert any("publish-fedora" in w for w in why)
    assert any("iperf3 -c" in w for w in why)
    assert not any("bench-all" in w or "5701" in w for w in why)
    assert m.local_busy_reasons("102 python3 dev.py publish-fedora\n", "", {102}) == []
    assert m.local_busy_reasons("", "ESTAB 0 0 a:5701 b:4444\n", set())


def test_interruptible_wait_returns_early_when_stopped():
    import threading
    import time

    m = _mod()
    stop = threading.Event()
    stop.set()
    t0 = time.monotonic()
    m.interruptible_wait(stop, 60.0)
    assert time.monotonic() - t0 < 1.0
    m.interruptible_wait(threading.Event(), 0.0)  # zero wait: no loop


# ---- the idle policy, with fake collaborators -------------------------------


def _runner(probe_answers, reports, baseline=None, starved_after=3):
    m = _mod()
    probes = iter(probe_answers)
    suite_reports = iter(reports)
    emitted, logged = [], []

    def probe(check_load):
        return next(probes)

    def suite(gate):
        r = next(suite_reports)
        if isinstance(r, Exception):
            raise r
        return r

    run = m.IdleRunner(
        probe, suite, emitted.append, logged.append, baseline or {}, starved_after
    )
    return m, run, emitted, logged


def _report(m, checks, metrics=None):
    r = m.Report(echo=False)
    for name, ok, detail in checks:
        r.add(name, ok, detail)
    r.metrics.update(metrics or {})
    return r


def test_idle_runner_is_silent_when_everything_passes(capsys):
    m = _mod()
    ok = _report(
        m, [("dma copy+xor", True, "fine")], {"dma dma0chan0 xor iops": 12700.0}
    )
    m, run, emitted, logged = _runner(
        [[], []], [ok, ok], {"dma dma0chan0 xor iops": 12743.0}
    )
    assert run.step() == "ran" and run.step() == "ran"
    assert emitted == []
    assert capsys.readouterr().out == ""  # Report(echo=False) prints nothing


def test_idle_runner_skips_a_busy_box_without_running_the_suite():
    m, run, emitted, logged = _runner([["load 2.00 > 0.75"]], [])
    assert run.step() == "skipped"
    assert emitted == [] and "load 2.00" in logged[0]


def test_idle_runner_reports_a_failure_once_and_its_recovery():
    m = _mod()
    bad = _report(m, [("dma copy+xor", False, "xor0: 3 failures")])
    good = _report(m, [("dma copy+xor", True, "fine")])
    m, run, emitted, _ = _runner([[], [], [], []], [bad, bad, good, good])
    for _ in range(4):
        run.step()
    assert len(emitted) == 2
    assert emitted[0].startswith("FAIL dma copy+xor: xor0")
    assert emitted[1] == "RECOVERED fail:dma copy+xor"


def test_idle_runner_reports_a_confirmed_regression_not_a_blip():
    m = _mod()
    slow = _report(m, [("eth 10G host->box", True, "x")], {"eth 10G host->box": 5000.0})
    fine = _report(m, [("eth 10G host->box", True, "x")], {"eth 10G host->box": 8600.0})
    base = {"eth 10G host->box": 8590.0}
    m, run, emitted, _ = _runner([[]] * 4, [slow, fine, slow, slow], base)
    for _ in range(4):
        run.step()
    assert len(emitted) == 1 and emitted[0].startswith("REGRESSION eth 10G host->box")


def test_idle_runner_discards_an_interrupted_cycle():
    m = _mod()
    bad = _report(m, [("dma copy+xor", False, "boom")])
    m, run, emitted, logged = _runner(
        [[], []], [m.CycleInterrupted(["session on pts/0 active 3 s ago"]), bad]
    )
    assert run.step() == "interrupted"
    assert emitted == []  # no bogus result from a cycle that was fought over
    assert any("discarded" in line for line in logged)


def test_idle_runner_stops_on_a_wedge():
    m = _mod()
    wedged = _report(
        m,
        [("dma stop", False, "dmatest would not stop: engine presumed wedged, reboot")],
    )
    m, run, emitted, _ = _runner([[]], [wedged])
    assert run.step() == "wedged"
    assert any(e.startswith("WEDGED") for e in emitted)


def test_idle_runner_says_so_once_when_idle_is_never_reached():
    m, run, emitted, _ = _runner([["busy"]] * 5, [], starved_after=3)
    for _ in range(5):
        run.step()
    assert len(emitted) == 1 and emitted[0].startswith("STARVED not idle for 3")
