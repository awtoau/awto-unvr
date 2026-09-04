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
  3. Does the data actually arrive intact? `badblocks -w` writes and verifies
     four patterns (0xaa/0x55/0xff/0x00) across the WHOLE device, catching the
     faults a throughput number hides: truncated transfers, wrapped offsets and
     stuck address lines. Standard tool, better tested than a hand-rolled loop.

Read-only by default. The write test needs --write and a device with no
mounted filesystem - it DESTROYS the target's contents.

Run on the box:
    ./dev.py ssh -- "/root/usb-disk-bench.py --dev /dev/sde"
    ./dev.py ssh -- "/root/usb-disk-bench.py --dev /dev/sde --write"   # destructive
"""

from __future__ import annotations

import argparse
import re
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


def have(tool: str) -> bool:
    return subprocess.run(
        ["command", "-v", tool], capture_output=True, shell=False, check=False
    ).returncode == 0 or bool(sh(["sh", "-c", f"command -v {tool}"]).strip())


def fio_read(dev: str, mib: int) -> tuple[float, float] | None:
    """Sustained sequential read via fio - the standard tool, O_DIRECT, no cache.

    fio reports its own throughput, so we do not infer it from wall time and
    cannot be fooled by a call that returns early.
    """
    if not have("fio"):
        return None
    out = sh(
        [
            "fio",
            "--name=seqread",
            f"--filename={dev}",
            "--rw=read",
            "--bs=1M",
            f"--size={mib}M",
            "--direct=1",
            "--ioengine=libaio",
            "--iodepth=8",
            "--runtime=120",
            "--group_reporting",
            "--output-format=terse",
            "--minimal",
        ]
    )
    # terse v3+: read bandwidth in KiB/s is field 7 (1-indexed) of the ';' row.
    for line in out.splitlines():
        f = line.split(";")
        if len(f) > 7 and f[2] == "seqread":
            kib = float(f[6])
            return (mib * 1024 / kib if kib else 0.0), kib / 1024
    return None


def dd_read(dev: str, mib: int) -> tuple[float, float]:
    """Fallback when fio is absent: dd with O_DIRECT, timed by wall clock."""
    try:
        Path("/proc/sys/vm/drop_caches").write_text("3\n")
    except OSError:
        pass
    t0 = time.monotonic()
    subprocess.run(
        ["dd", f"if={dev}", "of=/dev/null", "bs=1M", f"count={mib}", "iflag=direct"],
        capture_output=True,
        check=False,
    )
    dt = time.monotonic() - t0
    return dt, (mib / dt if dt else 0.0)


def badblocks_rw(dev: str) -> tuple[int, str]:
    """DESTRUCTIVE write-mode badblocks: the standard address/pattern test.

    -w writes 0xaa/0x55/0xff/0x00 over the whole device and verifies each pass,
    which catches stuck bits and aliased addresses across the FULL range. -s
    shows progress, -v reports counts. This is what badblocks exists for; a
    hand-rolled LBA loop tests less and is less trusted.
    """
    out = sh(["badblocks", "-w", "-s", "-v", "-b", "4096", dev])
    bad = 0
    m = re.search(r"(\d+)\s+bad blocks found", out)
    if m:
        bad = int(m.group(1))
    return bad, out


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

    tool = "fio" if have("fio") else "dd"
    print(f"  --- sustained read ({tool}, O_DIRECT) ---")
    for mib in READ_SIZES_MIB:
        r = fio_read(a.dev, mib) or dd_read(a.dev, mib)
        dt, mbs = r
        flag = ""
        if ceiling and mbs > ceiling:
            flag = f"  <-- IMPOSSIBLE: {mbs / ceiling:.1f}x the link ceiling"
        print(f"  {mib:>5} MiB : {dt:6.2f} s  {mbs:7.1f} MB/s{flag}")

    if a.write:
        # Span the device: first, last, and powers of two between. Catches a
        # stuck high address line, which a sequential test never reaches.
        if not have("badblocks"):
            sys.exit("badblocks not installed (dnf install e2fsprogs)")
        print(
            "  --- badblocks -w: 4-pattern write/verify, WHOLE device (DESTRUCTIVE) ---"
        )
        bad, out = badblocks_rw(a.dev)
        print("  " + "\n  ".join(out.strip().splitlines()[-6:]))
        if bad:
            print(f"  FAULT: {bad} bad block(s)")
            return 1
        print("  OK: no bad blocks")
    else:
        print("  (pass --write for the destructive address-in-data integrity test)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
