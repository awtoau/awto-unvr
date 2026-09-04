#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Characterise the USB-attached SSD from Linux: link speed, throughput, integrity.

Answers three questions U-Boot cannot:

  1. What link speed did the bridge actually train at, and why? SuperSpeed is
     expected of an ASM1051E, but topology can cap it - behind a USB 2.0 hub it
     can only reach high-speed (480 Mb/s ~ 60 MB/s), regardless of the chip.
  2. What is the real sustained read throughput? #137 measured ~25 MB/s. A
     figure far above the negotiated link's ceiling means the transfer is not
     happening, not that the disk is fast.
  3. Does the data actually arrive intact? An address-in-data pattern catches
     the faults a throughput number hides: truncated transfers, wrapped offsets,
     and stuck address lines that return the wrong block without erroring.

Read-only by default. The write test needs --write and a device with no
mounted filesystem - it DESTROYS the target's contents.

Run on the box:
    ./dev.py ssh -- "/root/usb-disk-bench.py --dev /dev/sde"
    ./dev.py ssh -- "/root/usb-disk-bench.py --dev /dev/sde --write"   # destructive
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import time
from pathlib import Path

SECTOR = 512
# Read sizes: 128 MiB is #137's known-good size and the throughput reference;
# 512 MiB is where stock U-Boot hung; 1 GiB is what our U-Boot claimed to do in
# 4 s, which would be 4x the link ceiling.
READ_SIZES_MIB = (128, 512, 1024)


def sh(cmd: list[str]) -> str:
    return subprocess.run(cmd, capture_output=True, text=True, check=False).stdout


def sysfs_of(dev: str) -> Path | None:
    """/sys/block/<name> for a whole disk, resolving through its USB parent."""
    name = Path(dev).name
    p = Path("/sys/block") / name
    return p if p.exists() else None


def link_info(dev: str) -> dict[str, str]:
    """Negotiated speed and the hub chain that determined it."""
    out: dict[str, str] = {}
    blk = sysfs_of(dev)
    if not blk:
        return out
    # /sys/block/sde -> ../devices/.../usb1/1-1/1-1.4/1-1.4.3/... : walk up for
    # the usb_device node, which is the one carrying `speed`.
    real = blk.resolve()
    for parent in [real, *real.parents]:
        sp = parent / "speed"
        if sp.exists() and (parent / "idVendor").exists():
            out["usb_path"] = parent.name
            out["speed_mbps"] = sp.read_text().strip()
            for f in ("idVendor", "idProduct", "version", "bMaxPacketSize0"):
                if (parent / f).exists():
                    out[f] = (parent / f).read_text().strip()
            break
    return out


def read_bench(dev: str, mib: int) -> tuple[float, float]:
    """Sustained read of `mib` MiB from offset 0, cache dropped first."""
    try:
        Path("/proc/sys/vm/drop_caches").write_text("3\n")
    except OSError:
        pass  # not fatal - iflag=direct below is what actually matters
    t0 = time.monotonic()
    subprocess.run(
        ["dd", f"if={dev}", "of=/dev/null", "bs=1M", f"count={mib}", "iflag=direct"],
        capture_output=True,
        check=False,
    )
    dt = time.monotonic() - t0
    return dt, (mib / dt if dt else 0.0)


def pattern_block(lba: int) -> bytes:
    """Address-in-data: every 8 bytes of the block encodes its own LBA.

    A truncated or wrapped transfer, or a stuck address line, returns data
    whose embedded LBA does not match where it was read from - which a
    throughput number and a plain zero/one pattern both miss.
    """
    return (lba.to_bytes(8, "little")) * (SECTOR // 8)


def write_verify(dev: str, lbas: list[int]) -> list[str]:
    """DESTRUCTIVE. Write the address-in-data pattern, read back, compare."""
    faults = []
    with open(dev, "rb+", buffering=0) as f:
        for lba in lbas:
            f.seek(lba * SECTOR)
            f.write(pattern_block(lba))
        os.fsync(f.fileno())
    try:
        Path("/proc/sys/vm/drop_caches").write_text("3\n")
    except OSError:
        pass
    with open(dev, "rb", buffering=0) as f:
        for lba in lbas:
            f.seek(lba * SECTOR)
            got = f.read(SECTOR)
            want = pattern_block(lba)
            if got != want:
                embedded = int.from_bytes(got[:8], "little") if len(got) >= 8 else -1
                faults.append(
                    f"LBA {lba} (0x{lba:x}): data says LBA {embedded} "
                    f"(0x{embedded:x}) - {'wrapped/aliased' if embedded != lba else 'corrupt'}"
                )
    return faults


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--dev", default="/dev/sde", help="whole disk, e.g. /dev/sde")
    ap.add_argument(
        "--write",
        action="store_true",
        help="DESTRUCTIVE address-in-data write/verify across the whole device",
    )
    a = ap.parse_args()

    if not Path(a.dev).exists():
        sys.exit(f"no such device: {a.dev}")

    print(f"=== {a.dev} ===")
    info = link_info(a.dev)
    if info:
        speed = info.get("speed_mbps", "?")
        # 5000 = SuperSpeed, 480 = high-speed (USB 2.0), 12 = full-speed.
        label = {"5000": "SuperSpeed", "480": "high-speed", "12": "full-speed"}.get(
            speed, "unknown"
        )
        ceiling = {"5000": 500.0, "480": 60.0, "12": 1.5}.get(speed, 0.0)
        print(f"  usb path   : {info.get('usb_path', '?')}")
        print(
            f"  id         : {info.get('idVendor', '?')}:{info.get('idProduct', '?')}"
        )
        print(f"  link speed : {speed} Mb/s ({label}), ~{ceiling:.0f} MB/s ceiling")
    else:
        print("  link speed : could not resolve the USB parent in sysfs")
        ceiling = 0.0

    print("  --- sustained read (O_DIRECT, caches dropped) ---")
    for mib in READ_SIZES_MIB:
        dt, mbs = read_bench(a.dev, mib)
        flag = ""
        if ceiling and mbs > ceiling:
            flag = f"  <-- IMPOSSIBLE: {mbs / ceiling:.1f}x the link ceiling"
        print(f"  {mib:>5} MiB : {dt:6.2f} s  {mbs:7.1f} MB/s{flag}")

    if a.write:
        # Span the device: first, last, and powers of two between. Catches a
        # stuck high address line, which a sequential test never reaches.
        blocks = int(sh(["blockdev", "--getsz", a.dev]).strip() or 0)
        if not blocks:
            sys.exit("could not read device size")
        lbas = [0, 1, 63, 255]
        n = 1024
        while n < blocks:
            lbas.append(n)
            n *= 2
        lbas.append(blocks - 1)
        lbas = sorted({x for x in lbas if 0 <= x < blocks})
        print(f"  --- address-in-data write/verify, {len(lbas)} LBAs (DESTRUCTIVE) ---")
        faults = write_verify(a.dev, lbas)
        if faults:
            for f in faults:
                print(f"  FAULT {f}")
            return 1
        print(f"  OK: all {len(lbas)} LBAs read back their own address")
    else:
        print("  (pass --write for the destructive address-in-data integrity test)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
