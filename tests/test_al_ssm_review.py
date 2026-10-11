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

import pytest

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


# ---- #300 model -------------------------------------------------------------


class Model:
    """enqueue: decide (chan lock) ... insert (backlog lock); worker cycle."""

    def __init__(self, drain_policy: str) -> None:
        self.policy = drain_policy  # "completed_only" (driver) | "if_backlog"
        self.pending = 0
        self.backlog = 0
        self.submitted_from_backlog = 0
        self.worker_scheduled = False

    def decide_full(self) -> bool:
        return self.pending >= MAX_IN_FLIGHT

    def insert_backlog(self) -> None:
        self.backlog += 1
        self.worker_scheduled = True  # al_ssm_schedule_poll()

    def worker_cycle(self, hw_done: int) -> bool:
        """Returns True if it reschedules itself."""
        completed = min(hw_done, self.pending)
        self.pending -= completed
        gate = completed > 0 if self.policy == "completed_only" else self.backlog > 0
        if gate:
            while self.backlog and self.pending < MAX_IN_FLIGHT:
                self.backlog -= 1
                self.pending += 1
                self.submitted_from_backlog += 1
        return self.pending + self.backlog > 0


def _lose_scenario(policy: str) -> Model:
    m = Model(policy)
    m.pending = MAX_IN_FLIGHT
    assert m.decide_full()  # enqueuer E decides ENOSPC, drops chan->lock
    # E is delayed; HW completes everything, worker cycle X runs on an empty backlog
    m.worker_cycle(hw_done=MAX_IN_FLIGHT)
    m.insert_backlog()  # E finally inserts and schedules the worker
    # worker cycles Y...: HW has nothing left to complete (hw_done = 0)
    for _ in range(100):
        if not m.worker_cycle(hw_done=0):
            break
    return m


def test_model_matches_driver_gate():
    body = _func_body("al_ssm_poll_completions")
    assert re.search(r"if \(completed > 0\)\s*al_ssm_drain_backlog", body), (
        "drain gate changed: update the #300 model and drop the xfail if fixed"
    )


@pytest.mark.parametrize(
    "policy",
    [
        pytest.param(
            "completed_only",
            marks=pytest.mark.xfail(strict=True, reason="#300 backlog request lost"),
        ),
        "if_backlog",
    ],
)
def test_backlogged_request_is_submitted(policy):
    m = _lose_scenario(policy)
    assert m.submitted_from_backlog == 1 and m.backlog == 0


# ---- #301 remove ordering ---------------------------------------------------


@pytest.mark.xfail(strict=True, reason="#301 buffers freed before engine quiesced")
def test_remove_quiesces_before_freeing_buffers():
    body = _func_body("al_ssm_pci_remove")
    reset = body.index("al_udma_q_reset")
    first_free = body.index("al_ssm_complete_one")
    assert reset < first_free


@pytest.mark.xfail(strict=True, reason="#301 lists walked without chan->lock")
def test_remove_walks_pending_under_lock():
    body = _func_body("al_ssm_pci_remove")
    walk = body.index("list_for_each_entry_safe(rctx, tmp, &dev->channel.pending")
    assert "spin_lock" in body[:walk]


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
