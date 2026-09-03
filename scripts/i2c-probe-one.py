#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Probe ONE i2c address on ONE bus. Runs ON the box.

Why not `i2cdetect`: a bus scan walks every address, and on this board the
s35390a RTC on mux channel 0 wedges the pld bus when poked, with **no working
recovery** (#86, [[i2c-rtc-sda-hold]]). So sweeping is not acceptable here - this
does exactly one addressed transaction against exactly one address.

Primary use: #202, is a BR410 retimer physically present? The stock board-cfg
declares one at `i2c-bus = <1>` (mux channel 1), `i2c-addr = <0x56>`, but with
`exist = "disabled"`, and that same DT is demonstrably wrong about this port
(#200). Bus numbering on the box: i2c-0 = root DesignWare adapter, i2c-1..4 =
TCA9546A mux channels 0..3, so vendor channel 1 is Linux **i2c-2**.

Probing method matters. A zero-length write is the gentlest addressed
transaction (it asserts the address and looks for ACK, transferring no data),
which is what i2cdetect uses for most addresses. A read is used instead for
address ranges where a bare write could latch state.

Usage (on the box):
    ./i2c-probe-one.py --bus 2 --addr 0x56
    ./i2c-probe-one.py --bus 2 --addr 0x56 --read 1
"""

from __future__ import annotations

import argparse
import fcntl
import sys

I2C_SLAVE = 0x0703
I2C_SLAVE_FORCE = 0x0706


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--bus", type=int, required=True, help="i2c bus number (/dev/i2c-N)"
    )
    ap.add_argument("--addr", help="7-bit address, e.g. 0x56")
    ap.add_argument(
        "--scan",
        action="store_true",
        help="probe every address 0x08-0x77 on --bus, reporting each that ACKs. "
        "NEVER point this at the s35390a's mux channel (#86) - one addressed "
        "read at a chosen address is what --addr is for.",
    )
    ap.add_argument(
        "--skip",
        default="",
        help="comma-separated hex addresses to leave untouched during --scan",
    )
    ap.add_argument(
        "--read",
        type=int,
        default=0,
        help="read this many bytes instead of a zero-length write probe",
    )
    ap.add_argument(
        "--force",
        action="store_true",
        help="use I2C_SLAVE_FORCE (address already claimed by a kernel driver)",
    )
    args = ap.parse_args()

    dev = f"/dev/i2c-{args.bus}"

    if args.scan:
        skip = {int(x, 16) for x in args.skip.split(",") if x.strip()}
        # 0x00-0x07 and 0x78-0x7f are reserved by the i2c spec - never addressed.
        hits = []
        for a in range(0x08, 0x78):
            if a in skip:
                print(f"  0x{a:02x} skipped")
                continue
            try:
                with open(dev, "r+b", buffering=0) as f:
                    fcntl.ioctl(f, I2C_SLAVE, a)
                    f.read(1)
                    hits.append(a)
                    print(f"  0x{a:02x} ACK")
            except OSError as exc:
                # EBUSY = a kernel driver owns it, which is still a device.
                if getattr(exc, "errno", 0) == 16:
                    hits.append(a)
                    print(f"  0x{a:02x} ACK (claimed by a kernel driver)")
        print(f"{dev}: {len(hits)} device(s): " + " ".join(f"0x{a:02x}" for a in hits))
        return 0

    if not args.addr:
        print("need --addr, or --scan")
        return 2
    addr = int(args.addr, 0)

    try:
        fd = open(dev, "r+b", buffering=0)
    except OSError as exc:
        print(f"FAIL: cannot open {dev}: {exc}")
        return 1

    with fd:
        try:
            fcntl.ioctl(fd, I2C_SLAVE_FORCE if args.force else I2C_SLAVE, addr)
        except OSError as exc:
            print(f"FAIL: cannot address 0x{addr:02x} on {dev}: {exc}")
            print("  (EBUSY means a kernel driver already holds it - that is a HIT)")
            return 1

        try:
            if args.read:
                data = fd.read(args.read)
                print(
                    f"ACK 0x{addr:02x} on {dev} - read {len(data)} bytes: {data.hex()}"
                )
            else:
                fd.write(b"")
                print(f"ACK 0x{addr:02x} on {dev} - device PRESENT")
        except OSError as exc:
            print(f"NAK 0x{addr:02x} on {dev}: {exc} - no device at this address")
            return 2

    return 0


if __name__ == "__main__":
    sys.exit(main())
