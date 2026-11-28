#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""I/O overhead bench for woomera: CPU cost per GB, IRQ/softirq rates, concurrent loads, knob A/B.

  ./dev.py overhead-bench run NAME LOAD[,LOAD...] [--knob K=V ...] [--secs 15] [--perf]
  ./dev.py overhead-bench knobs         # list knob names

- LOAD: net-rx[N] (box receives, N iperf3 streams), net-tx[N], sata, hdd, usb.
- All loads start together; counters are diffed over a steady-state window (ramp excluded).
- Disk loads are fio --readonly --direct=1 on whole devices resolved by serial. Nothing is written.
- Knobs are applied before the run and restored (to the value read first) in `finally`.
- Output: bench-history/overhead-<stamp>-<name>.json; log tmp/logs/overhead-bench.log. Method/results: issue #386 body.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import threading
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from _repo import make_log  # noqa: E402

_log = make_log("overhead-bench")


class log:  # noqa: N801 - printf-style shim over the repo logger
    @staticmethod
    def info(fmt: str, *a) -> None:
        _log(fmt % a if a else fmt)

REPO = HERE.parent
BOX_IF = "enp0s2"
HOST_IF = "enp8s0"  # dev host 10G NIC; MAC checked against the box's in `ip neigh`
IPERF_PORT = 5231
SATA_SSD = "S21BNXAG501763W"
HDDS = ("VGG8B0LG", "VGG8ASZG")
USB_SSDS = ("S3YBNB0M621787E", "S5GENJ0NB03022H", "S5GENJ0NB02962Z")
RAMP_S = 5  # fio QD32 + TCP slow start reach steady state in ~3 s; 5 s = 1.6x
SSH_S = 40  # ssh call: 1.25x of a 30 s worst case (perf report)


def box(cmd: str, timeout: int = SSH_S) -> str:
    r = subprocess.run(
        [sys.executable, str(HERE / "ssh-woomera.py"), "--", cmd],
        capture_output=True,
        text=True,
        timeout=timeout,
    )
    return "\n".join(
        ln for ln in (r.stdout + r.stderr).splitlines() if not ln.startswith("# woomera")
    )


def local(cmd: list[str], timeout: int) -> tuple[int, str]:
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    return r.returncode, r.stdout + r.stderr


# ---- knobs: name -> list of (read_cmd, write_cmd_template with {v}); {v} is the value ----
def _irq_ids(pattern: str) -> list[str]:
    out = box(f"grep -E '{pattern}' /proc/interrupts | cut -d: -f1")
    return [x.strip() for x in out.split() if x.strip().isdigit()]


def knob_cmds(name: str, val: str) -> list[tuple[str, str, str]]:
    """-> [(read_cmd, write_cmd, value_for_write)]; restore re-runs write with the read value."""
    if name == "irq-spread":  # val = "ahci" | "xhci" | "ahci:08" ; vectors round-robin over CPUs 0-3
        pat = {"ahci": "ahci-al", "xhci": "xhci_hcd"}.get(val, val)
        ids = _irq_ids(pat)
        return [
            (
                f"cat /proc/irq/{n}/smp_affinity_list",
                f"echo {{v}} > /proc/irq/{n}/smp_affinity_list",
                str(i % 4),
            )
            for i, n in enumerate(ids)
        ]
    if name == "irq-cpu":  # val = "ahci=1"
        pat, cpu = val.split("=")
        pat = {"ahci": "ahci-al", "xhci": "xhci_hcd"}.get(pat, pat)
        return [
            (f"cat /proc/irq/{n}/smp_affinity_list", f"echo {{v}} > /proc/irq/{n}/smp_affinity_list", cpu)
            for n in _irq_ids(pat)
        ]
    if name == "rx-usecs":
        return [
            (
                f"ethtool -c {BOX_IF} | awk '/^rx-usecs:/{{print $2}}'",
                f"ethtool -C {BOX_IF} rx-usecs {{v}}",
                val,
            )
        ]
    if name == "tx-usecs":
        return [
            (
                f"ethtool -c {BOX_IF} | awk '/^tx-usecs:/{{print $2}}'",
                f"ethtool -C {BOX_IF} tx-usecs {{v}}",
                val,
            )
        ]
    if name == "rx-frames":
        return [
            (
                f"ethtool -c {BOX_IF} | awk '/^rx-frames:/{{print $2}}'",
                f"ethtool -C {BOX_IF} rx-frames {{v}}",
                val,
            )
        ]
    if name == "adaptive-rx":
        return [
            (
                f"ethtool -c {BOX_IF} | awk '/^Adaptive RX:/{{print $3}}'",
                f"ethtool -C {BOX_IF} adaptive-rx {{v}}",
                val,
            )
        ]
    if name in ("gro", "tso", "gso", "rx-checksumming", "lro"):
        return [
            (
                f"ethtool -k {BOX_IF} | awk '/^{ {'gro':'generic-receive-offload','tso':'tcp-segmentation-offload','gso':'generic-segmentation-offload','rx-checksumming':'rx-checksumming','lro':'large-receive-offload'}[name] }:/{{print $2}}'",
                f"ethtool -K {BOX_IF} {name} {{v}}",
                val,
            )
        ]
    if name == "ring":
        return [
            (
                f"ethtool -g {BOX_IF} | awk '/Current/{{f=1}} f&&/^RX:/{{print $2; exit}}'",
                f"ethtool -G {BOX_IF} rx {{v}} tx {{v}}",
                val,
            )
        ]
    if name == "rps":  # val = hex mask applied to all rx queues
        return [
            (
                f"cat /sys/class/net/{BOX_IF}/queues/rx-{q}/rps_cpus",
                f"echo {{v}} > /sys/class/net/{BOX_IF}/queues/rx-{q}/rps_cpus",
                val,
            )
            for q in range(4)
        ]
    if name == "xps":
        return [
            (
                f"cat /sys/class/net/{BOX_IF}/queues/tx-{q}/xps_cpus",
                f"echo {{v}} > /sys/class/net/{BOX_IF}/queues/tx-{q}/xps_cpus",
                val,
            )
            for q in range(4)
        ]
    if name == "sysctl":  # val = key=value (value may contain spaces)
        k, v = val.split("=", 1)
        return [(f"sysctl -n {k}", f"sysctl -w {k}=\"{{v}}\"", v)]
    if name == "file":  # val = path=value
        p, v = val.split("=", 1)
        return [(f"cat {p}", f"echo {{v}} > {p}", v)]
    if name == "sd-queue":  # val = attr=value on all SATA/USB disks used: e.g. read_ahead_kb=4096
        a, v = val.split("=")
        return [
            (f"cat /sys/block/{d}/queue/{a}", f"echo {{v}} > /sys/block/{d}/queue/{a}", v)
            for d in ("sda", "sdd", "sdf", "sdg")
        ]
    if name == "mtu-box":
        return [(f"cat /sys/class/net/{BOX_IF}/mtu", f"ip link set {BOX_IF} mtu {{v}}", val)]
    raise SystemExit(f"unknown knob {name}")


KNOB_NAMES = "irq-spread irq-cpu rx-usecs rx-frames adaptive-rx gro tso gso lro rx-checksumming ring rps xps sysctl file sd-queue mtu-box (mtu-host is handled separately)".split()


class Knobs:
    def __init__(self, specs: list[str]):
        self.items: list[tuple[str, str, str]] = []  # (spec, write_cmd_tmpl, original)
        self.specs = specs
        self.mtu_host_orig: str | None = None

    def apply(self) -> None:
        for s in self.specs:
            name, _, val = s.partition("=")
            if name == "mtu-host":
                self.mtu_host_orig = Path(f"/sys/class/net/{HOST_IF}/mtu").read_text().strip()
                rc, o = local(["sudo", "-n", "ip", "link", "set", HOST_IF, "mtu", val], 10)
                log.info("host mtu %s -> %s rc=%s %s", self.mtu_host_orig, val, rc, o.strip())
                continue
            for rd, wr, v in knob_cmds(name, val):
                orig = box(rd).strip()
                self.items.append((s, wr, orig))
                out = box(wr.format(v=v))
                log.info("knob %s: %r -> %r (%s) %s", s, orig, v, wr.split()[0], out.strip()[:100])

    def restore(self) -> None:
        for s, wr, orig in reversed(self.items):
            out = box(wr.format(v=orig))
            log.info("restore %s -> %r %s", s, orig, out.strip()[:100])
        if self.mtu_host_orig:
            local(["sudo", "-n", "ip", "link", "set", HOST_IF, "mtu", self.mtu_host_orig], 10)
            log.info("host mtu restored %s", self.mtu_host_orig)


# ---- snapshots ----
SNAP_CMD = (
    "echo @@stat; grep ^cpu /proc/stat; grep ^ctxt /proc/stat; "
    "echo @@irq; cat /proc/interrupts; echo @@soft; cat /proc/softirqs; "
    f"echo @@net; grep {BOX_IF} /proc/net/dev; echo @@disk; cat /proc/diskstats; "
    "echo @@up; cat /proc/uptime; echo @@noise; "
    "for p in $(pgrep -x bees) $(pgrep -f '[f]ile-hash-cache') $(pgrep -x duperemove); do cat /proc/$p/stat; done"
)


def parse_snap(text: str) -> dict:
    sec = None
    d: dict = {"stat": {}, "irq": {}, "soft": {}, "disk": {}}
    for ln in text.splitlines():
        if ln.startswith("@@"):
            sec = ln[2:]
            continue
        f = ln.split()
        if not f:
            continue
        if sec == "stat":
            if f[0] == "ctxt":
                d["ctxt"] = int(f[1])
            else:
                d["stat"][f[0]] = [int(x) for x in f[1:]]
        elif sec == "irq" and f[0].endswith(":") and f[0][:-1].isdigit():
            cnt = []
            for x in f[1:5]:
                if x.isdigit():
                    cnt.append(int(x))
            label = " ".join(f[1 + len(cnt) :])
            d["irq"][f[0][:-1]] = (cnt, label)
        elif sec == "soft" and f[0].endswith(":"):
            d["soft"][f[0][:-1]] = [int(x) for x in f[1:5]]
        elif sec == "net":
            v = ln.split(":", 1)[1].split()
            d["rx_bytes"], d["tx_bytes"] = int(v[0]), int(v[8])
        elif sec == "disk" and len(f) >= 14:
            d["disk"][f[2]] = int(f[5]) * 512  # sectors read
        elif sec == "up":
            d["up"] = float(f[0])
        elif sec == "noise":  # bees / hash-cache / duperemove: utime+stime ticks (100 Hz), summed
            r = ln.rsplit(")", 1)[1].split()
            d["noise"] = d.get("noise", 0) + int(r[11]) + int(r[12])
    return d


def diff(a: dict, b: dict) -> dict:
    dt = b["up"] - a["up"]
    out: dict = {"dt_s": round(dt, 2)}
    busy = {}
    for cpu, x in a["stat"].items():
        y = b["stat"][cpu]
        dd = [q - p for p, q in zip(x, y)]
        tot = sum(dd[:8])
        busy[cpu] = round(100.0 * (tot - dd[3] - dd[4]) / tot, 1) if tot else 0.0
    noise = (b.get("noise", 0) - a.get("noise", 0)) / 100.0 / dt * 100.0  # % of one core
    out["bg_noise_pct_of_400"] = round(noise, 1)  # bees etc; NOT subtracted from busy, reported next to it
    out["busy_pct"] = busy  # per-CPU 0-100 each; "cpu" = whole system 0-100
    out["busy_of_400"] = round(busy.get("cpu", 0) * 4, 1)
    y = b["stat"]["cpu"]
    x = a["stat"]["cpu"]
    dd = [q - p for p, q in zip(x, y)]
    tot = sum(dd[:8]) or 1
    names = ["user", "nice", "sys", "idle", "iowait", "irq", "softirq", "steal"]
    out["split_pct"] = {n: round(100.0 * dd[i] / tot, 1) for i, n in enumerate(names)}
    out["ctxt_s"] = round((b["ctxt"] - a["ctxt"]) / dt)
    irq_cpu = [0, 0, 0, 0]
    irqs = {}
    for n, (cb, lab) in b["irq"].items():
        ca = a["irq"].get(n, ([0] * 4, ""))[0]
        dl = [(cb[i] if i < len(cb) else 0) - (ca[i] if i < len(ca) else 0) for i in range(4)]
        for i in range(4):
            irq_cpu[i] += dl[i]
        if sum(dl) > 0:
            irqs[f"{n} {lab}"] = [round(v / dt) for v in dl]
    out["irq_s_per_cpu"] = [round(v / dt) for v in irq_cpu]
    out["irq_s_total"] = round(sum(irq_cpu) / dt)
    out["irq_top"] = dict(sorted(irqs.items(), key=lambda kv: -sum(kv[1]))[:12])
    soft = {}
    for k, cb in b["soft"].items():
        dl = [q - p for p, q in zip(a["soft"].get(k, [0] * 4), cb)]
        if sum(dl) / dt >= 50:
            soft[k] = [round(v / dt) for v in dl]
    out["softirq_s_per_cpu"] = soft
    out["net_rx_MBps"] = round((b["rx_bytes"] - a["rx_bytes"]) / dt / 1e6, 1)
    out["net_tx_MBps"] = round((b["tx_bytes"] - a["tx_bytes"]) / dt / 1e6, 1)
    out["disk_MBps"] = {
        d: round((b["disk"][d] - a["disk"].get(d, 0)) / dt / 1e6, 1)
        for d in b["disk"]
        if b["disk"][d] - a["disk"].get(d, 0) > 1e6 * dt
    }
    return out


# ---- loads ----
def resolve_disks() -> dict[str, str]:
    out = box("lsblk -dno NAME,SERIAL")
    m = {}
    for ln in out.splitlines():
        f = ln.split()
        if len(f) == 2:
            m[f[1]] = f[0]
    return m


def fio_cmd(devs: list[str], secs: int) -> str:
    jobs = " ".join(
        f"--name=j{d} --filename=/dev/{d} --readonly --direct=1 --ioengine=libaio --rw=read "
        f"--bs=1M --iodepth=32 --time_based --runtime={secs}"
        for d in devs
    )
    return f"fio --output-format=terse {jobs} > /dev/shm/fio.out 2>&1"


def guard() -> None:
    out = box("ss -tn state established '( sport = :2049 or sport = :445 )' | tail -n +2")
    busy = box("cut -d' ' -f1-3 /proc/loadavg")
    if out.strip():
        raise SystemExit(f"STOP: active NFS/SMB clients:\n{out}")
    log.info("guard ok: no NFS/SMB clients, loadavg %s", busy.strip())


def run_once(name: str, loads: list[str], secs: int, perf: bool, knobs: list[str]) -> dict:
    guard()
    disks = resolve_disks()
    stamp = time.strftime("%Y%m%d-%H%M%S")
    k = Knobs(knobs)
    res: dict = {"name": name, "loads": loads, "knobs": knobs, "secs": secs, "stamp": stamp}
    total = secs + RAMP_S + 4
    threads: list[threading.Thread] = []
    iperf: dict = {}
    procs: list[subprocess.Popen] = []
    try:
        k.apply()
        box(f"pkill -f '[i]perf3 -s -p {IPERF_PORT}'; true")  # own call: the pattern text must not be in the starter's cmdline
        box(f"iperf3 -s -p {IPERF_PORT} -D")
        local(["ping", "-c1", "-W1", "-I", HOST_IF, "192.168.25.41"], 5)
        for ld in loads:
            m = re.fullmatch(r"net-(rx|tx)(\d*)", ld)
            if m:
                n = m.group(2) or "4"
                cmd = ["iperf3", "-c", "192.168.25.41", "-p", str(IPERF_PORT), "-t", str(total),
                       "-P", n, "--bind-dev", HOST_IF, "-J"] + (["-R"] if m.group(1) == "tx" else [])  # fmt: skip
                procs.append(subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True))
                iperf[ld] = procs[-1]
            else:
                devs = {
                    "sata": [disks[SATA_SSD]],
                    "hdd": [disks[s] for s in HDDS],
                    "usb": [disks[s] for s in USB_SSDS],
                }[ld]
                t = threading.Thread(target=lambda c=fio_cmd(devs, total): box(c, total + 30))
                t.start()
                threads.append(t)
        threading.Event().wait(RAMP_S)  # ramp: see RAMP_S
        pf = ""
        if perf:
            pf = (
                "rm -f /dev/shm/ovh.*; "
                "perf record -a -F 499 -o /dev/shm/ovh.data >/dev/shm/ovh.rec 2>&1 & echo $! > /dev/shm/ovh.rpid; "
                "perf stat -a -e cycles,instructions -o /dev/shm/ovh.stat >/dev/null 2>&1 & echo $! > /dev/shm/ovh.spid; "
            )
        a = parse_snap(box(pf + SNAP_CMD))
        threading.Event().wait(secs)
        b_txt = box(SNAP_CMD)
        b = parse_snap(b_txt)
        d = diff(a, b)
        if perf:
            box("kill -INT $(cat /dev/shm/ovh.spid) $(cat /dev/shm/ovh.rpid); true")
            threading.Event().wait(2)  # perf flushes data file on SIGINT
            res["perf_stat"] = box("cat /dev/shm/ovh.stat | grep -E 'cycles|instr|elapsed'").strip()
            res["perf_top"] = box(
                "perf report -i /dev/shm/ovh.data --no-children --sort symbol --stdio 2>/dev/null | grep -v '^#' | grep -v '^$' | head -22",
                90,
            ).strip().splitlines()
            m = re.search(r"([\d,]+)\s+cycles", res["perf_stat"])
            if m:
                cyc = int(m.group(1).replace(",", ""))
                res["cycles_per_s"] = cyc / d["dt_s"] if d["dt_s"] else 0
        res["diff"] = d
        moved = d["net_rx_MBps"] + d["net_tx_MBps"] + sum(d["disk_MBps"].values())
        res["total_MBps"] = round(moved, 1)
        if moved > 0:
            net = max(d["busy_of_400"] - d["bg_noise_pct_of_400"], 0)
            res["cpu_s_per_GB"] = round(net / 100 * 1000 / moved, 2)  # busy cores (bg noise removed) / (MB/s) * 1000
            if "cycles_per_s" in res:
                res["Mcycles_per_MB"] = round(res["cycles_per_s"] / 1e6 / moved, 2)
    finally:
        for p in procs:
            try:
                out, _ = p.communicate(timeout=total + 20)
                j = json.loads(out[out.find("{") :])
                res.setdefault("iperf", {})[f"{iperf and [k2 for k2, v in iperf.items() if v is p][0]}"] = {
                    "Mbps": round(j["end"]["sum_received"]["bits_per_second"] / 1e6),
                    "retr": j["end"].get("sum_sent", {}).get("retransmits"),
                }
            except Exception as e:  # noqa: BLE001
                log.info("iperf result parse failed: %s", e)
                p.kill()
        for t in threads:
            t.join(total + 40)
        box(f"pkill -f '[i]perf3 -s -p {IPERF_PORT}'; true")
        k.restore()
    out = REPO / "bench-history" / f"overhead-{stamp}-{name}.json"
    out.write_text(json.dumps(res, indent=1))
    log.info("wrote %s", out)
    return res


def show(res: dict) -> None:
    d = res["diff"]
    print(f"== {res['name']} loads={res['loads']} knobs={res['knobs']}")
    print(f"  moved {res['total_MBps']} MB/s  rx {d['net_rx_MBps']} tx {d['net_tx_MBps']} disks {d['disk_MBps']}")
    print(f"  busy {d['busy_of_400']}%/400  per-cpu {[d['busy_pct'].get(f'cpu{i}') for i in range(4)]}  split {d['split_pct']}")
    print(f"  irq/s {d['irq_s_total']} per-cpu {d['irq_s_per_cpu']}  ctxt/s {d['ctxt_s']}")
    print(f"  softirq/s {d['softirq_s_per_cpu']}")
    print(f"  cpu-s/GB {res.get('cpu_s_per_GB')}  Mcycles/MB {res.get('Mcycles_per_MB')}  iperf {res.get('iperf')}")
    for k, v in d["irq_top"].items():
        print(f"    irq {k}: {v}")
    for ln in res.get("perf_top", [])[:14]:
        print("   ", ln)


def main() -> None:
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("run")
    r.add_argument("name")
    r.add_argument("loads", help="comma list: net-rx4,net-tx1,sata,hdd,usb or 'idle'")
    r.add_argument("--knob", action="append", default=[])
    r.add_argument("--secs", type=int, default=15)
    r.add_argument("--perf", action="store_true")
    sub.add_parser("knobs")
    t = sub.add_parser("table", help="one line per bench-history/overhead-<stamp>-*.json from today")
    t.add_argument("pattern", nargs="?", default="")
    a = ap.parse_args()
    if a.cmd == "table":
        for f in sorted((REPO / "bench-history").glob("overhead-*.json")):
            r = json.loads(f.read_text())
            if a.pattern not in r["name"] and a.pattern not in " ".join(r["knobs"]):
                continue
            d = r["diff"]
            sf = d["softirq_s_per_cpu"]
            print(
                f"{r['name'][:30]:30} {r['total_MBps']:7} MB/s busy {d['busy_of_400']:5}% bg {d.get('bg_noise_pct_of_400')} "
                f"cpu-s/GB {r.get('cpu_s_per_GB')} irq/s {d['irq_s_total']:6} ctxt/s {d['ctxt_s']:6} "
                f"cpus {[d['busy_pct'].get(f'cpu{i}') for i in range(4)]} "
                f"NET_RX {sum(sf.get('NET_RX', [0]))} BLOCK {sum(sf.get('BLOCK', [0]))} {r.get('iperf') or ''} {r['knobs'] or ''}"
            )
        return
    if a.cmd == "knobs":
        print("\n".join(KNOB_NAMES))
        return
    loads = [] if a.loads == "idle" else a.loads.split(",")
    show(run_once(a.name, loads, a.secs, a.perf, a.knob))


if __name__ == "__main__":
    main()
