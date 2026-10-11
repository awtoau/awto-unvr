#!/usr/bin/env python3
"""The one test/benchmark harness for woomera. Add subtests here; do not add
another script (see "Not in here"). One run writes a comparable JSON snapshot;
--compare diffs two; --idle runs the suite unattended and prints only trouble.

Subtests (each is a function taking a Report; metrics land in the snapshot and
--compare / --idle pick them up by name):
- health: al_eth vermagic vs running kernel, carrier + speed, MAC error
  counters, kernel-log trouble lines. (test-eth.py's non-throughput half.)
- eth: bidirectional iperf3, 4 streams, per port (1G, 10G, any USB NIC), bound
  with SO_BINDTODEVICE (`-B ip%iface`); a plain -B bind picks the wrong NIC (#121).
- crypto: AES-256-XTS via AF_ALG. It binds by generic name, so it measures
  whichever driver wins on priority; /proc/crypto names it. al_ssm cannot win:
  AF_ALG masks it by design (#267), so these are CE numbers.
- disk / usb: `hdparm -t --direct` on each disk (read-only, root disk included).
  USB also records the negotiated link speed. hdparm understates ~4x against
  `fio --direct=1`: use the numbers for A/B only.
- dma: dmatest on dma0chan0, copy + xor, 20000 iterations; >0 failures FAILs,
  iops are metrics. Then an oversize run (131072 B; only 'prep error' may fail,
  #290). Every run starts and ends with `echo 0 > run`, so back-to-back runs
  exercise channel free/alloc (#289). dmatest is never unloaded: a dmatest
  thread outlives the module and holds the channel until reboot. A held channel
  (`in_use` > 0 with run=N) or an un-stoppable run is a SKIP, never a hang.
  The old "reload wedges the engine, so no dma" exclusion (#23) was our missing
  queue reset (#289).
- opt-in, never in --idle: --dma-reload (rmmod/modprobe al_dma; wedge risk until
  #289 is verified on hardware, reboot after), --dma-pq (#169), --ssm (al_ssm
  through dm-crypt: ops landed, cmp integrity, quiesce; drives #300/#182),
  --stress (memtester + stress-ng; heavy, needs both installed).

Baselines (bench-history/baseline.json; --idle compares against it):
- Measured 2026-10-11, kernel 7.3.0-rc6-6bd513f+, one run each:
  - dma0chan0, 20000 iterations: copy 0 failures 13716 iops; xor 0 failures
    12743 iops.
  - eth 10G, `--bind-dev enp0s2`, 4 streams: 8.59-9.38 Gbit/s, 0 retransmits.
    Stored: 8590 (low end). Taken one direction at a time; this suite runs
    --bidir, so the first attended run may sit lower: re-baseline.
  - eth 1G: 93.9 Mbit/s. The link partner advertises only 10/100, so the link
    is 100 Mb/s (not a driver fault).
  - link speeds: enp0s1 100, enp0s2 10000 Mb/s.
- No stored baseline: crypto, sata, usb. Their checks still FAIL on error.
- Tolerances: 10% below baseline (eth, --compare's threshold), 20% for dma
  iops (one baseline run; CPU scheduling moves dmatest), 0% for link speed.
- Re-baseline: an all-green attended run with --save-baseline.

Idle mode (--idle):
- Loop: probe, run the suite, wait --idle-gap (300 s), repeat. Stop: Ctrl-C or
  SIGTERM; the cycle's dmatest run and iperf3 servers are stopped first.
- "Otherwise idle" = none of: 1-min load > 0.75; a dmatest run or thread on the
  box; box processes (iperf3, fio, dd, hdparm, rsync, scp, dnf, modprobe,
  badblocks, nandwrite, ...); a login on the box active in the last 300 s
  (tty atime; --idle-allow-sessions ignores this); a deploy, flash, build or
  other benchmark on this host; an iperf3 client on this host's :5701 server.
  Re-checked before every subtest (load excluded: it is ours); a box that stops
  being idle discards the cycle. Busy: log the reason, re-probe in 60 s.
- Terminal gets only: FAIL <check>, REGRESSION <metric> (needs 2 consecutive
  cycles), RECOVERED, STARVED (30 busy cycles in a row), WEDGED (an ssh call
  hit its limit or the engine would not stop: exit 2, reboot the box).
  A finding is printed when it appears, not again while it persists.
- Everything (skips, per-cycle summary, metrics when something is off) goes to
  tmp/logs/bench-all.log; the last full snapshot to tmp/bench-idle-last.json.
- One instance at a time (tmp/bench-all-idle.lock).
- Health looks only at kernel-log lines and counter growth since idle started.

Not in here (standalone on purpose):
- soak-test.py: hour-long concurrent eth+SATA+USB load; a different question
  (`./dev.py soak-test`).
- nand-write-test.py: flash writes, needs per-run approval.
- i2c-wedge-test.py: provokes the bus wedge on purpose. usb-disk-bench.py:
  destructive `badblocks -w` integrity pass, runs on the box.
- reboot-verify-woomera.py: reboots the box. uboot-*.py/*.tcl, cpuon-xhci-test.py,
  catch-uboot.py: serial console / U-Boot stage.
- verify-nand-reads.py, sfp-ddm.py, eth-watch.py, tx-gap-probe.py: single-issue
  diagnostics (NAND reads vs a backup, i2c DDM, #111/#121 probes).
- Retired to debris/scripts/: al-dma-verify.py, test-eth.py, al-ssm-stress.py,
  mem-stress-woomera.py, hwverify-woomera.py (their checks live above).

  ./scripts/bench-all.py                            # full run, snapshot to bench-history/
  ./scripts/bench-all.py --out tmp/before.json      # throwaway snapshot instead
  ./scripts/bench-all.py --compare tmp/before.json tmp/after.json
  ./scripts/bench-all.py --skip-eth --skip-crypto --skip-disk --skip-usb   # health + dma
  ./scripts/bench-all.py --dma-reload --i-accept-a-reboot
  ./scripts/bench-all.py --ssm                      # al_ssm through dm-crypt
  ./scripts/bench-all.py --save-baseline            # all-green run -> baseline.json
  ./scripts/bench-all.py --idle                     # unattended, errors only

Snapshots default to bench-history/ (git-tracked, NOT tmp/) so there is a
committed record to diff against: `git log --oneline bench-history/` is the run
history. Commit each snapshot right after it is taken.
"""

from __future__ import annotations

import argparse
import base64
import datetime
import fcntl
import json
import os
import re
import secrets
import signal
import subprocess
import sys
import threading
import time
from dataclasses import dataclass
from pathlib import Path

import _box

DEFAULT_ROOT_PASSWORD = "unvr"  # documented default, see docs/fedora-on-ssd.md
LOG = Path("tmp/logs/bench-all.log")
LOCK = Path("tmp/bench-all-idle.lock")  # one --idle run at a time

# Fixed NIC pairing - both box and dev host have exactly one 1G and one 10G
# port; auto-detecting "the fastest local NIC" only
# works when testing a single target, not when we need both directions
# tested independently.
ETH_PAIRS = (
    {"box_iface": "enp0s1", "dev_iface": "enp5s0", "label": "1G", "port": 5602},
    {"box_iface": "enp0s2", "dev_iface": "enp7s0", "label": "10G", "port": 5603},
)

# Dev-host NICs available to pair against USB-attached box interfaces, which
# are discovered at run time (below) rather than hardcoded: their names encode
# the USB topology (enP1p1s0u1u1, ...) and change if the dock is replugged
# into a different port or hub chain.
USB_ETH_DEV_IFACES = ("enp6s0f0", "enp6s0f1")
USB_ETH_BASE_PORT = 5604

CRYPTO_ALG = "xts(aes)"

SATA_DEVICES = ("sdb", "sdc", "sdd", "sde")
USB_DEVICES = ("sda",)

REGRESSION_THRESHOLD = 0.10  # 10% drop flagged in --compare


def locate_woomera() -> str:
    return _box.require()


def ssh_cmd(host: str, password: str) -> list[str]:
    return [*_box.sshpass_argv(password, connect_timeout=8), f"root@{host}"]


def run_remote(
    host: str, password: str, remote_script: str, timeout: int = 60
) -> tuple[int, str]:
    """Never raises TimeoutExpired - an uncaught one here crashed this whole
    suite live (crypto's own SIGALRM only bounds a milder interruptible
    hang; a genuine kernel D-state wedge on the box, as actually observed,
    leaves the remote process stuck regardless - killing this LOCAL
    subprocess on timeout does not and cannot force-kill it, only a reboot
    of the box does. This just keeps that failure local and reported
    instead of taking down every other check in the suite with it."""
    try:
        result = subprocess.run(
            ssh_cmd(host, password) + [remote_script],
            capture_output=True,
            text=True,
            check=False,
            timeout=timeout,
        )
        return result.returncode, result.stdout + result.stderr
    except subprocess.TimeoutExpired:
        return 124, (
            f"ssh call timed out after {timeout}s - if this was the crypto "
            f"benchmark, the remote process may still be stuck (kernel D-state) "
            f"and need a reboot to clear, not just a retry"
        )


class Report:
    def __init__(self, echo: bool = True) -> None:
        self.echo = echo  # False in --idle: the terminal gets findings only
        self.checks: list[tuple[str, bool, str]] = []
        self.metrics: dict[str, float] = {}
        self.info: dict[str, str] = {}  # context for the snapshot, not judged

    def add(
        self, name: str, ok: bool, detail: str, metric: float | None = None
    ) -> None:
        self.checks.append((name, ok, detail))
        if self.echo:
            print(f"[{'PASS' if ok else 'FAIL'}] {name}: {detail}")
        if metric is not None:
            self.metrics[name] = metric


# ======== Ethernet ========


def _iface_ip(host: str, password: str, iface: str) -> str | None:
    rc, out = run_remote(host, password, f"ip -4 -o addr show {iface}")
    m = re.search(r"inet (\d+\.\d+\.\d+\.\d+)/", out)
    return m.group(1) if rc == 0 and m else None


def _local_ip(iface: str) -> str | None:
    result = subprocess.run(
        ["ip", "-4", "-o", "addr", "show", iface],
        capture_output=True,
        text=True,
        check=False,
    )
    m = re.search(r"inet (\d+\.\d+\.\d+\.\d+)/", result.stdout)
    return m.group(1) if m else None


def bench_eth_pair(
    host: str, password: str, pair: dict, duration: int, report: Report
) -> None:
    label = pair["label"]
    target = _iface_ip(host, password, pair["box_iface"])
    if target is None:
        report.add(f"eth {label}", False, f"couldn't get IP for {pair['box_iface']}")
        return
    local_ip = _local_ip(pair["dev_iface"])
    if local_ip is None:
        report.add(
            f"eth {label}", False, f"couldn't get local IP for {pair['dev_iface']}"
        )
        return

    # A multi-homed dev host (two NICs on the same /24) can end up with a
    # stale ARP entry for the target on the interface we're about to force
    # traffic out of - seen live: iperf3 hard-bound to enp7s0 got 100%
    # packet loss with a STALE neighbor entry, while unbound traffic (which
    # picked the other NIC) worked fine; flushing that one entry fixed it
    # immediately (confirmed by a gateway ICMP redirect right after). Not a
    # box-side issue - best-effort, ignore if we lack CAP_NET_ADMIN.
    subprocess.run(
        ["sudo", "-n", "ip", "neigh", "flush", "dev", pair["dev_iface"]],
        capture_output=True,
        check=False,
    )

    port = pair["port"]
    run_remote(
        host,
        password,
        f"pkill -f '^iperf3 -s -p {port}' 2>/dev/null; "
        f"nohup iperf3 -s -p {port} > /root/bench-all-iperf-server-{port}.log 2>&1 & disown; echo started",
    )
    for _ in range(40):
        _rc, out = run_remote(
            host, password, f"ss -ltn | grep -q ':{port} ' && echo up"
        )
        if "up" in out:
            break
    else:
        report.add(
            f"eth {label}", False, f"iperf3 server never bound to port {port} within 2s"
        )
        return

    try:
        result = subprocess.run(
            [
                "iperf3",
                "-c",
                target,
                "-B",
                f"{local_ip}%{pair['dev_iface']}",
                "-p",
                str(port),
                "-t",
                str(duration),
                "-P",
                "4",
                "--bidir",
                # iperf3 has no built-in connect timeout - if SO_BINDTODEVICE
                # picks an interface with no real L2 path to the target (dead
                # link, ARP not resolving), it hangs at connect() forever
                # rather than erroring, so the process-level timeout below is
                # the only thing that ever ends it.
            ],
            capture_output=True,
            text=True,
            check=False,
            timeout=duration + 15,
        )
    except subprocess.TimeoutExpired:
        report.add(
            f"eth {label}",
            False,
            f"iperf3 client hung past {duration + 15}s - {pair['dev_iface']} likely has no "
            f"working L2 path to {target} right now (check `ip neigh` / link state)",
        )
        run_remote(host, password, f"pkill -f '^iperf3 -s -p {port}' 2>/dev/null; true")
        return
    run_remote(host, password, f"pkill -f '^iperf3 -s -p {port}' 2>/dev/null; true")

    if result.returncode != 0 or "iperf Done" not in result.stdout:
        report.add(
            f"eth {label}",
            False,
            f"iperf3 failed (rc={result.returncode}): {result.stderr[:200]}",
        )
        return

    tx = re.search(
        r"\[SUM\]\[TX-C\]\s+[\d.]+-[\d.]+\s+sec\s+[\d.]+ \wBytes\s+([\d.]+) (\w+)/sec\s+\d+\s*sender",
        result.stdout,
    )
    rx = re.search(
        r"\[SUM\]\[RX-C\]\s+[\d.]+-[\d.]+\s+sec\s+[\d.]+ \wBytes\s+([\d.]+) (\w+)/sec\s+\d+\s*sender",
        result.stdout,
    )
    if not tx or not rx:
        report.add(f"eth {label}", False, "couldn't parse iperf3 output")
        return

    def to_mbps(val: str, unit: str) -> float:
        v = float(val)
        return v * 1000 if unit.lower().startswith("g") else v

    tx_mbps = to_mbps(tx.group(1), tx.group(2))
    rx_mbps = to_mbps(rx.group(1), rx.group(2))
    report.add(
        f"eth {label} host->box",
        True,
        f"{tx.group(1)} {tx.group(2)}/sec",
        metric=tx_mbps,
    )
    report.add(
        f"eth {label} box->host",
        True,
        f"{rx.group(1)} {rx.group(2)}/sec",
        metric=rx_mbps,
    )


def discover_usb_eth_pairs(host: str, password: str, report: Report) -> list[dict]:
    """USB-attached ethernet interfaces on the box, as bench pairs.

    Matched by driver (r8152/cdc_ether/cdc_ncm/asix/ax88179_178a) rather than
    by name: the names encode USB topology and change on a replug. Returns []
    when nothing is attached, so a run without the dock still works.
    """
    _rc, listing = run_remote(
        host,
        password,
        "for d in /sys/class/net/*/device/driver; do "
        "  i=${d%/device/driver}; i=${i#/sys/class/net/}; "
        '  echo "$i $(basename $(readlink -f $d))"; '
        "done 2>/dev/null",
    )
    usb_drivers = {"r8152", "cdc_ether", "cdc_ncm", "asix", "ax88179_178a", "usbnet"}
    found = []
    for line in (listing or "").splitlines():
        parts = line.split()
        if len(parts) == 2 and parts[1] in usb_drivers:
            found.append(parts[0])
    found.sort()

    pairs = []
    for idx, iface in enumerate(found):
        if idx >= len(USB_ETH_DEV_IFACES):
            report.add(
                f"eth usb {iface}",
                False,
                f"no dev-host NIC left to pair against (have "
                f"{len(USB_ETH_DEV_IFACES)}: {', '.join(USB_ETH_DEV_IFACES)})",
            )
            continue
        pairs.append(
            {
                "box_iface": iface,
                "dev_iface": USB_ETH_DEV_IFACES[idx],
                "label": f"USB{idx + 1}",
                "port": USB_ETH_BASE_PORT + idx,
            }
        )
    return pairs


def bench_ethernet(host: str, password: str, duration: int, report: Report) -> None:
    for pair in ETH_PAIRS:
        bench_eth_pair(host, password, pair, duration, report)
    for pair in discover_usb_eth_pairs(host, password, report):
        bench_eth_pair(host, password, pair, duration, report)


# ======== Crypto (AF_ALG) ========

_CRYPTO_REMOTE_SCRIPT = """
import socket, os, time, json, sys, re, signal

def active_driver(alg_name):
    # Highest-priority registered driver for `alg_name`, per /proc/crypto -
    # what AF_ALG's generic-name bind will actually resolve to.
    best_driver, best_prio = None, None
    with open("/proc/crypto") as f:
        blocks = f.read().split("\\n\\n")
    for block in blocks:
        m_name = re.search(r"^name\\s*:\\s*(\\S+)", block, re.M)
        m_driver = re.search(r"^driver\\s*:\\s*(\\S+)", block, re.M)
        m_prio = re.search(r"^priority\\s*:\\s*(-?\\d+)", block, re.M)
        if not (m_name and m_driver and m_prio):
            continue
        if m_name.group(1) != alg_name:
            continue
        p = int(m_prio.group(1))
        if best_prio is None or p > best_prio:
            best_prio, best_driver = p, m_driver.group(1)
    return best_driver

class _BenchTimeout(Exception):
    pass


def _bench_alarm(signum, frame):
    raise _BenchTimeout("bench() exceeded its overall time budget")


def bench(alg_name, total_mb, chunk):
    key = os.urandom(64)  # AES-256-XTS: two 32-byte keys concatenated
    iv = os.urandom(16)
    try:
        # No settimeout() on this parent socket - confirmed live it breaks
        # accept() after ~128 calls (hangs where an untimed accept() takes
        # 33ms total for the same 128 calls). Only the per-op socket below
        # needs a timeout, to bound a single stuck encrypt operation.
        s = socket.socket(socket.AF_ALG, socket.SOCK_SEQPACKET, 0)
        s.bind(("skcipher", alg_name))
        s.setsockopt(socket.SOL_ALG, socket.ALG_SET_KEY, key)
    except OSError as e:
        return {"available": False, "error": str(e)}
    buf = os.urandom(chunk)
    n = max(1, (total_mb << 20) // chunk)
    t0 = time.monotonic()
    # SIGALRM, not socket.settimeout() on `s` (see comment above - that
    # breaks accept() itself). A healthy run does ~128 accept()s in 33ms
    # per the comment above; even a slow real transfer at 32MB/50MBps
    # software-AES worst case is under a second. 15s is generous headroom
    # over that, bounding the documented "accept() gets pathologically
    # slow" case.
    #
    # Does NOT and cannot rescue a genuine kernel D-state (uninterruptible)
    # hang - a queued signal is not delivered until the blocking syscall
    # itself returns, which is the definition of D-state. That did happen
    # live (20+ min stuck, only cleared by a reboot) - the real backstop
    # for that is the caller's own ssh-level subprocess timeout (now also
    # fails gracefully instead of crashing the whole suite - see
    # run_remote()), not this alarm. If it recurs, expect this alarm to
    # NOT fire and the outer timeout to be what actually ends it.
    old_handler = signal.signal(signal.SIGALRM, _bench_alarm)
    signal.alarm(15)
    try:
        for _ in range(n):
            op, _ = s.accept()
            op.settimeout(10)
            sent = op.sendmsg(
                [buf],
                [(socket.SOL_ALG, socket.ALG_SET_OP, socket.ALG_OP_ENCRYPT.to_bytes(4, "little")),
                 (socket.SOL_ALG, socket.ALG_SET_IV, (len(iv)).to_bytes(4, "little") + iv)],
            )
            # sendmsg() on an AF_ALG op socket silently accepts less than
            # the whole buffer once past some per-op limit (seen live: a
            # 4MB send only queued 208KB, no error, and a following recv()
            # for the rest just hangs forever - no length check here means
            # a chunk that's too big for this kernel goes undetected until
            # it hangs the whole run). `chunk` must stay within whatever
            # that limit is (confirmed working at 128KB).
            if sent != chunk:
                raise OSError(f"sendmsg only queued {sent} of {chunk} bytes - chunk too large for this kernel's AF_ALG limit")
            op.recv(chunk)
            op.close()
    except (OSError, socket.timeout, _BenchTimeout) as e:
        s.close()
        return {"available": True, "error": f"op failed: {e}"}
    finally:
        signal.alarm(0)
        signal.signal(signal.SIGALRM, old_handler)
    dt = time.monotonic() - t0
    s.close()
    mb_done = (n * chunk) / (1 << 20)
    return {
        "available": True,
        "mb_per_sec": mb_done / dt,
        "seconds": dt,
        "mb": mb_done,
        "driver": active_driver(alg_name),
    }

alg_name = sys.argv[1]
total_mb = int(sys.argv[2])
chunk = 128 << 10  # 128KB - confirmed the largest size this kernel's AF_ALG
                    # skcipher accepts in one sendmsg (see the check above;
                    # 4MB silently truncated to 208KB with no error)
print(json.dumps(bench(alg_name, total_mb, chunk)))
"""


def bench_crypto(host: str, password: str, total_mb: int, report: Report) -> None:
    payload = base64.b64encode(_CRYPTO_REMOTE_SCRIPT.encode()).decode()
    cmd = (
        f"echo {payload} | base64 -d > /tmp/_bench_crypto.py && "
        f"python3 /tmp/_bench_crypto.py '{CRYPTO_ALG}' {total_mb}; "
        "rm -f /tmp/_bench_crypto.py"
    )
    rc, out = run_remote(host, password, cmd, timeout=120)
    if rc != 0:
        report.add("crypto", False, f"remote script failed (rc={rc}): {out[:300]}")
        return
    try:
        r = json.loads(out.strip().splitlines()[-1])
    except (json.JSONDecodeError, IndexError):
        report.add("crypto", False, f"couldn't parse output: {out[:300]}")
        return
    if not r.get("available"):
        report.add(f"crypto {CRYPTO_ALG}", False, r.get("error", "not available"))
    elif "mb_per_sec" in r:
        report.add(
            f"crypto {CRYPTO_ALG} [{r['driver']}]",
            True,
            f"{r['mb_per_sec']:.1f} MB/s ({r['mb']:.1f}MB in {r['seconds']:.2f}s)",
            metric=r["mb_per_sec"],
        )
    else:
        report.add(f"crypto {CRYPTO_ALG}", False, r.get("error", "op failed"))


# ======== Disk / USB (hdparm -t --direct) ========


def _hdparm_direct(
    host: str, password: str, dev: str
) -> tuple[bool, str, float | None]:
    rc, out = run_remote(host, password, f"hdparm -t --direct /dev/{dev}", timeout=30)
    m = re.search(r"=\s*([\d.]+)\s*MB/sec", out)
    if rc != 0 or not m:
        return False, out.strip().splitlines()[-1] if out.strip() else f"rc={rc}", None
    mbps = float(m.group(1))
    return True, f"{mbps:.1f} MB/sec", mbps


def _usb_link_mbps(host: str, password: str, dev: str) -> float | None:
    """Negotiated USB speed (Mb/s) of the device's usb_device node: SuperSpeed
    is 5000; behind a USB 2.0 hub it caps at 480 whatever the bridge can do."""
    walk = (
        f"p=$(readlink -f /sys/block/{dev}); "
        'while [ "$p" != / ]; do '
        '[ -f "$p/speed" ] && [ -f "$p/idVendor" ] && { cat "$p/speed"; break; }; '
        'p=$(dirname "$p"); done'
    )
    rc, out = run_remote(host, password, walk, timeout=DMA_SSH_CONNECT_S + DMA_SLACK_S)
    try:
        return float(out.strip().splitlines()[-1]) if rc == 0 and out.strip() else None
    except ValueError:
        return None


def bench_disks(
    host: str, password: str, devices: tuple[str, ...], label: str, report: Report
) -> None:
    for dev in devices:
        ok, detail, mbps = _hdparm_direct(host, password, dev)
        report.add(f"{label} {dev}", ok, detail, metric=mbps)
        if label == "usb":
            link = _usb_link_mbps(host, password, dev)
            if link is not None:
                report.add(
                    f"{label} {dev} link",
                    True,
                    f"{link:.0f} Mb/s negotiated",
                    metric=link,
                )


# ======== Health (link, MAC counters, kernel log) ========

HEALTH_IFACES = ("enp0s1", "enp0s2")
# Kernel-log lines that mean a driver or the engine is in trouble. Narrower
# than soak-test.py's list on purpose: an unattended run must not cry wolf.
DMESG_BAD = re.compile(
    r"WARNING:|BUG:|Oops|Call trace|Unable to handle|SError|hung task|soft lockup"
    r"|hard LOCKUP|rcu_sched|I/O error|ata\d+\.\d+: failed|reset failed|tx timeout"
    r"|Tx timeout|link is down|uncorrect|never completed|queue reset failed"
    r"|Failed to init DMA queue|completion error|DMA-API"
    r"|al_eth.*(error|fail|timeout|hang)",
    re.IGNORECASE,
)
# Known-benign and already tracked: #55 (MSI-X fallback works).
DMESG_BENIGN = ("of_irq_parse_pci: failed with rc=-22",)
# Error counters: any growth between two looks is a finding.
HEALTH_ERROR_COUNTERS = ("rx_errors", "rx_crc_errors", "tx_errors")


class HealthState:
    """What the health subtest carries from one cycle to the next (--idle)."""

    def __init__(self) -> None:
        self.mark: str | None = None  # dmesg marker: only look at lines after it
        self.stats: dict[str, dict[str, int]] | None = None
        # --idle: the first look records the counters instead of judging them,
        # so history from before the run is not a finding.
        self.baseline_first_look = False


def _health_script(mark: str | None) -> str:
    ifs = " ".join(HEALTH_IFACES)
    scan = f"sed -n '/{mark}/,$p'" if mark else "cat"
    return f"""echo '@@kernel'; uname -r
echo '@@vermagic'; modinfo -F vermagic al_eth 2>&1
echo '@@link'; for i in {ifs}; do echo "$i speed=$(cat /sys/class/net/$i/speed 2>/dev/null) carrier=$(cat /sys/class/net/$i/carrier 2>/dev/null) oper=$(cat /sys/class/net/$i/operstate 2>/dev/null)"; done
echo '@@stats'; for i in {ifs}; do echo "$i $(for c in {" ".join(HEALTH_ERROR_COUNTERS)}; do echo -n "$c=$(cat /sys/class/net/$i/statistics/$c 2>/dev/null) "; done)"; done
echo '@@dmesg'; dmesg | {scan}
"""


def parse_sections(text: str) -> dict[str, str]:
    """Split output on '@@name' marker lines."""
    out: dict[str, list[str]] = {}
    cur = None
    for line in text.splitlines():
        if line.startswith("@@"):
            cur = line[2:].strip()
            out[cur] = []
        elif cur is not None:
            out[cur].append(line)
    return {k: "\n".join(v).strip() for k, v in out.items()}


def parse_link(text: str) -> dict[str, dict[str, str]]:
    links = {}
    for m in re.finditer(r"^(\S+) speed=(\S*) carrier=(\S*) oper=(\S*)", text, re.M):
        links[m[1]] = {"speed": m[2], "carrier": m[3], "oper": m[4]}
    return links


def parse_counters(text: str) -> dict[str, dict[str, int]]:
    stats: dict[str, dict[str, int]] = {}
    for line in text.splitlines():
        parts = line.split()
        if not parts:
            continue
        stats[parts[0]] = {
            k: int(v)
            for k, v in (p.split("=", 1) for p in parts[1:] if "=" in p)
            if v.isdigit()
        }
    return stats


def counter_growth(
    prev: dict[str, dict[str, int]] | None, cur: dict[str, dict[str, int]]
) -> list[str]:
    """Error counters that grew since the previous look; with no previous look
    (one-shot run) any nonzero counter counts, as test-eth.py judged it."""
    grown = []
    for iface, ctrs in sorted(cur.items()):
        for name in HEALTH_ERROR_COUNTERS:
            if name not in ctrs:
                continue
            before = (prev or {}).get(iface, {}).get(name, 0)
            if ctrs[name] > before:
                grown.append(f"{iface} {name} {before}->{ctrs[name]}")
    return grown


def dmesg_trouble(text: str) -> list[str]:
    return [
        line.strip()
        for line in text.splitlines()
        if DMESG_BAD.search(line) and not any(b in line for b in DMESG_BENIGN)
    ]


def judge_health(
    sections: dict[str, str], state: HealthState
) -> tuple[list[tuple[str, bool, str]], dict[str, float]]:
    """(checks, metrics) from one health look. Pure: no I/O."""
    checks: list[tuple[str, bool, str]] = []
    metrics: dict[str, float] = {}
    kernel = sections.get("kernel", "")
    vermagic = (sections.get("vermagic", "").split() or [""])[0]
    checks.append(
        (
            "health al_eth module",
            bool(kernel) and vermagic == kernel,
            f"vermagic={vermagic or '?'} running={kernel or '?'}",
        )
    )
    links = parse_link(sections.get("link", ""))
    for iface in HEALTH_IFACES:
        link = links.get(iface)
        if not link:
            checks.append((f"health link {iface}", False, "no data"))
            continue
        up = link["carrier"] == "1" and link["oper"] == "up"
        checks.append(
            (
                f"health link {iface}",
                up,
                f"speed={link['speed']} carrier={link['carrier']} oper={link['oper']}",
            )
        )
        if link["speed"].isdigit():
            metrics[f"link {iface} speed Mbps"] = float(link["speed"])
    cur = parse_counters(sections.get("stats", ""))
    if state.stats is None and state.baseline_first_look:
        state.stats = cur
    grown = counter_growth(state.stats, cur)
    checks.append(
        (
            "health MAC counters",
            not grown and bool(cur),
            ", ".join(grown) or ("no growth" if state.stats else "all zero")
            if cur
            else "no counters",
        )
    )
    state.stats = cur
    bad = dmesg_trouble(sections.get("dmesg", ""))
    checks.append(
        (
            "health kernel log",
            not bad,
            f"{len(bad)} line(s): {bad[0]}" if bad else "no trouble lines",
        )
    )
    return checks, metrics


def bench_health(
    host: str, password: str, report: Report, state: HealthState | None = None
) -> None:
    state = state or HealthState()
    # Waits for: one ssh round trip plus dmesg. Limit = connect + slack.
    rc, out = run_remote(
        host,
        password,
        _health_script(state.mark),
        timeout=DMA_SSH_CONNECT_S + DMA_SLACK_S,
    )
    if rc != 0:
        report.add("health", False, f"rc={rc}: {out.strip()[-200:]}")
        return
    sections = parse_sections(out)
    report.info["kernel"] = sections.get("kernel", "")
    checks, metrics = judge_health(sections, state)
    for name, ok, detail in checks:
        report.add(name, ok, detail)
    report.metrics.update(metrics)


def mark_dmesg(host: str, password: str, state: HealthState, stamp: str) -> None:
    """Put a marker in the kernel log; the next health look reads from it."""
    mark = f"bench-all mark {stamp}"
    rc, _ = run_remote(
        host,
        password,
        f"echo '{mark}' > /dev/kmsg",
        timeout=DMA_SSH_CONNECT_S + DMA_SLACK_S,
    )
    if rc == 0:
        state.mark = mark


# ======== DMA engine (al_dma, via dmatest) ========

DMA_CHAN = "dma0chan0"
DMATEST_SYSFS = "/sys/module/dmatest/parameters"
# dmatest per-op completion timeout (ms). Waits for: one op's completion
# callback. Expected ~1 ms/op (#23); the driver aborts a stalled op at
# AL_DMA_STALL_MS = 50 (#288), so the abort lands first: 100 ms = 2x that.
# On expiry dmatest counts 'test timed out' and the check fails.
DMATEST_OP_TIMEOUT_MS = 100
# Slowest rate any dmatest thread has measured: pq 1998 iops, xor 2130 iops
# before the #286 fix (#23, 2026-08-29); xor 12743 / copy 13716 after.
DMA_MIN_IOPS = 2000
# dmatest sleeps 100 ms after every 'prep error' (dmatest.c, `if (!tx)`), so
# runs that expect refusals use few iterations: 300 x ~0.9 x 0.1 s ~= 27 s.
DMATEST_PREP_ERROR_SLEEP_S = 0.1
DMA_REFUSAL_ITERATIONS = 300
DMA_SSH_CONNECT_S = 8  # ConnectTimeout in _box.sshpass_argv
DMA_SLACK_S = 5  # dmatest thread start + dmesg grep over ssh
# Stopping a finished run is kthread_stop on idle threads (~ms). 15 s = far
# above that; expiry means a thread is stuck in the engine: skip, reboot.
DMA_STOP_TIMEOUT_S = 15

_DMATEST_SUMMARY = re.compile(
    r"dmatest: (?P<thr>\S+): summary (?P<tests>\d+) tests, (?P<fail>\d+) failures "
    r"(?P<iops>[\d.]+) iops (?P<kbs>\d+) KB/s"
)
_DMATEST_RESULT = re.compile(r"dmatest: (?P<thr>\S+): result #\d+: '(?P<what>[^']+)'")
_DMA_BAD_KERNEL = re.compile(
    r"Failed to init DMA queue|BUG:|Oops|WARNING:|never completed|queue reset failed"
    r"|completion error|rejected"
)


def dma_run_limit_s(iterations: int, refused_fraction: float = 0.0) -> int:
    """Wait limit for one blocking dmatest run: slowest expected thread
    (DMA_MIN_IOPS) plus dmatest's sleep per refused op, x 1.25, plus ssh
    connect and slack. Expiry: engine presumed wedged, the phase FAILs."""
    work = (
        iterations / DMA_MIN_IOPS
        + iterations * refused_fraction * DMATEST_PREP_ERROR_SLEEP_S
    )
    return int(work * 1.25) + DMA_SSH_CONNECT_S + DMA_SLACK_S


def parse_dmatest(text: str) -> dict:
    """Per-thread summaries, failure kinds, and kernel-log trouble."""
    summaries = {
        m["thr"]: {
            "tests": int(m["tests"]),
            "failures": int(m["fail"]),
            "iops": float(m["iops"]),
        }
        for m in _DMATEST_SUMMARY.finditer(text)
    }
    kinds: dict[str, dict[str, int]] = {}
    for m in _DMATEST_RESULT.finditer(text):
        k = kinds.setdefault(m["thr"], {})
        k[m["what"]] = k.get(m["what"], 0) + 1
    return {
        "summaries": summaries,
        "failure_kinds": kinds,
        "kernel_trouble": _DMA_BAD_KERNEL.findall(text),
    }


def judge_dmatest(parsed: dict, iterations: int, allow_kinds=frozenset()) -> list[str]:
    """Failure strings for one run; empty = pass. `allow_kinds`: dmatest
    result strings that may account for failures (e.g. 'prep error')."""
    fails: list[str] = []
    if not parsed["summaries"]:
        fails.append("no dmatest summary lines (did the run start?)")
    for thr, s in parsed["summaries"].items():
        kinds = parsed["failure_kinds"].get(thr, {})
        bad = {k: n for k, n in kinds.items() if k not in allow_kinds}
        if s["failures"] and (bad or not kinds):
            fails.append(f"{thr}: {s['failures']} failures {bad or '(kinds unknown)'}")
        elif kinds and s["failures"] != sum(kinds.values()):
            fails.append(f"{thr}: {s['failures']} failures but {kinds}")
        if s["tests"] != iterations:
            fails.append(f"{thr}: ran {s['tests']} of {iterations} tests")
    # 'rejected' is the driver's warning for a refused length: expected when
    # the run allows prep errors, a bug otherwise.
    trouble = [
        t
        for t in parsed["kernel_trouble"]
        if not (t == "rejected" and "prep error" in allow_kinds)
    ]
    if trouble:
        fails.append(f"kernel log: {sorted(set(trouble))}")
    return fails


def _dmatest_script(marker: str, iterations: int, buf: int) -> str:
    """Stop the previous run (releases the channel), configure, run, block on
    `wait`, print the dmesg since the marker. dmatest starts one thread per
    capability the channel advertises (copy0, xor0, and pq0 with enable_pq)."""
    return f"""set -e
cd {DMATEST_SYSFS}
echo 0 > run
echo '{marker}' > /dev/kmsg
echo {iterations} > iterations
echo {buf} > test_buf_size
echo {DMATEST_OP_TIMEOUT_MS} > timeout
echo 1 > threads_per_chan
echo {DMA_CHAN} > channel
echo 1 > run
cat wait > /dev/null
dmesg | sed -n '/{marker}/,$p'
"""


def _dma_phase(
    host,
    password,
    report,
    name,
    iterations,
    buf,
    allow_kinds=frozenset(),
    refused_fraction=0.0,
    metrics=False,
) -> bool:
    marker = f"bench-all dma {name} buf={buf}"
    rc, out = run_remote(
        host,
        password,
        _dmatest_script(marker, iterations, buf),
        timeout=dma_run_limit_s(iterations, refused_fraction),
    )
    if rc != 0:
        report.add(
            f"dma {name}",
            False,
            f"rc={rc}: {out.strip()[-300:]} (rc 124 = engine presumed wedged: reboot)",
        )
        return False
    parsed = parse_dmatest(out)
    fails = judge_dmatest(parsed, iterations, allow_kinds)
    detail = ", ".join(
        f"{t.split('-', 1)[-1]} {s['tests']} tests {s['failures']} fail "
        f"{s['iops']:.0f} iops"
        for t, s in sorted(parsed["summaries"].items())
    )
    report.add(f"dma {name}", not fails, "; ".join(fails) if fails else detail)
    if metrics:
        for t, s in parsed["summaries"].items():
            report.metrics[
                f"dma {DMA_CHAN} {t.split('-', 1)[-1].rstrip('0123456789')} iops"
            ] = s["iops"]
    return not fails


def dma_channel_state(text: str) -> dict:
    """Parse the channel probe: HAVE_CHAN marker, RUN=<Y|N>, INUSE=<n>."""
    run = re.search(r"^RUN=(\S*)", text, re.M)
    use = re.search(r"^INUSE=(\d+)", text, re.M)
    return {
        "have": "HAVE_CHAN" in text,
        "run": run[1] if run else "?",
        "in_use": int(use[1]) if use else None,
    }


def dma_skip_reason(state: dict, take_over: bool) -> str | None:
    """Why the DMA subtest must not start, or None. A dmatest thread outlives
    the module and holds the channel (`in_use` > 0) with `run` already N: that
    is a leaked client and only a reboot frees it (#289, #23)."""
    if not state["have"]:
        return f"no {DMA_CHAN} (al_dma not loaded?)"
    if state["run"] == "Y" and not take_over:
        return "another dmatest run is in progress"
    if state["run"] != "Y" and state["in_use"]:
        return (
            f"{DMA_CHAN} is held (in_use={state['in_use']}) with dmatest not "
            "running: a leaked client, reboot to free it; do not modprobe -r al_dma (#289)"
        )
    return None


def _dma_probe_script() -> str:
    return (
        "modprobe dmatest 2>&1; "
        f"test -d /sys/class/dma/{DMA_CHAN} && echo HAVE_CHAN; "
        f"echo RUN=$(cat {DMATEST_SYSFS}/run); "
        f"echo INUSE=$(cat /sys/class/dma/{DMA_CHAN}/in_use 2>/dev/null)"
    )


def _dma_stop(host: str, password: str) -> tuple[bool, str]:
    """`echo 0 > run` joins the dmatest threads and releases the channel.
    Waits for: kthread_stop on idle threads, ~ms. Limit DMA_STOP_TIMEOUT_S."""
    rc, out = run_remote(
        host, password, f"echo 0 > {DMATEST_SYSFS}/run", timeout=DMA_STOP_TIMEOUT_S
    )
    return rc == 0, f"rc={rc}: {out.strip()[-120:]}"


def bench_dma(
    host: str,
    password: str,
    iterations: int,
    report: Report,
    reload: bool = False,
    pq: bool = False,
    take_over: bool = True,
) -> None:
    """`take_over` False (--idle): never stop a dmatest run that is not ours."""
    quick = DMA_SSH_CONNECT_S + DMA_SLACK_S
    # Waits for: modprobe + sysfs reads, ms. Limit = connect + slack.
    rc, out = run_remote(host, password, _dma_probe_script(), timeout=quick)
    state = dma_channel_state(out)
    if state["run"] == "Y" and take_over:
        ok, why = _dma_stop(host, password)
        if not ok:
            report.add(
                "dma",
                True,
                f"SKIPPED: could not stop the previous dmatest run in "
                f"{DMA_STOP_TIMEOUT_S}s ({why}); a thread is stuck in the engine, "
                "reboot. Do not modprobe -r al_dma (#289)",
            )
            return
        rc, out = run_remote(host, password, _dma_probe_script(), timeout=quick)
        state = dma_channel_state(out)
    reason = dma_skip_reason(state, take_over)
    if reason:
        report.add("dma", True, f"SKIPPED: {reason}: {out.strip()[-150:]}")
        return
    try:
        _bench_dma_phases(host, password, iterations, report, reload, pq, quick)
    finally:
        ok, why = _dma_stop(host, password)
        if not ok:
            report.add(
                "dma stop",
                False,
                f"dmatest would not stop ({why}): engine presumed wedged, reboot",
            )


def _bench_dma_phases(host, password, iterations, report, reload, pq, quick) -> None:
    ok = _dma_phase(host, password, report, "copy+xor", iterations, 16384, metrics=True)
    ok = ok and _dma_phase(
        host,
        password,
        report,
        "oversize",
        min(iterations, DMA_REFUSAL_ITERATIONS),
        131072,
        allow_kinds={"prep error"},
        refused_fraction=0.9,  # xor: 87.5% of lengths 1..131072 exceed 16 KiB
    )
    if ok and reload:
        # Waits for: rmmod/modprobe, ~ms each. Limit = connect + slack.
        opt = "enable_pq=1" if pq else ""
        rc, out = run_remote(
            host,
            password,
            f"echo 0 > {DMATEST_SYSFS}/run; rmmod al_dma && modprobe al_dma {opt}",
            timeout=quick,
        )
        report.add("dma reload", rc == 0, out.strip()[-200:] or "rmmod+modprobe ok")
        ok = rc == 0 and _dma_phase(
            host, password, report, "after-reload", iterations, 16384
        )
    if ok and pq:
        ok = _dma_phase(host, password, report, "pq-8k", iterations, 8192)
        ok = ok and _dma_phase(
            host,
            password,
            report,
            "pq-16k",
            min(iterations, DMA_REFUSAL_ITERATIONS),
            16384,
            allow_kinds={"prep error"},
            refused_fraction=0.5,  # pq: lengths over 8 KiB are half of 1..16384
        )


# ======== al_ssm through dm-crypt (opt-in: --ssm) ========
# The only path AF_ALG cannot mask (#267). Opt-in: it drives the code under
# open #300 / #182, which can leave a writer in D state until a reboot.

SSM_MAP = "alssm-stress"
SSM_BACKING = "/var/tmp/alssm-stress.img"
SSM_PATTERN = "/dev/shm/alssm-stress.pat"
# Expected: 64 MiB write+read through the engine at the 235 MiB/s CE figure
# (#182) is ~0.6 s; 8x margin -> 5 s per phase, 120 s for the suite. On expiry
# the call returns rc 124 and the phase FAILs (a writer may be in D state).
SSM_SSH_TIMEOUT_S = 120
# Quiesce: all completions land within the 500 us inline window or one 100 us
# worker cycle; a 64-deep queue drains in < 100 ms. One poll = one ssh round
# trip (~50 ms); 40 polls ~ 2 s = 20x. On expiry: FAIL (#300/#182).
SSM_QUIESCE_POLLS = 40

_SSM_FIELD = re.compile(r"(\w+):\s+(\S+)")


def parse_ssm_diag(text: str) -> dict[str, str]:
    """`key: value` pairs from `devlink health diagnose ... reporter ssm`."""
    return dict(_SSM_FIELD.findall(text))


def find_ssm_bdf(show_text: str) -> str | None:
    """BDF of the ssm reporter from `devlink health show` output."""
    m = re.search(r"pci/(\S+):\s+reporter ssm\b", show_text)
    return m.group(1) if m else None


def judge_ssm(before: dict[str, str], after: dict[str, str], cmp_ok: bool) -> list[str]:
    """Failure strings for one dm-crypt run; empty = pass."""
    fails = []
    if not cmp_ok:
        fails.append("data mismatch or writer failed (cmp)")
    if after.get("in_flight") != "0" or after.get("backlog") != "0":
        fails.append(
            f"no quiesce: in_flight={after.get('in_flight')} "
            f"backlog={after.get('backlog')} (#300/#182)"
        )
    try:
        delta = {
            k: int(after[k]) - int(before[k])
            for k in ("submitted", "completed", "errors")
        }
    except (KeyError, ValueError):
        return [*fails, f"diagnose output lacks counters: {sorted(after)}"]
    if delta["submitted"] <= 0:
        fails.append("engine saw no ops: dm-crypt used another driver")
    if delta["completed"] != delta["submitted"]:
        fails.append(
            f"completed {delta['completed']} != submitted {delta['submitted']}"
        )
    if delta["errors"] != 0:
        fails.append(f"engine reported {delta['errors']} errors")
    return fails


def _ssm_setup_script(mib: int, key_hex: str) -> str:
    return f"""set -e
head -c {mib * 1024 * 1024} /dev/urandom > {SSM_PATTERN}
truncate -s {mib}M {SSM_BACKING}
LOOP=$(losetup --find --show {SSM_BACKING})
echo "loop=$LOOP"
dmsetup create {SSM_MAP} --table "0 {mib * 2048} crypt aes-xts-plain64 {key_hex} 0 $LOOP 0"
"""


def _ssm_writers_script(writers: int, mib: int) -> str:
    per = mib * 256 // writers  # 4 KiB blocks per writer
    return f"""set -e
for i in $(seq 0 {writers - 1}); do
  dd if={SSM_PATTERN} of=/dev/mapper/{SSM_MAP} bs=4k seek=$((i*{per})) skip=$((i*{per})) \\
     count={per} oflag=direct conv=notrunc status=none &
done
wait
dd if=/dev/mapper/{SSM_MAP} bs=4k count=$(({per}*{writers})) iflag=direct status=none \\
  | cmp -n $(({per}*{writers}*4096)) - {SSM_PATTERN} && echo CMP_OK
"""


def _ssm_teardown_script() -> str:
    return (
        f"dmsetup remove {SSM_MAP} 2>&1; "
        f"for l in $(losetup -j {SSM_BACKING} -O NAME -n); do losetup -d $l; done; "
        f"rm -f {SSM_BACKING} {SSM_PATTERN}"
    )


def bench_ssm(
    host: str,
    password: str,
    report: Report,
    writers: int = 4,
    mib: int = 64,
    unbind: bool = False,
) -> None:
    def remote(script: str) -> tuple[int, str]:
        return run_remote(host, password, script, timeout=SSM_SSH_TIMEOUT_S)

    rc, out = remote("devlink health show")
    bdf = find_ssm_bdf(out)
    if rc != 0 or not bdf:
        report.add(
            "ssm",
            True,
            f"SKIPPED: no ssm devlink reporter (al_ssm loaded?): {out[-200:]}",
        )
        return

    def diag() -> dict[str, str]:
        rc, out = remote(f"devlink health diagnose pci/{bdf} reporter ssm")
        return parse_ssm_diag(out) if rc == 0 else {}

    key = secrets.token_hex(64)  # aes-xts 512-bit key; fresh per run
    before = diag()
    try:
        rc, out = remote(_ssm_setup_script(mib, key))
        if rc != 0:
            report.add("ssm setup", False, f"rc={rc}: {out.strip()[-200:]}")
            return
        if unbind:
            remote(
                f"({_ssm_writers_script(writers, mib)}) & "
                f"echo {bdf} > /sys/bus/pci/drivers/al_ssm/unbind; wait"
            )
            report.add(
                "ssm unbind under load",
                True,
                "survived; check dmesg for KASAN/DMA-API (#301)",
            )
            return
        rc, out = remote(_ssm_writers_script(writers, mib))
        after = diag()
        for _ in range(SSM_QUIESCE_POLLS):
            if after.get("in_flight") == "0" and after.get("backlog") == "0":
                break
            after = diag()
        fails = judge_ssm(before, after, "CMP_OK" in out)
        detail = ", ".join(
            f"{k} {after.get(k, '?')}" for k in ("submitted", "completed", "errors")
        )
        report.add(
            f"ssm dm-crypt {writers}x{mib // writers} MiB",
            not fails,
            "; ".join(fails) if fails else detail,
        )
    finally:
        rc, out = remote(_ssm_teardown_script())
        if rc != 0:
            report.add("ssm teardown", False, f"rc={rc}: {out.strip()[-200:]}")


# ======== Memory + CPU stress (opt-in: --stress) ========
# memtester is memtest86-style over an mmap'd region; stress-ng loads CPU, VM
# and cache. Stability / thermal check, not a throughput number. Needs both on
# the box; bench-all does not install packages.

STRESS_MEMTESTER_MB = (
    256  # big enough to exercise the controller, small enough not to OOM
)


def bench_stress(host: str, password: str, report: Report, secs: int = 60) -> None:
    quick = DMA_SSH_CONNECT_S + DMA_SLACK_S
    rc, out = run_remote(
        host,
        password,
        "command -v memtester >/dev/null && command -v stress-ng >/dev/null && echo HAVE",
        timeout=quick,
    )
    if "HAVE" not in out:
        report.add(
            "stress",
            True,
            "SKIPPED: memtester/stress-ng missing on the box (dnf install memtester stress-ng)",
        )
        return
    # Waits for: one memtester pass, a few s/MB worst case -> 2 s/MB, min 120 s.
    limit = max(120, STRESS_MEMTESTER_MB * 2)
    rc, out = run_remote(
        host,
        password,
        f"memtester {STRESS_MEMTESTER_MB}M 1 2>&1 | tail -40",
        timeout=limit,
    )
    report.add(
        f"stress memtester {STRESS_MEMTESTER_MB}M",
        rc == 0 and "FAILURE" not in out,
        out.strip().splitlines()[-1] if out.strip() else f"rc={rc}",
    )
    # --cpu 0 = one CPU stressor per online core. Waits for: --timeout secs.
    rc, out = run_remote(
        host,
        password,
        f"stress-ng --cpu 0 --vm 2 --vm-bytes 256M --cache 2 --timeout {secs}s "
        "--metrics-brief 2>&1 | tail -30",
        timeout=secs + 120,
    )
    report.add(
        f"stress stress-ng {secs}s",
        rc == 0 and "successful run completed" in out.lower(),
        out.strip().splitlines()[-1] if out.strip() else f"rc={rc}",
    )


# ======== Baseline, findings and snapshot diffing (pure) ========

# Stored baseline: the metrics a known-good run produced. --idle compares each
# cycle against it; --save-baseline rewrites it from an attended run.
BASELINE = Path("bench-history/baseline.json")
# Allowed drop below baseline, as a fraction. Default = --compare's 10%.
# dma: dmatest iops move with CPU scheduling on the 4-core box and the
# baseline is one run, so 20%. link speed: any downshift is a finding.
DMA_TOLERANCE = 0.20
LINK_TOLERANCE = 0.0


@dataclass(frozen=True)
class Finding:
    key: str  # stable identity: the same key is the same problem
    text: str
    hard: bool = True  # False: a metric regression, needs confirming


def tolerance_for(name: str) -> float:
    if name.startswith("dma "):
        return DMA_TOLERANCE
    if name.startswith("link ") or name.endswith(" link"):
        return LINK_TOLERANCE
    return REGRESSION_THRESHOLD


def load_baseline(path: Path) -> dict[str, float]:
    """The metrics of a baseline or of any snapshot; {} when there is none."""
    try:
        data = json.loads(path.read_text())
    except (OSError, json.JSONDecodeError):
        return {}
    return {k: float(v) for k, v in data.get("metrics", {}).items()}


def find_regressions(
    metrics: dict[str, float], baseline: dict[str, float]
) -> list[Finding]:
    """A metric more than its tolerance below baseline. Only drops count; a
    metric absent from this run is the failing check's business."""
    out = []
    for name, base in sorted(baseline.items()):
        cur = metrics.get(name)
        if cur is None or base <= 0:
            continue
        tol = tolerance_for(name)
        if cur < base * (1 - tol):
            pct = (cur - base) / base * 100
            out.append(
                Finding(
                    f"regress:{name}",
                    f"{name}: {cur:.1f} vs baseline {base:.1f} "
                    f"({pct:+.1f}%, limit -{tol * 100:.0f}%)",
                    hard=False,
                )
            )
    return out


def failure_findings(checks: list[tuple[str, bool, str]]) -> list[Finding]:
    return [
        Finding(f"fail:{name}", f"{name}: {detail}")
        for name, ok, detail in checks
        if not ok
    ]


_WEDGE_SIGNS = ("rc=124", "timed out after", "presumed wedged")


def is_wedged(checks: list[tuple[str, bool, str]]) -> bool:
    """A failed check whose detail says an ssh call hit its limit or the
    engine is presumed wedged: piling on more load will not help."""
    return any(
        not ok and any(sign in detail for sign in _WEDGE_SIGNS)
        for _, ok, detail in checks
    )


class ChangeTracker:
    """Report on change. A hard finding is reported the first cycle it
    appears; a regression only when it has been present `confirm` cycles in a
    row. Nothing is repeated while it persists; a finding that goes away is
    reported once as cleared."""

    def __init__(self, confirm: int = 2) -> None:
        self.confirm = confirm
        self.streak: dict[str, int] = {}
        self.reported: set[str] = set()

    def update(self, findings: list[Finding]) -> tuple[list[Finding], list[str]]:
        now = {f.key: f for f in findings}
        new = []
        for key, f in now.items():
            self.streak[key] = self.streak.get(key, 0) + 1
            if key not in self.reported and (
                f.hard or self.streak[key] >= self.confirm
            ):
                self.reported.add(key)
                new.append(f)
        cleared = sorted(k for k in self.reported if k not in now)
        for key in list(self.streak):
            if key not in now:
                del self.streak[key]
        self.reported -= set(cleared)
        return new, cleared


def compare_metrics(
    a: dict[str, float], b: dict[str, float], threshold: float = REGRESSION_THRESHOLD
) -> list[tuple[str, float | None, float | None, float, bool]]:
    """(name, before, after, percent change, regressed) per metric in either."""
    rows = []
    for k in sorted(set(a) | set(b)):
        va, vb = a.get(k), b.get(k)
        if va is None or vb is None:
            rows.append((k, va, vb, 0.0, False))
            continue
        pct = (vb - va) / va * 100 if va else 0.0
        rows.append((k, va, vb, pct, pct <= -threshold * 100))
    return rows


# ======== Idle detection (pure parsing; the I/O is in probe_reasons) ========

# --idle waits for the box to be otherwise unused. Thresholds and their basis:
# 1-min load: 0.23 on 2026-10-11 with a console and an ssh login attached
# (4 cores); one busy core is 1.0, and a task wedged in D state sits at 1.0
# for good, so 0.75 reads both as busy.
IDLE_MAX_LOAD1 = 0.75
# A login whose tty was last touched under 300 s ago is a human at work.
IDLE_SESSION_QUIET_S = 300
# Cycle gap: a cycle is ~2 min of load; after 5 min the 1-min load average
# has decayed to ~0, so the next probe does not see our own cycle.
IDLE_GAP_S = 300
# Busy/unreachable: re-probe each minute (one ssh round trip, negligible load).
IDLE_RETRY_S = 60
# 30 busy cycles in a row (~30 min) is itself worth one line.
IDLE_STARVED_CYCLES = 30
# A regression must show in 2 consecutive cycles: one noisy run is not news.
IDLE_CONFIRM_CYCLES = 2

# Box processes that mean someone is using it (by comm).
BOX_BUSY_COMMS = frozenset(
    {
        "iperf3", "fio", "badblocks", "memtester", "stress-ng", "dd", "hdparm",
        "rsync", "scp", "dnf", "rpm", "modprobe", "insmod", "rmmod", "depmod",
        "dmsetup", "cryptsetup", "losetup", "nandwrite", "flash_erase", "nanddump",
        "mkfs.ext4", "fsck", "e2fsck", "tar",
    }
)  # fmt: skip
_BOX_DMATEST_THREAD = re.compile(r"^dma\d+chan\d+-")

IDLE_PROBE_SCRIPT = """echo '@@now'; date +%s
echo '@@load'; cat /proc/loadavg
echo '@@dmatest'; cat /sys/module/dmatest/parameters/run 2>/dev/null || echo absent
echo '@@procs'; ps -eo comm=
echo '@@sessions'; for t in $(who | awk '{print $2}'); do echo "$t $(stat -c %X /dev/$t 2>/dev/null)"; done
"""

# Dev-host work that must not be fought: deploys, flashes, other benchmarks,
# builds. The always-on iperf server (:5701) is only busy with a client on it.
LOCAL_BUSY = re.compile(
    r"dev\.py\s+(deploy-ssd|publish-fedora|flash\S*|netboot\S*|power-cycle|soak-test"
    r"|bench-all|uboot-test|ram-boot\S*|migrate\S*|build\S*|upgrade\S*)"
    r"|scripts/(deploy-\S+|publish-fedora|flash-\S+|netboot|ram-boot-deploy"
    r"|power-cycle|soak-test|bench-all|upgrade-ladder|uboot-test|migrate-\S+"
    r"|nand-\S+|build-\S+)"
    r"|\biperf3\s+-c\b"
)
IPERF_SERVER_PORT = 5701  # scripts/iperf-server.py


def parse_probe(text: str) -> dict | None:
    sec = parse_sections(text)
    try:
        now = int(sec["now"])
        load1 = float(sec["load"].split()[0])
    except (KeyError, ValueError, IndexError):
        return None
    sessions = []
    for line in sec.get("sessions", "").splitlines():
        parts = line.split()
        if len(parts) == 2 and parts[1].isdigit():
            sessions.append((parts[0], now - int(parts[1])))
    return {
        "load1": load1,
        "dmatest": sec.get("dmatest", "").strip(),
        "procs": sec.get("procs", "").split(),
        "sessions": sessions,
    }


def box_busy_reasons(
    probe: dict, check_load: bool = True, allow_sessions: bool = False
) -> list[str]:
    """Why the box is not idle; empty = idle. `check_load` is False between
    the subtests of a running cycle, where the load is our own."""
    why = []
    if check_load and probe["load1"] > IDLE_MAX_LOAD1:
        why.append(f"load {probe['load1']:.2f} > {IDLE_MAX_LOAD1}")
    if probe["dmatest"] == "Y":
        why.append("a dmatest run is in progress")
    procs = set(probe["procs"])
    busy = sorted(procs & BOX_BUSY_COMMS)
    if busy:
        why.append(f"box processes: {', '.join(busy)}")
    if any(_BOX_DMATEST_THREAD.match(p) for p in procs):
        why.append("dmatest threads exist")
    if not allow_sessions:
        for tty, idle_s in probe["sessions"]:
            if idle_s < IDLE_SESSION_QUIET_S:
                why.append(f"session on {tty} active {max(idle_s, 0)} s ago")
    return why


def local_busy_reasons(ps_text: str, ss_text: str, own_pids: set[int]) -> list[str]:
    """`ps -eo pid=,args=` and `ss -Htn state established` output on the dev
    host, as reasons. Other --idle runs are ignored; the lock handles those."""
    why = []
    for line in ps_text.splitlines():
        pid_s, _, args = line.strip().partition(" ")
        if not pid_s.isdigit() or int(pid_s) in own_pids:
            continue
        if "bench-all" in args and "--idle" in args:
            continue
        m = LOCAL_BUSY.search(args)
        if m:
            why.append(f"dev host: {m.group(0)[:60]}")
    if ss_text.strip():
        why.append(
            f"an iperf3 client is connected to the dev host :{IPERF_SERVER_PORT}"
        )
    return sorted(set(why))


def interruptible_wait(
    stop: threading.Event, seconds: float, poll_s: float = 1.0
) -> None:
    """Wait up to `seconds`, re-checking `stop` every `poll_s`. Ctrl-C lands
    in Event.wait, so this is a poll loop, not a blind sleep."""
    deadline = time.monotonic() + seconds
    while not stop.is_set():
        left = deadline - time.monotonic()
        if left <= 0:
            return
        stop.wait(min(poll_s, left))


class CycleInterrupted(Exception):
    """The box stopped being idle part-way through a cycle; discard it."""

    def __init__(self, reasons: list[str]) -> None:
        super().__init__("; ".join(reasons))
        self.reasons = reasons


class IdleRunner:
    """One --idle cycle at a time. The collaborators are injected so the
    policy (skip, discard, report-on-change, stop on a wedge) runs offline."""

    def __init__(
        self, probe, suite, emit, log, baseline, starved_after=IDLE_STARVED_CYCLES
    ):
        self.probe = probe  # (check_load: bool) -> list[str] of busy reasons
        self.suite = suite  # (gate) -> Report; may raise CycleInterrupted
        self.emit = emit  # a finding worth the terminal
        self.log = log  # everything else
        self.baseline = baseline
        self.tracker = ChangeTracker(IDLE_CONFIRM_CYCLES)
        self.starved_after = starved_after
        self.cycle = 0
        self.skips = 0
        self.starved = False

    def step(self) -> str:
        """One cycle: 'ran', 'skipped', 'interrupted' or 'wedged'."""
        self.cycle += 1
        n = self.cycle
        reasons = self.probe(True)
        if reasons:
            self.skips += 1
            self.log(
                f"cycle {n}: skipped ({self.skips} in a row): {'; '.join(reasons)}"
            )
            if self.skips == self.starved_after:
                self.starved = True
                self.emit(
                    f"STARVED not idle for {self.skips} cycles: {'; '.join(reasons)}"
                )
            return "skipped"
        if self.starved:
            self.emit(f"IDLE again after {self.skips} skipped cycles")
            self.starved = False
        self.skips = 0
        try:
            report = self.suite(lambda: self.probe(False))
        except CycleInterrupted as e:
            self.log(f"cycle {n}: interrupted, results discarded: {e}")
            return "interrupted"
        findings = failure_findings(report.checks) + find_regressions(
            report.metrics, self.baseline
        )
        new, cleared = self.tracker.update(findings)
        for f in new:
            self.emit(f"{'FAIL' if f.hard else 'REGRESSION'} {f.text}")
        for key in cleared:
            self.emit(f"RECOVERED {key}")
        bad = [f for f in findings]
        self.log(
            f"cycle {n}: {len(report.checks)} checks, {len(bad)} findings, "
            f"{len(new)} reported, {len(cleared)} cleared"
        )
        for f in bad:
            self.log(f"  {f.key}: {f.text}")
        if bad or not self.baseline:
            self.log("  metrics: " + json.dumps(report.metrics, sort_keys=True))
        if is_wedged(report.checks):
            self.emit(
                "WEDGED the box or the engine stopped answering: stopping, reboot it"
            )
            return "wedged"
        return "ran"


# ======== Orchestration ========


def now_iso() -> str:
    return datetime.datetime.now().astimezone().isoformat(timespec="seconds")


def log_line(msg: str) -> None:
    LOG.parent.mkdir(parents=True, exist_ok=True)
    with LOG.open("a") as f:
        f.write(f"{now_iso()}  {msg}\n")


def run_subtests(
    args,
    host: str,
    report: Report,
    gate=None,
    health_state: HealthState | None = None,
    take_over: bool = True,
) -> None:
    """Run the selected subtests. `gate()` (--idle) returns busy reasons and is
    checked before each one; any reason aborts with CycleInterrupted."""

    def check() -> None:
        reasons = gate() if gate else []
        if reasons:
            raise CycleInterrupted(reasons)

    pw = args.password
    if not args.skip_health:
        check()
        bench_health(host, pw, report, health_state)
    if not args.skip_eth:
        check()
        bench_ethernet(host, pw, args.eth_duration, report)
    if not args.skip_crypto:
        check()
        bench_crypto(host, pw, args.crypto_mb, report)
    if not args.skip_disk:
        check()
        bench_disks(host, pw, SATA_DEVICES, "sata", report)
    if not args.skip_usb:
        check()
        bench_disks(host, pw, USB_DEVICES, "usb", report)
    if not args.skip_dma:
        check()
        bench_dma(
            host,
            pw,
            args.dma_iterations,
            report,
            reload=args.dma_reload,
            pq=args.dma_pq,
            take_over=take_over,
        )
    if args.ssm:
        bench_ssm(
            host,
            pw,
            report,
            writers=args.ssm_writers,
            mib=args.ssm_mib,
            unbind=args.ssm_unbind_under_load,
        )
    if args.stress:
        bench_stress(host, pw, report, secs=args.stress_secs)


def snapshot_of(report: Report, host: str) -> dict:
    commit = subprocess.run(
        ["git", "rev-parse", "--short", "HEAD"],
        capture_output=True,
        text=True,
        check=False,
    ).stdout.strip()
    dirty = subprocess.run(
        ["git", "status", "--porcelain"], capture_output=True, text=True, check=False
    ).stdout.strip()
    return {
        "timestamp": now_iso(),
        "commit": commit + ("-dirty" if dirty else ""),
        "host": host,
        "kernel": report.info.get("kernel", ""),
        "checks": [{"name": n, "ok": ok, "detail": d} for n, ok, d in report.checks],
        "metrics": report.metrics,
        "all_ok": all(ok for _, ok, _ in report.checks),
    }


def run_suite(args) -> dict:
    host = args.host or locate_woomera()
    print(f"# woomera at {host}", file=sys.stderr)
    report = Report()
    run_subtests(args, host, report)
    return snapshot_of(report, host)


def do_compare(path_a: Path, path_b: Path) -> int:
    a = json.loads(path_a.read_text())
    b = json.loads(path_b.read_text())
    print(f"A: {path_a} ({a['timestamp']}, {a['commit']})")
    print(f"B: {path_b} ({b['timestamp']}, {b['commit']})")
    print()
    regressions = 0
    for k, va, vb, pct, bad in compare_metrics(a["metrics"], b["metrics"]):
        if va is None or vb is None:
            print(f"  {'?':>8}  {k}: missing in {'A' if va is None else 'B'}")
            continue
        regressions += bad
        print(
            f"  {pct:+7.1f}%  {k}: {va:.1f} -> {vb:.1f}{' <-- REGRESSION' if bad else ''}"
        )
    print()
    print(
        f"{regressions} regression(s) >= {REGRESSION_THRESHOLD * 100:.0f}%"
        if regressions
        else "no regressions"
    )
    return 1 if regressions else 0


# ======== Idle mode (--idle) ========


def _local(cmd: list[str]) -> str:
    # Waits for: ps / ss on this host, ms. 5 s = thousands of times that.
    try:
        return subprocess.run(
            cmd, capture_output=True, text=True, check=False, timeout=5
        ).stdout
    except subprocess.TimeoutExpired:
        return ""


def probe_reasons(
    host: str, password: str, check_load: bool, allow_sessions: bool
) -> list[str]:
    """Why the box (or the dev host) is busy now; empty = go ahead."""
    rc, out = run_remote(
        host, password, IDLE_PROBE_SCRIPT, timeout=DMA_SSH_CONNECT_S + DMA_SLACK_S
    )
    probe = parse_probe(out) if rc == 0 else None
    if probe is None:
        return [f"box unreachable or probe failed (rc={rc})"]
    why = box_busy_reasons(probe, check_load, allow_sessions)
    ps = _local(["ps", "-eo", "pid=,args="])
    ss = _local(
        ["ss", "-Htn", "state", "established", f"( sport = :{IPERF_SERVER_PORT} )"]
    )
    return why + local_busy_reasons(ps, ss, {os.getpid()})


def idle_cleanup(host: str, password: str) -> None:
    """Leave nothing running: stop dmatest (frees the channel) and our iperf3
    servers. Each call is bounded; failures are logged, not raised."""
    ok, why = _dma_stop(host, password)
    if not ok:
        log_line(f"cleanup: dmatest would not stop ({why}); engine presumed wedged")
    run_remote(
        host,
        password,
        "pkill -f '^iperf3 -s -p 56' 2>/dev/null; true",
        timeout=DMA_SSH_CONNECT_S + DMA_SLACK_S,
    )


def run_idle(args) -> int:
    LOCK.parent.mkdir(parents=True, exist_ok=True)
    lock = LOCK.open("w")
    try:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except OSError:
        print(f"another --idle run holds {LOCK}", file=sys.stderr)
        return 1

    def on_term(signum, frame):
        raise KeyboardInterrupt

    signal.signal(signal.SIGTERM, on_term)
    baseline = load_baseline(args.baseline)
    health_state = HealthState()
    health_state.baseline_first_look = True
    stop = threading.Event()

    def host_now() -> str | None:
        return args.host or _box.locate()

    def probe(check_load: bool) -> list[str]:
        host = host_now()
        if not host:
            return ["box not found on the LAN"]
        return probe_reasons(host, args.password, check_load, args.idle_allow_sessions)

    def suite(gate) -> Report:
        host = host_now()
        if not host:
            raise CycleInterrupted(["box not found on the LAN"])
        report = Report(echo=False)
        try:
            if health_state.mark is None:
                mark_dmesg(host, args.password, health_state, now_iso())
            run_subtests(
                args,
                host,
                report,
                gate=gate,
                health_state=health_state,
                take_over=False,
            )
        finally:
            idle_cleanup(host, args.password)
        mark_dmesg(host, args.password, health_state, now_iso())
        Path("tmp").mkdir(exist_ok=True)
        Path("tmp/bench-idle-last.json").write_text(
            json.dumps(snapshot_of(report, host), indent=2)
        )
        return report

    def emit(msg: str) -> None:
        line = f"{now_iso()}  {msg}"
        print(line, flush=True)
        log_line(msg)

    runner = IdleRunner(probe, suite, emit, log_line, baseline)
    log_line(
        f"idle mode start: baseline {args.baseline} ({len(baseline)} metrics), "
        f"gap {args.idle_gap}s, retry {args.idle_retry}s, cycles {args.idle_cycles or 'unbounded'}"
    )
    if not baseline:
        log_line(
            f"idle mode: no baseline at {args.baseline}; failures only, no regressions"
        )
    try:
        while True:
            outcome = runner.step()
            if outcome == "wedged":
                return 2
            if args.idle_cycles and runner.cycle >= args.idle_cycles:
                return 0
            interruptible_wait(
                stop, args.idle_gap if outcome == "ran" else args.idle_retry
            )
    except KeyboardInterrupt:
        log_line("idle mode: interrupted, exiting")
        return 0


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--host", help="woomera's address (default: auto-locate by MAC OUI)"
    )
    ap.add_argument("--password", default=DEFAULT_ROOT_PASSWORD)
    ap.add_argument("--skip-health", action="store_true")
    ap.add_argument("--skip-eth", action="store_true")
    ap.add_argument("--skip-crypto", action="store_true")
    ap.add_argument("--skip-disk", action="store_true")
    ap.add_argument("--skip-usb", action="store_true")
    ap.add_argument("--skip-dma", action="store_true")
    ap.add_argument(
        "--dma-iterations",
        type=int,
        default=20000,
        help="dmatest iterations per thread",
    )
    ap.add_argument(
        "--dma-reload",
        action="store_true",
        help="also rmmod/modprobe al_dma and run again (#289/#23); can wedge the engine",
    )
    ap.add_argument(
        "--dma-pq",
        action="store_true",
        help="with --dma-reload: reload with enable_pq=1 and run PQ at 8/16 KiB (#169)",
    )
    ap.add_argument("--i-accept-a-reboot", action="store_true")
    ap.add_argument(
        "--ssm",
        action="store_true",
        help="also drive al_ssm through dm-crypt: integrity + quiesce (#267, #300)",
    )
    ap.add_argument("--ssm-writers", type=int, default=4)
    ap.add_argument("--ssm-mib", type=int, default=64)
    ap.add_argument(
        "--ssm-unbind-under-load",
        action="store_true",
        help="with --ssm: PCI-unbind al_ssm while writers run (#301); needs --i-accept-a-crash",
    )
    ap.add_argument("--i-accept-a-crash", action="store_true")
    ap.add_argument(
        "--stress",
        action="store_true",
        help="also run memtester + stress-ng on the box (heavy; not for --idle)",
    )
    ap.add_argument("--stress-secs", type=int, default=60)
    ap.add_argument(
        "--eth-duration", type=int, default=8, help="seconds per direction per port"
    )
    ap.add_argument(
        "--crypto-mb", type=int, default=32, help="MB encrypted per crypto driver"
    )
    ap.add_argument(
        "--out",
        type=Path,
        help="write JSON snapshot here (default: bench-history/bench-<timestamp>.json, "
        "git-tracked - commit it after the run)",
    )
    ap.add_argument(
        "--baseline",
        type=Path,
        default=BASELINE,
        help="known-good metrics --idle compares against (default: %(default)s)",
    )
    ap.add_argument(
        "--save-baseline",
        action="store_true",
        help="after an all-green run, write its metrics to --baseline",
    )
    ap.add_argument(
        "--idle",
        action="store_true",
        help="loop while the box is otherwise unused; print findings only "
        "(failures, regressions vs --baseline); everything goes to the log",
    )
    ap.add_argument(
        "--idle-gap", type=int, default=IDLE_GAP_S, help="seconds between cycles"
    )
    ap.add_argument(
        "--idle-retry",
        type=int,
        default=IDLE_RETRY_S,
        help="seconds between probes while busy",
    )
    ap.add_argument(
        "--idle-cycles",
        type=int,
        default=0,
        help="stop after N cycles (0 = until Ctrl-C)",
    )
    ap.add_argument(
        "--idle-allow-sessions",
        action="store_true",
        help="do not treat a recently active login on the box as busy",
    )
    ap.add_argument(
        "--compare",
        nargs=2,
        metavar=("BEFORE.json", "AFTER.json"),
        help="skip running, just diff two prior snapshots",
    )
    args = ap.parse_args()
    if (args.dma_reload or args.dma_pq) and not args.i_accept_a_reboot:
        sys.exit(
            "--dma-reload/--dma-pq can wedge the engine (#23): add --i-accept-a-reboot"
        )
    if args.ssm_unbind_under_load and not (args.ssm and args.i_accept_a_crash):
        sys.exit(
            "--ssm-unbind-under-load can crash the box: needs --ssm --i-accept-a-crash"
        )
    if args.ssm and (args.ssm_mib * 256) % args.ssm_writers:
        sys.exit("--ssm-mib must split into whole 4 KiB blocks per --ssm-writers")
    if args.dma_pq and not args.dma_reload:
        sys.exit("--dma-pq needs --dma-reload (enable_pq is a module parameter)")
    if args.idle:
        heavy = [
            f
            for f in ("ssm", "stress", "dma_reload", "dma_pq", "save_baseline")
            if getattr(args, f)
        ]
        if heavy or args.out:
            sys.exit(
                "--idle is unattended: no --ssm/--stress/--dma-reload/--dma-pq "
                "(they can wedge the box), no --save-baseline/--out"
            )
        return run_idle(args)

    if args.compare:
        return do_compare(Path(args.compare[0]), Path(args.compare[1]))

    result = run_suite(args)

    LOG.parent.mkdir(parents=True, exist_ok=True)
    with LOG.open("a") as f:
        f.write(f"\n=== {result['timestamp']} ({result['commit']}) ===\n")
        for c in result["checks"]:
            f.write(f"[{'PASS' if c['ok'] else 'FAIL'}] {c['name']}: {c['detail']}\n")

    out_path = args.out or Path(
        f"bench-history/bench-{datetime.datetime.now().astimezone().strftime('%Y%m%d-%H%M%S')}.json"
    )
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(result, indent=2))
    print(f"\nsnapshot written to {out_path}")
    if args.save_baseline:
        if result["all_ok"]:
            keep = {
                k: result[k]
                for k in ("timestamp", "kernel", "commit", "host", "metrics")
            }
            args.baseline.parent.mkdir(parents=True, exist_ok=True)
            args.baseline.write_text(json.dumps(keep, indent=2) + "\n")
            print(f"baseline written to {args.baseline}")
        else:
            print("not saving a baseline: some checks failed")

    print("\nALL CHECKS PASSED" if result["all_ok"] else "\nSOME CHECKS FAILED")
    return 0 if result["all_ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
