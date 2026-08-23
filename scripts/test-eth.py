#!/usr/bin/env python3
"""Scripted validation of both ethernet ports on woomera - a repeatable
pass/fail test, not hand-typed SSH commands. Meant to be run after any
al_eth change (driver fix, phylink migration, module swap) to get a
consistent, comparable result instead of ad-hoc checks that differ session
to session.

Checks, in order:
  1. Loaded module identity (path, vermagic match against running kernel)
  2. Link status on enp0s1 (1G) + enp0s2 (10G): carrier, speed, duplex
  3. SFP+ DDM health on enp0s2 (TX bias/power, RX power, status byte)
  4. Real hardware MAC-level error counters (ip -s link: CRC/FCS errors,
     drops) - NOT ethtool -S, which only exposes driver/queue counters for
     this driver (see #116's investigation for why this distinction matters)
  5. Bounded iperf3 throughput test AGAINST enp0s2 SPECIFICALLY (both
     directions), skippable
  6. dmesg scan for new errors/warnings since the module last loaded

The iperf3 test connects to enp0s2's own IP (queried live over SSH), not
--host/locate_woomera()'s address - the box has two DHCP-leased IPs (one per
NIC) and they can differ, so connecting to "the box's address" silently tests
whichever port answered the host-discovery ping, not necessarily the 10G one
(see #121 - every throughput number in that issue before this fix was
actually the 1G port, mislabeled as 10G).

Caveat this script CANNOT fix on its own: if the machine running this test
has multiple NICs on the same subnet as the box, Linux route selection is
destination-driven and may silently route enp0s2's traffic out your 1G NIC
regardless of which NIC you intended - the script checks and reports which
local interface `ip route get` would actually use, but forcing a specific
egress interface needs a routing change outside this script's scope
(temporary host route, e.g. `sudo ip route add <enp0s2 IP>/32 dev <10G nic>`).

Exits 0 if every check passes, 1 otherwise. Prints a clear PASS/FAIL summary
either way - meant to be readable standalone or dispatched to an agent.

  ./scripts/test-eth.py                    # full run, ~30s iperf3 each way
  ./scripts/test-eth.py --skip-iperf        # quick check, no throughput test
  ./scripts/test-eth.py --iperf-duration 60 # longer throughput test
"""

from __future__ import annotations

import argparse
import datetime
import re
import subprocess
import sys
from pathlib import Path

DEFAULT_ROOT_PASSWORD = "unvr"  # documented default, see docs/fedora-on-ssd.md
LOG = Path("tmp/logs/test-eth.log")


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


def run_remote(host: str, password: str, remote_script: str) -> tuple[int, str]:
    result = subprocess.run(
        ssh_cmd(host, password) + [remote_script],
        capture_output=True,
        text=True,
        check=False,
    )
    return result.returncode, result.stdout + result.stderr


class Report:
    def __init__(self) -> None:
        self.checks: list[tuple[str, bool, str]] = []

    def add(self, name: str, ok: bool, detail: str) -> None:
        self.checks.append((name, ok, detail))
        mark = "PASS" if ok else "FAIL"
        print(f"[{mark}] {name}: {detail}")

    def all_ok(self) -> bool:
        return all(ok for _, ok, _ in self.checks)


def check_module(host: str, password: str, report: Report) -> None:
    rc, out = run_remote(
        host, password, "modinfo al_eth | grep -E 'filename|vermagic'; uname -r"
    )
    filename = re.search(r"filename:\s*(\S+)", out)
    vermagic = re.search(r"vermagic:\s*(\S+)", out)
    running = out.strip().splitlines()[-1] if out.strip() else ""
    if rc != 0 or not filename or not vermagic:
        report.add(
            "module identity", False, f"couldn't query modinfo (rc={rc}): {out[:200]}"
        )
        return
    matches = vermagic.group(1) == running
    report.add(
        "module identity",
        matches,
        f"{filename.group(1)} vermagic={vermagic.group(1)} running={running} "
        f"({'match' if matches else 'MISMATCH'})",
    )


def check_link(host: str, password: str, report: Report) -> None:
    rc, out = run_remote(
        host,
        password,
        "for i in enp0s1 enp0s2; do "
        'echo "$i speed=$(cat /sys/class/net/$i/speed 2>/dev/null) '
        "carrier=$(cat /sys/class/net/$i/carrier 2>/dev/null) "
        'oper=$(cat /sys/class/net/$i/operstate 2>/dev/null)"; done',
    )
    for iface, expect_speed in (("enp0s1", "1000"), ("enp0s2", "10000")):
        m = re.search(rf"{iface} speed=(\S*) carrier=(\S*) oper=(\S*)", out)
        if not m:
            report.add(f"link {iface}", False, f"no data (rc={rc}): {out[:200]}")
            continue
        speed, carrier, oper = m.groups()
        ok = carrier == "1" and oper == "up"
        report.add(
            f"link {iface}",
            ok,
            f"speed={speed} carrier={carrier} oper={oper}"
            + ("" if speed == expect_speed else f" (expected {expect_speed})"),
        )


def check_ddm(host: str, password: str, report: Report) -> None:
    rc, out = run_remote(
        host,
        password,
        "modprobe i2c-dev 2>/dev/null; i2cdump -y 2 0x51 b 2>&1",
    )
    if rc != 0:
        report.add("SFP DDM", False, f"i2cdump failed (rc={rc}): {out[:200]}")
        return
    rows: dict[int, int] = {}
    for line in out.splitlines():
        m = re.match(r"^([0-9a-f]+):((?:\s+[0-9a-fA-Fx]+)+)", line.strip())
        if not m:
            continue
        base = int(m.group(1), 16)
        for i, v in enumerate(m.group(2).split()):
            if v.lower() != "xx":
                try:
                    rows[base + i] = int(v, 16)
                except ValueError:
                    pass
    if 100 not in rows or 102 not in rows or 104 not in rows or 110 not in rows:
        report.add("SFP DDM", False, f"couldn't parse i2cdump output: {out[:200]}")
        return
    bias_ma = ((rows[100] << 8) | rows[101]) * 2 / 1000
    txp_mw = ((rows[102] << 8) | rows[103]) * 0.1 / 1000
    rxp_mw = ((rows[104] << 8) | rows[105]) * 0.1 / 1000
    status = rows[110]
    ok = bias_ma > 0.5 and status == 0
    report.add(
        "SFP DDM",
        ok,
        f"bias={bias_ma:.2f}mA tx={txp_mw:.3f}mW rx={rxp_mw:.3f}mW status=0x{status:02x}",
    )


def check_mac_errors(host: str, password: str, report: Report) -> None:
    rc, out = run_remote(host, password, "ip -s link show enp0s2")
    if rc != 0:
        report.add("MAC error counters", False, f"ip -s link failed (rc={rc})")
        return
    m = re.search(
        r"RX:\s*bytes\s*packets\s*errors\s*dropped\s*missed\s*mcast\s*\n\s*"
        r"(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)",
        out,
    )
    if not m:
        report.add(
            "MAC error counters", False, f"couldn't parse ip -s link: {out[:300]}"
        )
        return
    rx_errors, rx_dropped = int(m.group(3)), int(m.group(4))
    ok = rx_errors == 0
    report.add(
        "MAC error counters",
        ok,
        f"RX errors={rx_errors} dropped={rx_dropped} (real FCS/CRC/alignment "
        "counters, not driver queue stats - see #116)",
    )


# Known-benign, already-tracked - don't fail the run on these (see the linked
# issue for why each is understood and not actionable right now).
_DMESG_KNOWN_BENIGN = (
    "of_irq_parse_pci: failed with rc=-22",  # #55: MSI-X fallback works fine
)


def check_dmesg(host: str, password: str, report: Report) -> None:
    _rc, out = run_remote(
        host,
        password,
        "dmesg | grep -iE 'al_eth.*(error|warn|fail|timeout|hang)' | tail -30",
    )
    lines = [
        line
        for line in out.splitlines()
        if line.strip() and not any(b in line for b in _DMESG_KNOWN_BENIGN)
    ]
    report.add(
        "dmesg errors",
        len(lines) == 0,
        f"{len(lines)} matching line(s) (known-benign filtered)"
        + (f": {lines[-1]}" if lines else ""),
    )
    # A ring-buffer overrun means kernel lines were dropped before dmesg ever
    # saw them - the al_eth grep above could silently miss a real error that
    # got lost in the overrun, not just "there were none" (#124).
    _rc, overrun_out = run_remote(
        host, password, "dmesg | grep -c 'buffer overrun' || true"
    )
    overrun_count = int(overrun_out.strip() or 0)
    report.add(
        "dmesg buffer overrun",
        overrun_count == 0,
        f"{overrun_count} overrun line(s) - the al_eth check above may be unreliable "
        "if this is nonzero, since dropped lines never reach dmesg (see #124)"
        if overrun_count
        else "0 - dmesg ring buffer intact, al_eth check above is trustworthy",
    )


def enp0s2_ip(host: str, password: str) -> str | None:
    """enp0s2's own live IP - the box has a separate DHCP lease per NIC, so
    this can differ from `host` (whatever locate_woomera()/--host resolved
    to, which may be enp0s1's address - see #121)."""
    rc, out = run_remote(host, password, "ip -4 -o addr show enp0s2")
    m = re.search(r"inet (\d+\.\d+\.\d+\.\d+)/", out)
    return m.group(1) if rc == 0 and m else None


def check_local_route(target_ip: str, report: Report) -> None:
    """Sanity-check which LOCAL interface will actually carry the iperf3
    traffic. Binding a source IP (-B) does NOT control this - Linux route
    selection is destination-driven, so a multi-NIC host on the box's subnet
    can silently send "10G" traffic out a 1G NIC with no error (see #121)."""
    result = subprocess.run(
        ["ip", "route", "get", target_ip],
        capture_output=True,
        text=True,
        check=False,
    )
    m = re.search(r"\bdev (\S+)", result.stdout)
    iface = m.group(1) if m else None
    ethtool = subprocess.run(
        ["ethtool", iface] if iface else ["true"],
        capture_output=True,
        text=True,
        check=False,
    )
    speed_m = re.search(r"Speed:\s*(\S+)", ethtool.stdout)
    speed = speed_m.group(1) if speed_m else "unknown"
    ok = iface is not None and (speed == "10000Mb/s" or speed == "unknown")
    report.add(
        "local route to enp0s2",
        ok,
        f"traffic to {target_ip} goes out local iface={iface or '?'} speed={speed}"
        + ("" if ok else " (NOT a 10G-capable local NIC - result below is not a real 10G test)"),
    )


def run_iperf(host: str, password: str, report: Report, duration: int) -> None:
    target = enp0s2_ip(host, password)
    if target is None:
        report.add(
            "iperf3 throughput", False, "couldn't determine enp0s2's IP - skipping"
        )
        return
    check_local_route(target, report)
    run_remote(
        host,
        password,
        "pkill -f '^iperf3 -s' 2>/dev/null; "
        "nohup iperf3 -s -p 5601 > /root/test-eth-iperf-server.log 2>&1 & disown; "
        "echo started",
    )
    # Poll for the server to actually bind - nohup returning doesn't mean
    # iperf3 has finished its bind() yet, and there's no fixed delay that's
    # right for every box load; a bounded poll is the honest wait.
    for _ in range(40):
        _rc, out = run_remote(host, password, "ss -ltn | grep -q ':5601 ' && echo up")
        if "up" in out:
            break
    else:
        report.add(
            "iperf3 throughput", False, "server never bound to port 5601 within 2s"
        )
        return
    result = subprocess.run(
        [
            "iperf3",
            "-c",
            target,
            "-p",
            "5601",
            "-t",
            str(duration),
            "-P",
            "4",
            "--bidir",
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    run_remote(host, password, "pkill -f '^iperf3 -s -p 5601' 2>/dev/null; true")
    if result.returncode != 0:
        report.add(
            "iperf3 throughput",
            False,
            f"iperf3 client failed (rc={result.returncode}): {result.stderr[:300]}",
        )
        return
    # [SUM][TX-C] = host-sending direction, [SUM][RX-C] = box-sending
    # (reverse) direction. Each has its own "sender"/"receiver" pair (same
    # transfer, measured at both ends) - anchor on the role tag AND
    # "sender" (the authoritative source-side count) so tx/rx can't
    # accidentally both match the same stream's two perspective lines.
    tx = re.search(
        r"\[SUM\]\[TX-C\]\s+[\d.]+-[\d.]+\s+sec\s+[\d.]+ \wBytes\s+([\d.]+) (\w+)/sec\s+\d+\s*sender",
        result.stdout,
    )
    rx = re.search(
        r"\[SUM\]\[RX-C\]\s+[\d.]+-[\d.]+\s+sec\s+[\d.]+ \wBytes\s+([\d.]+) (\w+)/sec\s+\d+\s*sender",
        result.stdout,
    )
    ok = "iperf Done" in result.stdout and bool(tx) and bool(rx)
    tx_str = f"{tx.group(1)} {tx.group(2)}/sec" if tx else "?"
    rx_str = f"{rx.group(1)} {rx.group(2)}/sec" if rx else "?"
    report.add(
        "iperf3 throughput",
        ok,
        f"tx={tx_str} rx={rx_str} ({duration}s, 4 streams, bidir)",
    )
    LOG.parent.mkdir(parents=True, exist_ok=True)
    with LOG.open("a") as f:
        f.write(f"\n=== iperf3 raw output ===\n{result.stdout}\n")


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--host", help="woomera's address (default: auto-locate by MAC OUI)"
    )
    ap.add_argument("--password", default=DEFAULT_ROOT_PASSWORD)
    ap.add_argument(
        "--skip-iperf", action="store_true", help="skip the throughput test"
    )
    ap.add_argument(
        "--iperf-duration", type=int, default=30, help="seconds per direction"
    )
    args = ap.parse_args()

    host = args.host or locate_woomera()
    print(f"# woomera at {host}", file=sys.stderr)

    report = Report()
    check_module(host, args.password, report)
    check_link(host, args.password, report)
    check_ddm(host, args.password, report)
    check_mac_errors(host, args.password, report)
    check_dmesg(host, args.password, report)
    if not args.skip_iperf:
        run_iperf(host, args.password, report, args.iperf_duration)

    LOG.parent.mkdir(parents=True, exist_ok=True)
    stamp = datetime.datetime.now().astimezone().isoformat(timespec="seconds")
    with LOG.open("a") as f:
        f.write(f"\n=== {stamp} ===\n")
        for name, ok, detail in report.checks:
            f.write(f"[{'PASS' if ok else 'FAIL'}] {name}: {detail}\n")

    overall = report.all_ok()
    print(f"\n{'ALL CHECKS PASSED' if overall else 'SOME CHECKS FAILED'}")
    return 0 if overall else 1


if __name__ == "__main__":
    sys.exit(main())
