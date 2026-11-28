#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Read-only throughput test of USB-attached disks on woomera (#384).

  ./dev.py usb-perf SERIAL [SERIAL ...]   # serials from `lsblk -o NAME,SERIAL,TRAN`

- Per disk alone: 1M seq read QD32, 4k rand read QD32, 4k rand read QD1; then all disks together (1M seq).
- fio --readonly --direct=1 on the whole device: nothing is written. Results -> bench-history/usb-perf-<stamp>.json.
- `sweep [--knobs a,b+c] SERIAL...`: one runtime knob at a time (KNOBS), alone + together, mpstat per CPU; sysfs originals restored after each.
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


# ---- sweep: one runtime knob at a time, originals recorded and restored (#384) ----
# knob = {"blk": {path under /sys/block/<dev>/: value}, "irq": cpulist, "fio": extra fio args}
KNOBS = {
    "baseline": {},
    "sched_none": {"blk": {"queue/scheduler": "none"}},
    "sched_mq-deadline": {"blk": {"queue/scheduler": "mq-deadline"}},
    "sched_kyber": {"blk": {"queue/scheduler": "kyber"}},
    "nr_requests_256": {"blk": {"queue/scheduler": "none", "queue/nr_requests": "256"}},
    "rq_affinity_2": {"blk": {"queue/rq_affinity": "2"}},
    "nomerges_2": {"blk": {"queue/nomerges": "2"}},
    "add_random_0": {"blk": {"queue/add_random": "0"}},
    "qdepth_16": {"blk": {"device/queue_depth": "16"}},
    "qdepth_8": {"blk": {"device/queue_depth": "8"}},
    "max_sectors_256k": {"blk": {"queue/max_sectors_kb": "256"}},
    "max_sectors_128k": {"blk": {"queue/max_sectors_kb": "128"}},
    "irq_cpu1": {"irq": "1"},
    "irq_cpu3": {"irq": "3"},
    "fio_cpu1": {"fio": "--cpus_allowed=1"},
    "fio_cpu0": {"fio": "--cpus_allowed=0"},
    "fio_io_uring": {"fio": "--ioengine=io_uring"},
    "fio_bs512k": {"fio": "--bs=512k"},
    "fio_bs128k": {"fio": "--bs=128k"},
    "fio_qd8": {"fio": "--iodepth=8"},
    "fio_qd64": {"fio": "--iodepth=64"},
}


def sysfs_read(path: str) -> str:
    v = box(f"cat {path}").strip()
    if "[" in v:  # scheduler: "none mq-deadline [bfq]"
        v = v.split("[")[1].split("]")[0]
    return v


def sysfs_write(path: str, val: str) -> None:
    box(f"echo {val} > {path}")


def mpstat_summary(txt: str) -> str:
    cpus = {}
    for ln in txt.splitlines():
        f = ln.split()
        if len(f) > 10 and f[0] == "Average:" and f[1].isdigit():
            cpus[f[1]] = 100 - float(f[-1])  # busy% = 100 - idle
    return " ".join(f"c{c}:{b:.0f}" for c, b in sorted(cpus.items()))


def fio_cpu(devs: list[str], args: str) -> tuple[dict, str]:
    jobs = " ".join(f"--name=j{i} --filename={d}" for i, d in enumerate(devs))
    cmd = (
        "mpstat -P ALL 12 1 > /dev/shm/mp.txt & "
        f"fio --readonly --direct=1 --ioengine=libaio --runtime={RUNTIME_S} --time_based "
        f"--group_reporting=0 --output-format=json {args} {jobs}; wait; cat /dev/shm/mp.txt"
    )
    out = box(cmd, SSH_TIMEOUT_S + 15)
    start = out.find("{")
    j, end = json.JSONDecoder().raw_decode(out[start:])
    bw = {jb["jobname"]: round(jb["read"]["bw"] / 1024, 1) for jb in j["jobs"]}
    return bw, mpstat_summary(out[start + end :])


def sweep(args: list[str]) -> int:
    names = list(KNOBS)
    if "--knobs" in args:
        i = args.index("--knobs")
        names = args[i + 1].split(",")
        args = args[:i] + args[i + 2 :]
    disks = [resolve(s) for s in args]
    irq = box('awk \'/xhci_hcd/{sub(":","",$1);print $1}\' /proc/interrupts').strip()
    rows = []
    for name in names:
        spec: dict = {"blk": {}, "irq": None, "fio": ""}
        for part in name.split("+"):  # combos: a+b
            k = KNOBS[part]
            spec["blk"].update(k.get("blk", {}))
            spec["irq"] = k.get("irq", spec["irq"])
            spec["fio"] = f"{spec['fio']} {k.get('fio', '')}".strip()
        fa = spec["fio"]
        fio_args = TESTS["seq1M_qd32"]
        for opt in ("--iodepth", "--bs"):  # fio-side overrides replace the base option
            if opt in fa:
                fio_args = " ".join(
                    t for t in fio_args.split() if not t.startswith(opt + "=")
                )
        orig: list[tuple[str, str]] = []
        try:
            for d in disks:
                for rel, val in spec["blk"].items():
                    path = f"/sys/block/{d['dev'][5:]}/{rel}"
                    orig.append((path, sysfs_read(path)))
                    sysfs_write(path, val)
            if spec["irq"]:
                path = f"/proc/irq/{irq}/smp_affinity_list"
                orig.append((path, sysfs_read(path)))
                sysfs_write(path, spec["irq"])
            row = {"knob": name, "orig": dict(orig), "alone": {}, "cpu": {}}
            for d in disks:
                bw, mp = fio_cpu([d["dev"]], f"{fio_args} {fa}")
                row["alone"][d["serial"]] = bw["j0"]
                row["cpu"][d["serial"]] = mp
            bw, mp = fio_cpu([d["dev"] for d in disks], f"{fio_args} {fa}")
            row["together"] = list(bw.values())
            row["cpu"]["together"] = mp
            log(
                f"SWEEP {name}: alone={row['alone']} together={row['together']} sum={sum(bw.values()):.1f} cpu_together={mp} orig={row['orig']}"
            )
            rows.append(row)
        finally:
            for path, val in reversed(orig):
                sysfs_write(path, val)
            if orig:
                log(f"  restored {name}: {dict(orig)}")
    out = (
        HERE.parent
        / "bench-history"
        / f"usb-perf-sweep-{time.strftime('%Y%m%d-%H%M%S')}.json"
    )
    out.write_text(json.dumps(rows, indent=1))
    log(f"saved {out}")
    return 0


def main() -> int:
    if sys.argv[1:2] == ["sweep"]:
        return sweep(sys.argv[2:])
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
