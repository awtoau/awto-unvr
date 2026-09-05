#!/usr/bin/env python3
"""Flash awto-uboot into NAND and point stock U-Boot's bootcmd at it (#216).

Makes awto-uboot the default boot stage instead of stock loading a kernel
directly. awto-uboot then boots the kernel from the SSD (its own compiled
CONFIG_BOOTCOMMAND scans scsi for /boot/uImage), which is ~6x faster than
NAND (148 ms vs 1580 ms for the same 19 MB) and puts OUR bootargs in charge -
so console=ttyS0,1500000 (#220) actually takes effect. Stock's compiled
bootargs say 115200 and cannot be changed; they win on the direct-NAND path.

Layout (NAND offsets):
  awto-uboot @ 0x1300000  (start of the unused rootfs partition, 1 MiB span)
  stock kernel @ 0x300000 is left intact - `run bootnand` still recovers it.

Stock loads it as a RAW image at awto-uboot's link address (CONFIG_TEXT_BASE
0x01100000) and jumps with `go`, not bootm: u-boot.bin has no uImage header.

Recovery if this goes wrong: interrupt stock with <Esc><Esc> and either
`run bootnand` (factory kernel) or re-run ./dev.py flash to restore the
direct-NAND kernel boot.
"""

from __future__ import annotations

import os
import socket
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
from _net import UNVR_IPADDR as IPADDR  # noqa: E402
from _net import detect_server_ip  # noqa: E402
from _repo import make_log  # noqa: E402

# tmp/tftp, matching dev.py's TFTP_ROOT and what the running tftpd serves.
# images/tftp is the deploy-artifact dir and is NOT the server root (#dev.py:1012).
TFTP_DIR = REPO / "tmp/tftp"

SOCK = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "tio-unvr.sock"

UBOOT_BIN = REPO / "tmp/uboot-build/u-boot.bin"
TFTP_NAME = "u-boot-awto.bin"

# awto-uboot's link address (CONFIG_TEXT_BASE in alpine_v2_unvr_defconfig).
# It must be loaded at, and entered at, exactly this address.
TEXT_BASE = "0x01100000"
# Staging address for tftp/nand-read. Well clear of TEXT_BASE so the copy
# never overlaps the region it is about to run from.
STAGE = "0x02000000"

U_NAND = "0x1300000"
# 1 MiB: u-boot.bin is ~837 KB. NAND erase/write must be block-aligned and
# this is the next block boundary above the binary.
U_SPAN = "0x100000"

# Stock's bootcmd: pull awto-uboot out of NAND and enter it. `go` because
# u-boot.bin is a raw image with no uImage header for bootm to parse.
BOOTCMD = f"nand read {TEXT_BASE} {U_NAND} {U_SPAN}; go {TEXT_BASE}"


log = make_log("flash-awto-uboot")


def step(s, cmd, needle, limit, label):
    s.sendall(cmd.encode() + b"\n")
    buf = b""
    end = time.monotonic() + limit
    while time.monotonic() < end:
        try:
            c = s.recv(4096)
        except TimeoutError:
            continue
        if not c:
            break
        buf += c
        if needle.encode() in buf:
            log(f"  OK: {label}")
            return buf
    log(
        f"  FAIL: {label} — did not see {needle!r} in {limit}s\n{buf.decode(errors='replace')[-400:]}"
    )
    raise SystemExit(3)


def main():
    if not UBOOT_BIN.exists():
        sys.exit(f"no awto-uboot binary: {UBOOT_BIN} (run ./dev.py build-uboot)")
    size = UBOOT_BIN.stat().st_size
    if size > int(U_SPAN, 16):
        sys.exit(f"u-boot.bin is {size} B, larger than the {U_SPAN} NAND span")
    log(f"awto-uboot: {UBOOT_BIN} ({size} B), link/entry {TEXT_BASE}")

    staged = TFTP_DIR / TFTP_NAME
    staged.write_bytes(UBOOT_BIN.read_bytes())
    log(f"staged for tftp: {staged}")

    if not SOCK.exists():
        sys.exit(f"console socket absent: {SOCK}")
    server_ip = detect_server_ip()
    log(f"tftp server IP (auto-detected): {server_ip}")

    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(0.1)
    s.connect(str(SOCK))

    step(s, "", "ALPINE_UBNT_NAS_ALL>", 5, "at stock U-Boot prompt")
    log("=== flashing awto-uboot to NAND (stock kernel @0x300000 preserved) ===")
    step(s, f"setenv ipaddr {IPADDR}", "ALPINE_UBNT_NAS_ALL>", 5, "set ipaddr")
    step(s, f"setenv serverip {server_ip}", "ALPINE_UBNT_NAS_ALL>", 5, "set serverip")
    step(
        s, f"tftpboot {STAGE} {TFTP_NAME}", "Bytes transferred", 60, f"tftp {TFTP_NAME}"
    )
    step(s, f"nand erase {U_NAND} {U_SPAN}", "OK", 30, "erase awto-uboot region")
    step(s, f"nand write {STAGE} {U_NAND} {U_SPAN}", "OK", 60, "write awto-uboot")

    # Read back to TEXT_BASE before committing bootcmd: a bad write that only
    # surfaces at the next cold boot would leave the box in stock with no
    # kernel path. `cmp` is not confirmed present in this 2015.07 build, so
    # this proves the read succeeds, not that the bytes match.
    step(s, f"nand read {TEXT_BASE} {U_NAND} {U_SPAN}", "OK", 30, "read back")

    step(s, f"setenv bootcmd '{BOOTCMD}'", "ALPINE_UBNT_NAS_ALL>", 5, "set bootcmd")
    step(s, "saveenv", "done", 15, "saveenv")
    log("DONE — awto-uboot in NAND, stock bootcmd chainloads it. Reset to verify.")
    log("If it fails to come up: <Esc><Esc> at stock, then `run bootnand`.")
    log("")
    log("NEXT, and it is REQUIRED: awto-uboot has a saved env in mtd3 pinning")
    log("baudrate=115200, and a saved env beats compiled CONFIG_BAUDRATE. At the")
    log("awto-nas# prompt run:  env default -a; saveenv")
    log("Then the console is 1500000 from awto-uboot's banner onward - reattach")
    log("with UNVR_CONSOLE_BAUD=1500000. Until then it stays 115200 (#220).")
    s.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
