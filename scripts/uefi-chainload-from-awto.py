#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Chainload the EDK2 P0 FD from OUR U-Boot's awto-nas# prompt.

scripts/uefi-chainload-probe.py does the same jump from *stock* U-Boot
(ALPINE_UBNT_NAS_ALL>). This one starts from awto-nas#, which is the
shipped chain (stock -> awto-uboot -> UEFI) and what docs/uefi.md P2
(SATA) needs: awto-uboot's PCA9575 @0x21 gpio-hogs power the four SATA
bays and its board_init() does the CCU/PCIe bring-up, so EDK2 entered
from here inherits a fully set-up board. UEFI's own job is then only
what a UEFI kernel boot needs.

Preconditions:
  - the box is ALREADY at awto-nas# - `./dev.py uboot-test --cold` gets
    it there. This script does not power-cycle or catch; it only types
    at a prompt that already exists.
  - the same FD is already at /boot/UNVR.fd on the SSD - `scp <fd>
    root@<box>:/boot/UNVR.fd` from a Linux boot. It is loaded with
    ext4load, not tftp, because awto-uboot's own ethernet cannot
    transmit (#90).

RAM payload only, never touches flash - a power-cycle always returns the
box to its normal stock boot.

Usage:
    ./dev.py uefi-chainload-from-awto [--shell-cmd "pci"] [--shell-cmd "map -r"]
"""

from __future__ import annotations

import argparse
import os
import sys
import time
import zlib
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
import _console  # noqa: E402

EDK2_OUT = Path("/mnt/2tb/unvr-port-refs/edk2")
FD_ADDR = "0x20000000"
PROMPT = "awto-nas#"
# The FD is read off the SSD's ext4 root (scsi device 1, partition 2 - the
# same device/part docs/fedora-on-ssd.md boots from), not over the network.
SSD_PART = "1:2"
FD_ON_SSD = "/boot/UNVR.fd"

SUCCESS_PATTERN = "UEFI Interactive Shell"
CRASH_PATTERNS = ("Synchronous Exception", "Data Abort", "Instruction Abort")

# Same numbers uefi-chainload-probe.py justifies: BdsWait's hotkey window
# is ~3s but starts at an unfixed point once PciBus/Xhci/Ahci dispatch is
# in the component list; spam well past it. Holding 's' longer only types
# into the shell once reached.
HOTKEY_SPAM_S = 25.0
HOTKEY_INTERVAL_S = 0.15
# Waits for: "UEFI Interactive Shell" after the jump. Measured 2026-09-04:
# ~145s on a DEBUG build with the full P1.5 USB + P2 AHCI + P3 eth component
# list (the DEBUG trace itself is most of it - every dispatch line is 115200
# baud). 1.25x = 180s. On expiry: print what arrived and tell the caller to
# power-cycle; RAM payload, so nothing is at risk. A failed jump never gets
# here - jump_failed() catches it in <1s.
PROBE_TIMEOUT_S = 180
# Waits for: `bootedk2`'s "## bootedk2: ..." banner, a printf on the line
# before the branch. Expected: one console round-trip, ~30ms at 115200 for a
# 60-char line. 1.5s is ~50x - loose on purpose because it only bounds the
# failure case and a false abort costs a whole cold-boot cycle.
# On expiry: report the command never announced itself and stop.
JUMP_BANNER_S = 1.5


def send_expect(
    s, cmd: str, want: str, limit_s: float, *, or_prompt: bool = False
) -> str:
    """Send one command, read until `want` appears or limit_s elapses.

    or_prompt: also stop when PROMPT returns after the command's own echo. A
    command that failed (missing file, no such device) prints its error and
    re-prompts at once, so waiting out limit_s after that adds nothing.

    On expiry: return what arrived so far. Every caller checks for its
    own marker, so a partial read surfaces as a named failure rather
    than a silent wrong answer.
    """
    s.sendall(cmd.encode() + b"\r")
    buf = b""
    end = time.monotonic() + limit_s
    while time.monotonic() < end:
        try:
            d = s.recv(4096)
            if d:
                buf += d
        except TimeoutError:
            continue
        text = buf.decode(errors="replace")
        if want in text:
            break
        if or_prompt and cmd and cmd in text and PROMPT in text.split(cmd, 1)[1]:
            break
    return buf.decode(errors="replace")


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--target", choices=["DEBUG", "RELEASE", "NOOPT"], default="DEBUG")
    ap.add_argument("--fd", type=Path, default=None)
    ap.add_argument("--no-hotkey", action="store_true")
    ap.add_argument(
        "--shell-cmd",
        action="append",
        default=[],
        help="UEFI Shell command to run once the Shell prompt is reached "
        "(repeatable, run in order)",
    )
    args = ap.parse_args()
    if args.fd is None:
        args.fd = EDK2_OUT / f"Build/UNVR/{args.target}_GCC/FV/UNVR.fd"
    if not args.fd.exists():
        print(f"FATAL: {args.fd} missing - run ./dev.py build-uefi-p0 first")
        return 1

    fd_bytes = args.fd.read_bytes()
    local_crc = zlib.crc32(fd_bytes) & 0xFFFFFFFF
    print(f"UNVR.fd: {args.fd} ({len(fd_bytes)} B, crc32=0x{local_crc:08x})")
    print(f"expecting this exact image already at {FD_ON_SSD} on the SSD")

    s = _console.connect()
    # awto-nas# is NOT sticky - CONFIG_BOOTDELAY=2 keeps counting, and a
    # passive read loses that race (commands then land at stock's prompt
    # as garbled text). Spam CR to interrupt, as uboot-test.tcl does.
    # 30 tries x ~1s covers the 2s countdown many times over; on expiry
    # abort rather than type into whatever prompt is actually there.
    ok = False
    for _ in range(30):
        if PROMPT in send_expect(s, "", PROMPT, 1.0):
            ok = True
            break
    if not ok:
        print(f"FATAL: not at {PROMPT} - run ./dev.py uboot-test --cold first")
        return 1

    # Load off the SSD, NOT tftp: awto-uboot's own ethernet cannot transmit
    # (issue #90, al_eth UDMA TX hang - "TX completion timeout: 1 descs left"
    # then "ARP Retry count exceeded", confirmed live 2026-09-04 on both the
    # 1G and 10G ports). Its AHCI/SCSI works fine, so ext4load is the only
    # working transport from this prompt. Deploy the FD with
    # `scp <fd> root@<box>:/boot/UNVR.fd` from a Linux boot first.
    out = send_expect(s, "scsi scan", PROMPT, 60)
    if "Device 1:" not in out:
        print(f"FATAL: scsi scan did not find the SSD\n{out}")
        return 1
    # or_prompt: a missing file re-prompts at once, so the 60s bound only
    # ever applies to a genuinely-running read.
    out = send_expect(
        s,
        f"ext4load scsi {SSD_PART} {FD_ADDR} {FD_ON_SSD}",
        "bytes read",
        60,
        or_prompt=True,
    )
    if "bytes read" not in out:
        print(f"FATAL: ext4load of {FD_ON_SSD} did not complete\n{out}")
        return 1
    # Wait for the crc VALUE itself. Matching the prompt (still in the
    # buffer from the previous command) or crc32's "==>" (arrives a read
    # before the digits) both return empty and read as a false mismatch.
    # Size passed literally, not ${filesize}, so this needs no U-Boot var.
    want_crc = f"{local_crc:08x}"
    # or_prompt: crc32 prints its value BEFORE re-prompting, so stopping at
    # the prompt cannot truncate a match - it only ends a mismatch early.
    out = send_expect(
        s, f"crc32 {FD_ADDR} 0x{len(fd_bytes):x}", want_crc, 30, or_prompt=True
    )
    if want_crc not in out.lower():
        print(f"FATAL: on-device crc32 mismatch (want 0x{want_crc})\n{out}")
        return 1
    print(f"crc32 verified: 0x{local_crc:08x}\n")

    # bootedk2, not `go` + manual `dcache off; icache off`. awto-uboot runs
    # with MMU + caches ON and do_go skips cleanup_before_linux(), so the FD
    # aborts in its own text (esr 0x86000004). bootedk2 does the cleanup
    # itself, so the requirement cannot be forgotten. 0c2b4d4, docs/uefi.md P2.
    print(f"=== bootedk2 {FD_ADDR} - transferring control to EDK2 SEC ===")
    s.sendall(f"bootedk2 {FD_ADDR}\r".encode())

    start = time.monotonic()
    buf = b""
    last_key = -1.0
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
        verdict = _console.jump_failed(text, hotkey, PROMPT)
        if verdict:
            break
        if now > JUMP_BANNER_S and _console.jump_banner_missing(text):
            verdict = f"bootedk2 never announced itself within {JUMP_BANNER_S}s"
            break

        if hotkey and now < HOTKEY_SPAM_S and now - last_key > HOTKEY_INTERVAL_S:
            s.sendall(hotkey.encode())
            last_key = now
        if SUCCESS_PATTERN in text or any(p in text for p in CRASH_PATTERNS):
            break

    text = buf.decode(errors="replace")
    print(text)
    if verdict:
        print(f"\nRESULT: {verdict} (aborted after {time.monotonic() - start:.1f}s).")
        if "Unknown command" in text:
            print("  -> this awto-uboot predates `bootedk2` (0c2b4d4); reflash it.")
        return 1
    if any(p in text for p in CRASH_PATTERNS):
        print("\nRESULT: EDK2 crashed (exception/abort).")
        return 1
    if SUCCESS_PATTERN not in text:
        print(f"\nRESULT: no Shell within {PROBE_TIMEOUT_S}s - power-cycle to recover.")
        return 1
    print("\nRESULT: SUCCESS - reached the UEFI Interactive Shell.")

    for cmd in args.shell_cmd:
        print(f"\n=== Shell> {cmd}\n")
        # Local shell commands answer in well under a second; the DEBUG
        # trace is what keeps the line busy. Read for a fixed 16s window
        # (~8x the observed answer time) rather than matching a marker,
        # since each command's output shape differs.
        print(send_expect(s, cmd, "\x00-never-matches", 16))
    return 0


if __name__ == "__main__":
    sys.exit(main())
