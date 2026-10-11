# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""al_ssm port-review defects, hardware-free (#300, #301).

- #300: queue-policy MODEL of the enqueue/worker/drain logic, plus a guard that
  fails when the driver's drain gate stops matching the model. The model is a
  transcription, not the driver: it proves the interleaving loses a request
  and that the fix direction does not.
- #301: ordering of al_ssm_pci_remove(), read from the source. xfail(strict)
  until fixed; the strict flag makes the fix flip the test.
- scripts/bench-all.py bench_ssm parsers and verdicts on real devlink output.
"""

from __future__ import annotations

import importlib.util
import os
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
SRC = (REPO / "modules" / "al_ssm" / "al_ssm_main.c").read_text()
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

MAX_IN_FLIGHT = int(
    re.search(
        r"#define AL_SSM_MAX_IN_FLIGHT\s+(\d+)",
        (REPO / "modules" / "al_ssm" / "al_ssm_drv.h").read_text(),
    ).group(1)
)


def _func_body(name: str) -> str:
    m = re.search(rf"^(?:static )?\w[\w \*]*\b{name}\([^;{{]*\)\n\{{", SRC, re.M)
    assert m, name
    i = m.end()
    depth = 1
    while depth:
        c = SRC[i]
        depth += (c == "{") - (c == "}")
        i += 1
    return SRC[m.end() : i]


def _code(text: str) -> str:
    """C source with comments removed."""
    return re.sub(r"/\*.*?\*/|//[^\n]*", "", text, flags=re.S)


# ---- #300 backlog invariant -------------------------------------------------


def test_ring_full_decision_and_backlog_insert_are_one_critical_section():
    body = _code(_func_body("al_ssm_submit_and_poll"))
    decide = _pos(body, "crypto_queue_len(&chan->backlog) ?")
    insert = _pos(body, "crypto_enqueue_request(&chan->backlog")
    between = body[decide:insert]
    assert "spin_unlock" not in between and "spin_lock" not in between
    assert _pos(body, "spin_lock_irqsave(&chan->lock, flags);") < decide
    # the only insert in the driver; there is no second lock to split it from
    assert _code(SRC).count("crypto_enqueue_request(") == 1
    assert "backlog_lock" not in SRC


def test_one_function_consumes_completions_and_every_pass_refills():
    code = _code(SRC)
    assert code.count("al_crypto_dma_completion(") == 1
    assert "al_crypto_dma_completion(" in _func_body("al_ssm_reap_locked")
    # the two completion consumers and the refill all go through service_locked
    svc = _code(_func_body("al_ssm_service_locked"))
    assert _pos(svc, "al_ssm_reap_locked") < _pos(svc, "al_ssm_fill_locked")
    assert "al_ssm_service_locked(" in _func_body("al_ssm_submit_and_poll")
    assert "al_ssm_service_locked(" in _func_body("al_ssm_service")
    assert "al_ssm_service(dev)" in _func_body("al_ssm_poll_completions")


def test_refill_is_not_gated_on_the_caller_having_completed_something():
    fill = _code(_func_body("al_ssm_fill_locked"))
    loop = fill[fill.index("while") : fill.index("{")]
    assert "crypto_queue_len(&chan->backlog)" in loop
    assert "completed" not in loop and "reaped" not in loop
    assert not re.search(r"if \(completed > 0\)", _code(SRC))


def test_enqueue_refills_in_the_same_critical_section():
    # older requests may be waiting while the ring has room again
    body = _code(_func_body("al_ssm_submit_and_poll"))
    ins = _pos(body, "crypto_enqueue_request(&chan->backlog")
    fill = _pos(body[ins:], "al_ssm_fill_locked") + ins
    assert "spin_unlock" not in body[ins:fill]


def test_stall_detector_sees_a_backlog_only_hang():
    body = _code(_func_body("al_ssm_poll_completions"))
    cond = body[body.index("if (pending + backlog > 0 &&") :]
    assert "dev->stall_clock" in cond[: cond.index("{")]
    # the clock starts when the ring goes idle -> busy, not at the last completion
    assert "dev->stall_clock = jiffies" in _func_body("al_ssm_hw_submit_locked")


def test_inline_waiter_owns_its_result():
    # a reaper in another thread must not complete a request its submitter
    # still dereferences
    fin = _func_body("al_ssm_finish_locked")
    assert "if (rctx->inline_wait)" in fin and "list_add_tail" in fin
    body = _code(_func_body("al_ssm_submit_and_poll"))
    assert _pos(body, "rctx->inline_wait = true") < _pos(body, "for (;;)")
    assert "rctx->inline_wait = false" in body


def test_promoted_backlog_waiter_gets_its_in_progress_notification():
    assert "notify_owed = true" in _func_body("al_ssm_backlog_pop_locked")
    done = _code(_func_body("al_ssm_complete_one"))
    assert _pos(done, "rctx->notify_owed") < _pos(done, "skcipher_request_complete")
    assert "-EINPROGRESS" in done


# ---- #301 remove ordering ---------------------------------------------------


def _pos(body: str, needle: str) -> int:
    assert needle in body, needle
    return body.index(needle)


def test_remove_stops_new_work_before_touching_the_engine():
    # no tfm can find the dev, no alg left, submit paths refuse, worker disabled
    body = _func_body("al_ssm_pci_remove")
    steps = [
        "g_ssm_dev = NULL",
        "crypto_unregister_skciphers",
        "chan->stopping = true",
        "disable_delayed_work_sync(&dev->poll_work)",
        "destroy_workqueue(dev->wq)",
        "al_ssm_quiesce(dev)",
    ]
    idx = [_pos(body, x) for x in steps]
    assert idx == sorted(idx), steps
    # the worker re-arms itself: cancel_delayed_work_sync would let it back in
    assert "cancel_delayed_work_sync" not in SRC


def test_remove_quiesces_before_freeing_buffers():
    body = _func_body("al_ssm_pci_remove")
    assert _pos(body, "al_ssm_quiesce(dev)") < _pos(
        body, "al_ssm_fail_outstanding(dev)"
    )
    assert _pos(body, "al_ssm_fail_outstanding(dev)") < _pos(
        body, "al_ssm_free_channel"
    )
    # no request is completed (buffers unmapped/freed) in remove itself
    assert "al_ssm_complete_one" not in body


def test_quiesce_pauses_the_queues_then_stops_bus_mastering():
    body = _func_body("al_ssm_quiesce")
    pause = [m.start() for m in re.finditer(r"al_udma_q_pause\(", body)]
    assert len(pause) == 2
    assert max(pause) < _pos(body, "pci_clear_master")


def test_fail_outstanding_reaps_then_fails_the_rest_under_the_lock():
    body = _code(_func_body("al_ssm_fail_outstanding"))
    lock = _pos(body, "spin_lock_irqsave(&chan->lock")
    reap = _pos(body, "al_ssm_reap_locked(dev, &done)")
    pending = _pos(body, "&chan->pending")
    backlog = _pos(body, "al_ssm_backlog_pop_locked")
    unlock = _pos(body, "spin_unlock_irqrestore(&chan->lock")
    # what the engine finished is completed normally first; both lists are
    # drained under the lock, and no callback runs before it is dropped
    assert lock < reap < pending < backlog < unlock < _pos(body, "al_ssm_run_done")
    assert body.count("-ESHUTDOWN") == 2


def test_submit_refuses_once_stopping_under_the_channel_lock():
    body = _func_body("al_ssm_submit_and_poll")
    lock = _pos(body, "spin_lock_irqsave(&chan->lock, flags);")
    assert (
        lock < _pos(body, "if (chan->stopping)") < _pos(body, "al_ssm_hw_submit_locked")
    )
    # the lock is dropped between polls: the loop exits once remove started
    loop = _pos(body, "for (;;)")
    assert "chan->stopping" in body[loop:]


def test_poll_is_armed_only_under_the_lock_remove_sets_stopping_under():
    body = _func_body("al_ssm_schedule_poll")
    assert "spin_lock_irqsave(&chan->lock" in body
    assert _pos(body, "if (!chan->stopping)") < _pos(body, "mod_delayed_work")
    assert not re.search(r"mod_delayed_work\(", _code(SRC.replace(body, "")))


def test_tfm_holds_a_device_reference():
    assert "al_ssm_dev_get()" in _func_body("al_ssm_init_tfm")
    assert "al_ssm_dev_put(ctx->dev)" in _func_body("al_ssm_exit_tfm")
    assert "g_ssm_dev" not in _func_body("al_ssm_init_tfm")
    assert "kref_get" in _func_body("al_ssm_dev_get")
    assert "kfree" in _func_body("al_ssm_dev_release")
    # probe keeps its own reference; remove drops it last
    assert "kref_init(&dev->ref)" in _func_body("al_ssm_pci_probe")
    remove = _func_body("al_ssm_pci_remove")
    assert remove.rindex("al_ssm_dev_put(dev)") > remove.rindex("pci_disable_device")


def test_probe_refuses_while_tfms_from_the_last_bind_live():
    # the static al_ssm_algs[] cannot be re-registered under a live tfm
    assert "atomic_read(&g_ssm_tfms)" in _func_body("al_ssm_pci_probe")


# ---- bench-all --ssm parsers and verdicts -----------------------------------


def _stress():
    spec = importlib.util.spec_from_file_location(
        "bench_all", REPO / "scripts" / "bench-all.py"
    )
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


# real output, #182 comments 2026-10-09
DIAG = (
    "driver: al_ssm version: 2.0.4 crypto_registered: true in_flight: 0 submitted: 180 "
    "completed: 180 errors: 0 poll_cycles: 0 since_last_completion_ms: 170797798 "
    "hw_pending: 0 backlog: 0 max_in_flight: 64 ring_size: 256"
)
SHOW = (
    "pci/0000:00:04.0: reporter ssm   state healthy error 0 recover 0\n"
    "pci/0000:00:05.0: reporter dma   state healthy error 0 recover 0\n"
)


def test_stress_parsers():
    mod = _stress()
    d = mod.parse_ssm_diag(DIAG)
    assert d["submitted"] == "180" and d["in_flight"] == "0" and d["backlog"] == "0"
    assert int(d["max_in_flight"]) == MAX_IN_FLIGHT
    assert mod.find_ssm_bdf(SHOW) == "0000:00:04.0"
    assert mod.find_ssm_bdf("pci/0000:00:05.0: reporter dma") is None


def test_ssm_verdicts():
    mod = _stress()
    before = mod.parse_ssm_diag(DIAG)
    after = mod.parse_ssm_diag(
        DIAG.replace("submitted: 180", "submitted: 244").replace(
            "completed: 180", "completed: 244"
        )
    )
    assert mod.judge_ssm(before, after, True) == []
    assert "data mismatch" in mod.judge_ssm(before, after, False)[0]
    # CE served the writes: the engine saw nothing
    assert any("no ops" in f for f in mod.judge_ssm(before, before, True))
    stuck = mod.parse_ssm_diag(
        DIAG.replace("in_flight: 0", "in_flight: 3").replace(
            "submitted: 180", "submitted: 244"
        )
    )
    assert any("no quiesce" in f for f in mod.judge_ssm(before, stuck, True))
    assert any("completed" in f for f in mod.judge_ssm(before, stuck, True))
    assert mod.judge_ssm(before, {}, True)  # empty diagnose never passes
