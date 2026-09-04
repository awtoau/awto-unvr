#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""docs/uefi.md P4 end-to-end: awto-nas# -> EDK2 -> Shell -> our kernel.

One script for the whole payoff path, so the awto-nas# prompt's non-sticky
autoboot is only raced once:

  1. ext4load the EDK2 FD off the SSD, crc32-verify it, `go` to it.
     (ext4load, not tftp: awto-uboot's own ethernet cannot transmit -
     #90's UDMA TX hang - but its AHCI/SCSI works.)
  2. Spam the BdsWait hotkey to land at Shell> rather than autobooting.
  3. `connect -r` so the storage tree is bound (BDS auto-connect runs
     before those drivers matter, 2046e1b), then `map -r`.
  4. Launch the kernel directly - `Image` is already a PE/COFF UEFI
     application, and its EFI stub loads the DTB from `dtb=`, so there
     is no GRUB and no gFdtTableGuid config table in this path (#251).

RAM payload only, never touches flash - a power-cycle always returns the
box to its normal stock boot.

Usage:
    ./dev.py uefi-p4-boot [--fs fs0] [--no-boot]
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
from _repo import LOGS  # noqa: E402

FD = Path("/mnt/2tb/unvr-port-refs/edk2/Build/UNVR/DEBUG_GCC/FV/UNVR.fd")
FD_ADDR = "0x20000000"
FD_ON_SSD = "/boot/UNVR.fd"
# scsi device 1 partition 2 = the ext4 root that carries /boot (device
# letters shift across boots, but U-Boot's own scsi numbering is stable
# for this disk set - `scsi scan` output is checked before use).
SSD_PART = "1:2"
PROMPT = "awto-nas#"

# Same root the working awto-uboot chain passes (uboot-port/configs/
# alpine_v2_unvr_defconfig CONFIG_BOOTARGS). Only the LOADER changes in
# this test, so the rootfs arguments must not.
DEFAULT_ROOT = "root=PARTUUID=dcdc291e-9956-48cd-9d7c-48219877881a"
DEFAULT_ARGS = (
    "console=ttyS0,115200 rootfstype=ext4 rw rootwait selinux=0 panic=15 "
    "reboot=cold ahci.mask_port_map=0x5 ahci_alpine.mask_port_map=0x5"
)

SHELL_MARK = "UEFI Interactive Shell"
CRASHES = ("Synchronous Exception", "Data Abort", "Instruction Abort")

# BdsWait's hotkey window is ~3s but starts at an unfixed point once the
# full driver set is dispatching; spam well past it. Extra 's' after the
# Shell is reached only types at the prompt, which is harmless.
HOTKEY_SPAM_S = 25.0
HOTKEY_INTERVAL_S = 0.15
# Measured 2026-09-04: `go` -> Shell took ~145s on a DEBUG build with the
# full USB + AHCI + eth component list (the 115200-baud DEBUG trace is
# most of it). 1.25x = 180s. On expiry we print what arrived.
SHELL_TIMEOUT_S = 180

# `connect -r` walks every handle, each line 115200-baud trace; measured
# ~25s. 60s is ~2.4x. Local Shell commands answer in well under a second,
# so 3s of silence means done.
CONNECT_LIMIT_S = 60.0
CMD_LIMIT_S = 20.0
QUIET_S = 3.0

# 62 MiB kernel through EDK2's 1 MiB-chunked block+FAT reads, then a boot
# to login (~35s on the normal chain). 120s is ~2x that total.
BOOT_LIMIT_S = 120.0

STAGES = [
    ("stub-loaded", "EFI stub: Booting Linux Kernel"),
    ("dtb-cmdline", "Using DTB from command line"),
    ("dtb-cfgtable", "Using DTB from configuration table"),
    ("dtb-empty", "Generating empty DTB"),
    ("exit-bs", "Exiting boot services"),
    ("decompress", "Booting Linux on physical CPU"),
    ("early-console", "Linux version"),
    ("rootfs", "Mounted root"),
    ("init", "systemd"),
    ("login", "login:"),
]
FAILURES = [
    ("stub-dtb-ignored", "Ignoring DTB from command line"),
    ("stub-dtb-failed", "Failed to load device tree"),
    ("stub-open-failed", "Failed to open file"),
    ("shell-not-found", "is not recognized as an internal or external command"),
    ("exception", "Synchronous Exception"),
    ("panic", "Kernel panic"),
]


def make_log():
    LOGS.mkdir(parents=True, exist_ok=True)
    f = (LOGS / "uefi-p4-boot.log").open("a")

    def log(m: str) -> None:
        print(m, flush=True)
        f.write(m + "\n")
        f.flush()

    return log


def expect(s, cmd: str, want: str, limit_s: float) -> str:
    """Send one command, read until `want` appears or limit_s elapses.

    On expiry: return what arrived. Every caller checks its own marker, so
    a partial read surfaces as a named failure, not a silent wrong answer.
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
        if want in buf.decode(errors="replace"):
            break
    return buf.decode(errors="replace")


def quiet(s, cmd: str, limit_s: float) -> str:
    """Send one line, read until QUIET_S of silence or limit_s elapses."""
    s.sendall(cmd.encode() + b"\r")
    buf = b""
    start = last = time.monotonic()
    while True:
        now = time.monotonic()
        if now - start > limit_s or (buf and now - last > QUIET_S):
            break
        try:
            d = s.recv(4096)
        except TimeoutError:
            continue
        if d:
            buf += d
            last = time.monotonic()
    return buf.decode(errors="replace")


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--fs", default="fs0", help="Shell mapping holding the ESP")
    ap.add_argument("--image", default="Image.efi")
    ap.add_argument("--dtb", default=r"\unvr.dtb", help="dtb= path, image-relative")
    ap.add_argument("--root", default=DEFAULT_ROOT)
    ap.add_argument("--args", default=DEFAULT_ARGS)
    ap.add_argument("--no-boot", action="store_true", help="stop at the Shell prompt")
    a = ap.parse_args()

    log = make_log()
    log(f"\n=== uefi-p4-boot {time.strftime('%Y-%m-%dT%H:%M:%S%z')}")
    if not FD.exists():
        log(f"FATAL: {FD} missing - run ./dev.py build-uefi-p0")
        return 1
    crc = zlib.crc32(FD.read_bytes()) & 0xFFFFFFFF
    log(f"FD: {FD.name} ({FD.stat().st_size} B, crc32=0x{crc:08x})")
    log(f"expecting this exact image already at {FD_ON_SSD} (scp it from Linux)")

    s = _console.connect()
    # awto-nas# is NOT sticky: the autoboot countdown keeps running, so a
    # passive read loses the race and lands in Linux. Spam CR to hold it.
    if not any(PROMPT in expect(s, "", PROMPT, 1.0) for _ in range(30)):
        log(f"FATAL: not at {PROMPT} - run ./dev.py uboot-test --cold first")
        return 1

    out = expect(s, "scsi scan", PROMPT, 60)
    if "Device 1:" not in out:
        log(f"FATAL: scsi scan did not find the SSD\n{out}")
        return 1
    out = expect(s, f"ext4load scsi {SSD_PART} {FD_ADDR} {FD_ON_SSD}", "bytes read", 60)
    if "bytes read" not in out:
        log(f"FATAL: ext4load of {FD_ON_SSD} did not complete\n{out}")
        return 1
    expect(s, "", PROMPT, 6)

    out = expect(s, f"crc32 {FD_ADDR} ${{filesize}}", PROMPT, 20)
    if f"{crc:08x}" not in out.lower():
        log(f"FATAL: on-device crc32 mismatch (want 0x{crc:08x})\n{out}")
        return 1
    log(f"crc32 verified: 0x{crc:08x}")

    log(f"=== go {FD_ADDR} - transferring control to EDK2 SEC ===")
    s.sendall(f"go {FD_ADDR}\r".encode())
    buf = b""
    start = time.monotonic()
    last_key = -1.0
    while time.monotonic() - start < SHELL_TIMEOUT_S:
        try:
            d = s.recv(4096)
            if d:
                buf += d
        except TimeoutError:
            pass
        now = time.monotonic() - start
        if now < HOTKEY_SPAM_S and now - last_key > HOTKEY_INTERVAL_S:
            s.sendall(b"s")
            last_key = now
        text = buf.decode(errors="replace")
        if SHELL_MARK in text or any(p in text for p in CRASHES):
            break
    text = buf.decode(errors="replace")
    log(text[-4000:])
    if any(p in text for p in CRASHES):
        log("RESULT: EDK2 crashed (exception/abort).")
        return 1
    if SHELL_MARK not in text:
        log(f"RESULT: no Shell within {SHELL_TIMEOUT_S}s - power-cycle to recover.")
        return 1
    log("reached the UEFI Interactive Shell")

    log("\n--- connect -r")
    log(quiet(s, "connect -r", CONNECT_LIMIT_S)[-3000:])
    log("\n--- map -r")
    log(quiet(s, "map -r", CMD_LIMIT_S)[-3000:])

    log(f"\n--- {a.fs}:")
    out = quiet(s, f"{a.fs}:", CMD_LIMIT_S)
    log(out[-2000:])
    if "is not a valid mapping" in out:
        log(f"FATAL: no {a.fs}: mapping - check `map -r` above for the right fsN")
        return 1

    log("\n--- ls")
    out = quiet(s, "ls", CMD_LIMIT_S)
    log(out[-3000:])
    if a.image not in out:
        log(f"FATAL: {a.image} not on {a.fs}: - run ./dev.py uefi-esp-stage --yes")
        return 1

    if a.no_boot:
        log("--no-boot: stopping at the Shell prompt.")
        return 0

    cmdline = f"{a.image} dtb={a.dtb} {a.root} {a.args}"
    log(f"\n=== launching: {cmdline}\n")
    s.sendall(cmdline.encode() + b"\r")
    buf = b""
    start = time.monotonic()
    while time.monotonic() - start < BOOT_LIMIT_S:
        try:
            d = s.recv(4096)
            if d:
                buf += d
        except TimeoutError:
            continue
        text = buf.decode(errors="replace")
        if any(m in text for _, m in FAILURES) or "login:" in text:
            break
    text = buf.decode(errors="replace")
    log(text)
    reached = [n for n, m in STAGES if m in text]
    failed = [n for n, m in FAILURES if m in text]
    log(f"\nRESULT: reached={reached or ['nothing']} failed={failed or ['none']}")
    return 0 if reached and not failed else 1


if __name__ == "__main__":
    sys.exit(main())
