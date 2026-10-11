# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""scripts/bench-all.py pure logic, offline: health, baseline, idle, snapshots.

Each test feeds the function the text the box would have produced; nothing
here opens a socket.
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
