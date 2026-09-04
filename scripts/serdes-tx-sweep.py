#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Sweep the 10G SerDes TX equalisation knobs and measure TX throughput at each.

Why: #121's 10G TX failure is transmit-only, rate-independent (~2 Mbit/s even
when only 100 Mbit is offered), and shows a roughly CONSTANT FRACTIONAL loss
rate - retransmits scale with throughput. That is a signal-integrity
signature, not a driver-logic one, and it survived every driver-side fix
tried (TX interrupt moderation, MDIO errno contract, PCS reset).

The optic's own DDM readings are nominal (-2.47 dBm launch, -2.52 dBm
receive, no alarms), which rules out gross optical overload/underrun - but
AVERAGE power says nothing about extinction ratio. An under-driven modulator
produces correct average power with a closed eye, and `amp` currently sits at
1 = 952mVdiff-pkpk, the LOWEST supported swing.

These knobs only became functional today: al_eth_store_serdes_tx_param()
mutated a stack local and threw it away, and the setter that commits it,
al_eth_lm_static_parameters_override(), had ZERO call sites in the entire
tree. So no previous tuning attempt on this port ever changed anything -
treat any historical "we tried tuning TX" conclusion as void.

Field semantics (struct al_serdes_adv_tx_params, al_hal_serdes_interface.h):
  amp                 3-bit swing select: 1=952mV ... 7=1331mV diff-pkpk
  total_driver_units  size of the driver-unit pool (19 here)
  c_minus_1           units to the first PRE-cursor tap
  c_plus_1 / c_plus_2 units to the first / second POST-cursor taps
  slew_rate           0=31ps 1=33ps 2=68ps 3=170ps
Main-cursor units = total - (c_minus_1 + c_plus_1 + c_plus_2).

Values are written as hex (the store uses kstrtoul base 16).

Usage:
    ./dev.py serdes-tx-sweep                  # sweep amp 1..7 (default)
    ./dev.py serdes-tx-sweep --param c_plus_1 --values 0,1,2,3,4
    ./dev.py serdes-tx-sweep --duration 8
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
import _console
from _net import detect_server_ip

# 10G port: enp0s2, PCI 0000:00:02.0
SYSFS_DIR = "/sys/devices/platform/soc/fbc00000.pci/pci0000:00/0000:00:02.0"
IFACE = "10G"
BOX_IFACE = "enp0s2"
IPERF_PORT = 5701
TX_PARAMS = (
    "amp",
    "total_driver_units",
    "c_minus_1",
    "c_plus_1",
    "c_plus_2",
    "slew_rate",
)
# Link needs to re-establish for an override to be applied - the params are
# consumed by al_eth_serdes_static_tx_params_set() during link setup, not
# applied to a live link. 6s covers observed 10G re-link time with margin.
LINK_SETTLE_S = 6

# Receiver-side physical-layer error counters on the dev host's NIC. Names vary
# by driver; mlx4_en exposes rx_crc_errors, mlx5 adds rx_symbol_err_phy and
# rx_corrected_bits_phy. Unknown names are simply absent and contribute nothing.
FAR_END_ERROR_COUNTERS = {
    "rx_crc_errors",
    "rx_errors",
    "rx_frame_errors",
    "rx_length_errors",
    "rx_over_errors",
    "rx_symbol_err_phy",
    "rx_crc_errors_phy",
    "rx_corrected_bits_phy",
    "rx_err_lane_0_phy",
}


def sh(s, cmd, timeout=30):
    return _console.sh(s, cmd, timeout=timeout)[1]


def marked(s, cmd, timeout=30):
    """Run cmd and extract its output between markers - the serial console
    echoes the command itself and interleaves kernel messages, so a bare
    read of the transcript is not parseable."""
    out = sh(s, f"echo MK$({cmd})MK", timeout=timeout)
    m = re.search(r"MK(.*?)MK", out, re.DOTALL)
    return m.group(1).strip() if m else ""


def box_iface_ip(s, iface: str) -> str:
    """The box's own IPv4 on `iface`, read live over the console. iperf3 is
    bound to it (-B ip%iface, #121); a lease baked in here went stale."""
    out = marked(
        s,
        f"ip -4 -o addr show dev {iface} | awk '{{print $4}}' | cut -d/ -f1 | head -n1",
    )
    if not re.fullmatch(r"\d+\.\d+\.\d+\.\d+", out):
        raise SystemExit(f"FATAL: no IPv4 address on {iface} on the box: {out!r}")
    return out


def read_params(s) -> dict[str, str]:
    vals = {}
    for p in TX_PARAMS:
        v = marked(s, f"cat {SYSFS_DIR}/serdes_tx_{p} 2>/dev/null")
        vals[p] = v.strip()
    return vals


def far_end_rx_errors(iface: str) -> int | None:
    """Sum this host's physical-layer RX error counters - a CONTROL, not the metric.

    Topology is box -> TP-Link switch -> this host, so these counters measure the
    SECOND hop (switch -> host), not the link being tuned. They cannot see the
    box's transmit quality: a store-and-forward switch DROPS bad-CRC frames
    rather than forwarding them, so corruption on hop 1 arrives here as missing
    frames, not as errors.

    Their job is to prove hop 2 is clean, so UDP loss can be attributed to hop 1.
    Non-zero here invalidates the measurement rather than informing it.

    Returns None if no counter could be read - a missing counter must not read as
    "zero errors".
    """
    try:
        out = subprocess.run(
            ["ethtool", "-S", iface],
            capture_output=True,
            text=True,
            timeout=15,
            check=False,
        ).stdout
    except (OSError, subprocess.SubprocessError):
        return None

    total = 0
    seen = False
    for line in out.splitlines():
        m = re.match(r"\s*(\S+):\s+(\d+)\s*$", line)
        if not m:
            continue
        name, val = m.group(1), int(m.group(2))
        if name in FAR_END_ERROR_COUNTERS:
            total += val
            seen = True
    return total if seen else None


def measure_udp_loss(
    s, server_ip: str, box_ip: str, duration: int, mbit: int
) -> tuple[float, int]:
    """Offer a FIXED UDP rate and return (loss_percent, datagrams_lost).

    Better than throughput as a tuning metric. TCP throughput saturates at line
    rate, so c_plus_1 4/5/6/7 all read 9.42 Gbit/s and cannot be ranked. Offering
    a fixed rate well below line rate means the link is never the bottleneck, so
    any loss is corruption rather than congestion - and loss scales with the bit
    error rate instead of clipping.

    Still indirect: it counts frames the switch discarded for bad CRC, not the
    eye itself. It ranks settings; it does not characterise them (#207).
    """
    out = sh(
        s,
        f"iperf3 -c {server_ip} -p {IPERF_PORT} -u -b {mbit}M -t {duration} "
        f"-B {box_ip}%{BOX_IFACE} 2>&1 | grep -E '%\\)'",
        timeout=duration + 35,
    )
    m = re.search(r"(\d+)/\s*(\d+)\s+\(([\d.]+)%\)", out)
    if not m:
        return (-1.0, -1)
    return (float(m.group(3)), int(m.group(1)))


def measure_tx(s, server_ip: str, box_ip: str, duration: int) -> tuple[float, int]:
    """Returns (Mbit/s, retransmits). SO_BINDTODEVICE-bound per #121's own
    wrong-NIC caveat."""
    out = sh(
        s,
        f"iperf3 -c {server_ip} -p {IPERF_PORT} -t {duration} -P 4 "
        f"-B {box_ip}%{BOX_IFACE} 2>&1 | grep -E 'SUM.*sender'",
        timeout=duration + 35,
    )
    m = re.search(r"([\d.]+)\s+([KMG])bits/sec\s+(\d+)", out)
    if not m:
        return (0.0, -1)
    val, unit, retr = float(m.group(1)), m.group(2), int(m.group(3))
    mbps = val * (1000 if unit == "G" else 0.001 if unit == "K" else 1)
    return (mbps, retr)


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--param", default="amp", choices=TX_PARAMS)
    ap.add_argument(
        "--values",
        default="1,2,3,4,5,6,7",
        help="comma-separated values to try (decimal; written as hex)",
    )
    ap.add_argument("--duration", type=int, default=6, help="iperf3 seconds per point")
    ap.add_argument(
        "--metric",
        default="tcp",
        choices=("tcp", "udp-loss"),
        help="tcp = throughput+retransmits (SATURATES at line rate, cannot rank "
        "settings that all reach 9.42 Gbit/s). udp-loss = fixed offered rate, "
        "report loss %% - does not saturate, so it ranks them.",
    )
    ap.add_argument(
        "--udp-mbit",
        type=int,
        default=2000,
        help="offered rate for --metric udp-loss, well under line rate so any "
        "loss is corruption rather than congestion",
    )
    ap.add_argument(
        "--control-iface",
        default="",
        help="local NIC to sanity-check: its RX errors must stay 0, proving the "
        "hop we are NOT tuning is clean (see far_end_rx_errors)",
    )
    args = ap.parse_args()

    values = [int(v.strip()) for v in args.values.split(",") if v.strip()]
    server_ip = detect_server_ip()
    print(f"dev-host iperf3 target: {server_ip}:{IPERF_PORT}")
    print(f"NOTE: start the server first:  iperf3 -s -p {IPERF_PORT}\n")

    s = _console.connect()
    try:
        _console.login(s)
        sh(s, "dmesg -n 1")
        box_ip = box_iface_ip(s, BOX_IFACE)
        print(f"box {BOX_IFACE} address (live): {box_ip}")

        original = read_params(s)
        print(f"original TX params: {original}\n")
        if not original.get(args.param):
            print(
                f"FATAL: could not read serdes_tx_{args.param} - is this the 10G port?"
            )
            return 1

        def measure(s):
            if args.metric == "udp-loss":
                loss, lost = measure_udp_loss(
                    s, server_ip, box_ip, args.duration, args.udp_mbit
                )
                return (loss, lost)
            return measure_tx(s, server_ip, box_ip, args.duration)

        ctrl0 = far_end_rx_errors(args.control_iface) if args.control_iface else None

        base_mbps, base_retr = measure(s)
        unit = "loss %" if args.metric == "udp-loss" else "Mbit/s"
        cnt = "lost" if args.metric == "udp-loss" else "retrans"
        print(f"{'value':>6}  {unit:>9}  {cnt:>8}   note")
        print(
            f"{original[args.param]:>6}  {base_mbps:>9.1f}  {base_retr:>8}   (baseline, unchanged)"
        )

        results = []
        for v in values:
            if str(v) == original[args.param]:
                continue
            sh(s, f"echo {v:x} > {SYSFS_DIR}/serdes_tx_{args.param}")
            # Bounce the link so the override is consumed at re-establishment.
            sh(s, f"ip link set {BOX_IFACE} down", timeout=20)
            sh(s, f"ip link set {BOX_IFACE} up", timeout=20)
            time.sleep(LINK_SETTLE_S)
            readback = marked(s, f"cat {SYSFS_DIR}/serdes_tx_{args.param}")
            mbps, retr = measure(s)
            note = "" if readback == str(v) else f"READBACK MISMATCH ({readback!r})"
            print(f"{v:>6}  {mbps:>9.1f}  {retr:>8}   {note}")
            results.append((v, mbps, retr))

        # Restore.
        sh(
            s,
            f"echo {int(original[args.param]):x} > {SYSFS_DIR}/serdes_tx_{args.param}",
        )
        sh(s, f"ip link set {BOX_IFACE} down", timeout=20)
        sh(s, f"ip link set {BOX_IFACE} up", timeout=20)
        time.sleep(LINK_SETTLE_S)
        print(f"\nrestored serdes_tx_{args.param}={original[args.param]}")

        if ctrl0 is not None:
            ctrl1 = far_end_rx_errors(args.control_iface)
            if ctrl1 is not None and ctrl1 != ctrl0:
                print(
                    f"WARNING: {args.control_iface} RX errors moved "
                    f"{ctrl0} -> {ctrl1}. The hop that should be clean is not, so "
                    f"these results are not attributable to the link under test."
                )

        if results:
            best = max(results, key=lambda r: r[1])
            if best[1] > base_mbps * 1.5:
                print(
                    f"BEST: {args.param}={best[0]} -> {best[1]:.1f} Mbit/s "
                    f"({best[1] / max(base_mbps, 0.001):.1f}x baseline)"
                )
            else:
                print(
                    f"No value beat the baseline by >1.5x - "
                    f"{args.param} is probably not the limiting factor."
                )
    finally:
        s.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
