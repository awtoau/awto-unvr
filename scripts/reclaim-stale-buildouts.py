#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Move stale kernel build outputs to the machine-wide wastebasket.

`/mnt/2tb/unvr-port-refs/` is 62 GB, of which ~38 GB is `build-out-*` -
derived artifacts, not reference material. Only `build-out-fedora` is live.

Derived artifacts have no legacy (CLAUDE.md): regenerable from current source,
so the rebuild IS the migration. But these are GB-scale, so they go to the
machine-wide wastebasket rather than `rm` - recoverable until the user empties
it. NOT `debris/`, which is tracked-in-git for non-regenerable user content.

Kept: `build-out-fedora` (live - the deploy source). `build-out-ea16` and
`build-out-fedora-kasan` are OUTPUT paths in build-linux-ea16.py and
_repo.py, so moving them is safe; those scripts recreate them.

The dmatest-before/after pair is doubly dead: it was an A/B for #23's
"UDMA state survives a module reload" theory, which #289 disproved (our
missing queue reset). Reload-based A/B is valid now, so they prove nothing.

    ./scripts/reclaim-stale-buildouts.py            # survey, no changes
    ./scripts/reclaim-stale-buildouts.py --move     # move them
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

from _repo import make_log  # noqa: E402

log = make_log("reclaim-stale-buildouts")

REFS = Path("/mnt/2tb/unvr-port-refs")
WASTEBASKET = Path("/mnt/2tb/wastebasket")

# build-out-fedora is the live deploy source. Everything else is Aug-Sep.
KEEP = {"build-out-fedora"}


def size_gb(p: Path) -> float:
    out = subprocess.run(
        ["du", "-sb", str(p)], capture_output=True, text=True, check=False
    ).stdout
    return int(out.split()[0]) / 1e9 if out else 0.0


def stale() -> list[Path]:
    return sorted(
        d for d in REFS.glob("build-out*") if d.is_dir() and d.name not in KEEP
    )


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--move", action="store_true", help="move to the wastebasket")
    a = ap.parse_args()

    found = stale()
    if not found:
        log("no stale build-out dirs - nothing to do")
        return 0

    total = 0.0
    for d in found:
        g = size_gb(d)
        total += g
        mtime = (
            datetime.fromtimestamp(d.stat().st_mtime, tz=timezone.utc)
            .astimezone()
            .strftime("%Y-%m-%d")
        )
        log(f"  {d.name:34s} {g:6.2f} GB  last touched {mtime}")
    log(f"total {total:.1f} GB across {len(found)} dirs")
    for k in sorted(KEEP):
        log(f"keeping {k} ({size_gb(REFS / k):.1f} GB, live)")

    if not a.move:
        log("DRY RUN - pass --move to reclaim")
        return 0

    slot = (
        WASTEBASKET
        / f"unvr-buildouts-{datetime.now(tz=timezone.utc).astimezone():%Y%m%d-%H%M%S}"
    )
    slot.mkdir(parents=True, exist_ok=True)
    log(f"wastebasket slot: {slot}")

    for d in found:
        dest = slot / d.name
        log(f"  moving {d.name} ...")
        try:
            # Same filesystem (/mnt/2tb) so this is a rename, not a copy.
            shutil.move(str(d), str(dest))
        except OSError as e:
            log(f"  FAILED {d.name}: {e}")
            return 1

    log(f"moved {total:.1f} GB. Empty the slot when confident:")
    log(f"  rm -rf {slot}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
