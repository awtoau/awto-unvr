#!/usr/bin/env python3
"""Isolated (non-bidir) iperf3 throughput + simultaneous packet capture on
both ends, with tshark TCP-anomaly analysis - the exact methodology used by
hand, repeatedly, at high token cost, to diagnose #121's al_eth TX collapse.
Wrap it once, reuse it, instead of re-typing SSH/tcpdump/tshark chains.

Why isolated, not --bidir: iperf3's --bidir mode runs both directions
concurrently over the same connections, and the two directions can starve
each other - misleading for isolating which direction is actually broken.
This runs ONE direction per invocation, sequentially if you need both.

Why capture on both ends: a capture on the box's own egress interface shows
whether packets that should egress actually do (a wrong-route/wrong-NIC bug,
e.g. #170, shows up as "expected direction has zero packets" here); a
capture on the receiving end shows what actually arrived. Comparing both against
`ip -s link` byte counters catches the case where iperf3's own userspace
stats don't reflect real wire traffic at all.

Requires: local passwordless sudo for tcpdump on the bind interface (see
bench-all.py's ARP-flush precedent for the same requirement), tcpdump+tshark
locally and on the box, an iperf3 server the box can run (auto-started).

  ./scripts/eth-tx-capture-diag.py --box-ip <box-ip> --box-iface enp0s2 \\
      --bind-ip <dev-host-ip> --bind-iface enp7s0 --port 5603 --direction tx
  ./scripts/eth-tx-capture-diag.py --box-ip <box-ip> --box-iface enp0s2 \\
      --bind-ip <dev-host-ip> --bind-iface enp7s0 --port 5603 --direction rx \\
      --streams 1 --duration 8 --no-capture   # throughput only, no pcap
"""

from __future__ import annotations

import argparse
import json
import shlex
import subprocess
import sys
import time
from pathlib import Path

DEFAULT_ROOT_PASSWORD = "unvr"
LOG_DIR = Path("tmp/logs")
LOG_FILE = LOG_DIR / "eth-tx-capture-diag.log"
SSH_CONNECT_TIMEOUT = 8  # matches run_remote's own ssh -o ConnectTimeout

_log_lines: list[str] = []


def log(msg: str) -> None:
    line = f"{time.strftime('%Y-%m-%dT%H:%M:%S%z')}  {msg}"
    print(line, flush=True)
    _log_lines.append(line)


def flush_log() -> None:
    LOG_DIR.mkdir(parents=True, exist_ok=True)
    with open(LOG_FILE, "a") as f:
        f.write("\n".join(_log_lines) + "\n")


def ssh_cmd(box_ip: str, password: str) -> list[str]:
    return [
        "sshpass",
        "-p",
        password,
        "ssh",
        "-o",
        f"ConnectTimeout={SSH_CONNECT_TIMEOUT}",
        "-o",
        "StrictHostKeyChecking=accept-new",
        "-o",
        "PreferredAuthentications=password",
        "-o",
        "PubkeyAuthentication=no",
        f"root@{box_ip}",
    ]


def run_remote(
    box_ip: str, password: str, cmd: str, timeout: int = 30
) -> tuple[int, str]:
    """Never raises TimeoutExpired - callers get (124, <message>) on timeout
    instead of an uncaught crash that would skip cleanup (matches the
    incident this script exists to prevent: an unbounded/uncaught SSH call
    hanging and leaving orphaned remote+local processes)."""
    try:
        p = subprocess.run(
            ssh_cmd(box_ip, password) + [cmd],
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        return p.returncode, (p.stdout + p.stderr)
    except subprocess.TimeoutExpired:
        return 124, f"ssh call timed out after {timeout}s: {cmd[:200]}"


def scp_pull(
    box_ip: str, password: str, remote_path: str, local_path: Path, timeout: int = 30
) -> tuple[int, str]:
    """scp with the same password auth + bounded timeout as run_remote -
    the original version had neither (no sshpass, no timeout at all),
    silently failing every pull on a password-only box and risking an
    unbounded hang identical to the SSH incident this script replaces."""
    try:
        p = subprocess.run(
            [
                "sshpass",
                "-p",
                password,
                "scp",
                "-o",
                f"ConnectTimeout={SSH_CONNECT_TIMEOUT}",
                "-o",
                "StrictHostKeyChecking=accept-new",
                "-o",
                "PreferredAuthentications=password",
                "-o",
                "PubkeyAuthentication=no",
                f"root@{box_ip}:{remote_path}",
                str(local_path),
            ],
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        return p.returncode, (p.stdout + p.stderr)
    except subprocess.TimeoutExpired:
        return 124, f"scp timed out after {timeout}s pulling {remote_path}"


def poll_remote_listening(
    box_ip: str, password: str, port: int, attempts: int = 20, interval: float = 0.5
) -> bool:
    """Poll instead of a blind sleep - bench-all.py's own established
    pattern (ss -ltn check) for this exact "did the server actually bind
    yet" problem, not reused in the first version of this script."""
    for _ in range(attempts):
        rc, out = run_remote(
            box_ip, password, f"ss -ltn | grep -q ':{port} ' && echo up", timeout=10
        )
        if "up" in out:
            return True
        time.sleep(interval)
    return False


def tshark_flag_count(pcap: Path, flag: str) -> int:
    p = subprocess.run(
        ["tshark", "-r", str(pcap), "-Y", f"tcp.analysis.{flag}"],
        capture_output=True,
        text=True,
    )
    return len([l for l in p.stdout.splitlines() if l.strip()])


def tshark_direction_packets(pcap: Path, src_ip: str) -> int:
    p = subprocess.run(
        ["tshark", "-r", str(pcap), "-Y", f"ip.src=={src_ip}"],
        capture_output=True,
        text=True,
    )
    return len([l for l in p.stdout.splitlines() if l.strip()])


def _capture_analysis(pcap: Path, box_ip: str, bind_ip: str) -> dict:
    flags = {}
    for flag in (
        "retransmission",
        "fast_retransmission",
        "out_of_order",
        "duplicate_ack",
        "zero_window",
    ):
        flags[flag] = tshark_flag_count(pcap, flag)
    return {
        "packets_from_box": tshark_direction_packets(pcap, box_ip),
        "packets_from_peer": tshark_direction_packets(pcap, bind_ip),
        **flags,
    }


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--box-ip", required=True, help="box IP to connect to (must match --box-iface)"
    )
    ap.add_argument(
        "--box-iface",
        required=True,
        help="box interface expected to carry this traffic",
    )
    ap.add_argument(
        "--bind-ip", required=True, help="local IP to bind the iperf3 client to"
    )
    ap.add_argument(
        "--bind-iface",
        required=True,
        help="local interface to bind the iperf3 client to",
    )
    ap.add_argument("--port", type=int, default=5601)
    ap.add_argument(
        "--direction",
        choices=["tx", "rx"],
        required=True,
        help="tx = box sends (iperf3 -R); rx = box receives",
    )
    ap.add_argument("--streams", type=int, default=4)
    ap.add_argument("--duration", type=int, default=10)
    ap.add_argument("--password", default=DEFAULT_ROOT_PASSWORD)
    ap.add_argument(
        "--no-capture", action="store_true", help="skip tcpdump/tshark, iperf3 only"
    )
    ap.add_argument("--out", type=Path, default=None, help="write JSON result here")
    args = ap.parse_args()

    # Interface names / IPs get interpolated into remote shell command
    # strings below - quote them so a stray space/quote/semicolon in an
    # argument can't execute as additional remote root commands.
    box_iface_q = shlex.quote(args.box_iface)
    box_pcap_remote = f"/root/txdiag-{args.port}.pcap"
    box_pcap_remote_q = shlex.quote(box_pcap_remote)
    # #181: `pkill -f <pattern>` matches every process's FULL cmdline,
    # including the remote shell (`bash -c "<this whole ssh command>"`)
    # that's running the pkill itself - since that shell's own argv
    # literally contains the pattern string, an unbracketed pkill -f
    # self-matches and kills its own session before the real target ever
    # runs (100% reproducible, confirmed live). Bracket one character of
    # the tcpdump pattern so it still matches the real tcpdump's argv but
    # no longer matches this pkill command's own argv containing the
    # pattern literally; the iperf3 kill instead matches on process name
    # only (`-x`, no `-f`), sidestepping the same class of bug entirely.
    tcpdump_pkill_pat = f"tcpdump.*[t]xdiag-{args.port}\\.pcap"

    scratch = Path("tmp")
    scratch.mkdir(exist_ok=True)
    box_pcap_local = scratch / f"txdiag-box-{args.port}.pcap"
    host_pcap_local = scratch / f"txdiag-host-{args.port}.pcap"

    log(
        f"=== eth-tx-capture-diag: direction={args.direction} box={args.box_ip}/{args.box_iface} "
        f"local={args.bind_ip}/{args.bind_iface} port={args.port} streams={args.streams} "
        f"duration={args.duration}s ==="
    )

    # Confirm the box will actually route this connection's replies out the
    # expected interface - #170: a multi-homed box can silently egress via a
    # different NIC than the one under test.
    rc, out = run_remote(
        args.box_ip,
        args.password,
        f"ip route get {shlex.quote(args.bind_ip)} from {shlex.quote(args.box_ip)}",
    )
    if args.box_iface not in out:
        log(
            f"WARNING: box route for {args.bind_ip} from {args.box_ip} does NOT go via "
            f"{args.box_iface} - result: {out.strip()!r}. This test may not exercise the "
            f"interface you think it does (see #170). Add a policy route first if needed."
        )

    # Kill any stale server/capture from a prior interrupted run of THIS
    # port specifically (not a blind "pkill tcpdump" that would hit other
    # engineers' unrelated captures on the box).
    run_remote(
        args.box_ip,
        args.password,
        f"pkill -x iperf3 2>/dev/null; "
        f"pkill -f {shlex.quote(tcpdump_pkill_pat)} 2>/dev/null; true",
    )

    rc, out = run_remote(
        args.box_ip,
        args.password,
        f"nohup iperf3 -s -p {args.port} > /root/txdiag-iperf-{args.port}.log 2>&1 "
        f"& disown; echo started",
    )
    log(f"iperf3 server launch: {out.strip()}")
    if not poll_remote_listening(args.box_ip, args.password, args.port):
        log(f"ERROR: iperf3 server never bound to port {args.port} within 10s")
        flush_log()
        return 1

    local_tcpdump = None
    remote_capture_started = False
    try:
        if not args.no_capture:
            rc, out = run_remote(
                args.box_ip,
                args.password,
                f"nohup tcpdump -i {box_iface_q} -w {box_pcap_remote_q} -s 128 "
                f"'tcp port {args.port}' > /root/txdiag-tcpdump-{args.port}.log 2>&1 & disown; echo started",
            )
            log(f"box tcpdump: {out.strip()}")
            remote_capture_started = rc == 0 and "started" in out
            local_tcpdump = subprocess.Popen(
                [
                    "sudo",
                    "-n",
                    "tcpdump",
                    "-i",
                    args.bind_iface,
                    "-w",
                    str(host_pcap_local),
                    "-s",
                    "128",
                    f"tcp port {args.port}",
                ],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            time.sleep(2)  # tcpdump's own startup - no readiness signal to poll for

        subprocess.run(
            ["sudo", "-n", "ip", "neigh", "flush", "dev", args.bind_iface],
            capture_output=True,
            timeout=10,
        )

        iperf_cmd = [
            "iperf3",
            "-c",
            args.box_ip,
            "-B",
            f"{args.bind_ip}%{args.bind_iface}",
            "-p",
            str(args.port),
            "-t",
            str(args.duration),
            "-P",
            str(args.streams),
        ]
        if args.direction == "tx":
            iperf_cmd.append("-R")
        log(f"running: {' '.join(iperf_cmd)}")
        try:
            p = subprocess.run(
                iperf_cmd, capture_output=True, text=True, timeout=args.duration + 30
            )
        except subprocess.TimeoutExpired:
            log(f"ERROR: iperf3 client hung past {args.duration + 30}s")
            flush_log()
            return 1
        print(p.stdout)
        if p.returncode != 0:
            log(f"iperf3 FAILED rc={p.returncode}: {p.stderr[:500]}")
    finally:
        # Always attempt cleanup, even if iperf3/capture raised above -
        # this is exactly the gap that left orphaned tcpdump/iperf3
        # processes in the version of this script without a finally block.
        if local_tcpdump:
            subprocess.run(
                ["sudo", "-n", "kill", str(local_tcpdump.pid)], capture_output=True
            )
            try:
                local_tcpdump.wait(timeout=10)
            except subprocess.TimeoutExpired:
                subprocess.run(
                    ["sudo", "-n", "kill", "-9", str(local_tcpdump.pid)],
                    capture_output=True,
                )
        if remote_capture_started:
            run_remote(
                args.box_ip,
                args.password,
                f"pkill -f {shlex.quote(tcpdump_pkill_pat)} 2>/dev/null; true",
                timeout=10,
            )

    result: dict = {
        "direction": args.direction,
        "box_ip": args.box_ip,
        "box_iface": args.box_iface,
        "bind_ip": args.bind_ip,
        "bind_iface": args.bind_iface,
        "streams": args.streams,
        "duration": args.duration,
        "iperf_rc": p.returncode,
        "iperf_stdout_tail": p.stdout[-2000:],
    }
    capture_ok = True

    if not args.no_capture and remote_capture_started:
        rc, scp_out = scp_pull(
            args.box_ip, args.password, box_pcap_remote, box_pcap_local
        )
        if rc != 0 or not box_pcap_local.exists():
            log(f"WARNING: failed to pull box pcap (rc={rc}): {scp_out.strip()}")
            capture_ok = False
        else:
            flags = _capture_analysis(box_pcap_local, args.box_ip, args.bind_ip)
            result["box_capture"] = flags
            log(f"box-side capture TCP analysis: {flags}")
            if flags["packets_from_box"] == 0 and args.direction == "tx":
                log(
                    f"WARNING: zero packets captured FROM the box on {args.box_iface} despite "
                    f"testing box->host - matches #170's wrong-egress-NIC signature. Check the "
                    f"routing warning above."
                )

        if host_pcap_local.exists():
            result["host_capture"] = _capture_analysis(
                host_pcap_local, args.box_ip, args.bind_ip
            )
            log(f"host-side capture TCP analysis: {result['host_capture']}")
        else:
            log("WARNING: no host-side pcap produced")
            capture_ok = False

    if args.out:
        args.out.write_text(json.dumps(result, indent=2))
        log(f"result written to {args.out}")

    flush_log()
    return 0 if (p.returncode == 0 and capture_ok) else 1


if __name__ == "__main__":
    sys.exit(main())
