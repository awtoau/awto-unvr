#!/usr/bin/env python3
"""A/B regression benchmark suite for woomera: ethernet, crypto, SATA disk,
USB. One run produces a comparable JSON snapshot; --compare diffs two
snapshots and flags regressions. Meant to be run before and after any driver/
kernel change to catch a regression in a subsystem you weren't even touching
(see #121: an ethernet queue-selection change was investigated for weeks
before anyone had a quick way to check "did this also break something else").

Ethernet: bidirectional iperf3 on BOTH ports (1G enp0s1, 10G enp0s2), bound
via SO_BINDTODEVICE - see test-eth.py's docstring for why a plain -B bind
silently picks the wrong NIC (#121).

Crypto: AES-256-XTS throughput via AF_ALG. AF_ALG only binds by the generic
algorithm name ("xts(aes)") - the kernel always resolves that to whichever
registered implementation has the highest crypto_alg priority; binding by
specific driver name (e.g. "xts-aes-al-ssm") is NOT supported by
algif_skcipher, it fails ENOENT (confirmed empirically - don't re-add this
without re-checking). So this benchmarks whatever's actually in the live
path today, and separately reads /proc/crypto to report which driver that
was - a priority change that silently knocks al_ssm out of first place
(rather than a throughput regression in al_ssm itself) shows up as a
different driver name next to the same-ish number, which is its own kind
of regression worth flagging.

Disk/USB: hdparm -t --direct (O_DIRECT sequential read, bypasses page
cache) on every SATA-attached disk and the USB-attached SSD. Read-only,
non-destructive - safe to run against the root device too.

Known limitation (see #23, 2026-08-29): al_dma/dmatest is NOT included.
Its hardware queue state was found to survive a module reload (rmmod/
modprobe doesn't reset the UDMA queue state machine), so back-to-back runs
without a reboot between them silently corrupt later results - not safe to
fold into a suite meant to be run repeatedly without a reboot each time.

  ./scripts/bench-all.py                            # full run, ~2min
  ./scripts/bench-all.py --out tmp/before.json
  ./scripts/bench-all.py --compare tmp/before.json tmp/after.json
  ./scripts/bench-all.py --skip-eth --skip-crypto --skip-disk --skip-usb
  ./scripts/bench-all.py --eth-duration 5 --crypto-mb 16   # faster, noisier
"""

from __future__ import annotations

import argparse
import base64
import datetime
import json
import re
import subprocess
import sys
from pathlib import Path

DEFAULT_ROOT_PASSWORD = "unvr"  # documented default, see docs/fedora-on-ssd.md
LOG = Path("tmp/logs/bench-all.log")

# Fixed NIC pairing - both box and dev host have exactly one 1G and one 10G
# port; auto-detecting "the fastest local NIC" (test-eth.py's approach) only
# works when testing a single target, not when we need both directions
# tested independently.
ETH_PAIRS = (
    {"box_iface": "enp0s1", "dev_iface": "enp5s0", "label": "1G", "port": 5602},
    {"box_iface": "enp0s2", "dev_iface": "enp7s0", "label": "10G", "port": 5603},
)

CRYPTO_ALG = "xts(aes)"

SATA_DEVICES = ("sdb", "sdc", "sdd", "sde")
USB_DEVICES = ("sda",)

REGRESSION_THRESHOLD = 0.10  # 10% drop flagged in --compare


def locate_woomera() -> str:
    out = subprocess.run(
        [sys.executable, "scripts/ssh-woomera.py", "--print"],
        capture_output=True,
        text=True,
        check=True,
    )
    return out.stdout.strip()


def ssh_cmd(host: str, password: str) -> list[str]:
    return [
        "sshpass",
        "-p",
        password,
        "ssh",
        "-o",
        "ConnectTimeout=8",
        "-o",
        "StrictHostKeyChecking=accept-new",
        "-o",
        "PreferredAuthentications=password",
        "-o",
        "PubkeyAuthentication=no",
        f"root@{host}",
    ]


def run_remote(host: str, password: str, remote_script: str, timeout: int = 60) -> tuple[int, str]:
    result = subprocess.run(
        ssh_cmd(host, password) + [remote_script],
        capture_output=True,
        text=True,
        check=False,
        timeout=timeout,
    )
    return result.returncode, result.stdout + result.stderr


class Report:
    def __init__(self) -> None:
        self.checks: list[tuple[str, bool, str]] = []
        self.metrics: dict[str, float] = {}

    def add(self, name: str, ok: bool, detail: str, metric: float | None = None) -> None:
        self.checks.append((name, ok, detail))
        mark = "PASS" if ok else "FAIL"
        print(f"[{mark}] {name}: {detail}")
        if metric is not None:
            self.metrics[name] = metric


# ======== Ethernet ========


def _iface_ip(host: str, password: str, iface: str) -> str | None:
    rc, out = run_remote(host, password, f"ip -4 -o addr show {iface}")
    m = re.search(r"inet (\d+\.\d+\.\d+\.\d+)/", out)
    return m.group(1) if rc == 0 and m else None


def _local_ip(iface: str) -> str | None:
    result = subprocess.run(
        ["ip", "-4", "-o", "addr", "show", iface], capture_output=True, text=True, check=False
    )
    m = re.search(r"inet (\d+\.\d+\.\d+\.\d+)/", result.stdout)
    return m.group(1) if m else None


def bench_eth_pair(host: str, password: str, pair: dict, duration: int, report: Report) -> None:
    label = pair["label"]
    target = _iface_ip(host, password, pair["box_iface"])
    if target is None:
        report.add(f"eth {label}", False, f"couldn't get IP for {pair['box_iface']}")
        return
    local_ip = _local_ip(pair["dev_iface"])
    if local_ip is None:
        report.add(f"eth {label}", False, f"couldn't get local IP for {pair['dev_iface']}")
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
        capture_output=True, check=False,
    )

    port = pair["port"]
    run_remote(
        host,
        password,
        f"pkill -f '^iperf3 -s -p {port}' 2>/dev/null; "
        f"nohup iperf3 -s -p {port} > /root/bench-all-iperf-server-{port}.log 2>&1 & disown; echo started",
    )
    for _ in range(40):
        _rc, out = run_remote(host, password, f"ss -ltn | grep -q ':{port} ' && echo up")
        if "up" in out:
            break
    else:
        report.add(f"eth {label}", False, f"iperf3 server never bound to port {port} within 2s")
        return

    try:
        result = subprocess.run(
            [
                "iperf3", "-c", target,
                "-B", f"{local_ip}%{pair['dev_iface']}",
                "-p", str(port), "-t", str(duration), "-P", "4", "--bidir",
                # iperf3 has no built-in connect timeout - if SO_BINDTODEVICE
                # picks an interface with no real L2 path to the target (dead
                # link, ARP not resolving), it hangs at connect() forever
                # rather than erroring, so the process-level timeout below is
                # the only thing that ever ends it.
            ],
            capture_output=True, text=True, check=False, timeout=duration + 15,
        )
    except subprocess.TimeoutExpired:
        report.add(
            f"eth {label}", False,
            f"iperf3 client hung past {duration + 15}s - {pair['dev_iface']} likely has no "
            f"working L2 path to {target} right now (check `ip neigh` / link state)",
        )
        run_remote(host, password, f"pkill -f '^iperf3 -s -p {port}' 2>/dev/null; true")
        return
    run_remote(host, password, f"pkill -f '^iperf3 -s -p {port}' 2>/dev/null; true")

    if result.returncode != 0 or "iperf Done" not in result.stdout:
        report.add(f"eth {label}", False, f"iperf3 failed (rc={result.returncode}): {result.stderr[:200]}")
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
        f"eth {label} host->box", True, f"{tx.group(1)} {tx.group(2)}/sec", metric=tx_mbps
    )
    report.add(
        f"eth {label} box->host", True, f"{rx.group(1)} {rx.group(2)}/sec", metric=rx_mbps
    )


def bench_ethernet(host: str, password: str, duration: int, report: Report) -> None:
    for pair in ETH_PAIRS:
        bench_eth_pair(host, password, pair, duration, report)


# ======== Crypto (AF_ALG) ========

_CRYPTO_REMOTE_SCRIPT = """
import socket, os, time, json, sys, re

def active_driver(alg_name):
    # Highest-priority registered driver for `alg_name`, per /proc/crypto -
    # what AF_ALG's generic-name bind will actually resolve to.
    best_driver, best_prio = None, None
    name = driver = None
    prio = None
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
    except (OSError, socket.timeout) as e:
        s.close()
        return {"available": True, "error": f"op failed: {e}"}
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
            f"crypto {CRYPTO_ALG} [{r['driver']}]", True,
            f"{r['mb_per_sec']:.1f} MB/s ({r['mb']:.1f}MB in {r['seconds']:.2f}s)",
            metric=r["mb_per_sec"],
        )
    else:
        report.add(f"crypto {CRYPTO_ALG}", False, r.get("error", "op failed"))


# ======== Disk / USB (hdparm -t --direct) ========


def _hdparm_direct(host: str, password: str, dev: str) -> tuple[bool, str, float | None]:
    rc, out = run_remote(host, password, f"hdparm -t --direct /dev/{dev}", timeout=30)
    m = re.search(r"=\s*([\d.]+)\s*MB/sec", out)
    if rc != 0 or not m:
        return False, out.strip().splitlines()[-1] if out.strip() else f"rc={rc}", None
    mbps = float(m.group(1))
    return True, f"{mbps:.1f} MB/sec", mbps


def bench_disks(host: str, password: str, devices: tuple[str, ...], label: str, report: Report) -> None:
    for dev in devices:
        ok, detail, mbps = _hdparm_direct(host, password, dev)
        report.add(f"{label} {dev}", ok, detail, metric=mbps)


# ======== Orchestration ========


def run_suite(args) -> dict:
    host = args.host or locate_woomera()
    print(f"# woomera at {host}", file=sys.stderr)
    report = Report()

    if not args.skip_eth:
        bench_ethernet(host, args.password, args.eth_duration, report)
    if not args.skip_crypto:
        bench_crypto(host, args.password, args.crypto_mb, report)
    if not args.skip_disk:
        bench_disks(host, args.password, SATA_DEVICES, "sata", report)
    if not args.skip_usb:
        bench_disks(host, args.password, USB_DEVICES, "usb", report)

    stamp = datetime.datetime.now().astimezone().isoformat(timespec="seconds")
    commit = subprocess.run(
        ["git", "rev-parse", "--short", "HEAD"], capture_output=True, text=True, check=False
    ).stdout.strip()
    dirty = subprocess.run(
        ["git", "status", "--porcelain"], capture_output=True, text=True, check=False
    ).stdout.strip()

    return {
        "timestamp": stamp,
        "commit": commit + ("-dirty" if dirty else ""),
        "host": host,
        "checks": [{"name": n, "ok": ok, "detail": d} for n, ok, d in report.checks],
        "metrics": report.metrics,
        "all_ok": all(ok for _, ok, _ in report.checks),
    }


def do_compare(path_a: Path, path_b: Path) -> int:
    a = json.loads(path_a.read_text())
    b = json.loads(path_b.read_text())
    print(f"A: {path_a} ({a['timestamp']}, {a['commit']})")
    print(f"B: {path_b} ({b['timestamp']}, {b['commit']})")
    print()
    regressions = 0
    keys = sorted(set(a["metrics"]) | set(b["metrics"]))
    for k in keys:
        va, vb = a["metrics"].get(k), b["metrics"].get(k)
        if va is None or vb is None:
            print(f"  {'?':>8}  {k}: missing in {'A' if va is None else 'B'}")
            continue
        pct = (vb - va) / va * 100 if va else 0.0
        flag = ""
        if pct <= -REGRESSION_THRESHOLD * 100:
            flag = " <-- REGRESSION"
            regressions += 1
        print(f"  {pct:+7.1f}%  {k}: {va:.1f} -> {vb:.1f}{flag}")
    print()
    print(f"{regressions} regression(s) >= {REGRESSION_THRESHOLD*100:.0f}%" if regressions else "no regressions")
    return 1 if regressions else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--host", help="woomera's address (default: auto-locate by MAC OUI)")
    ap.add_argument("--password", default=DEFAULT_ROOT_PASSWORD)
    ap.add_argument("--skip-eth", action="store_true")
    ap.add_argument("--skip-crypto", action="store_true")
    ap.add_argument("--skip-disk", action="store_true")
    ap.add_argument("--skip-usb", action="store_true")
    ap.add_argument("--eth-duration", type=int, default=8, help="seconds per direction per port")
    ap.add_argument("--crypto-mb", type=int, default=32, help="MB encrypted per crypto driver")
    ap.add_argument("--out", type=Path, help="write JSON snapshot here (default: tmp/bench-<timestamp>.json)")
    ap.add_argument(
        "--compare", nargs=2, metavar=("BEFORE.json", "AFTER.json"),
        help="skip running, just diff two prior snapshots",
    )
    args = ap.parse_args()

    if args.compare:
        return do_compare(Path(args.compare[0]), Path(args.compare[1]))

    result = run_suite(args)

    LOG.parent.mkdir(parents=True, exist_ok=True)
    with LOG.open("a") as f:
        f.write(f"\n=== {result['timestamp']} ({result['commit']}) ===\n")
        for c in result["checks"]:
            f.write(f"[{'PASS' if c['ok'] else 'FAIL'}] {c['name']}: {c['detail']}\n")

    out_path = args.out or Path(f"tmp/bench-{datetime.datetime.now().strftime('%Y%m%d-%H%M%S')}.json")
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(result, indent=2))
    print(f"\nsnapshot written to {out_path}")

    print("\nALL CHECKS PASSED" if result["all_ok"] else "\nSOME CHECKS FAILED")
    return 0 if result["all_ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
