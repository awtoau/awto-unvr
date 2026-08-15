#!/usr/bin/env python3
"""Is the rescue complete for the data that actually matters?

Only ~684 MiB of the 7.4 GiB stick is allocated; the rest is free space nobody
needs. This answers whether every ALLOCATED ext4 block has been rescued, so the
rescue can be stopped early with evidence rather than optimism.

Why the obvious check does not work: ddrescue writes a SPARSE image, so an
un-rescued region reads back as ZEROS, not as an I/O error. Mounting the partial
image and finding the files readable proves nothing - a file in a gap returns
zeros silently. The only sound test is set arithmetic on the block bitmap.

Method:
  1. `dumpe2fs` the image -> per-group "Free blocks" ranges
  2. invert -> the allocated ranges, in bytes
  3. parse the ddrescue mapfile -> the rescued ranges, in bytes
  4. report any allocated byte that is not rescued

Run: ./scripts/verify-rescue-coverage.py
"""

from __future__ import annotations

import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import LOGS, IMAGES, rel  # noqa: E402

STEM = "unvr-usb-07190176402AEE98"
IMG = IMAGES / f"{STEM}.img"
MAP = IMAGES / f"{STEM}.map"


def log(msg, level="INFO"):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {level:<5} {msg}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    with (LOGS / "verify-rescue-coverage.log").open("a") as fh:
        fh.write(line + "\n")


def merge(ranges: list[tuple[int, int]]) -> list[tuple[int, int]]:
    """Coalesce [start, end) ranges."""
    out: list[tuple[int, int]] = []
    for s, e in sorted(ranges):
        if out and s <= out[-1][1]:
            out[-1] = (out[-1][0], max(out[-1][1], e))
        else:
            out.append((s, e))
    return out


def subtract(a: list[tuple[int, int]], b: list[tuple[int, int]]) -> list[tuple[int, int]]:
    """a minus b, both merged and sorted."""
    out = []
    for s, e in a:
        cur = s
        for bs, be in b:
            if be <= cur or bs >= e:
                continue
            if bs > cur:
                out.append((cur, min(bs, e)))
            cur = max(cur, be)
            if cur >= e:
                break
        if cur < e:
            out.append((cur, e))
    return out


def ext4_allocated(img: Path) -> tuple[list[tuple[int, int]], int, int]:
    """Return (allocated byte ranges, block_size, block_count)."""
    p = subprocess.run(["dumpe2fs", str(img)], capture_output=True, text=True)
    if p.returncode != 0 and not p.stdout:
        sys.exit(f"dumpe2fs failed: {p.stderr.strip()}")
    text = p.stdout
    bs = int(re.search(r"^Block size:\s+(\d+)", text, re.M).group(1))
    bc = int(re.search(r"^Block count:\s+(\d+)", text, re.M).group(1))

    free: list[tuple[int, int]] = []
    for m in re.finditer(r"^\s*Free blocks: (.*)$", text, re.M):
        body = m.group(1).strip()
        if not body:
            continue
        for tok in body.split(","):
            tok = tok.strip()
            if not tok:
                continue
            if "-" in tok:
                a, b = tok.split("-", 1)
                free.append((int(a), int(b) + 1))
            else:
                free.append((int(tok), int(tok) + 1))
    free = merge(free)
    whole = [(0, bc)]
    alloc_blocks = subtract(whole, free)
    alloc_bytes = [(s * bs, e * bs) for s, e in alloc_blocks]
    return merge(alloc_bytes), bs, bc


def rescued_ranges(mapfile: Path) -> list[tuple[int, int]]:
    out = []
    for line in mapfile.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        # Two 3-field line shapes exist. The status line is
        # `<current_pos> <status_char> <pass>` - only the block lines have hex
        # in BOTH the first and second field, so require that.
        if len(parts) < 3 or not parts[0].startswith("0x") or not parts[1].startswith("0x"):
            continue
        pos, size, status = int(parts[0], 16), int(parts[1], 16), parts[2]
        if status == "+":
            out.append((pos, pos + size))
    return merge(out)


def human(n: int) -> str:
    for unit in ("B", "KiB", "MiB", "GiB"):
        if abs(n) < 1024 or unit == "GiB":
            return f"{n:.1f} {unit}" if unit != "B" else f"{n} B"
        n /= 1024
    return str(n)


def write_domain(alloc: list[tuple[int, int]], size: int, out: Path) -> None:
    """Write a ddrescue domain mapfile covering only the allocated ranges.

    `ddrescue --domain-mapfile=F` restricts work to blocks marked finished (+)
    in F. Feeding it the ext4 allocation map means the rescue skips free space
    entirely - seconds of work instead of grinding through gigabytes of blank
    flash on a failing device that we would rather touch as little as possible.
    """
    lines = ["# Domain mapfile: ext4-allocated ranges only.",
             "# Generated by scripts/verify-rescue-coverage.py --domain",
             "0x00000000     ?               1",
             "#      pos        size  status"]
    cur = 0
    for s, e in alloc:
        if s > cur:
            lines.append(f"0x{cur:08X}  0x{s - cur:08X}  -")
        lines.append(f"0x{s:08X}  0x{e - s:08X}  +")
        cur = e
    if cur < size:
        lines.append(f"0x{cur:08X}  0x{size - cur:08X}  -")
    out.write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    for f in (IMG, MAP):
        if not f.exists():
            sys.exit(f"absent: {rel(f)}")
    log(f"image   : {rel(IMG)}")
    log(f"mapfile : {rel(MAP)}")

    alloc, bs, bc = ext4_allocated(IMG)
    total_alloc = sum(e - s for s, e in alloc)
    log(f"ext4    : block size {bs}, {bc} blocks, {human(bc * bs)} device")
    log(f"allocated: {human(total_alloc)} in {len(alloc)} range(s)")

    resc = rescued_ranges(MAP)
    total_resc = sum(e - s for s, e in resc)
    log(f"rescued : {human(total_resc)} in {len(resc)} range(s)")

    if "--domain" in sys.argv:
        dom = IMAGES / f"{STEM}.domain.map"
        write_domain(alloc, bc * bs, dom)
        log(f"wrote domain mapfile {rel(dom)} ({len(alloc)} allocated range(s))")
        log("finish only the data that matters with:")
        log(f"  sudo ddrescue --idirect -b 512 -v --domain-mapfile={rel(dom)} \\")
        log(f"       -r3 /dev/sdX {rel(IMG)} {rel(MAP)}")

    missing = subtract(alloc, resc)
    total_missing = sum(e - s for s, e in missing)

    if not missing:
        log("COMPLETE - every allocated block has been rescued.", "INFO")
        log("The remaining un-rescued space is free space. Safe to stop.", "INFO")
        sys.exit(0)

    log(f"INCOMPLETE - {human(total_missing)} of allocated data not yet rescued, "
        f"in {len(missing)} range(s):", "ERROR")
    for s, e in missing[:20]:
        log(f"  {s:#014x} .. {e:#014x}  ({human(e - s)})", "ERROR")
    if len(missing) > 20:
        log(f"  ... and {len(missing) - 20} more", "ERROR")
    sys.exit(1)
