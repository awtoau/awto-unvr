#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Test whether i2c-sda-hold-time-ns actually stops the ch0 SDA-hold wedge (#86).

The claim in dts/alpine-v2-ubnt-unvr-ea16.dts:331 is that restoring
`i2c-sda-hold-time-ns = <300>` (commit 5338bdc) fixes the wedge: without it the
DW driver reuses a leftover DW_IC_SDA_HOLD value too short for the timing-fussy
s35390a, which then holds SDA and takes the whole bus down.

That has never been tested. On 2026-09-04 an `i2cdetect -y` sweep across mux ch0
wedged the bus anyway - with the property present and active in the running DT.
So either the fix is insufficient, or the wedge has a second trigger.

**This test deliberately provokes the wedge.** It is safe in that a reboot always
clears it, but while wedged the box loses:
  - adt7475 reads: fan control and thermal monitoring go blind
  - SFP soft-status polling (module presence/LOS/TX-fault)
  - any i2c access blocks its caller
Networking, SSH and the disks are unaffected.

The key insight from the incident: `i2cdetect` exiting 0 proves NOTHING. It only
means the scan finished. The wedge shows up on the NEXT access, so the test must
poke the bus again afterwards and count errors.

Run: ./dev.py ssh -- '/root/i2c-wedge-test.py'          # ch0, the RTC channel
     ./dev.py ssh -- '/root/i2c-wedge-test.py --dry-run' # report state, poke nothing
"""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path

# The s35390a sits at 0x30 on mux channel 0. Bus numbering shifts with probe
# order, so resolve by name rather than assuming i2c-N.
RTC_ADDR = 0x30
DT_PROP = Path("/proc/device-tree/soc/i2c@fd880000/i2c-sda-hold-time-ns")


def sh(cmd: str) -> str:
    return subprocess.run(
        ["sh", "-c", cmd], capture_output=True, text=True, check=False
    ).stdout


def timeout_count() -> int:
    """`controller timed out` lines in dmesg - the wedge's signature."""
    return int(sh("dmesg | grep -c 'controller timed out'").strip() or 0)


def mux_bus() -> str | None:
    """The i2c bus number for mux channel 0, by adapter name."""
    for line in sh("i2cdetect -l").splitlines():
        if "chan_id 0" in line or "i2c-0-mux (chan_id 0)" in line:
            return line.split()[0].replace("i2c-", "")
    return None


def probe_bus_health(bus: str) -> tuple[bool, str]:
    """Read a KNOWN-GOOD device to prove the bus still works.

    The adt7475 (0x2e, mux ch3) is tolerant and is what fan control uses, so a
    failure here is exactly the operational consequence we care about.
    """
    # Read a VALUE, not just the adapter banner - `sensors` prints the chip
    # name even when every read behind it fails, so the first lines prove
    # nothing. A wedged bus yields no temperature at all.
    out = sh("sensors 2>&1")
    ok = "°C" in out
    temp = next((ln.strip() for ln in out.splitlines() if "°C" in ln), "")
    return ok, temp or (out.strip().splitlines() or ["(no output)"])[0]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--dry-run", action="store_true", help="report state, do not poke the bus"
    )
    a = ap.parse_args()

    have_prop = DT_PROP.exists()
    val = ""
    if have_prop:
        raw = DT_PROP.read_bytes()
        val = str(int.from_bytes(raw[:4], "big")) if len(raw) >= 4 else "?"
    print(
        f"i2c-sda-hold-time-ns in the RUNNING DT: {'yes, ' + val + ' ns' if have_prop else 'NO'}"
    )
    if not have_prop:
        print("  -> the fix is not active; a wedge here proves nothing about it")

    bus = mux_bus()
    print(f"mux channel 0 is i2c-{bus}" if bus else "mux channel 0: not found")

    before = timeout_count()
    ok, line = probe_bus_health(bus or "")
    print(f"before: {before} timeouts, bus {'OK' if ok else 'ALREADY WEDGED'} ({line})")
    if not ok:
        print("bus is already wedged - reboot before testing")
        return 2
    if a.dry_run or bus is None:
        print("dry-run: not poking the bus")
        return 0

    print(f"\nprobing 0x{RTC_ADDR:02x} on i2c-{bus} (this is the provocation) ...")
    out = sh(f"i2cget -y {bus} 0x{RTC_ADDR:02x} 2>&1")
    print(f"  i2cget said: {out.strip() or '(no output)'}")

    # The whole point: exit status of the probe is not the result. Re-access the
    # bus and count errors, because the wedge only surfaces on the NEXT access.
    ok, line = probe_bus_health(bus)
    after = timeout_count()
    print(
        f"\nafter: {after} timeouts (+{after - before}), bus {'OK' if ok else 'WEDGED'}"
    )
    print(f"  {line}")

    if ok and after == before:
        print("\nRESULT: no wedge. The sda-hold fix appears to hold for a single read.")
        return 0
    print(
        "\nRESULT: WEDGED. i2c-sda-hold-time-ns is NOT sufficient (#86, #232).\n"
        "Recover with a reboot; nothing in software clears a held SDA on this bus."
    )
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
