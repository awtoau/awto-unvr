# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Independent software model of the Alpine NAND controller's BCH ECC (#208).

Pure stdlib, no box access. Exists so a page written by `al_nand` can be
checked against something other than the controller that wrote it: the
`corrected_bits`/`ecc_failures` counters only say the controller agrees with
itself. Method and evidence: docs/nand-study.md section 4.

Model, derived from stock-written pages (64 of 64 codewords match):
  - binary BCH over GF(2^15), primitive polynomial x^15 + x + 1, t = 8
  - message = one 512-byte codeword, bytes MSB-first; parity = 120 bits
    (remainder of msg * x^120 mod g(x), g = lcm of minimal polynomials of a^1..a^16)
  - stored per codeword in a 16-byte slot as four little-endian 32-bit words;
    the top byte of the first word (slot byte 3) is NOT parity - it varies per
    codeword and is not modelled
  - OOB [0,4) stays 0xff; slots at 4 + 16*cw, cw 0..7. OOB [132,224) is 0xff
    on Linux-written pages and a constant stale pattern on U-Boot-written ones;
    it is not protected by anything and is reported, not failed
"""

from __future__ import annotations

from dataclasses import dataclass

M = 15
PRIM = (1 << 15) | 0b11  # x^15 + x + 1
T = 8
PARITY_BITS = 15 * T  # 120
CW_BYTES = 512
CW_PER_PAGE = 8
PAGE_BYTES = CW_BYTES * CW_PER_PAGE
OOB_BYTES = 224
SLOT_BYTES = 16
PARITY_OFFSET = 4
PARITY_END = PARITY_OFFSET + SLOT_BYTES * CW_PER_PAGE  # 132
_MASK = (1 << PARITY_BITS) - 1


def _gf_tables() -> tuple[list[int], list[int]]:
    n = (1 << M) - 1
    exp = [0] * (2 * n)
    log = [0] * (n + 1)
    x = 1
    for i in range(n):
        exp[i] = x
        log[x] = i
        x <<= 1
        if x >> M:
            x ^= PRIM
    for i in range(n, 2 * n):
        exp[i] = exp[i - n]
    return exp, log


def _polymul(a: list[int], b: list[int], exp: list[int], log: list[int]) -> list[int]:
    res = [0] * (len(a) + len(b) - 1)
    for i, ai in enumerate(a):
        if not ai:
            continue
        for j, bj in enumerate(b):
            if bj:
                res[i + j] ^= exp[log[ai] + log[bj]]
    return res


def _generator() -> int:
    """g(x) as an int, bit k = coefficient of x^k."""
    exp, log = _gf_tables()
    n = (1 << M) - 1
    seen: set[int] = set()
    g = [1]
    for i in range(1, 2 * T + 1):
        if i in seen:
            continue
        coset = []
        j = i
        while j not in coset:
            coset.append(j)
            j = (j * 2) % n
        seen.update(coset)
        mp = [1]
        for c in coset:
            mp = _polymul(mp, [exp[c], 1], exp, log)
        g = _polymul(g, mp, exp, log)
    assert all(c in (0, 1) for c in g), "generator not binary"
    return sum(1 << k for k, c in enumerate(g) if c)


GENERATOR = _generator()
assert GENERATOR.bit_length() - 1 == PARITY_BITS
_G_LOW = GENERATOR & _MASK


def _build_table() -> list[int]:
    table = []
    for b in range(256):
        reg = b << (PARITY_BITS - 8)
        for _ in range(8):
            top = (reg >> (PARITY_BITS - 1)) & 1
            reg = (reg << 1) & _MASK
            if top:
                reg ^= _G_LOW
        table.append(reg)
    return table


_TABLE = _build_table()


def parity(cw: bytes) -> int:
    """120-bit parity of one 512-byte codeword."""
    if len(cw) != CW_BYTES:
        raise ValueError(f"codeword is {CW_BYTES} bytes, got {len(cw)}")
    r = 0
    for b in cw:
        r = ((r << 8) & _MASK) ^ _TABLE[((r >> (PARITY_BITS - 8)) ^ b) & 0xFF]
    return r


def _impulse_parities() -> dict[int, int]:
    """parity delta for a single flipped data bit -> its bit index in the
    codeword (0 = last bit of the last byte)."""
    out: dict[int, int] = {}
    d = _G_LOW  # x^120 mod g
    for p in range(CW_BYTES * 8):
        out[d] = p
        top = (d >> (PARITY_BITS - 1)) & 1
        d = (d << 1) & _MASK
        if top:
            d ^= _G_LOW
    return out


_IMPULSE = _impulse_parities()


def stored_parity(slot: bytes) -> tuple[int, int]:
    """(120-bit parity, unmodelled top byte) from a 16-byte OOB slot."""
    if len(slot) != SLOT_BYTES:
        raise ValueError(f"slot is {SLOT_BYTES} bytes, got {len(slot)}")
    be = b"".join(
        int.from_bytes(slot[i : i + 4], "little").to_bytes(4, "big")
        for i in range(0, SLOT_BYTES, 4)
    )
    return int.from_bytes(be[1:], "big"), be[0]


def encode_slot(cw: bytes, pad: int = 0xFF) -> bytes:
    """16-byte OOB slot for a codeword (pad = the unmodelled top byte)."""
    be = bytes([pad]) + parity(cw).to_bytes(PARITY_BITS // 8, "big")
    return b"".join(
        int.from_bytes(be[i : i + 4], "big").to_bytes(4, "little")
        for i in range(0, SLOT_BYTES, 4)
    )


@dataclass
class CwResult:
    cw: int
    ok: bool
    note: str  # "" when ok, else what the mismatch looks like


def check_cw(cw_data: bytes, slot: bytes, index: int = 0) -> CwResult:
    want = parity(cw_data)
    got, _pad = stored_parity(slot)
    if want == got:
        return CwResult(index, True, "")
    delta = want ^ got
    if delta in _IMPULSE:
        p = _IMPULSE[delta]
        byte = CW_BYTES - 1 - p // 8
        return CwResult(
            index,
            False,
            f"1 data bit flipped (byte {byte} bit {p % 8}); ECC would correct",
        )
    if delta.bit_count() == 1:
        return CwResult(index, False, "1 parity bit flipped; ECC would correct")
    return CwResult(index, False, f"{delta.bit_count()} parity bits differ")


@dataclass
class PageResult:
    cws: list[CwResult]
    oob_head_ff: bool
    oob_tail_ff: bool  # informational: stock U-Boot leaves a stale pattern here

    @property
    def ok(self) -> bool:
        return self.oob_head_ff and all(c.ok for c in self.cws)


def check_page(raw: bytes) -> PageResult:
    """raw = 4096 data bytes + 224 OOB bytes, as `nanddump --noecc --oob` emits."""
    if len(raw) != PAGE_BYTES + OOB_BYTES:
        raise ValueError(f"raw page is {PAGE_BYTES + OOB_BYTES} bytes, got {len(raw)}")
    data, oob = raw[:PAGE_BYTES], raw[PAGE_BYTES:]
    cws = [
        check_cw(
            data[i * CW_BYTES : (i + 1) * CW_BYTES],
            oob[PARITY_OFFSET + i * SLOT_BYTES : PARITY_OFFSET + (i + 1) * SLOT_BYTES],
            i,
        )
        for i in range(CW_PER_PAGE)
    ]
    return PageResult(
        cws,
        all(b == 0xFF for b in oob[:PARITY_OFFSET]),
        all(b == 0xFF for b in oob[PARITY_END:]),
    )
