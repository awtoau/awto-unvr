# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""scripts/nand_bch.py against codewords the stock firmware wrote (docs/nand-study.md s4)."""

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "scripts"))

import nand_bch as nb  # noqa: E402

VECTORS = json.loads(
    (Path(__file__).parent / "data" / "nand-stock-cw.json").read_text()
)["vectors"]


def test_generator_degree():
    assert nb.GENERATOR.bit_length() - 1 == 120


def test_stock_parity_matches():
    for v in VECTORS:
        assert nb.check_cw(bytes.fromhex(v["cw"]), bytes.fromhex(v["slot"])).ok


def test_single_data_bit_flip_is_located():
    v = VECTORS[0]
    cw = bytearray.fromhex(v["cw"])
    cw[207] ^= 0x01
    r = nb.check_cw(bytes(cw), bytes.fromhex(v["slot"]))
    assert not r.ok
    assert "byte 207 bit 0" in r.note


def test_parity_garbage_is_not_a_single_flip():
    v = VECTORS[0]
    slot = bytearray.fromhex(v["slot"])
    slot[5] ^= 0xFF
    r = nb.check_cw(bytes.fromhex(v["cw"]), bytes(slot))
    assert not r.ok
    assert "parity bits differ" in r.note


def test_encode_roundtrip_keeps_pad_byte_out_of_the_check():
    cw = bytes(range(256)) * 2
    for pad in (0x00, 0xFF, 0xA5):
        assert nb.check_cw(cw, nb.encode_slot(cw, pad)).ok


def test_page_layout_checks():
    cw = bytes(range(256)) * 2
    data = cw * 8
    oob = bytearray(b"\xff" * 224)
    for i in range(8):
        oob[4 + 16 * i : 20 + 16 * i] = nb.encode_slot(cw)
    assert nb.check_page(data + bytes(oob)).ok
    oob[140] = 0x00  # tail is informational: reported, not failed
    r = nb.check_page(data + bytes(oob))
    assert r.ok and not r.oob_tail_ff
    oob[1] = 0x00  # head must stay 0xff
    assert not nb.check_page(data + bytes(oob)).ok
