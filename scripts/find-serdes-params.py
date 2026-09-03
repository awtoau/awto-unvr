#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Find `struct al_serdes_adv_tx_params` tables inside vendor binaries.

Why: #121's root cause was the 10G optic TX equaliser tap `c_plus_1` sitting at
the direct-attach-copper value. To know whether that is an original-SDK defect or
something this port introduced, we need the values the VENDOR's own shipping
firmware uses - which means reading them out of stock images, not source.

The struct is a statically initialised global, so it lands in .data verbatim:

    al_bool  override;            int, 4 bytes LE
    uint8_t  amp;                 1 = 952mVdiff-pkpk .. 7 = 1331mV
    uint8_t  total_driver_units;
    uint8_t  c_plus_1;            first POST-cursor tap
    uint8_t  c_plus_2;            second post-cursor tap
    uint8_t  c_minus_1;           first PRE-cursor tap
    uint8_t  slew_rate;           0=31ps 1=33ps 2=68ps 3=170ps

=> 01 00 00 00 <amp> <tdu> <c+1> <c+2> <c-1> <slew>, padded to 12 bytes.

Known-good reference values from the vendor source (al_init_eth_lm.c):
    da_tx_params      tdu=0x13 c+1=0x2 c-1=0x2   direct-attach copper
    tx_params_br410   tdu=0x1b c+1=0x6 c-1=0     "calibrated for br410 on EVP only"
    optic_tx_params   tdu=0x13 c+1=0x2 c-1=0     the uncalibrated catch-all (#121)

Compressed payloads (uImage/vmlinuz/squashfs) are searched too: every embedded
gzip/xz/lz4 stream is inflated and scanned as well as the raw bytes.

Usage:
    ./scripts/find-serdes-params.py <file> [<file> ...]
    ./scripts/find-serdes-params.py /path/to/stock/*.img
"""

from __future__ import annotations

import bz2
import gzip
import lzma
import sys
import zlib
from pathlib import Path

# Plausibility bounds. amp and slew_rate are hard-bounded by their field widths
# (3-bit and 2-bit); tdu is the driver-unit pool, ~0x10-0x20 on every known
# Alpine table. Tighter bounds mean fewer false hits in a 1GB rootfs.
AMP_RANGE = range(1, 8)
TDU_RANGE = range(0x08, 0x21)
TAP_MAX = 0x1F
SLEW_MAX = 3

SIG = b"\x01\x00\x00\x00"

REFERENCE = {
    (0x13, 0x2, 0x2): "da_tx_params (direct-attach copper)",
    (0x1B, 0x6, 0x0): "tx_params_br410 (optic w/ BR410 retimer, EVP-calibrated)",
    (0x13, 0x2, 0x0): "optic_tx_params AS SHIPPED - the #121 defect",
    (0x13, 0x4, 0x0): "optic_tx_params FIXED (#121)",
}


def candidates(buf: bytes, origin: str) -> list[dict]:
    """Scan buf for plausible al_serdes_adv_tx_params images."""
    out = []
    pos = 0
    while True:
        i = buf.find(SIG, pos)
        if i < 0:
            return out
        pos = i + 4
        if i + 10 > len(buf):
            return out
        amp, tdu, cp1, cp2, cm1, slew = buf[i + 4 : i + 10]
        if (
            amp in AMP_RANGE
            and tdu in TDU_RANGE
            and cp1 <= TAP_MAX
            and cp2 <= TAP_MAX
            and cm1 <= TAP_MAX
            and slew <= SLEW_MAX
        ):
            out.append(
                {
                    "origin": origin,
                    "offset": i,
                    "amp": amp,
                    "tdu": tdu,
                    "c_plus_1": cp1,
                    "c_plus_2": cp2,
                    "c_minus_1": cm1,
                    "slew": slew,
                }
            )


def decompressed_views(buf: bytes) -> list[tuple[str, bytes]]:
    """Yield (label, bytes) for every embedded compressed stream we can inflate.

    Vendor kernels and squashfs images hold the module payload compressed, so a
    raw scan alone would report nothing and be mistaken for absence.
    """
    views: list[tuple[str, bytes]] = []
    magics = [
        (b"\x1f\x8b\x08", "gzip", lambda b: gzip.decompress(b)),
        (b"\xfd7zXZ\x00", "xz", lambda b: lzma.decompress(b)),
        (b"BZh", "bzip2", lambda b: bz2.decompress(b)),
    ]
    for magic, name, fn in magics:
        pos = 0
        n = 0
        while n < 64:  # bound the work on a large image
            i = buf.find(magic, pos)
            if i < 0:
                break
            pos = i + 1
            n += 1
            try:
                data = fn(buf[i:])
            except (OSError, EOFError, ValueError, lzma.LZMAError, zlib.error):
                # A magic-byte match that is not a real stream, or a stream that
                # runs past the end of the image. Expected constantly when
                # scanning raw flash; only successful inflations matter.
                continue
            if len(data) > 4096:
                views.append((f"{name}@0x{i:x}", data))
    return views


def scan_file(path: Path) -> list[dict]:
    buf = path.read_bytes()
    found = candidates(buf, "raw")
    for label, data in decompressed_views(buf):
        found += candidates(data, label)
    return found


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 2

    any_found = False
    for arg in sys.argv[1:]:
        p = Path(arg)
        if not p.is_file():
            continue
        try:
            hits = scan_file(p)
        except Exception as exc:  # noqa: BLE001 - report and continue over a file set
            print(f"{p.name}: SCAN FAILED ({exc})")
            continue
        if not hits:
            continue
        any_found = True
        print(f"\n=== {p.name} ({p.stat().st_size} bytes)")
        seen = set()
        for h in hits:
            key = (h["tdu"], h["c_plus_1"], h["c_minus_1"], h["amp"], h["slew"])
            if key in seen:
                continue
            seen.add(key)
            note = REFERENCE.get((h["tdu"], h["c_plus_1"], h["c_minus_1"]), "")
            print(
                f"  {h['origin']:>14} +0x{h['offset']:08x}  "
                f"amp={h['amp']} tdu=0x{h['tdu']:02x} "
                f"c+1={h['c_plus_1']} c+2={h['c_plus_2']} c-1={h['c_minus_1']} "
                f"slew={h['slew']}   {note}"
            )

    if not any_found:
        print("no candidate tx_params tables found")
    return 0


if __name__ == "__main__":
    sys.exit(main())
