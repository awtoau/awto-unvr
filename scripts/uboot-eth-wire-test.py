#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
"""#234 discriminator: does awto-uboot's al_eth TX actually reach the wire?

Runs tcpdump on this dev host (same L2 segment as woomera) filtered to the
box's two al_eth MACs, while driving a ping/tftp from the awto-nas# prompt.

  packets captured -> TX works; the bug is RX or completion-visibility
  nothing captured  -> TX never leaves the MAC/PCS

Box must already be at awto-nas#. Log: tmp/logs/uboot-eth-wire-test.log
"""

import argparse
import pathlib
import subprocess
import sys
import time

REPO = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
import _console  # noqa: E402
from _box import MAC_1G, MAC_10G  # noqa: E402
from _net import UNVR_IPADDR, detect_server_ip  # noqa: E402

LOG = REPO / "tmp" / "logs" / "uboot-eth-wire-test.log"


def logw(msg):
    line = f"{time.strftime('%Y-%m-%dT%H:%M:%S%z')}  {msg}"
    print(line, flush=True)
    with LOG.open("a") as fh:
        fh.write(line + "\n")


def console_cmd(sock, text, settle_s):
    """Send one U-Boot command. settle_s: how long to read output for.

    Bound source: U-Boot's own ARP retry is 5s x 4 = 20s before it gives up
    (net/arp.c ARP_TIMEOUT default), so a ping that fails takes ~20s. 1.25x
    of that is 25s; the caller passes the actual per-command need.
    On expiry: return whatever was read; the caller judges it.
    """
    sock.sendall(text.encode() + b"\r")
    buf = b""
    end = time.monotonic() + settle_s
    while time.monotonic() < end:
        try:
            chunk = sock.recv(4096)
        except TimeoutError:
            continue
        if not chunk:
            break
        buf += chunk
    return buf.decode(errors="replace")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--iface", default="enp7s0", help="dev-host capture interface")
    ap.add_argument("--port", choices=["1g", "10g"], default="1g")
    ap.add_argument(
        "--peer",
        default=None,
        help="ping target (this host; default: detected, _net.detect_server_ip)",
    )
    ap.add_argument("--ipaddr", default=UNVR_IPADDR, help="box IP in U-Boot (#252)")
    args = ap.parse_args()
    if args.peer is None:
        args.peer = detect_server_ip()

    LOG.parent.mkdir(parents=True, exist_ok=True)
    mac = MAC_1G if args.port == "1g" else MAC_10G
    ethact = "eth0" if args.port == "1g" else "eth1"
    logw(
        f"#234 wire test: port={args.port} mac={mac} ethact={ethact} iface={args.iface}"
    )

    pcap = REPO / "tmp" / f"uboot-eth-{args.port}.pcap"
    # -c 50: an ARP burst is 4 packets; 50 is ample and bounds the capture so
    # tcpdump exits on its own if traffic does flow. We kill it regardless.
    cap = subprocess.Popen(
        [
            "sudo",
            "-n",
            "tcpdump",
            "-i",
            args.iface,
            "-n",
            "-e",
            "-c",
            "50",
            "-w",
            str(pcap),
            "ether",
            "host",
            mac,
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        text=True,
    )
    # tcpdump needs its socket bound before the box sends. Poll for the pcap
    # file to appear rather than sleeping: expected <100ms, bound 5s (50x, a
    # generous margin on a local process start). On expiry: proceed anyway and
    # say so - a missed early packet is visible as an empty capture.
    end = time.monotonic() + 5.0
    while not pcap.exists() and time.monotonic() < end:
        pass
    if not pcap.exists():
        logw("WARN: tcpdump did not create the pcap within 5s; capturing anyway")

    sock = _console.connect()
    try:
        out = console_cmd(sock, f"setenv ipaddr {args.ipaddr}", 2)
        out += console_cmd(sock, f"setenv ethact {ethact}", 2)
        out += console_cmd(sock, f"ping {args.peer}", 30)
    finally:
        sock.close()
    logw("--- console ---\n" + out)

    cap.terminate()
    try:
        cap.wait(timeout=5)
    except subprocess.TimeoutExpired:
        cap.kill()
    logw("tcpdump stderr: " + (cap.stderr.read() if cap.stderr else ""))

    rd = subprocess.run(
        ["sudo", "-n", "tcpdump", "-n", "-e", "-r", str(pcap)],
        capture_output=True,
        text=True,
    )
    logw("--- wire ---\n" + rd.stdout + rd.stderr)
    n = len([ln for ln in rd.stdout.splitlines() if ln.strip()])
    logw(
        f"VERDICT: {n} frame(s) from {mac} on the wire -> "
        + ("TX REACHES THE WIRE" if n else "TX NEVER LEAVES THE MAC")
    )


if __name__ == "__main__":
    main()
