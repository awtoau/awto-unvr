#!/usr/bin/env python3
"""Read the SFP+ module's SFF-8472 DDM block (0x51, bytes 96-113) over SSH.

Read-only i2c dump - answers "is the optic actually transmitting" without
touching U-Boot's SerDes path (#111). Reports temp/Vcc/TX bias/TX power/RX
power plus the live status byte (TX_DISABLE, TX_FAULT, RX_LOS bits) so a
dark link can be split into "module isn't lasing" vs "module is fine, host
SerDes isn't training it" before further work on #111.

  ./scripts/sfp-ddm.py
"""

from __future__ import annotations

import argparse
import datetime
import re
import subprocess
import sys
from pathlib import Path

# Documented default (docs/fedora-on-ssd.md) - not a secret, this box's own lab
# root password. Override with --password if it's been changed.
DEFAULT_ROOT_PASSWORD = "unvr"

LOG = Path("tmp/logs/sfp-ddm.log")

# mux ch1 = Linux i2c-2 per docs/i2c-map.md; DDM lives at 0x51 on that bus.
DDM_BUS = 2
DDM_ADDR = 0x51

REMOTE_SCRIPT = f"""
set -e
modprobe i2c-dev 2>/dev/null || true
i2cdump -y {DDM_BUS} {DDM_ADDR:#x} b
"""


def locate_woomera() -> str:
    out = subprocess.run(
        [sys.executable, "scripts/ssh-woomera.py", "--print"],
        capture_output=True,
        text=True,
        check=True,
    )
    return out.stdout.strip()


def parse_i2cdump(text: str) -> dict[int, int] | None:
    """i2cdump -y <bus> <addr> b prints 16 bytes/row as "00: xx xx ... xx"."""
    bytes_by_offset: dict[int, int] = {}
    for line in text.splitlines():
        m = re.match(r"^([0-9a-f]+):((?:\s+[0-9a-fA-Fx]+)+)", line.strip())
        if not m:
            continue
        row_start = int(m.group(1), 16)
        vals = m.group(2).split()
        for i, v in enumerate(vals):
            if v in ("XX", "xx"):
                continue
            try:
                bytes_by_offset[row_start + i] = int(v, 16)
            except ValueError:
                continue
    return bytes_by_offset or None


def u16(b: dict[int, int], off: int) -> int | None:
    if off not in b or off + 1 not in b:
        return None
    return (b[off] << 8) | b[off + 1]


def s16(b: dict[int, int], off: int) -> int | None:
    v = u16(b, off)
    if v is None:
        return None
    return v - 0x10000 if v & 0x8000 else v


def mw_to_dbm(mw: float) -> float:
    import math

    return -40.0 if mw <= 0 else 10 * math.log10(mw)


def report(b: dict[int, int]) -> str:
    lines = []
    t = s16(b, 96)
    if t is not None:
        lines.append(f"  temp:      {t / 256:.1f} C")
    vcc = u16(b, 98)
    if vcc is not None:
        lines.append(f"  Vcc:       {vcc * 100 / 1e6:.3f} V")
    bias = u16(b, 100)
    if bias is not None:
        lines.append(f"  TX bias:   {bias * 2 / 1000:.2f} mA")
    txp = u16(b, 102)
    if txp is not None:
        mw = txp * 0.1 / 1000
        lines.append(f"  TX power:  {mw:.3f} mW ({mw_to_dbm(mw):.2f} dBm)")
    rxp = u16(b, 104)
    if rxp is not None:
        mw = rxp * 0.1 / 1000
        lines.append(f"  RX power:  {mw:.3f} mW ({mw_to_dbm(mw):.2f} dBm)")
    status = b.get(110)
    if status is not None:
        bits = {
            7: "TX_DISABLE(input pin state)",
            6: "SOFT_TX_DISABLE",
            2: "TX_FAULT",
            1: "RX_LOS",
            0: "DATA_READY_BAR",
        }
        set_bits = [name for bit, name in bits.items() if status & (1 << bit)]
        lines.append(
            f"  status:    0x{status:02x} -> {', '.join(set_bits) or '(clear)'}"
        )
    if not lines:
        lines.append("  (no recognized DDM bytes in dump - see raw output above)")
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--host", help="woomera's address (default: auto-locate by MAC OUI)"
    )
    ap.add_argument("--password", default=DEFAULT_ROOT_PASSWORD)
    args = ap.parse_args()

    host = args.host or locate_woomera()
    print(f"# woomera at {host}", file=sys.stderr)

    result = subprocess.run(
        [
            "sshpass",
            "-p",
            args.password,
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
            REMOTE_SCRIPT,
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    output = result.stdout + result.stderr

    LOG.parent.mkdir(parents=True, exist_ok=True)
    stamp = datetime.datetime.now().astimezone().isoformat(timespec="seconds")
    with LOG.open("a") as f:
        f.write(f"\n=== {stamp} ===\n{output}\n")

    print(output)
    if result.returncode != 0:
        return result.returncode

    parsed = parse_i2cdump(result.stdout)
    if parsed is None:
        print("could not parse i2cdump output", file=sys.stderr)
        return 1
    print(report(parsed))
    return 0


if __name__ == "__main__":
    sys.exit(main())
