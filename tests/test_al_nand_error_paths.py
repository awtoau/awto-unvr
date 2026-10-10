# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""al_nand error-propagation invariants (#208, #273, #274).

The driver cannot run on the host, so these pin the SHAPE that makes a failure
unmissable, as source checks. They do not prove behaviour on the box; they
stop the shape being undone by a later edit.

Shape: void callbacks (cmd_ctrl/read_buf/read_byte/write_buf) only latch into
op_err via al_nand_err_set(); every int-returning hook reports it via
al_nand_err_report().
"""

from __future__ import annotations

import re
from pathlib import Path

SRC = (
    Path(__file__).resolve().parent.parent / "modules" / "al_nand" / "al_nand_main.c"
).read_text()


def body(name: str) -> str:
    """Text of the C function `name` (from its signature to the closing brace)."""
    m = re.search(rf"^(?:static )?[\w *]+\b{name}\(", SRC, re.M)
    assert m, f"function {name} not found"
    i = SRC.index("{", m.end())
    depth = 0
    for j in range(i, len(SRC)):
        depth += (SRC[j] == "{") - (SRC[j] == "}")
        if depth == 0:
            return SRC[m.start() : j + 1]
    raise AssertionError(f"unbalanced braces in {name}")


def test_old_write_err_is_gone():
    assert "write_err" not in SRC


def test_op_err_is_only_assigned_by_the_two_latch_helpers():
    stripped = SRC
    for fn in ("al_nand_err_set", "al_nand_err_report"):
        stripped = stripped.replace(body(fn), "")
    assert not re.search(r"->op_err\s*=[^=]", stripped)


def test_all_six_page_hooks_are_installed():
    for hook, fn in {
        "read_page": "al_nand_ecc_read_page",
        "write_page": "al_nand_ecc_write_page",
        "read_page_raw": "al_nand_ecc_read_page_raw",
        "write_page_raw": "al_nand_ecc_write_page_raw",
        "read_oob": "al_nand_ecc_read_oob",
        "write_oob": "al_nand_ecc_write_oob",
    }.items():
        assert re.search(rf"chip->ecc\.{hook}\s*=\s*{fn};", SRC), hook


def test_waitfunc_is_wrapped_and_reports():
    assert "chip->legacy.waitfunc = al_nand_waitfunc;" in SRC
    assert "al_nand_err_report" in body("al_nand_waitfunc")


def test_every_int_hook_reports_the_latch():
    for fn in (
        "al_nand_ecc_read_page",
        "al_nand_ecc_write_page",
        "al_nand_ecc_read_page_raw",
        "al_nand_ecc_read_oob",
        "al_nand_prog_page_finish",
    ):
        assert "al_nand_err_report" in body(fn), fn
    # raw/oob writes go through the shared finisher
    for fn in ("al_nand_ecc_write_page_raw", "al_nand_ecc_write_oob"):
        assert "al_nand_prog_page_finish" in body(fn), fn


def test_page_program_checks_the_latch_after_pageprog():
    # defect 1: the check must come AFTER nand_prog_page_end_op, not only before
    wp = body("al_nand_ecc_write_page")
    assert re.search(r"al_nand_err_report\([^;]*nand_prog_page_end_op", wp, re.S)


def test_void_callbacks_latch_instead_of_dropping():
    for fn in ("al_nand_cmd_ctrl", "al_nand_write_buf", "al_nand_read_words"):
        assert "al_nand_err_set" in body(fn), fn


def test_failed_read_is_poisoned_not_left_stale():
    rw = body("al_nand_read_words")
    assert "memset(buf, AL_NAND_READ_POISON" in rw


def test_read_byte_does_not_advance_the_cache_on_failure():
    rb = body("al_nand_read_byte")
    fail = rb.index("return AL_NAND_READ_POISON")
    assert fail < rb.index("cache_pos = 0"), "cache_pos set before the failure return"


def test_write_buf_has_the_word_alignment_guard():
    assert "WARN_ON(len & 3)" in body("al_nand_write_buf")


def test_corrected_page_reports_bitflips_to_mtd():
    rp = body("al_nand_ecc_read_page")
    assert "return chip->ecc.strength;" in rp


def test_probe_refuses_to_register_after_a_scan_error():
    pr = body("al_nand_probe")
    scan = pr.index("nand_scan(chip, 1)")
    assert "al_nand_err_report" in pr[scan : pr.index("mtd_device_register")]
