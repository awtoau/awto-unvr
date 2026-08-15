#!/usr/bin/env python3
"""Reverse the UNVR's MTD partitions - decode what each one actually contains.

`file` calls several of these "ISO-8859 text with very long lines", which just
means long runs of 0xFF (erased flash). This decodes them structurally instead:

  u-boot env  -> CRC32 + optional flags byte + NUL-separated key=value pairs.
                 This is what settles whether THIS unit boots unsigned.
  EEPROM      -> Ubiquiti identity block: MAC, board ID, HW revision, device ID,
                 and a redundant copy at 0x8000 marked with the magic "UBNT".
                 Layout from linux-alpine-v2's reverse-engineering notes.
  device_tree -> FDT (magic 0xd00dfeed), extracted and decompiled with dtc.
  Factory     -> strings + non-erased regions.
  al_boot / u-boot -> version strings and build identity.
  cksum       -> non-erased content.

Run: ./scripts/analyse-mtd.py                 # newest dump set(s)
     ./scripts/analyse-mtd.py --set <dirname>
"""

from __future__ import annotations

import argparse
import binascii
import re
import struct
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import IMAGES, LOGS, rel  # noqa: E402

MTD_ROOT = IMAGES / "mtd"
FDT_MAGIC = 0xD00DFEED

# Ubiquiti EEPROM layout, from linux-alpine-v2 docs (docs/sources.md).
EEPROM_FIELDS = [
    (0x0000, 6, "base MAC"),
    (0x000C, 2, "board ID"),
    (0x000E, 2, "hardware revision"),
    (0x0010, 4, "device ID"),
]
EEPROM_MAGIC_OFF = 0x8000


def log(msg, level="INFO"):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {level:<5} {msg}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    with (LOGS / "analyse-mtd.log").open("a") as fh:
        fh.write(line + "\n")


def erased_ratio(b: bytes) -> float:
    return b.count(0xFF) / len(b) if b else 0.0


def used_extent(b: bytes) -> tuple[int, int]:
    """First and last non-0xFF/non-0x00 byte - the region that carries content."""
    first, last = None, None
    for i, ch in enumerate(b):
        if ch not in (0xFF, 0x00):
            if first is None:
                first = i
            last = i
    return (first or 0), (last or 0)


def decode_uboot_env(blob: bytes, name: str) -> None:
    """CRC32 (LE) + optional flags byte + NUL-separated key=value, ending in \\0\\0."""
    for has_flags in (True, False):
        off = 5 if has_flags else 4
        stored = struct.unpack("<I", blob[:4])[0]
        flags = blob[4] if has_flags else None
        body = blob[off:]
        calc = binascii.crc32(body) & 0xFFFFFFFF
        if calc == stored:
            log(f"  CRC32 {stored:#010x} OK "
                f"({'redundant env, flags=%#04x' % flags if has_flags else 'plain env'})")
            entries = [e for e in body.split(b"\x00") if e]
            log(f"  {len(entries)} variable(s):")
            for e in entries:
                s = e.decode("utf-8", "replace")
                log(f"    {s}")
            return
    log(f"  CRC32 did not verify either way (stored {struct.unpack('<I', blob[:4])[0]:#010x})",
        "WARN")
    entries = [e for e in blob[4:].split(b"\x00") if e and b"=" in e]
    if entries:
        log(f"  raw scan found {len(entries)} key=value pair(s):")
        for e in entries[:80]:
            log(f"    {e.decode('utf-8', 'replace')}")


def decode_eeprom(blob: bytes) -> None:
    for off, size, label in EEPROM_FIELDS:
        raw = blob[off:off + size]
        if label == "base MAC":
            log(f"  {label:20s} @{off:#06x} = " + ":".join(f"{b:02x}" for b in raw))
        else:
            log(f"  {label:20s} @{off:#06x} = 0x{raw.hex()}")
    magic = blob[EEPROM_MAGIC_OFF:EEPROM_MAGIC_OFF + 4]
    log(f"  magic @{EEPROM_MAGIC_OFF:#06x}        = {magic!r} "
        + ("(UBNT - redundant copy present)" if magic == b"UBNT" else "(not UBNT)"))
    txt = [s for s in re.findall(rb"[\x20-\x7e]{4,}", blob)][:20]
    if txt:
        log("  strings: " + ", ".join(t.decode() for t in txt))


def decode_fdt(blob: bytes, outdir: Path, stem: str) -> None:
    idx = blob.find(struct.pack(">I", FDT_MAGIC))
    if idx == -1:
        log("  no FDT magic (0xd00dfeed) found", "WARN")
        return
    total = struct.unpack(">I", blob[idx + 4:idx + 8])[0]
    log(f"  FDT at offset {idx} (0x{idx:x}), totalsize {total}")
    if total <= 0 or idx + total > len(blob):
        log("  FDT totalsize implausible", "WARN")
        return
    outdir.mkdir(parents=True, exist_ok=True)
    dtb = outdir / f"{stem}.dtb"
    dtb.write_bytes(blob[idx:idx + total])
    log(f"  wrote {rel(dtb)}")
    dts = outdir / f"{stem}.dts"
    r = subprocess.run(["dtc", "-I", "dtb", "-O", "dts", "-o", str(dts), str(dtb)],
                       capture_output=True, text=True)
    if r.returncode == 0:
        n = len(dts.read_text().splitlines())
        log(f"  decompiled -> {rel(dts)} ({n} lines)")
        for line in dts.read_text().splitlines()[:25]:
            log(f"    {line}")
    else:
        log(f"  dtc failed: {r.stderr.strip()[:200]}", "WARN")


def decode_strings(blob: bytes, label: str, limit: int = 30) -> None:
    hits = re.findall(rb"[\x20-\x7e]{6,}", blob)
    interesting = [h.decode() for h in hits
                   if re.search(rb"(?i)u-?boot|version|alpine|annapurna|ubnt|ubiquiti"
                                rb"|build|gcc|\d+\.\d+\.\d+", h)]
    log(f"  {len(hits)} string(s), {len(interesting)} interesting:")
    for s in interesting[:limit]:
        log(f"    {s}")


def analyse(img: Path, outdir: Path) -> None:
    blob = img.read_bytes()
    name = img.stem
    # Partition names contain hyphens ("u-boot_env"), so anchor on the trailing
    # "-<size>B-" field rather than assuming the name is hyphen-free.
    m = re.search(r"-mtd(\d+)-(.+?)-\d+B-", name)
    num, part = (m.group(1), m.group(2)) if m else ("??", name)
    er = erased_ratio(blob)
    zr = blob.count(0x00) / len(blob) if blob else 0.0
    first, last = used_extent(blob)
    log(f"--- mtd{num} \"{part}\" {len(blob)} B ---")
    if last:
        log(f"  0xFF {er:.1%}, 0x00 {zr:.1%}; content between {first} and {last} "
            f"({last - first + 1} B)")
    elif zr > 0.99:
        log(f"  0xFF {er:.1%}, 0x00 {zr:.1%} - partition is ZEROED (not erased flash; "
            f"erased NAND/NOR reads 0xFF, so this was actively written with nulls)")
    else:
        log(f"  0xFF {er:.1%}, 0x00 {zr:.1%}; no content found")

    p = part.lower()
    if "u-boot_env" in p or p.startswith("u-boot_env"):
        decode_uboot_env(blob, part)
    elif "eeprom" in p:
        decode_eeprom(blob)
    elif "device_tree" in p:
        decode_fdt(blob, outdir, f"mtd{num}-device_tree")
    elif "factory" in p:
        decode_eeprom(blob)          # same family of identity block
        decode_strings(blob, part)
    elif "u-boot" in p or "al_boot" in p:
        decode_strings(blob, part)
    else:
        decode_strings(blob, part, limit=15)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--set", help="dump-set directory under images/mtd/")
    a = ap.parse_args()

    sets = sorted([d for d in MTD_ROOT.iterdir() if d.is_dir()],
                  key=lambda d: d.name, reverse=True) if MTD_ROOT.is_dir() else []
    if not sets:
        sys.exit(f"no dump sets under {rel(MTD_ROOT)}")
    targets = [d for d in sets if d.name == a.set] if a.set else sets
    if not targets:
        sys.exit(f"no such set: {a.set}")

    for s in targets:
        imgs = sorted(s.glob("*.img"))
        if not imgs:
            continue
        log(f"=== {s.name} ===")
        for img in imgs:
            analyse(img, s / "decoded")
    return 0


if __name__ == "__main__":
    sys.exit(main())
