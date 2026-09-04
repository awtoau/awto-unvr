#!/usr/bin/env python3
"""Make woomera boot Fedora standalone from the SSD — set U-Boot bootcmd + saveenv.

The stock U-Boot (ubnt_nas, 2015.07) has CONFIG_SCSI_AHCI + CMD_SCSI + CMD_EXT4,
so it can read the SATA SSD directly — no NAND flashing. This catches U-Boot,
records the ORIGINAL bootcmd (for recovery), verifies `scsi init` finds a disk,
then sets bootcmd to load our gzip uImage + DTB from /boot on the ext4 root and
boots with root=PARTUUID, and `saveenv`s it. Fully reversible (env only; NAND
untouched). It does NOT reset — verify the standalone boot separately.

SAFETY: if `scsi init` finds no disk, it aborts WITHOUT saveenv (env untouched).
"""

from __future__ import annotations

import os
import socket
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import LOGS

SOCK = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "tio-unvr.sock"
PROMPT = b"ALPINE_UBNT_NAS_ALL>"
ESC_INTERVAL = 0.05

PARTUUID = "dcdc291e-9956-48cd-9d7c-48219877881a"
KERNEL = "/boot/uImage-unvr-ea16-7.1-fedora-gz"
DTB = "/boot/alpine-v2-ubnt-unvr-ea16-7.1-fedora.dtb"
KADDR, DTADDR = "0x02000000", "0x04078000"
BOOTARGS = (
    f"console=ttyS0,115200 root=PARTUUID={PARTUUID} rootfstype=ext4 "
    f"rw rootwait selinux=0 panic=15 reboot=cold"
)
BOOTCMD = (
    f"scsi init; ext4load scsi 0:2 {KADDR} {KERNEL}; "
    f"ext4load scsi 0:2 {DTADDR} {DTB}; bootm {KADDR} - {DTADDR}"
)


def log(m):
    line = (
        f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    )
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    (LOGS / "persist-boot.log").open("a").write(line + "\n")


def send(s, line):
    s.sendall(line.encode() + b"\n")


def read_until(s, needle, limit_s, capture=False):
    buf = b""
    end = time.monotonic() + limit_s
    while time.monotonic() < end:
        try:
            chunk = s.recv(4096)
        except TimeoutError:
            continue
        if not chunk:
            break
        buf += chunk
        if needle in buf:
            return buf if capture else True
    return buf if capture else False


def main():
    if not SOCK.exists():
        sys.exit(f"console socket absent: {SOCK}")
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(0.05)
    s.connect(str(SOCK))

    # catch U-Boot
    catch_s = float(os.environ.get("CATCH_S", "1800"))
    log(
        f"streaming ESC — REBOOT/POWER-CYCLE woomera now (waiting up to {catch_s:.0f}s)"
    )
    buf = b""
    last = 0.0
    end = time.monotonic() + catch_s
    caught = False
    while time.monotonic() < end:
        now = time.monotonic()
        if now - last >= ESC_INTERVAL:
            try:
                s.sendall(b"\x1b")
            except TimeoutError:
                pass  # transient; keep streaming
            except OSError as e:
                log(f"ESC send failed: {e}")
                return 1
            last = now
        try:
            chunk = s.recv(4096)
        except TimeoutError:
            continue
        if not chunk:
            break
        buf = (buf + chunk)[-16384:]
        if PROMPT in buf:
            caught = True
            break
    if not caught:
        log("did not reach U-Boot prompt")
        return 1
    log("U-Boot prompt reached")

    send(s, "version")
    read_until(s, PROMPT, 3)
    # record original bootcmd for recovery
    send(s, "printenv bootcmd")
    orig = read_until(s, PROMPT, 5, capture=True)
    log("ORIGINAL bootcmd:\n" + orig.decode(errors="replace"))
    # verify SATA
    send(s, "scsi init")
    scsi = read_until(s, PROMPT, 20, capture=True).decode(errors="replace")
    log("scsi init output:\n" + scsi)
    # A real detection prints "Device N: ... (Samsung/WDC...)"; "Init SCSI" contains
    # the substring "SCSI" so do NOT match on that. Abort on an explicit 0 count or
    # no real device line.
    if "Found 0 device" in scsi or not any(
        k in scsi for k in ("Device 0:", "Device 1:", "Samsung", "WDC")
    ):
        log("ABORT: scsi init found no disk (links timed out) — env left UNTOUCHED.")
        s.close()
        return 2

    # set + save the standalone boot config
    send(s, f"setenv bootargs '{BOOTARGS}'")
    read_until(s, PROMPT, 3)
    send(s, f"setenv bootcmd '{BOOTCMD}'")
    read_until(s, PROMPT, 3)
    send(s, "saveenv")
    sav = read_until(s, PROMPT, 15, capture=True).decode(errors="replace")
    log("saveenv output:\n" + sav)
    log("DONE — bootcmd set + saved. Run 'boot' or reset to verify standalone boot.")
    s.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
