#!/usr/bin/env python3
"""Decode the 0x57 EEPROM *preload* region (0x000-0x2e3) — the part before the DDR records.

Complements scripts/decode-ddr-records.py, which decodes the DDR records at base 0x400.
This one covers everything below that, previously unattributed:

  0x000        12-byte "opening encapsulation" marker
  0x00c-0x0d7  17 preload register-write records {value:LE32, addr:LE32, pad:LE32}
  0x0d8        12-byte "closing encapsulation" marker (same pattern)
  0x0e4-0x1e3  RSA-2048 modulus  (0x100 B)
  0x1e4-0x2e3  RSA-2048 signature (0x100 B)
  0x2e4-0x3ff  zero fill
  0x600-0x1fff erased (0xff)

Format is from al_boot `eeprom_preload_parser` (FUN_01003230,
docs/nor-reference/preboot-alboot-decompiled.c:1857): it walks the EEPROM in
AL_I2C_TAR_10BIT_ADDR_SHIFT(=0xc)-byte records with a 2-byte offset, compares each to a
12-byte marker, and on the closing marker reads 0x100 modulus then 0x100 signature.
Ghidra names the 0xc literal as an I2C symbol; it is a plain 12.

Offline only: reads the .bin, does not touch hardware.
Log -> tmp/logs/decode-eeprom-preload.log

Copyright (c) 2026 Awto / Daniel Tyrrell
SPDX-License-Identifier: GPL-2.0-or-later
"""

import os
import struct
from pathlib import Path

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(REPO, "docs/nor-reference/ddr-config-eeprom-0x57-8k.bin")
LOG = os.path.join(REPO, "tmp/logs/decode-eeprom-preload.log")
os.makedirs(os.path.dirname(LOG), exist_ok=True)
Path(LOG).write_text("")

REC = 12  # AL_I2C_TAR_10BIT_ADDR_SHIFT in the decompile = plain 12
SRAM_DEV_INFO = 0xFBFF4100  # al_hal_iomap.h SRAM_DEV_INFO_ADDRESS


def out(*a):
    s = " ".join(str(x) for x in a)
    print(s)
    with open(LOG, "a") as f:
        f.write(s + "\n")


# Register annotations. Bases from al_hal_iomap.h / the preboot reverse
# (docs/ddr-config-reverse.md, docs/preboot-decompile.md).
REGS = {
    0xFD8B400C: "SGPO @0xfd8b4000 +0x0c",
    0xFD8A8118: "PBS regfile @0xfd8a8000 +0x118 (boot_strap is +0x110)",
    0xFD8A811C: "PBS regfile @0xfd8a8000 +0x11c",
    0xF00700A4: "NB service @0xf0070000 +0xa4 (north-bridge, DDR side)",
    0xFD860C00: "NB PLL @0xfd860c00 +0x00 — the DDR clock PLL (field layout not decoded)",
    0xFD860C04: "NB PLL @0xfd860c00 +0x04 (field layout not decoded)",
    0xFD860C20: "NB PLL @0xfd860c00 +0x20 (field layout not decoded)",
}


def main() -> int:
    d = Path(BIN).read_bytes()
    out("=" * 72)
    out(f"EEPROM 0x57 preload region  ({BIN.split('/')[-1]}, {len(d)} B)")
    out("=" * 72)

    marker = d[0:REC]
    hits = [i for i in range(0, 0x400, REC) if d[i : i + REC] == marker]
    out(f"\nencapsulation marker = {marker.hex(' ')}")
    out(f"  found at: {', '.join(f'0x{i:03x}' for i in hits)}")
    if len(hits) < 2:
        out("  !! expected an opening AND a closing marker — layout not as decoded")
        return 1
    open_off, close_off = hits[0], hits[1]
    blob = close_off + REC
    out(f"  opening @0x{open_off:03x}, closing @0x{close_off:03x}")
    out(f"  => modulus @0x{blob:03x}, signature @0x{blob + 0x100:03x}")

    # ------------------------------------------------------------ preload writes
    out("\n" + "=" * 72)
    out("preload register writes  {value:LE32, addr:LE32, pad:LE32}")
    out("=" * 72)
    dev_info = bytearray(0x20)
    n = 0
    for off in range(open_off + REC, close_off, REC):
        val, addr, pad = struct.unpack("<III", d[off : off + REC])
        n += 1
        note = REGS.get(addr, "")
        if SRAM_DEV_INFO <= addr < SRAM_DEV_INFO + 0x20:
            idx = addr - SRAM_DEV_INFO
            dev_info[idx : idx + 4] = struct.pack("<I", val)
            note = f"SRAM DEV_INFO +0x{idx:02x}"
        if not note and not (0xF0000000 <= addr <= 0xFE000000):
            note = "?? not a known MMIO/SRAM address"
        flag = "" if pad == 0 else f"  pad=0x{pad:08x}"
        out(f"  {off:#06x}  [0x{addr:08x}] = 0x{val:08x}   {note}{flag}")
    out(f"\n  {n} records")
    out(
        "\n  al_boot's eeprom_preload_parser only SCANS these records to find the closing"
    )
    out("  marker (and thus the RSA blob offset) — it never applies the writes. The")
    out(
        "  targets (NB PLL, PBS regfile, SRAM DEV_INFO) are all set before al_boot runs,"
    )
    out(
        "  and DEV_INFO must exist before the S2 reads its record base, so the applier is"
    )
    out(
        "  the mask BootROM. Not proven from the carved blobs — the BootROM is not in NOR."
    )

    # ------------------------------------------------------------ DEV_INFO image
    out("\n" + "=" * 72)
    out(f"reconstructed SRAM DEV_INFO image @0x{SRAM_DEV_INFO:08x}")
    out("=" * 72)
    out(f"  {bytes(dev_info).hex(' ')}")
    # field names: u-boot board/annapurna-labs/alpine_ubnt/dev_info_layout.h
    dev_id = dev_info[0]
    lsb, msb = dev_info[10], dev_info[11]
    early = lsb | (msb << 8)
    out(
        f"\n  DEV_INFO_DEV_ID_0 [0x00]        = 0x{dev_id:02x}  ({'nonzero' if dev_id else 'zero'})"
    )
    out(
        f"  DEV_INFO_EARLY_INIT_ADDR  [10/11] = 0x{lsb:02x} 0x{msb:02x} -> 0x{early:04x}"
    )
    if dev_id:
        out(
            f"  => S2 record base = 0x{early:04x}, taken from DEV_INFO (dev_id nonzero)."
        )
        out("     Equals the 0x400 hardcoded default, so the two paths agree — but the")
        out("     value is now confirmed BY DATA, not assumed.")
    else:
        out("  => S2 record base = 0x0400 (hardcoded default; DEV_INFO[0]==0)")
    rsvd = dev_info[12]
    out(
        f"  DEV_INFO_RSVD [12] = 0x{rsvd:02x}  xmodem_load={rsvd & 1}  spi_load={(rsvd >> 1) & 1}"
    )

    tag = bytes(dev_info[0x14:0x20])
    printable = all(0x20 <= c < 0x7F for c in tag)
    out(f"  [0x14..0x1f] = {tag!r}{'  (ASCII revision tag)' if printable else ''}")

    # caps bitfields decoded by al_boot stg3_board_init (:900-911)
    out(
        "\n  caps bitfields as read by stg3_board_init (preboot-alboot-decompiled.c:900):"
    )
    b7, b8, b9, ba = dev_info[7], dev_info[8], dev_info[9], dev_info[0x0A]
    for expr, v in (
        ("byte[0x07] >> 6", b7 >> 6),
        ("byte[0x08] & 3", b8 & 3),
        ("(byte[0x08] & 0xf) >> 2", (b8 & 0xF) >> 2),
        ("byte[0x09] & 1", b9 & 1),
        ("(byte[0x09] & 3) >> 1", (b9 & 3) >> 1),
        ("(byte[0x09] & 7) >> 2", (b9 & 7) >> 2),
        ("(byte[0x09] & 0xf) >> 3", (b9 & 0xF) >> 3),
        ("byte[0x0a]", ba),
    ):
        out(f"    {expr:24} = {v}")
    out("    (this is the PRE-caps default image the preload writes; the RSA-verified")
    out("     caps blob patches it before the DRAM/CPU frequency-violation checks)")

    # ------------------------------------------------------------ RSA blob
    out("\n" + "=" * 72)
    out("RSA-2048 capabilities blob")
    out("=" * 72)
    mod = d[blob : blob + 0x100]
    sig = d[blob + 0x100 : blob + 0x200]
    out(
        f"  modulus   @0x{blob:03x}..0x{blob + 0xFF:03x}  ({len(mod)} B)  {mod[:8].hex(' ')} ..."
    )
    out(
        f"  signature @0x{blob + 0x100:03x}..0x{blob + 0x1FF:03x}  ({len(sig)} B)  {sig[:8].hex(' ')} ..."
    )
    out("  root of trust = SHA-256 of the modulus, fused at OTP 0xfd89608c")
    out("  read by al_boot eeprom_preload_parser -> FUN_01002f38 (authenticate)")
    out("  gates SoC capability flags only; the DRAM/CPU frequency-violation checks")
    out("  log and set a flag, they do not abort boot (docs/overclock-and-caps.md §3)")

    # ------------------------------------------------------------ coverage
    out("\n" + "=" * 72)
    out("byte-coverage of the 8 KiB device")
    out("=" * 72)
    spans = [
        (0x000, 0x00B, "opening encapsulation marker"),
        (0x00C, close_off - 1, f"{n} preload register writes"),
        (close_off, close_off + 0x0B, "closing encapsulation marker"),
        (blob, blob + 0x0FF, "RSA-2048 modulus"),
        (blob + 0x100, blob + 0x1FF, "RSA-2048 signature"),
    ]
    tail = blob + 0x200
    zend = tail
    while zend < 0x400 and d[zend] == 0:
        zend += 1
    spans.append((tail, 0x3FF, "zero fill"))
    spans.append((0x400, 0x406, "0xAA pointer record (decode-ddr-records.py)"))
    spans.append((0x407, 0x40A, "unused (zero)"))
    spans.append((0x40B, 0x40D, "0xBB DRAM-voltage GPIO record"))
    spans.append((0x40E, 0x423, "0xCC impedance override record"))
    spans.append((0x424, 0x43F, "0xDD UART record slot — absent (0xff/0x00)"))
    spans.append((0x440, 0x53F, "JEDEC DDR4 SPD image (256 B)"))
    spans.append((0x540, 0x5FF, "zero fill"))
    spans.append((0x600, 0x1FFF, "erased (0xff) — unused"))
    covered = 0
    for s, e, what in spans:
        covered += e - s + 1
        out(f"  0x{s:04x}-0x{e:04x}  {e - s + 1:5} B  {what}")
    out(f"\n  total {covered} / {len(d)} B accounted for")
    if covered != len(d):
        out(f"  !! {len(d) - covered} B UNATTRIBUTED")

    out(f"\nlog -> {LOG}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
