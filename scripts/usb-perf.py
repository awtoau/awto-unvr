#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Read-only throughput test of USB-attached disks on woomera (#384).

  ./dev.py usb-perf SERIAL [SERIAL ...]   # serials from `lsblk -o NAME,SERIAL,TRAN`

- Per disk alone: 1M seq read QD32, 4k rand read QD32, 4k rand read QD1; then all disks together (1M seq).
- fio --readonly --direct=1 on the whole device: nothing is written. Results -> bench-history/usb-perf-<stamp>.json.
- Also reports bus speed, driver (uas vs usb-storage) and USB hub path per disk.
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

log = make_log("usb-perf")
RUNTIME_S = 15  # steady state for a SATA SSD behind USB3 is reached in ~5 s; 15 s averages out GC noise
# per-run ssh budget: RUNTIME_S + ramp + json dump, 1.25x of ~20 s expected
SSH_TIMEOUT_S = 40
TESTS = {
    "seq1M_qd32": "--rw=read --bs=1M --iodepth=32",
    "rand4k_qd32": "--rw=randread --bs=4k --iodepth=32",
    "rand4k_qd1": "--rw=randread --bs=4k --iodepth=1",
}


def box(cmd: str, timeout: int = SSH_TIMEOUT_S) -> str:
    r = subprocess.run(
        [sys.executable, str(HERE / "ssh-woomera.py"), "--", cmd],
        capture_output=True,
        text=True,
        timeout=timeout,
    )
    return "\n".join(
        ln for ln in r.stdout.splitlines() if not ln.startswith("# woomera")
    )


def resolve(serial: str) -> dict:
    out = box(f"lsblk -dno NAME,SERIAL,MODEL,TRAN | grep -F {serial}")
    name = out.split()[0] if out.strip() else None
    if not name:
        raise SystemExit(f"serial {serial} not found")
    info = box(
        f"d=/sys/block/{name}/device; readlink -f $d | grep -o 'usb[0-9]/[^ ]*' | head -1; "
        f"basename $(readlink -f $d/../../../driver); "
        f"cat $(readlink -f $d/../../../..)/speed 2>/dev/null || cat $(readlink -f $d/../../..)/../speed 2>/dev/null"
    ).split()
    return {"serial": serial, "dev": f"/dev/{name}", "lsblk": out.strip(), "usb": info}


def fio(devs: list[str], args: str) -> dict:
    jobs = " ".join(f"--name=j{i} --filename={d}" for i, d in enumerate(devs))
    cmd = (
        f"fio --readonly --direct=1 --ioengine=libaio --runtime={RUNTIME_S} --time_based "
        f"--group_reporting=0 --output-format=json {args} {jobs}"
    )
    out = box(cmd)
    j = json.JSONDecoder().raw_decode(out[out.find("{") :])[0]
    return {
        jb["jobname"]: {
            "bw_MBps": round(jb["read"]["bw"] / 1024, 1),
            "iops": round(jb["read"]["iops"]),
            "lat_us_mean": round(jb["read"]["clat_ns"]["mean"] / 1000, 1),
        }
        for jb in j["jobs"]
    }


def main() -> int:
    serials = sys.argv[1:]
    if not serials:
        print(__doc__)
        return 2
    disks = [resolve(s) for s in serials]
    res = {"disks": disks, "alone": {}, "together": {}}
    for d in disks:
        log(f"{d['serial']} {d['dev']}: {d['lsblk']} usb={d['usb']}")
        res["alone"][d["serial"]] = {}
        for name, args in TESTS.items():
            r = fio([d["dev"]], args)["j0"]
            res["alone"][d["serial"]][name] = r
            log(f"  alone {name}: {r}")
    if len(disks) > 1:
        r = fio([d["dev"] for d in disks], TESTS["seq1M_qd32"])
        res["together"] = r
        log(f"  together seq1M_qd32: {r}")
    out = (
        HERE.parent
        / "bench-history"
        / f"usb-perf-{time.strftime('%Y%m%d-%H%M%S')}.json"
    )
    out.write_text(json.dumps(res, indent=1))
    log(f"saved {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
