#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Concurrent long-duration soak of ethernet + SATA + USB on woomera.

bench-all.py answers "is throughput still what it was" in ~2 minutes. This
answers a different question: does anything break when all three DMA engines
are driven hard, at the same time, for an hour. #90's UDMA TX hang and the
al_dma queue-state bugs are the class of fault that only shows up under
sustained concurrent load, not in a short single-subsystem run.

Three workers run in parallel for --duration, each looping its own workload:

  eth   iperf3 to the dev host on BOTH al_eth ports (1G enp0s1, 10G enp0s2),
        alternating direction each round so TX and RX both stay hot. Runs on
        the DEV HOST and connects in, so a box-side wedge is observable rather
        than taking the measurement process down with it.
  sata  fio --direct=1 sequential read on every non-root SATA disk, round
        robin. O_DIRECT only - `hdparm -t` understates by ~4x on this box and
        is not evidence. WRITES ARE NEVER ISSUED: see _safe_disks().
  usb   fio --direct=1 sequential read on the USB-attached SSD.

Health is sampled every --sample-interval into the log so a crash leaves a
partial record: dmesg tail, per-port ethtool -S deltas, interrupt counts,
link state. dmesg is captured whole before and after and diffed; any new
WARN/ERR/oops/call-trace is reported as a finding.

Safety: the script refuses to write to any disk that is mounted, holds the
root filesystem, or carries an ESP. It only ever opens disks O_RDONLY via
fio's read workloads - there is no write path in this script at all.

    ./scripts/soak-test.py                      # 1 hour, all three
    ./scripts/soak-test.py --duration 300       # 5 min smoke test
    ./scripts/soak-test.py --skip-usb
"""

from __future__ import annotations

import argparse
import datetime
import json
import re
import shlex
import subprocess
import sys
import threading
import time
from pathlib import Path

import _box

REPO = Path(__file__).resolve().parent.parent
LOG = REPO / "tmp" / "logs" / "soak-test.log"

# Box-side interface <-> dev-host interface pairing. Same fixed pairing as
# bench-all.py: both machines have exactly one 1G and one 10G port, and
# auto-detecting "the fastest NIC" picks the wrong one when both are tested.
ETH_PAIRS = (
    {"box": "enp0s1", "dev": "enp5s0", "label": "1G", "port": 5612},
    {"box": "enp0s2", "dev": "enp7s0", "label": "10G", "port": 5613},
)

SSH_OPTS = _box.SSH_OPTS_BATCH

# dmesg lines worth reporting. Deliberately broad - a soak that stays silent
# through a call trace is worse than one that reports a few benign lines.
DMESG_BAD = re.compile(
    r"WARNING|BUG:|Oops|Call trace|panic|Unable to handle|SError|"
    r"segfault|hung task|rcu_sched|soft lockup|hard LOCKUP|"
    r"I/O error|ata\d+\.\d+: failed|reset failed|link is down|"
    r"tx timeout|Tx timeout|watchdog|EDAC|uncorrect",
    re.IGNORECASE,
)


def now() -> str:
    return datetime.datetime.now().astimezone().isoformat(timespec="seconds")


_log_lock = threading.Lock()


def log(msg: str) -> None:
    """Append to the log AND stdout, flushed every line - a soak that dies at
    minute 50 must leave the first 49 minutes on disk."""
    line = f"{now()}  {msg}"
    with _log_lock:
        print(line, flush=True)
        with LOG.open("a") as fh:
            fh.write(line + "\n")
            fh.flush()


class Box:
    """SSH to the box. Address is resolved once at startup and reused, because
    re-resolving mid-soak would mask exactly the network fault we are hunting."""

    def __init__(self, host: str) -> None:
        self.host = host

    def run(self, cmd: str, timeout: int = 120) -> subprocess.CompletedProcess:
        """Timeout: sized per call site. Default 120 s covers the longest
        single fio round (60 s runtime + fio startup + ssh setup) at ~2x.
        On expiry the caller logs the command and the elapsed limit."""
        return subprocess.run(
            ["ssh", *SSH_OPTS, f"root@{self.host}", cmd],
            capture_output=True,
            text=True,
            check=False,
            timeout=timeout,
        )

    def out(self, cmd: str, timeout: int = 120) -> str:
        try:
            return self.run(cmd, timeout=timeout).stdout
        except subprocess.TimeoutExpired:
            log(f"TIMEOUT after {timeout}s running on box: {cmd}")
            return ""

    def alive(self) -> bool:
        try:
            return self.run("echo ok", timeout=15).stdout.strip() == "ok"
        except subprocess.TimeoutExpired:
            return False


def resolve_box(explicit: str | None) -> str:
    """Find woomera by MAC via _box.locate() - the project rule, no IP
    fallback. The ARP-flux case that once tempted one (the 1G IP resolving
    to the 10G port's MAC) is fixed in the resolver itself: cdc58d1 accepts
    either of the box's own MACs."""
    if explicit:
        return explicit
    cand = _box.locate()
    if cand:
        return cand
    sys.exit("FATAL: could not resolve woomera by MAC (_box.locate)")


# --------------------------------------------------------------------------
# disk safety


def _safe_disks(box: Box) -> tuple[list[str], str | None]:
    """Whole SATA disks that are safe to READ, plus the USB disk.

    Every disk here is opened read-only. The exclusion of mounted/root disks is
    belt-and-braces: it means a future edit that adds a write workload cannot
    silently target the boot SSD.
    """
    mounts = box.out("cat /proc/mounts")
    lsblk = box.out("lsblk -P -o NAME,TYPE,MOUNTPOINT,PKNAME,SIZE")

    busy: set[str] = set()
    for line in lsblk.splitlines():
        f = dict(re.findall(r'(\w+)="([^"]*)"', line))
        if not f:
            continue
        # A mounted partition makes its PARENT disk off-limits for writes.
        if f.get("MOUNTPOINT"):
            busy.add(f.get("PKNAME") or f.get("NAME"))
    # /dev/root hides the real device name in /proc/mounts; resolve it.
    if "/dev/root" in mounts:
        rootdev = box.out("findmnt -n -o SOURCE / ; readlink -f /dev/root").split()
        for tok in rootdev:
            m = re.search(r"(sd[a-z])", tok)
            if m:
                busy.add(m.group(1))

    sata, usb = [], None
    for line in lsblk.splitlines():
        f = dict(re.findall(r'(\w+)="([^"]*)"', line))
        if f.get("TYPE") != "disk":
            continue
        name = f["NAME"]
        if name.startswith("zram"):
            continue
        path = box.out(f"readlink -f /sys/block/{name}").strip()
        if "/usb" in path:
            usb = usb or f"/dev/{name}"
            continue
        sata.append((f"/dev/{name}", name in busy))
    # Read-only workload, so a busy disk is still readable - but rank the
    # idle ones first so the boot SSD is not the disk we hammer hardest.
    sata.sort(key=lambda t: t[1])
    return [d for d, _ in sata], usb


# --------------------------------------------------------------------------
# workers


class Worker(threading.Thread):
    def __init__(self, name: str, deadline: float) -> None:
        super().__init__(name=name, daemon=True)
        self.deadline = deadline
        self.results: list[dict] = []
        self.errors: list[str] = []

    def left(self) -> float:
        return self.deadline - time.monotonic()


class EthWorker(Worker):
    """iperf3 from the dev host to the box, both ports, alternating direction.

    Server runs on the BOX, client on the dev host, so if the box's TX path
    hangs (#90) the client reports it rather than the measurement dying with
    the box."""

    def __init__(self, box: Box, deadline: float, round_s: int) -> None:
        super().__init__("eth", deadline)
        self.box = box
        self.round_s = round_s

    def _start_servers(self) -> bool:
        """Start one iperf3 daemon per port and CONFIRM it is listening.

        The pkill must not be in the same shell command as the start: `pkill -f
        'iperf3 -s -p N'` matches the very command string it is running inside,
        so the server is killed the instant it comes up and every later connect
        fails with an empty JSON body."""
        ok = True
        for p in ETH_PAIRS:
            port = p["port"]
            self.box.run(f"pkill -f 'iperf3 -s -p {port}'", timeout=30)
            self.box.run(f"nohup iperf3 -s -p {port} -D >/dev/null 2>&1", timeout=30)
            listening = self.box.out(
                f"ss -lnt | grep -c ':{port} ' || true", timeout=30
            ).strip()
            if listening == "0" or not listening:
                self.errors.append(
                    f"eth {p['label']}: iperf3 server not listening on port {port}"
                )
                log(f"FAIL eth {p['label']}: iperf3 server did not start on {port}")
                ok = False
            else:
                log(f"eth {p['label']}: iperf3 server listening on {port}")
        return ok

    def _one(self, pair: dict, reverse: bool) -> None:
        # Bind the dev-host socket to the matching local NIC. Both box IPs are
        # on one subnet, so without --bind-dev the kernel sends BOTH flows out
        # whichever interface owns the route (here: the dev host's 10G port),
        # and the "1G" number is really the 10G NIC talking to the box's 1G
        # port. Same class as #121.
        cmd = [
            "iperf3",
            "-c",
            pair_ip(pair["label"]),
            "-p",
            str(pair["port"]),
            "-t",
            str(self.round_s),
            "-J",
            "--connect-timeout",
            "5000",
            "--bind-dev",
            pair["dev"],
        ]
        if reverse:
            cmd.append("-R")
        try:
            r = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                check=False,
                # 1.5x the round runtime plus 15 s for connect/teardown.
                timeout=int(self.round_s * 1.5) + 15,
            )
        except subprocess.TimeoutExpired:
            msg = f"eth {pair['label']} {'RX' if reverse else 'TX'}: iperf3 TIMED OUT"
            self.errors.append(msg)
            log("FAIL " + msg)
            return
        try:
            j = json.loads(r.stdout)
            end = j["end"]["sum_received"]
            gbps = end["bits_per_second"] / 1e9
            retr = j["end"].get("sum_sent", {}).get("retransmits")
            rec = {
                "iface": pair["label"],
                "dir": "RX" if reverse else "TX",
                "gbps": round(gbps, 3),
                "retransmits": retr,
                "t": now(),
            }
            self.results.append(rec)
            log(f"eth {pair['label']} {rec['dir']}: {gbps:.2f} Gbit/s retr={retr}")
        except (ValueError, KeyError):
            # iperf3 reports a refused connection in JSON's "error" field, not
            # on stderr - pull that out rather than logging the whole blob.
            err = ""
            try:
                err = json.loads(r.stdout).get("error", "")
            except ValueError:
                pass
            err = (err or r.stderr or r.stdout)[:160].replace("\n", " ")
            self.errors.append(f"eth {pair['label']}: {err}")
            log(f"FAIL eth {pair['label']}: {err}")

    def run(self) -> None:
        if not self._start_servers():
            log("eth: no server, worker exiting rather than spinning")
            return
        rev = False
        # A failed round returns instantly, so without this the loop retries
        # thousands of times a second and floods the log. Back off and give
        # the server one restart before giving up on the port entirely.
        consecutive_fail = 0
        while self.left() > self.round_s:
            before = len(self.errors)
            for pair in ETH_PAIRS:
                if self.left() <= 0:
                    break
                self._one(pair, rev)
            rev = not rev
            if len(self.errors) > before:
                consecutive_fail += 1
                # A link flap makes connects fail for a few seconds and then
                # recover, so never give up on the port - just restart the
                # servers periodically and keep going. Giving up would hide a
                # flap that resolves, which is precisely what we are measuring.
                if consecutive_fail % 3 == 0:
                    log(
                        f"eth: {consecutive_fail} consecutive failed rounds, "
                        f"restarting servers (link flap?)"
                    )
                    for p in ETH_PAIRS:
                        st = self.box.out(
                            f"cat /sys/class/net/{p['box']}/operstate", timeout=20
                        ).strip()
                        log(f"eth {p['label']} operstate={st}")
                    self._start_servers()
                # Back off so a hard failure does not burn the hour retrying.
                time.sleep(min(5 * consecutive_fail, 30))
            else:
                consecutive_fail = 0


def pair_ip(label: str) -> str:
    return ETH_IPS[label]


ETH_IPS: dict[str, str] = {}


class DiskWorker(Worker):
    """fio --direct=1 sequential read, round robin over the given devices.

    O_DIRECT bypasses the page cache, which is the whole point: without it the
    second pass reads RAM and reports a number the disk cannot do."""

    def __init__(
        self, name: str, box: Box, deadline: float, devs: list[str], round_s: int
    ) -> None:
        super().__init__(name, deadline)
        self.box = box
        self.devs = devs
        self.round_s = round_s

    def _one(self, dev: str) -> None:
        # --readonly is fio's own guard: it refuses any write workload on the
        # job even if one were configured. Defence in depth over rw=read.
        cmd = (
            f"fio --name=soak --filename={shlex.quote(dev)} --rw=read "
            f"--direct=1 --readonly --bs=1M --ioengine=libaio --iodepth=16 "
            f"--runtime={self.round_s} --time_based --output-format=json"
        )
        out = self.box.out(cmd, timeout=self.round_s * 2 + 30)
        if not out:
            self.errors.append(f"{self.name} {dev}: fio produced no output")
            log(f"FAIL {self.name} {dev}: fio produced no output")
            return
        try:
            j = json.loads(out)
            r = j["jobs"][0]["read"]
            mbs = r["bw_bytes"] / 1e6
            rec = {
                "dev": dev,
                "mb_s": round(mbs, 1),
                "iops": round(r["iops"], 1),
                "lat_us_mean": round(r["lat_ns"]["mean"] / 1000, 1),
                "err": j["jobs"][0].get("error", 0),
                "t": now(),
            }
            self.results.append(rec)
            log(
                f"{self.name} {dev}: {mbs:.1f} MB/s iops={rec['iops']} "
                f"lat={rec['lat_us_mean']}us err={rec['err']}"
            )
        except (ValueError, KeyError, IndexError) as e:
            self.errors.append(f"{self.name} {dev}: unparseable fio output ({e})")
            log(f"FAIL {self.name} {dev}: unparseable fio output ({e})")

    def run(self) -> None:
        i = 0
        while self.left() > self.round_s:
            self._one(self.devs[i % len(self.devs)])
            i += 1


# --------------------------------------------------------------------------
# health sampling


def counters(box: Box) -> dict:
    snap: dict = {}
    for p in ETH_PAIRS:
        raw = box.out(f"ethtool -S {p['box']} 2>/dev/null", timeout=30)
        d = {}
        for line in raw.splitlines():
            if ":" in line:
                k, _, v = line.partition(":")
                v = v.strip()
                if v.isdigit():
                    d[k.strip()] = int(v)
        snap[p["box"]] = d
        snap[p["box"] + "_link"] = box.out(
            f"cat /sys/class/net/{p['box']}/operstate", timeout=20
        ).strip()
    snap["interrupts"] = box.out("cat /proc/interrupts", timeout=30)
    return snap


def interesting_delta(before: dict, after: dict) -> list[str]:
    """Counter changes worth a human look: anything error-shaped that moved."""
    out = []
    for p in ETH_PAIRS:
        b, a = before.get(p["box"], {}), after.get(p["box"], {})
        for k in sorted(set(b) | set(a)):
            if not re.search(
                r"err|drop|discard|fail|reset|overrun|"
                r"crc|abort|timeout|underrun|missed",
                k,
                re.IGNORECASE,
            ):
                continue
            d = a.get(k, 0) - b.get(k, 0)
            if d:
                out.append(f"{p['box']}: {k} +{d} ({b.get(k, 0)} -> {a.get(k, 0)})")
        lb, la = before.get(p["box"] + "_link"), after.get(p["box"] + "_link")
        if lb != la:
            out.append(f"{p['box']}: operstate {lb} -> {la}")
    return out


def dmesg_new(before: str, after: str) -> list[str]:
    """Lines present after but not before. dmesg is a ring buffer, so compare
    by line content after stripping the timestamp rather than by offset."""

    def strip(s: str) -> list[str]:
        return [re.sub(r"^\[\s*\d+\.\d+\]\s*", "", ln) for ln in s.splitlines()]

    seen = set(strip(before))
    return [ln for ln in strip(after) if ln not in seen]


# --------------------------------------------------------------------------


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--duration",
        type=int,
        default=3600,
        help="total soak seconds (default 3600 = 1 hour)",
    )
    ap.add_argument(
        "--eth-round",
        type=int,
        default=30,
        help="seconds per iperf3 round (default 30)",
    )
    ap.add_argument(
        "--disk-round", type=int, default=60, help="seconds per fio round (default 60)"
    )
    ap.add_argument(
        "--sample-interval",
        type=int,
        default=120,
        help="seconds between health samples (default 120)",
    )
    ap.add_argument("--host", help="box address (default: resolve by MAC)")
    ap.add_argument("--skip-eth", action="store_true")
    ap.add_argument("--skip-sata", action="store_true")
    ap.add_argument("--skip-usb", action="store_true")
    ap.add_argument("--out", type=Path, default=REPO / "tmp" / "soak-result.json")
    args = ap.parse_args()

    LOG.parent.mkdir(parents=True, exist_ok=True)
    log("=" * 70)
    log(f"SOAK START duration={args.duration}s")

    host = resolve_box(args.host)
    box = Box(host)
    if not box.alive():
        sys.exit(f"FATAL: box at {host} not answering ssh")
    log(f"box={host}  {box.out('uname -a').strip()}")

    # Resolve each box port's own address so iperf3 targets the right NIC.
    for p in ETH_PAIRS:
        ip = box.out(
            f"ip -4 -br addr show {p['box']} | awk '{{print $3}}' | cut -d/ -f1"
        ).strip()
        ETH_IPS[p["label"]] = ip
        log(f"eth {p['label']} {p['box']} -> {ip}")

    sata, usb = _safe_disks(box)
    log(f"SATA read targets: {sata}")
    log(f"USB read target:   {usb}")
    log("all disk workloads are READ-ONLY (fio --readonly, rw=read)")

    dmesg_before = box.out("dmesg", timeout=60)
    (REPO / "tmp" / "soak-dmesg-before.txt").write_text(dmesg_before)
    ctr_before = counters(box)
    smart_before = {
        d: box.out(f"smartctl -A {d} 2>/dev/null", timeout=40)
        for d in sata + ([usb] if usb else [])
    }

    deadline = time.monotonic() + args.duration
    workers: list[Worker] = []
    if not args.skip_eth:
        workers.append(EthWorker(box, deadline, args.eth_round))
    if not args.skip_sata and sata:
        workers.append(DiskWorker("sata", box, deadline, sata, args.disk_round))
    if not args.skip_usb and usb:
        workers.append(DiskWorker("usb", box, deadline, [usb], args.disk_round))
    if not workers:
        sys.exit("FATAL: nothing to run")

    for w in workers:
        w.start()
    log(f"started workers: {[w.name for w in workers]}")

    # Health sampling loop. Also the liveness check: if the box stops
    # answering, that is the result and we stop rather than run out the clock.
    wedged_at: float | None = None
    start = time.monotonic()
    next_sample = start + args.sample_interval
    while time.monotonic() < deadline and any(w.is_alive() for w in workers):
        time.sleep(1)
        if time.monotonic() < next_sample:
            continue
        next_sample += args.sample_interval
        elapsed = int(time.monotonic() - start)
        if not box.alive():
            wedged_at = elapsed
            log(f"*** BOX NOT RESPONDING at t+{elapsed}s - soak aborted ***")
            break
        tail = box.out("dmesg | tail -40", timeout=30)
        bad = [ln for ln in dmesg_new(dmesg_before, tail) if DMESG_BAD.search(ln)]
        for ln in bad:
            log(f"DMESG t+{elapsed}s: {ln}")
        # Link flaps are a headline result here, not incidental noise: both
        # al_eth ports were seen flapping before the soak even started.
        for p in ETH_PAIRS:
            st = box.out(f"cat /sys/class/net/{p['box']}/operstate", timeout=20).strip()
            if st != "up":
                log(f"LINK t+{elapsed}s: {p['box']} operstate={st}")
        log(
            f"--- t+{elapsed}s/{args.duration}s alive, "
            f"load={box.out('cat /proc/loadavg', timeout=20).split()[:3]}"
        )

    for w in workers:
        w.join(timeout=180)

    log("workers done, collecting final state")
    alive = box.alive()
    dmesg_after = box.out("dmesg", timeout=60) if alive else ""
    if dmesg_after:
        (REPO / "tmp" / "soak-dmesg-after.txt").write_text(dmesg_after)
    ctr_after = counters(box) if alive else {}
    smart_after = (
        {
            d: box.out(f"smartctl -A {d} 2>/dev/null", timeout=40)
            for d in sata + ([usb] if usb else [])
        }
        if alive
        else {}
    )

    new_lines = dmesg_new(dmesg_before, dmesg_after) if dmesg_after else []
    findings = [ln for ln in new_lines if DMESG_BAD.search(ln)]
    flaps = [ln for ln in new_lines if re.search(r"Link is (Up|Down)", ln)]
    log(f"link transitions during soak: {len(flaps)}")
    for f in flaps:
        log(f"  LINK: {f}")
    deltas = interesting_delta(ctr_before, ctr_after) if ctr_after else []

    log("=" * 70)
    log(f"SOAK END  survived={alive and wedged_at is None}")
    if wedged_at is not None:
        log(f"BOX WEDGED at t+{wedged_at}s")
    for w in workers:
        log(f"{w.name}: {len(w.results)} rounds, {len(w.errors)} errors")
        for e in w.errors[:20]:
            log(f"  ERR {w.name}: {e}")
    log(f"new dmesg lines: {len(new_lines)}, of which flagged: {len(findings)}")
    for f in findings:
        log(f"  DMESG FINDING: {f}")
    log(f"counter deltas of interest: {len(deltas)}")
    for d in deltas:
        log(f"  COUNTER: {d}")

    # Per-subsystem summary with the conditions attached, so a number in the
    # log can be judged later rather than floating free.
    summary: dict = {}
    for w in workers:
        if not w.results:
            continue
        if w.name == "eth":
            for key in {(r["iface"], r["dir"]) for r in w.results}:
                vals = [r["gbps"] for r in w.results if (r["iface"], r["dir"]) == key]
                summary[f"eth_{key[0]}_{key[1]}_gbps"] = {
                    "min": round(min(vals), 3),
                    "max": round(max(vals), 3),
                    "mean": round(sum(vals) / len(vals), 3),
                    "n": len(vals),
                    "round_s": args.eth_round,
                }
        else:
            for dev in {r["dev"] for r in w.results}:
                vals = [r["mb_s"] for r in w.results if r["dev"] == dev]
                summary[f"{w.name}_{dev}_MBps"] = {
                    "min": min(vals),
                    "max": max(vals),
                    "mean": round(sum(vals) / len(vals), 1),
                    "n": len(vals),
                    "round_s": args.disk_round,
                    "bs": "1M",
                    "iodepth": 16,
                    "direct": 1,
                }
    for k, v in summary.items():
        log(f"SUMMARY {k}: {v}")

    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(
        json.dumps(
            {
                "started": now(),
                "duration_s": args.duration,
                "host": host,
                "survived": alive and wedged_at is None,
                "wedged_at_s": wedged_at,
                "summary": summary,
                "rounds": {w.name: w.results for w in workers},
                "errors": {w.name: w.errors for w in workers},
                "dmesg_findings": findings,
                "link_transitions": flaps,
                "dmesg_new_count": len(new_lines),
                "counter_deltas": deltas,
                "smart_before": smart_before,
                "smart_after": smart_after,
            },
            indent=2,
        )
    )
    log(f"wrote {args.out}")

    return 0 if (alive and wedged_at is None and not findings) else 1


if __name__ == "__main__":
    sys.exit(main())
