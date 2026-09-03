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
    # No flush=True here - this can run in a tight loop draining a burst of
    # confirmed losses, and a flush syscall per line is exactly the kind of
    # per-event cost that risks falling behind the sender (see SO_RCVBUF
    # comment above). Buffered is fine; flush_log() persists to file anyway.
    line = f"{time.strftime('%Y-%m-%dT%H:%M:%S%z')}  {msg}"
    print(line)
    _log_lines.append(line)


def flush_log() -> None:
    LOG_DIR.mkdir(parents=True, exist_ok=True)
    with open(LOG_FILE, "a") as f:
        f.write("\n".join(_log_lines) + "\n")


def run_send(args: argparse.Namespace) -> int:
    """Sender: fire sequence-numbered UDP packets at max rate for
    --duration seconds, then print a machine-readable summary line."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    if args.src_iface:
        # SO_BINDTODEVICE, not just a source-address bind: binding only the
        # address does NOT force the egress interface when there's a single
        # route to the destination (confirmed live - a src-ip-only bind sent
        # 0 packets out the intended interface, all traffic still went out
        # the route table's preferred NIC). This is the exact "wrong NIC"
        # bug class #121 already found once in test-eth.py.
        sock.setsockopt(
            socket.SOL_SOCKET, socket.SO_BINDTODEVICE, args.src_iface.encode() + b"\0"
        )
    if args.src_ip:
        sock.bind((args.src_ip, 0))
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
    # Request the largest receive buffer this host's net.core.rmem_max allows
    # (4MB here) - a Python recvfrom() loop can't always drain a max-rate
    # sender fast enough per-packet, and a too-small kernel UDP buffer drops
    # packets BEFORE this script ever sees them, which looks identical to
    # real loss on the box's TX path. Confirmed via `netstat -su` showing
    # nonzero "receive buffer errors" on this host during earlier runs -
    # this is a real confound, not a hypothetical one.
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4 * 1024 * 1024)
    actual_rcvbuf = sock.getsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF)
    sock.bind((args.bind_ip, args.port))
    sock.settimeout(0.2)
    recv_bufsize = 9200  # covers up to jumbo-frame payload sizes
    log(
        f"SO_RCVBUF requested 4194304, kernel gave {actual_rcvbuf} "
        f"(reported value is 2x the real allocation - normal Linux behavior)"
    )

    start = time.monotonic()
    last_rx = start
    expected: int | None = None
    received_count = 0
    ooo: dict[int, float] = {}  # seq -> monotonic arrival time, for seq > expected
    grace_s = args.grace_ms / 1000.0
    idle_timeout_s = 5.0  # stream considered finished after this much silence
    lost: list[tuple[int, float]] = []  # (seq, elapsed_s when confirmed lost)
    stop_on_first = args.stop_on_first_gap

    def confirm_losses() -> None:
        """expected's grace period may have elapsed - and once it's marked
        lost and skipped, the NEW expected might already be timed out too
        (a burst of consecutive losses). Drain all of them in one pass so a
        cluster of losses is recorded individually, not merged into one."""
        nonlocal expected
        while ooo:
            earliest = min(ooo.values())
            if time.monotonic() - earliest <= grace_s:
                break
            elapsed = time.monotonic() - start
            lost.append((expected, elapsed))
            log(
                f"  LOST: seq {expected} at {elapsed:.3f}s "
                f"({received_count} received so far)"
            )
            expected += 1
            while expected in ooo:
                del ooo[expected]
                expected += 1

    # Checking confirm_losses() (an O(pending-gaps) scan) on every single
    # packet adds real per-packet CPU cost in the hot receive loop - at
    # max sender rate that cost is exactly what risks falling behind and
    # causing the kernel to drop packets itself (the SO_RCVBUF confound
    # above). grace_s is generously long (default 50ms) relative to how
    # often a syscall-bound recv loop iterates, so checking every 128
    # packets instead of every 1 loses no real precision.
    CHECK_EVERY = 128
    since_check = 0

    while True:
        try:
            data, _addr = sock.recvfrom(recv_bufsize)
            now = time.monotonic()
            last_rx = now
            if len(data) < HEADER.size:
                continue
            (seq,) = HEADER.unpack_from(data)
            received_count += 1
            since_check += 1
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
            if since_check < CHECK_EVERY:
                continue
            since_check = 0
        except socket.timeout:
            now = time.monotonic()
            if expected is not None and now - last_rx > idle_timeout_s:
                confirm_losses()
                break

        confirm_losses()
        if stop_on_first and lost:
            missing, elapsed = lost[0]
            log(
                f"GAP CONFIRMED (stopping on first, per --stop-on-first-gap): "
                f"seq {missing} never arrived"
            )
            flush_log()
            print(
                f"RESULT=GAP missing_seq={missing} received_before={received_count} "
                f"elapsed_s={elapsed:.3f}"
            )
            return 1

    elapsed = time.monotonic() - start
    if lost:
        seqs = [s for s, _ in lost]
        times = [f"{t:.3f}" for _, t in lost]
        log(f"{len(lost)} packet(s) confirmed lost: seqs={seqs}")
        log(f"  at times(s)={times}")
        log(
            f"  received {received_count} total, expected pointer reached "
            f"{expected}, run lasted {elapsed:.3f}s"
        )
        flush_log()
        print(
            f"RESULT=GAP count={len(lost)} first_seq={seqs[0]} first_elapsed_s={times[0]} "
            f"last_seq={seqs[-1]} last_elapsed_s={times[-1]} received={received_count}"
        )
        return 1

    log(
        f"no gap detected: received {received_count} packets, "
        f"expected pointer reached {expected}, {elapsed:.3f}s, "
        f"idle {idle_timeout_s}s ended the run"
    )
    flush_log()
    print(
        f"RESULT=CLEAN received={received_count} expected_reached={expected} "
        f"elapsed_s={elapsed:.3f}"
    )
    return 0


def run_orchestrate(args: argparse.Namespace) -> int:
    log(
        f"tx-gap-probe: box {args.box_ip} -> {args.bind_ip}%{args.bind_iface}:"
        f"{args.port}, {args.duration}s, grace {args.grace_ms}ms, "
        f"payload {args.payload_size}B"
    )

    recv_cmd = [
        sys.executable,
        __file__,
        "--mode",
        "recv",
        "--bind-ip",
        args.bind_ip,
        "--port",
        str(args.port),
        "--grace-ms",
        str(args.grace_ms),
    ]
    if args.stop_on_first_gap:
        recv_cmd.append("--stop-on-first-gap")
    recv_proc = subprocess.Popen(
        recv_cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    time.sleep(0.3)  # let the receiver's bind() land before traffic starts

    script_src = Path(__file__).read_text()
    send_cmd = (
        f"python3 - --mode send --host {args.bind_ip} --port {args.port} "
        f"--duration {args.duration} --payload-size {args.payload_size}"
    )
    if args.src_ip:
        send_cmd += f" --src-ip {args.src_ip}"
    if args.src_iface:
        send_cmd += f" --src-iface {args.src_iface}"
    ssh = subprocess.run(
        [
            "ssh",
            "-o",
            f"ConnectTimeout={SSH_CONNECT_TIMEOUT}",
            "-o",
            "StrictHostKeyChecking=accept-new",
            f"root@{args.box_ip}",
            send_cmd,
        ],
        input=script_src,
        capture_output=True,
        text=True,
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
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--mode",
        choices=["send", "recv"],
        default=None,
        help="internal: run as sender or receiver only. Omit to "
        "orchestrate both (local recv + remote send over SSH).",
    )
    ap.add_argument("--box-ip", help="woomera's IP (orchestrate mode)")
    ap.add_argument("--bind-ip", help="local IP to receive on (10G-capable NIC)")
    ap.add_argument("--bind-iface", default="", help="local interface (informational)")
    ap.add_argument("--host", help="send mode: destination IP (the receiver)")
    ap.add_argument(
        "--src-ip",
        help="send mode: source IP to bind (cosmetic - "
        "sets the packet's source address; use --src-iface to "
        "actually control which NIC it egresses)",
    )
    ap.add_argument(
        "--src-iface",
        help="send mode: box-side interface name "
        "(e.g. enp0s1) to force egress through via "
        "SO_BINDTODEVICE - required when the box has multiple "
        "NICs on the same subnet as the receiver, since routing "
        "alone picks one NIC regardless of source-address bind",
    )
    ap.add_argument("--port", type=int, default=5604)
    ap.add_argument(
        "--duration", type=int, default=20, help="seconds of sending at max rate"
    )
    ap.add_argument(
        "--grace-ms",
        type=int,
        default=50,
        help="how long a later packet must have been waiting "
        "before we declare an earlier one truly missing",
    )
    ap.add_argument(
        "--payload-size",
        type=int,
        default=DEFAULT_PAYLOAD_SIZE,
        help="UDP payload bytes per packet, header included "
        "(min 8 for the sequence number)",
    )
    ap.add_argument(
        "--stop-on-first-gap",
        action="store_true",
        help="exit immediately on the first confirmed loss "
        "instead of running the full --duration and "
        "reporting every loss (default: report all)",
    )
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
