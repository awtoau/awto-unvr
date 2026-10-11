# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""al_eth source-shape invariants (#296-#299, #304, #305, #314).

al_eth_main.c cannot run on the host, so these pin the SHAPE that makes each
defect impossible, as checks on the source text (same approach as
tests/test_al_nand_error_paths.py). They stop a later edit undoing a fix; they
do not prove behaviour on the box. Behaviour is checked on hardware by
bench-all's health subtest (ethtool -S drop / refill counters, adaptive
moderation off) and by the kernel's own checks.

AL_ETH_MAIN_C=<path> points the checks at another copy of the file.
"""

from __future__ import annotations

import os
import re
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
MAIN_C = Path(os.environ.get("AL_ETH_MAIN_C", REPO / "modules/al_eth/al_eth_main.c"))
HAL_UDMA_H = REPO / "modules/al_eth/al_hal_udma.h"
SRC = MAIN_C.read_text()


def body(name: str) -> str:
    """Text of the C function `name`, signature to closing brace (not a prototype)."""
    pat = rf"^(?:{re.escape(name)}\(|[A-Za-z_][^;\n]*\b{re.escape(name)}\()"
    for m in re.finditer(pat, SRC, re.M):
        i = SRC.index("{", m.end())
        if ";" in SRC[m.end() : i]:
            continue
        depth = 0
        for j in range(i, len(SRC)):
            depth += (SRC[j] == "{") - (SRC[j] == "}")
            if depth == 0:
                return SRC[m.start() : j + 1]
    raise AssertionError(f"function {name} not found")


# ---- #304: adaptive interrupt moderation defaults off -------------------------


def test_adaptive_moderation_default_is_unconditionally_off():
    m = re.search(r"^#define AL_ETH_DEFAULT_ADAPTIVE_INT_MODERATION\s+(\w+)", SRC, re.M)
    assert m and m[1] == "AL_FALSE"
    window = SRC[max(0, m.start() - 400) : m.end() + 200]
    # a bare #ifdef falls back to the TRUE arm when the symbol is absent
    assert not re.search(r"^#(if|ifdef|else)\b", window, re.M)


# ---- #305 / #314: rx_poll leaves polling mode on every path -------------------


def test_rx_poll_never_schedules_itself():
    for fn in ("al_eth_rx_poll", "al_eth_rx_poll_finish"):
        assert "napi_schedule(" not in body(fn), fn


def test_rx_poll_finish_completes_and_unmasks_when_under_budget():
    b = body("al_eth_rx_poll_finish")
    # the completion must not sit behind the refill outcome
    assert re.search(r"if \(budget_left > 0\) \{\s*napi_complete\(napi\);", b)
    assert "unmask_reg_offset" in b


def test_rx_poll_has_one_work_return_and_hands_off_to_finish():
    b = body("al_eth_rx_poll")
    returns = re.findall(r"\breturn\b[^;]*;", b)
    assert returns == ["return 0;", "return work_done;"], returns
    assert b.index("al_eth_rx_poll_finish(") < b.rindex("return work_done;")


def test_rx_poll_ignores_a_zero_budget():
    # netpoll polls with budget 0 for TX only; `while (budget)` would wrap (#314)
    b = body("al_eth_rx_poll")
    assert re.search(r"if \(unlikely\(budget <= 0\)\)\s*return 0;", b)
    assert b.index("budget <= 0") < b.index("al_eth_pkt_rx(")


def test_empty_rx_ring_watchdog_is_wired():
    assert "napi_schedule(rx_ring->napi)" in body("al_eth_check_for_empty_rx_ring")
    assert "al_eth_check_for_empty_rx_ring(adapter)" in body("al_eth_rx_watchdog")
    assert "timer_shutdown_sync(&adapter->rx_watchdog)" in body("al_eth_down")
    up = body("al_eth_up")
    assert up.index("adapter->up = true") < up.index("mod_timer(&adapter->rx_watchdog")


# ---- #299: probe -----------------------------------------------------------------


def test_register_netdev_is_the_last_fallible_probe_step():
    b = body("al_eth_probe")
    after = b[b.index("rc = register_netdev(") :]
    after = after[after.index("}") :]  # past the register failure branch
    tail = after[: after.index("return 0;")]
    assert not re.search(r"\bgoto\b|\breturn\b", tail), tail
    assert "al_eth_sysfs_init(" in b[: b.index("rc = register_netdev(")]


def test_probe_error_tail_unwinds_everything_it_created():
    b = body("al_eth_probe")
    tail = b[b.index("err_register:") :]
    assert tail.index("al_eth_sysfs_terminate(") < tail.index(
        "al_eth_phylink_teardown("
    )
    assert tail.index("al_eth_phylink_teardown(") < tail.index("free_netdev(netdev)")
    assert tail.index("err_hw_init:") > tail.index("al_eth_sysfs_terminate(")


# ---- #298: UDMA HAL errors are not discarded -------------------------------------


def test_udma_hal_header_marks_must_check():
    hdr = HAL_UDMA_H.read_text()
    for fn in (
        "al_udma_init",
        "al_udma_q_init",
        "al_udma_q_reset",
        "al_udma_q_handle_get",
        "al_udma_state_set",
    ):
        assert re.search(rf"__must_check\s+int\s+{fn}\(", hdr), fn


def test_no_discarded_udma_results_in_the_driver():
    for fn in ("al_udma_q_reset", "al_udma_q_handle_get", "al_udma_init"):
        assert not re.search(rf"^\s*{fn}\(", SRC, re.M), f"{fn} result discarded"


def test_probe_checks_the_hal_setup_calls():
    b = body("al_eth_probe")
    assert re.search(r"rc = al_eth_hal_adapter_init\(adapter\);\s*if \(rc\)", b)
    assert re.search(r"rc = al_eth_init_rings\(adapter\);\s*if \(rc\)", b)


def test_queue_reset_failure_reaches_hw_stop():
    assert re.search(
        r"if \(al_eth_udma_queues_reset_all\(adapter\)\)\s*rc = -EIO;",
        body("al_eth_hw_stop"),
    )
    assert "return rc;" in body("al_eth_udma_queues_reset_all")


# ---- #297: TX checksum offload ---------------------------------------------------


def test_features_check_is_wired_and_strips_csum_and_gso():
    ops = re.search(r"struct net_device_ops al_eth_netdev_ops = \{.*?\};", SRC, re.S)
    assert ops and ".ndo_features_check" in ops[0]
    assert "~(NETIF_F_CSUM_MASK | NETIF_F_GSO_MASK)" in body("al_eth_features_check")


def test_tx_csum_falls_back_before_setting_hw_flags():
    b = body("al_eth_tx_csum")
    assert "skb_checksum_help(skb)" in b
    assert b.index("al_eth_tx_l4_proto(skb)") < b.index("hal_pkt->flags |=")
    assert 'pr_warn("%s: l4 protocol' not in b


# ---- #296 / #313: drops are counted ---------------------------------------------


def test_every_tx_drop_in_start_xmit_is_counted():
    b = body("al_eth_start_xmit")
    for reason in ("pad_err", "dma_mapping_err", "csum_err", "frag_err"):
        assert f"al_eth_tx_drop(tx_ring, &tx_ring->tx_stats.{reason})" in b, reason
    # the three error labels each count before they free
    for label in ("tx_csum_error:", "dma_error:"):
        assert "al_eth_tx_drop(" in b[b.index(label) : b.index(label) + 400], label
    head = b[b.index("dma_mapping_error(tx_ring->dev, dma)") :]
    assert "al_eth_tx_drop(" in head[: head.index("return")]


def test_freed_skb_is_not_left_in_the_tx_slot():
    b = body("al_eth_start_xmit")
    for m in re.finditer(r"dev_kfree_skb\(skb\);", b):
        assert "tx_info->skb = NULL;" in b[max(0, m.start() - 900) : m.start()]


def test_head_map_failure_clears_the_slot_before_freeing():
    b = body("al_eth_start_xmit")
    assert re.search(
        r"tx_info->skb = NULL;\s*dev_kfree_skb\(skb\);\s*al_eth_tx_drop", b
    )


def test_netdev_stats_use_the_driver_counters():
    b = body("al_eth_get_stats64")
    assert "al_eth_ring_drops(" in b
    assert not re.search(r"rx_dropped\s*=\s*0", b)


def test_rx_error_completions_are_counted_not_received():
    b = body("al_eth_rx_poll")
    assert "rx_dropped++" in b
    assert "rx_stats.packets += work_done - rx_dropped" in b
    assert "rx_stats.dropped += rx_dropped" in b


def test_drop_counters_are_exposed_through_ethtool():
    for name in ("dropped", "pad_err", "dma_mapping_err", "csum_err", "frag_err"):
        assert f"AL_ETH_ETHTOOL_STAT_TX_ENTRY({name})" in SRC, name
    for name in ("dropped", "refill_fail", "empty_ring"):
        assert f"AL_ETH_ETHTOOL_STAT_RX_ENTRY({name})" in SRC, name
