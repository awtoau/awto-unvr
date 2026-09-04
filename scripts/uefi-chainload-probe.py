#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""docs/uefi.md §5's dry chainload probe: tftp the P0 UNVR.fd into RAM,
CRC-verify it, `go` to it, then race the 's' hotkey to launch Boot0001
(UEFI Shell) directly - watch for the real Shell prompt on ttyS0.

RAM payload only, never touches flash (docs/uefi.md §1) - if EDK2 hangs
or crashes, a power-cycle always returns the box to its normal stock
boot, nothing is at risk beyond that.

Why the 's' hotkey race, not just watching the boot menu: confirmed live
2026-09-02 that Boot0000 (BootManagerMenuApp) is what BDS's automatic
phase tries first and succeeds at, landing on ITS OWN interactive menu -
whose 2 entries are a separate, generic device enumeration that can
never resolve to anything bootable (see docs/uefi.md). The real path to
Boot0001 (UEFI Shell) is the hotkey ArmPkg's PlatformBootManagerLib
registers for it (Key.UnicodeChar = 's'), live during BdsDxe's ~3s
BdsWait window (PcdPlatformBootTimeOut) - confirmed by spamming 's'
throughout early boot and catching "[Bds]Hotkey for Boot0001 pressed -
Success" in the trace. A single console-send round trip is too slow to
land inside that window (tftp+crc32 alone eat several seconds), so this
sends `go` directly over a raw socket and starts spamming 's'
immediately, rather than going through dev.py console-send's blocking
subprocess wrapper for that step.

Reuses the proven catch-uboot.py-race + power_cycle_verified +
ensure_tftpd/tftp_and_verify pattern (ram-boot-deploy.py), rather than a
new hand-rolled reimplementation, for everything up to the `go` jump.

Usage:
    ./dev.py uefi-chainload-probe [--fd PATH] [--no-hotkey]
"""

from __future__ import annotations

import argparse
import importlib.util
import os
import subprocess
import sys
import time
import zlib
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
import _console  # noqa: E402
from _net import detect_server_ip  # noqa: E402
from _power import power_cycle_verified  # noqa: E402

_rbd_spec = importlib.util.spec_from_file_location(
    "_ram_boot_deploy", REPO / "scripts/ram-boot-deploy.py"
)
_rbd = importlib.util.module_from_spec(_rbd_spec)
_rbd_spec.loader.exec_module(_rbd)

EDK2_OUT = Path("/mnt/2tb/unvr-port-refs/edk2")
FD_ADDR = "0x20000000"

# "UEFI Interactive Shell" only ever appears once Shell.efi genuinely
# launches and runs - unlike "UEFI Shell"/"Shell>" text, which also
# appears (as a false-positive match) in BDS's own boot-options-dump
# debug trace ("Boot0001: UEFI Shell") before anything has launched.
SUCCESS_PATTERN = "UEFI Interactive Shell"
CRASH_PATTERN = "Synchronous Exception|Data Abort|Instruction Abort|Kernel panic"

# Confirmed live: BdsDxe's own boot timeout (PcdPlatformBootTimeOut=3 in
# Unvr.dsc) opens the hotkey window; "[Bds]BdsWait(3)/(2)/(1)" spans
# ~3s from whenever BDS itself starts. That start point isn't fixed,
# though - P1.5's added PciBusDxe/XhciDxe/UsbBusDxe/UsbMassStorageDxe
# dispatch (real controller resets/timeouts, not just image loads) push
# it out further than P0/P1's ~8s total was sized for; confirmed live
# 2026-09-03 that 8s missed the window entirely (boot reached the menu
# safely, probe just never sent 's' before BdsWait closed). Widened
# with real margin for a growing component list, not just this one
# case - holding 's' down forever would just get typed into the shell
# once reached, so this still isn't "as long as possible."
HOTKEY_SPAM_S = 25.0
HOTKEY_INTERVAL_S = 0.15

# Waits for: "UEFI Interactive Shell" after `go`. Observed: a fresh boot +
# hotkey race reaches it in low tens of seconds on this P0 component list
# (which is far shorter than P4's - see uefi-chainload-from-awto.py's 180s).
# 60s is ~2x. On expiry: dump the transcript, tell the caller to power-cycle;
# RAM payload, nothing at risk. A failed jump aborts in <1s via jump_failed().
PROBE_TIMEOUT_S = 60
# Waits for: `go`'s "## Starting application at 0x..." banner, a printf on
# the line before the branch. Expected: one console round-trip, ~30ms at
# 115200 for a 60-char line. 1.5s is ~50x - loose on purpose because it only
# bounds the failure case and a false abort costs a whole power-cycle.
# On expiry: report that `go` never announced itself and stop.
JUMP_BANNER_S = 1.5
# Stock's prompt. `go` returning to it after the jump command is proof the
# jump did not take (bad address, or the payload returned).
STOCK_PROMPT = "ALPINE_UBNT_NAS_ALL>"


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--target",
        choices=["DEBUG", "RELEASE", "NOOPT"],
        default="DEBUG",
        help="must match the --target used with ./dev.py build-uefi-p0",
    )
    ap.add_argument(
        "--fd",
        type=Path,
        default=None,
        help="override the FD path directly instead of deriving it from --target",
    )
    ap.add_argument(
        "--no-hotkey",
        action="store_true",
        help="don't race the 's' hotkey - just watch BDS's automatic "
        "phase land on Boot0000's own menu (the pre-hotkey-fix behavior)",
    )
    args = ap.parse_args()
    if args.fd is None:
        args.fd = EDK2_OUT / f"Build/UNVR/{args.target}_GCC/FV/UNVR.fd"

    if not args.fd.exists():
        print(
            f"FATAL: {args.fd} missing - run "
            f"'./dev.py build-uefi-p0 --target {args.target}' first"
        )
        return 1
    local_crc = zlib.crc32(args.fd.read_bytes()) & 0xFFFFFFFF
    print(
        f"UNVR.fd: {args.fd} ({args.fd.stat().st_size} bytes, crc32=0x{local_crc:08x})"
    )

    _rbd.ensure_tftpd()
    server_ip = detect_server_ip()
    print(f"tftp server IP (auto-detected): {server_ip}")

    print("starting catch-uboot.py to win the autoboot race")
    catch = subprocess.Popen(
        [sys.executable, "scripts/catch-uboot.py", "--seconds", "60"], cwd=REPO
    )
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

    _rbd.run_devpy(
        "--expect",
        "ALPINE_UBNT_NAS_ALL>",
        "--timeout",
        "8",
        f"setenv ipaddr {_rbd.IPADDR}",
    )
    _rbd.run_devpy(
        "--expect",
        "ALPINE_UBNT_NAS_ALL>",
        "--timeout",
        "8",
        f"setenv serverip {server_ip}",
    )
    try:
        _rbd.tftp_and_verify(args.fd, FD_ADDR)
    except (RuntimeError, subprocess.TimeoutExpired) as e:
        print(f"FATAL: tftp of UNVR.fd failed: {e}")
        return 1

    # Length LITERAL, never ${filesize}: that variable is set only by U-Boot's
    # own load commands, so anything that skips the tftp leaves it empty and
    # crc32 then aborts on a byte-correct image. The local file is the
    # authority for both the length and the expected value.
    out = _rbd.run_devpy(
        "--expect",
        "ALPINE_UBNT_NAS_ALL>",
        "--timeout",
        "10",
        f"crc32 {FD_ADDR} 0x{args.fd.stat().st_size:x}",
    )
    if f"{local_crc:08x}".lower() not in out.lower():
        print(
            f"FATAL: on-device crc32 does not match local 0x{local_crc:08x} "
            f"- refusing to jump into a corrupted transfer\n{out}"
        )
        return 1
    print(f"crc32 verified: 0x{local_crc:08x} matches\n")

    print(f"=== go {FD_ADDR} - transferring control to EDK2 SEC ===")
    s = _console.connect()
    s.sendall(f"go {FD_ADDR}\r".encode())

    start = time.monotonic()
    buf = b""
    last_key_send = -1.0
    verdict = None
    hotkey = "" if args.no_hotkey else "s"
    while time.monotonic() - start < PROBE_TIMEOUT_S:
        try:
            d = s.recv(4096)
            if d:
                buf += d
        except TimeoutError:
            pass
        now = time.monotonic() - start
        text = buf.decode(errors="replace")

        # Abort on PROOF the jump failed rather than spamming a dead prompt
        # and then waiting out PROBE_TIMEOUT_S. See _console.jump_failed.
        verdict = _console.jump_failed(text, hotkey, STOCK_PROMPT)
        if verdict:
            break
        if now > JUMP_BANNER_S and _console.jump_banner_missing(text):
            verdict = f"`go` never announced itself within {JUMP_BANNER_S}s"
            break

        if hotkey and now < HOTKEY_SPAM_S and now - last_key_send > HOTKEY_INTERVAL_S:
            s.sendall(hotkey.encode())
            last_key_send = now
        if SUCCESS_PATTERN in text or any(p in text for p in CRASH_PATTERN.split("|")):
            break

    text = buf.decode(errors="replace")
    print(text)

    if verdict:
        print(f"\nRESULT: {verdict} (aborted after {time.monotonic() - start:.1f}s).")
        return 1
    if any(p in text for p in CRASH_PATTERN.split("|")):
        print("\nRESULT: EDK2 crashed (exception/abort).")
        return 1
    if SUCCESS_PATTERN in text:
        print("\nRESULT: SUCCESS - reached the real UEFI Interactive Shell prompt.")
        return 0
    print(
        f"\nRESULT: no response within {PROBE_TIMEOUT_S}s - likely hung. "
        "Power-cycle (./dev.py power-cycle) to return to normal boot; "
        "this never touched flash."
    )
    return 1


if __name__ == "__main__":
    sys.exit(main())
