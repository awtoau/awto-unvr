#!/usr/bin/env python3
# Copyright (c) Awto / Daniel Tyrrell. SPDX-License-Identifier: GPL-2.0-or-later
"""Run duperemove on woomera's /srv/mirror music-duplicates tree (awto-music#22).

- Default is report-only (no -d). `--dedupe` adds -d (shares extents, never deletes files).
- 1 I/O thread (single reader on a 2-HDD RAID1: more readers = seek thrash, #372).
- Hashfile on the system SSD, not the data. Output -> tmp/logs/duperemove-mirror.log.
- Stall guard: no output for STALL_S = kill (host-process rule, awto-dan#53).
"""
import argparse
import select
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
LOG = REPO / "tmp/logs/duperemove-mirror.log"
BIN = "/srv/mirror/awto-music-duplicates/.tools/usr/bin/duperemove"
ROOT = "/srv/mirror/awto-music-duplicates"
HASHFILE = "/var/tmp/duperemove-mirror.hash"
STALL_S = 120  # hashing prints progress per file/second; 2x the 60 s default for big files


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--dedupe", action="store_true", help="add -d (share extents)")
    ap.add_argument("--exclude", action="append", default=["*#recycle*"])
    a = ap.parse_args()
    LOG.parent.mkdir(parents=True, exist_ok=True)
    ex = " ".join(f"--exclude='{p}'" for p in a.exclude)
    remote = (
        f"{BIN} -r{'d' if a.dedupe else ''} -h --io-threads=1 --cpu-threads=2 "
        f"--hashfile={HASHFILE} {ex} '{ROOT}/quarantine' '{ROOT}/copies' 2>&1"
    )
    cmd = [sys.executable, str(REPO / "scripts/ssh-woomera.py"), "--", remote]
    with LOG.open("ab") as log:
        log.write(f"\n=== {time.strftime('%Y-%m-%dT%H:%M:%S%z')} {remote}\n".encode())
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        last = time.monotonic()
        while p.poll() is None:
            r, _, _ = select.select([p.stdout], [], [], 5)
            if r:
                chunk = p.stdout.read1(65536)
                if chunk:
                    log.write(chunk)
                    log.flush()
                    last = time.monotonic()
            elif time.monotonic() - last > STALL_S:
                p.kill()
                log.write(f"\nSTALL: no output {STALL_S}s, killed\n".encode())
                return 2
        log.write(p.stdout.read())
        log.write(f"\nexit {p.returncode}\n".encode())
    return p.returncode


if __name__ == "__main__":
    sys.exit(main())
