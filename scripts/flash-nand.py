#!/usr/bin/env python3
"""RECOVERY ONLY - flash a Fedora kernel + DTB into NAND. NOT the deploy path.

**This OVERWRITES awto-uboot.** NAND 0x1300000 held the kernel under the old
layout; since #216 it holds awto-uboot itself, and this script writes 18.9 MiB
there. Running it destroys the bootloader and the box falls back to stock.

The deploy path is `./dev.py deploy-ssd` - awto-uboot boots /boot/uImage from
the SSD, so a deploy is an scp, not a flash. Use this only to put a bootable
kernel back in NAND when the SSD path is broken, and re-run
`./dev.py flash-uboot` afterwards to restore awto-uboot.

Requires --i-know-this-overwrites-awto-uboot. Phase 1 is ./dev.py
publish-fedora (needs Fedora+SSH up).

Runs against a device ALREADY at the U-Boot prompt (ALPINE_UBNT_NAS_ALL>). tftp's
our gzip uImage + ea16 DTB from the host and writes them into the DEAD stock
rootfs region of NAND (the stock kernel @0x300000 is left intact as recovery),
then sets bootcmd to nand-read + bootm and saveenv. Rootfs stays on the SSD
(root=PARTUUID, already in bootargs). Fully NAND-side; SSD untouched.

Refuses to run if the published tftp artifacts are older than the build-out
they should match (assert_fresh()) - the exact staleness that caused the
2026-08-20 incident this now can't do silently.

Layout (NAND offsets):
  kernel  @ 0x1300000  (start of the unused rootfs partition), read span 0x1200000
  dtb     @ 0x2800000
"""

from __future__ import annotations

import os
import socket
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _fedora_deploy import TFTP_DIMG, TFTP_KIMG, assert_fresh

from _net import UNVR_IPADDR as IPADDR
from _net import detect_server_ip
from _repo import make_log

SOCK = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "tio-unvr.sock"
PROMPT = b"ALPINE_UBNT_NAS_ALL>"

KIMG = TFTP_KIMG.name
DIMG = TFTP_DIMG.name
K_NAND, K_SPAN = "0x1300000", "0x1200000"  # 18.9 MiB span (kernel ~18.5)
D_NAND, D_ERASE, D_READ = "0x2800000", "0x40000", "0x20000"
K_RAM, D_RAM = "0x02000000", "0x04078000"
BOOTCMD = (
    f"nand read {K_RAM} {K_NAND} {K_SPAN}; "
    f"nand read {D_RAM} {D_NAND} {D_READ}; bootm {K_RAM} - {D_RAM}"
)


log = make_log("flash-nand")


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
    if "--i-know-this-overwrites-awto-uboot" not in sys.argv:
        sys.exit(
            "REFUSING: this writes 18.9 MiB to NAND 0x1300000, where awto-uboot\n"
            "now lives (#216) - it would destroy the bootloader.\n\n"
            "  Deploying a kernel?   ./dev.py deploy-ssd --reboot\n"
            "  Really recovering?    add --i-know-this-overwrites-awto-uboot,\n"
            "                        then ./dev.py flash-uboot to restore it."
        )
    assert_fresh()
    if not SOCK.exists():
        sys.exit(f"console socket absent: {SOCK}")
    server_ip = detect_server_ip()
    log(f"tftp server IP (auto-detected): {server_ip}")
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(0.1)
    s.connect(str(SOCK))
    # confirm we're at the prompt
    step(s, "", "ALPINE_UBNT_NAS_ALL>", 5, "at U-Boot prompt")
    log("=== flashing Fedora kernel+DTB to NAND (stock kernel @0x300000 preserved) ===")
    step(s, f"setenv ipaddr {IPADDR}", "ALPINE_UBNT_NAS_ALL>", 5, "set ipaddr")
    step(s, f"setenv serverip {server_ip}", "ALPINE_UBNT_NAS_ALL>", 5, "set serverip")
    # kernel
    step(s, f"tftpboot {K_RAM} {KIMG}", "Bytes transferred", 60, f"tftp {KIMG}")
    step(s, f"nand erase {K_NAND} {K_SPAN}", "OK", 30, "erase kernel region")
    step(s, f"nand write {K_RAM} {K_NAND} {K_SPAN}", "OK", 60, "write kernel")
    # dtb
    step(s, f"tftpboot {D_RAM} {DIMG}", "Bytes transferred", 30, f"tftp {DIMG}")
    step(s, f"nand erase {D_NAND} {D_ERASE}", "OK", 15, "erase dtb block")
    step(s, f"nand write {D_RAM} {D_NAND} {D_ERASE}", "OK", 15, "write dtb")
    # bootcmd + save
    step(s, f"setenv bootcmd '{BOOTCMD}'", "ALPINE_UBNT_NAS_ALL>", 5, "set bootcmd")
    step(s, "saveenv", "done", 15, "saveenv")
    log("DONE — kernel+DTB in NAND, bootcmd set + saved. 'boot' or reset to verify.")
    s.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
