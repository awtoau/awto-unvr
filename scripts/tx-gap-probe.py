#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Sequence-numbered UDP probe: send box(TX)->host with an incrementing
8-byte sequence number in every packet, abort the instant one is confirmed
missing. Complements iperf3/tcpdump-based tests (see eth-tx-capture-diag.py)
with an unambiguous ground truth - no TCP retransmit/congestion-control
behaviour to interpret, just "packet N never arrived".

We're on a closed LAN (no other traffic, no WAN) - real loss here should be
exactly zero. Any gap is either a real drop on the wire/switch/NIC or a bug
in this probe; there's no third explanation (background traffic, transient
internet congestion) available to blame it on.

A "gap" is declared only after a grace period: once a packet numbered above
`expected` arrives, `expected` is at risk, but we wait --grace-ms before
concluding it's truly lost - long enough that a same-LAN reorder (sub-ms
latency, confirmed in #121) couldn't explain it, so any real gap fires long
before the grace period would ever need to matter for a legitimate reorder.

  ./scripts/tx-gap-probe.py --box-ip 192.168.25.133 \\
      --bind-ip 192.168.25.147 --bind-iface enp7s0 --port 5604 --duration 20

Runs the receiver locally, pipes this same file over SSH to run as the
sender on the box (--mode send), and reports the first gap or a clean pass.
"""

from __future__ import annotations

import argparse
import socket
import struct
import subprocess
import sys
import time
from pathlib import Path

LOG_DIR = Path("tmp/logs")
LOG_FILE = LOG_DIR / "tx-gap-probe.log"
SSH_CONNECT_TIMEOUT = 8
HEADER = struct.Struct(">Q")  # 8-byte big-endian sequence number
DEFAULT_PAYLOAD_SIZE = 1200  # + 28B UDP/IP header = 1228B, under 1500 MTU

_log_lines: list[str] = []


def log(msg: str) -> None:
    line = f"{time.strftime('%Y-%m-%dT%H:%M:%S%z')}  {msg}"
    print(line, flush=True)
    _log_lines.append(line)


def flush_log() -> None:
    LOG_DIR.mkdir(parents=True, exist_ok=True)
    with open(LOG_FILE, "a") as f:
        f.write("\n".join(_log_lines) + "\n")


def run_send(args: argparse.Namespace) -> int:
    """Sender: fire sequence-numbered UDP packets at max rate for
    --duration seconds, then print a machine-readable summary line."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    filler = b"\0" * (args.payload_size - HEADER.size)
    dest = (args.host, args.port)
    seq = 0
    deadline = time.monotonic() + args.duration
    check_every = 256
    while True:
        sock.sendto(HEADER.pack(seq) + filler, dest)
        seq += 1
        if seq % check_every == 0 and time.monotonic() >= deadline:
            break
    print(f"SENT_TOTAL={seq}", flush=True)
    return 0


def run_recv(args: argparse.Namespace) -> int:
    """Receiver: track a forward `expected` pointer + an out-of-order
    buffer of arrived-early packets. A gap is confirmed once the oldest
    out-of-order arrival has been sitting for --grace-ms with `expected`
    still unfilled - see module docstring for why that's a safe threshold
    on this LAN."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((args.bind_ip, args.port))
    sock.settimeout(0.2)
    recv_bufsize = 9200  # covers up to jumbo-frame payload sizes

    start = time.monotonic()
    last_rx = start
    expected: int | None = None
    received_count = 0
    ooo: dict[int, float] = {}  # seq -> monotonic arrival time, for seq > expected
    grace_s = args.grace_ms / 1000.0
    idle_timeout_s = 5.0  # stream considered finished after this much silence

    def check_gap() -> tuple[int, list[int]] | None:
        if not ooo:
            return None
        earliest = min(ooo.values())
        if time.monotonic() - earliest > grace_s:
            return expected, sorted(ooo.keys())[:10]
        return None

    while True:
        try:
            data, _addr = sock.recvfrom(recv_bufsize)
            now = time.monotonic()
            last_rx = now
            if len(data) < HEADER.size:
                continue
            (seq,) = HEADER.unpack_from(data)
            received_count += 1
            if expected is None:
                expected = seq
            if seq == expected:
                expected += 1
                while expected in ooo:
                    del ooo[expected]
                    expected += 1
            elif seq > expected:
                ooo[seq] = now
            # seq < expected: a late arrival for something already resolved -
            # ignore, it's not a gap (something eventually filled it).
        except socket.timeout:
            now = time.monotonic()
            if expected is not None and now - last_rx > idle_timeout_s:
                break

        gap = check_gap()
        if gap is not None:
            missing, arrived_after = gap
            elapsed = time.monotonic() - start
            log(f"GAP CONFIRMED: seq {missing} never arrived "
                f"(grace {args.grace_ms}ms elapsed since a later packet did)")
            log(f"  packets after the gap that DID arrive (first 10): {arrived_after}")
            log(f"  received {received_count} packets cleanly before this, "
                f"{elapsed:.3f}s into the run")
            flush_log()
            print(f"RESULT=GAP missing_seq={missing} received_before={received_count} "
                  f"elapsed_s={elapsed:.3f}")
            return 1

    elapsed = time.monotonic() - start
    log(f"no gap detected: received {received_count} packets, "
        f"expected pointer reached {expected}, {elapsed:.3f}s, "
        f"idle {idle_timeout_s}s ended the run")
    flush_log()
    print(f"RESULT=CLEAN received={received_count} expected_reached={expected} "
          f"elapsed_s={elapsed:.3f}")
    return 0


def run_orchestrate(args: argparse.Namespace) -> int:
    log(f"tx-gap-probe: box {args.box_ip} -> {args.bind_ip}%{args.bind_iface}:"
        f"{args.port}, {args.duration}s, grace {args.grace_ms}ms, "
        f"payload {args.payload_size}B")

    recv_proc = subprocess.Popen(
        [sys.executable, __file__, "--mode", "recv",
         "--bind-ip", args.bind_ip, "--port", str(args.port),
         "--grace-ms", str(args.grace_ms)],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
    )
    time.sleep(0.3)  # let the receiver's bind() land before traffic starts

    script_src = Path(__file__).read_text()
    send_cmd = (
        f"python3 - --mode send --host {args.bind_ip} --port {args.port} "
        f"--duration {args.duration} --payload-size {args.payload_size}"
    )
    ssh = subprocess.run(
        ["ssh", "-o", f"ConnectTimeout={SSH_CONNECT_TIMEOUT}",
         "-o", "StrictHostKeyChecking=accept-new",
         f"root@{args.box_ip}", send_cmd],
        input=script_src, capture_output=True, text=True,
        timeout=args.duration + SSH_CONNECT_TIMEOUT + 10,
    )
    log(f"sender (box) output: {ssh.stdout.strip()!r} stderr={ssh.stderr.strip()!r}")

    try:
        recv_out, _ = recv_proc.communicate(timeout=15)
    except subprocess.TimeoutExpired:
        recv_proc.kill()
        recv_out, _ = recv_proc.communicate()
        log("receiver did not exit on its own idle-timeout - killed it")

    for line in recv_out.splitlines():
        log(f"  [recv] {line}")
    flush_log()

    if "RESULT=GAP" in recv_out:
        print("FAIL: real packet loss confirmed on the box's TX path - see log above")
        return 1
    if "RESULT=CLEAN" in recv_out:
        print("PASS: zero packet loss over the run")
        return 0
    print("INCONCLUSIVE: receiver produced no RESULT line - check the log")
    return 2


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--mode", choices=["send", "recv"], default=None,
                     help="internal: run as sender or receiver only. Omit to "
                          "orchestrate both (local recv + remote send over SSH).")
    ap.add_argument("--box-ip", help="woomera's IP (orchestrate mode)")
    ap.add_argument("--bind-ip", help="local IP to receive on (10G-capable NIC)")
    ap.add_argument("--bind-iface", default="", help="local interface (informational)")
    ap.add_argument("--host", help="send mode: destination IP (the receiver)")
    ap.add_argument("--port", type=int, default=5604)
    ap.add_argument("--duration", type=int, default=20,
                     help="seconds of sending at max rate")
    ap.add_argument("--grace-ms", type=int, default=50,
                     help="how long a later packet must have been waiting "
                          "before we declare an earlier one truly missing")
    ap.add_argument("--payload-size", type=int, default=DEFAULT_PAYLOAD_SIZE,
                     help="UDP payload bytes per packet, header included "
                          "(min 8 for the sequence number)")
    args = ap.parse_args()

    if args.payload_size < HEADER.size:
        ap.error(f"--payload-size must be >= {HEADER.size} (sequence header)")

    if args.mode == "send":
        return run_send(args)
    if args.mode == "recv":
        return run_recv(args)
    if not args.box_ip or not args.bind_ip:
        ap.error("--box-ip and --bind-ip are required to orchestrate a run")
    return run_orchestrate(args)


if __name__ == "__main__":
    sys.exit(main())
