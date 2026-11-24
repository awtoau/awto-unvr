#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Record woomera's boot ID before a test, check it after (#243).

  ./dev.py boot-id snapshot   # save boot_id + uptime to tmp/boot-id.json
  ./dev.py boot-id check      # exit 0 same boot, 1 rebooted, 2 box unreachable

- A hang/disappearance is only a reproduction if check says 0 or 2 with the console log silent.
- Exit 1 = someone rebooted the box (boot_id changed or uptime below the snapshot's): not evidence.
- Exit 2 = unreachable: look at tmp/logs/unvr-console.log for a boot banner before recording anything.
"""

from __future__ import annotations

import json
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from _repo import make_log  # noqa: E402

log = make_log("boot-id")
SNAP = HERE.parent / "tmp" / "boot-id.json"
# One ssh read of two /proc files over LAN, expected <3 s.
SSH_TIMEOUT_S = 20
READ_CMD = "cat /proc/sys/kernel/random/boot_id; cut -d' ' -f1 /proc/uptime"


def read_box() -> dict | None:
    r = subprocess.run(
        [sys.executable, str(HERE / "ssh-woomera.py"), "--", READ_CMD],
        capture_output=True,
        text=True,
        timeout=SSH_TIMEOUT_S,
    )
    lines = [ln for ln in r.stdout.splitlines() if ln and not ln.startswith("#")]
    if r.returncode or len(lines) < 2:
        return None
    return {"boot_id": lines[0].strip(), "uptime_s": float(lines[1])}


def compare(old: dict, new: dict | None) -> tuple[int, str]:
    if new is None:
        return (
            2,
            "box unreachable: check tmp/logs/unvr-console.log for a boot banner before recording",
        )
    if new["boot_id"] != old["boot_id"] or new["uptime_s"] < old["uptime_s"]:
        return 1, (
            f"REBOOTED: boot_id {old['boot_id']} -> {new['boot_id']}, "
            f"uptime {old['uptime_s']:.0f}s -> {new['uptime_s']:.0f}s; not a reproduction"
        )
    return 0, f"same boot (boot_id {new['boot_id']}, uptime {new['uptime_s']:.0f}s)"


def main() -> int:
    cmd = sys.argv[1] if len(sys.argv) > 1 else ""
    if cmd == "snapshot":
        cur = read_box()
        if cur is None:
            log("snapshot failed: box unreachable")
            return 2
        cur["at"] = time.strftime("%Y-%m-%dT%H:%M:%S%z")
        SNAP.parent.mkdir(parents=True, exist_ok=True)
        SNAP.write_text(json.dumps(cur))
        log(f"snapshot boot_id {cur['boot_id']} uptime {cur['uptime_s']:.0f}s")
        return 0
    if cmd == "check":
        if not SNAP.exists():
            log("no snapshot: run snapshot first")
            return 2
        code, msg = compare(json.loads(SNAP.read_text()), read_box())
        log(msg)
        return code
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
