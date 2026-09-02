#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""docs/uefi.md §5's dry chainload probe: tftp the P0 UNVR.fd into RAM,
CRC-verify it, then `go` to it - watch for the EDK2 banner / UEFI Shell
prompt on ttyS0.

RAM payload only, never touches flash (docs/uefi.md §1) - if EDK2 hangs
or crashes, a power-cycle always returns the box to its normal stock
boot, nothing is at risk beyond that. Entry EL is unconfirmed (docs/
uefi.md §6), so this genuinely might not work first try; that's exactly
what this step exists to find out.

Reuses the proven catch-uboot.py-race + power_cycle_verified +
ensure_tftpd/tftp_and_verify pattern (ram-boot-deploy.py), rather than a
new hand-rolled reimplementation.

Usage:
    ./dev.py uefi-chainload-probe [--fd PATH]
"""

from __future__ import annotations

import argparse
import importlib.util
import os
import subprocess
import sys
import zlib
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
from _net import detect_server_ip  # noqa: E402
from _power import power_cycle_verified  # noqa: E402

_rbd_spec = importlib.util.spec_from_file_location(
    "_ram_boot_deploy", REPO / "scripts/ram-boot-deploy.py")
_rbd = importlib.util.module_from_spec(_rbd_spec)
_rbd_spec.loader.exec_module(_rbd)

DEFAULT_FD = Path(
    "/mnt/2tb/unvr-port-refs/edk2/Build/UNVR/DEBUG_GCC/FV/UNVR.fd")
FD_ADDR = "0x20000000"
# EDK2's own banner text before the shell prompt (standard UEFI Shell
# startup banner) - "UEFI Shell" is present regardless of shell version.
# "Kernel panic"/"Synchronous Exception"/"Data Abort" catch a hard crash
# so the script doesn't just sit out the full timeout on those.
#
# Confirmed live (2026-09-02, first attempt ever): P0 reaches BDS's
# "Please select boot device" menu (Tianocore/EDK2 banner printed,
# "Press ESCAPE for boot options" seen) - NOT a direct Shell launch, so
# that's the real P0 success signal, not "UEFI Shell" text (which only
# appears after a boot option actually launches - it currently doesn't,
# see docs/uefi.md's HiiDatabase/BmDriverHealth note).
SUCCESS_PATTERN = "Please select boot device|UEFI Shell|Shell>"
CRASH_PATTERN = "Synchronous Exception|Data Abort|Instruction Abort|Kernel panic"
# No prior data point for "how long EDK2 P0 takes to reach a shell" -
# this is the FIRST real boot attempt. Budget generously (2x a normal
# Linux boot's ballpark) since a hang is recoverable (power-cycle) and a
# timeout that's too short would abort a genuinely-still-booting probe.
PROBE_TIMEOUT_S = 60


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                  formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--fd", type=Path, default=DEFAULT_FD)
    args = ap.parse_args()

    if not args.fd.exists():
        print(f"FATAL: {args.fd} missing - run ./dev.py build-uefi-p0 first")
        return 1
    local_crc = zlib.crc32(args.fd.read_bytes()) & 0xFFFFFFFF
    print(f"UNVR.fd: {args.fd} ({args.fd.stat().st_size} bytes, "
          f"crc32=0x{local_crc:08x})")

    _rbd.ensure_tftpd()
    server_ip = detect_server_ip()
    print(f"tftp server IP (auto-detected): {server_ip}")

    print("starting catch-uboot.py to win the autoboot race")
    catch = subprocess.Popen(
        [sys.executable, "scripts/catch-uboot.py", "--seconds", "60"], cwd=REPO)
    power_cycle_verified(log=print)
    try:
        rc = catch.wait(timeout=70)
    except subprocess.TimeoutExpired:
        catch.kill()
        print("FATAL: catch-uboot.py hung waiting for the U-Boot prompt")
        return 1
    if rc != 0:
        print("FATAL: catch-uboot.py did not reach the U-Boot prompt (autoboot won)")
        return 1
    print("U-Boot prompt reached, autoboot stopped\n")

    _rbd.run_devpy("--expect", "ALPINE_UBNT_NAS_ALL>", "--timeout", "8",
                    f"setenv ipaddr {_rbd.IPADDR}")
    _rbd.run_devpy("--expect", "ALPINE_UBNT_NAS_ALL>", "--timeout", "8",
                    f"setenv serverip {server_ip}")
    try:
        _rbd.tftp_and_verify(args.fd, FD_ADDR)
    except (RuntimeError, subprocess.TimeoutExpired) as e:
        print(f"FATAL: tftp of UNVR.fd failed: {e}")
        return 1

    out = _rbd.run_devpy("--expect", "ALPINE_UBNT_NAS_ALL>", "--timeout", "10",
                          f"crc32 {FD_ADDR} ${{filesize}}")
    if f"{local_crc:08x}".lower() not in out.lower():
        print(f"FATAL: on-device crc32 does not match local 0x{local_crc:08x} "
              f"- refusing to jump into a corrupted transfer\n{out}")
        return 1
    print(f"crc32 verified: 0x{local_crc:08x} matches\n")

    print(f"=== go {FD_ADDR} - transferring control to EDK2 SEC ===")
    p = subprocess.run(
        [sys.executable, "./dev.py", "console-send", "--expect",
         f"{SUCCESS_PATTERN}|{CRASH_PATTERN}", "--timeout", str(PROBE_TIMEOUT_S),
         f"go {FD_ADDR}"],
        cwd=REPO, capture_output=True, text=True, timeout=PROBE_TIMEOUT_S + 15,
    )
    out = p.stdout + p.stderr
    print(out)

    if any(pat in out for pat in CRASH_PATTERN.split("|")):
        print("\nRESULT: EDK2 crashed (exception/abort) before reaching the shell.")
        return 1
    if "<<MATCHED:" in out and any(pat in out for pat in SUCCESS_PATTERN.split("|")):
        print("\nRESULT: SUCCESS - reached the UEFI Shell prompt.")
        return 0
    print(f"\nRESULT: no response within {PROBE_TIMEOUT_S}s - likely hung. "
          "Power-cycle (./dev.py power-cycle) to return to normal boot; "
          "this never touched flash.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
