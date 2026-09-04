#!/usr/bin/env python3
"""Atomic netboot: catch U-Boot, tftp kernel+DTB, bootm - all in one socket session.

Why one process: the stock U-Boot has a ~50s watchdog at the prompt. Catching
the prompt in one tool call and sending commands in the next leaves a gap the
watchdog resets into, and the next autoboot lands in the stock OS. This streams
ESC to catch the prompt, then fires the whole netboot sequence back-to-back on
the same socket, reading for each command's completion marker (no log-follower
race). tftp keeps U-Boot busy so the watchdog never idles out.

Run (device must be power-cycling or about to reboot):
    ./scripts/netboot.py --tag 7.1
    ./scripts/netboot.py --tag 6.18   # --ip and --server default from _net.py
"""

from __future__ import annotations

import argparse
import os
import socket
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _net import UNVR_IPADDR, detect_server_ip
from _repo import LOGS

SOCK = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "tio-unvr.sock"

PROMPT = b"ALPINE_UBNT_NAS_ALL>"
ESC_INTERVAL = 0.05  # 20 ESC/s across the bootdelay=2 window


def log(msg: str) -> None:
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {msg}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    with (LOGS / "netboot.log").open("a") as fh:
        fh.write(line + "\n")


def send(s: socket.socket, line: str) -> None:
    s.sendall(line.encode() + b"\n")


def wait_for(s: socket.socket, needle: bytes, *, what: str, limit_s: float) -> bool:
    """Read the socket until `needle` appears. limit_s derived per-marker, not round."""
    buf = b""
    end = time.monotonic() + limit_s
    while time.monotonic() < end:
        try:
            chunk = s.recv(4096)
        except TimeoutError:
            continue
        if not chunk:
            break
        buf = (buf + chunk)[-65536:]
        if needle in buf:
            return True
    log(f"TIMEOUT waiting for {what!r} after {limit_s:.0f}s (watch console log)")
    return False


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--tag", required=True, help="image suffix, e.g. 7.1 or 6.18")
    ap.add_argument("--ip", default=UNVR_IPADDR, help="box IP in U-Boot (_net.py)")
    ap.add_argument(
        "--server",
        default=None,
        help="tftp server (this dev host) IP; default: auto-detect via "
        "_net.detect_server_ip() (this host's DHCP lease drifts, so a "
        "hardcoded default goes stale)",
    )
    ap.add_argument("--kaddr", default="0x02000000")
    ap.add_argument("--dtaddr", default="0x04078000")
    ap.add_argument(
        "--panic",
        type=int,
        default=15,
        help="kernel panic= seconds (auto-reboot on panic; 0 = omit)",
    )
    ap.add_argument("--uimage", help="explicit tftp uImage filename (overrides --tag)")
    ap.add_argument("--dtb", help="explicit tftp DTB filename (overrides --tag)")
    ap.add_argument(
        "--initrd", help="tftp initrd/initramfs filename (adds external ramdisk)"
    )
    ap.add_argument(
        "--iaddr",
        default="0x20000000",
        help="initrd load addr (clear of kernel decompress)",
    )
    ap.add_argument("--bootargs", help="full bootargs override (e.g. root=PARTUUID=..)")
    # Catch ceiling: long enough to power-cycle + preboot (~15s) with margin.
    ap.add_argument("--catch-seconds", type=float, default=180.0)
    a = ap.parse_args()
    if a.server is None:
        a.server = detect_server_ip()
        log(f"tftp server IP (auto-detected): {a.server}")

    uimage = a.uimage or f"uImage-unvr-ea16-{a.tag}"
    dtb = a.dtb or f"alpine-v2-ubnt-unvr-ea16-{a.tag}.dtb"
    panic = f" panic={a.panic}" if a.panic else ""
    bootargs = a.bootargs or (
        f"console=ttyS0,115200 sysid=ea16 ubnthal.sysid=ea16 "
        f"reboot=warm rw iommu.passthrough=1 pci=pcie_bus_perf{panic}"
    )

    if not SOCK.exists():
        sys.exit(f"console socket absent: {SOCK}\nStart it with ./dev.py console")

    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(0.05)
    s.connect(str(SOCK))

    # --- Phase 1: stream ESC until the U-Boot prompt ---
    log(
        f"streaming ESC to catch U-Boot - POWER-CYCLE/REBOOT THE UNVR NOW (tag={a.tag})"
    )
    buf = b""
    last = 0.0
    end = time.monotonic() + a.catch_seconds
    caught = False
    while time.monotonic() < end:
        now = time.monotonic()
        if now - last >= ESC_INTERVAL:
            try:
                s.sendall(b"\x1b")
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
        log("did not reach U-Boot prompt within catch window")
        return 1
    log("U-Boot prompt reached - firing netboot sequence")

    # --- Phase 2: netboot, back-to-back on the same session ---
    send(s, "version")  # absorbs trailing ESC noise
    send(s, f"setenv ipaddr {a.ip}")
    send(s, f"setenv serverip {a.server}")
    send(s, f"setenv bootargs '{bootargs}'")

    # tftp keeps U-Boot busy => watchdog stays fed. ~18MB at ~2.5MB/s ~= 8s;
    # 40s limit = 5x margin. DTB is ~15KB: sub-second, 15s limit is generous.
    send(s, f"tftpboot {a.kaddr} {uimage}")
    if not wait_for(s, b"Bytes transferred", what=f"{uimage} tftp", limit_s=40):
        return 2
    log(f"{uimage} loaded")
    send(s, f"tftpboot {a.dtaddr} {dtb}")
    if not wait_for(s, b"Bytes transferred", what=f"{dtb} tftp", limit_s=15):
        return 2
    log(f"{dtb} loaded")

    rd = "-"
    if a.initrd:
        # This stock U-Boot (2015.07) lacks CONFIG_SUPPORT_RAW_INITRD, so the raw
        # addr:size syntax fails ("Wrong Ramdisk Image Format"). The initrd must be
        # a uImage-wrapped ramdisk (scripts/mkuimage.py --ramdisk); bootm then reads
        # its header, no :size needed. ~35MB at ~2.5MB/s ~14s; 60s limit = margin.
        send(s, f"tftpboot {a.iaddr} {a.initrd}")
        if not wait_for(s, b"Bytes transferred", what=f"{a.initrd} tftp", limit_s=60):
            return 2
        log(f"{a.initrd} loaded at {a.iaddr}")
        rd = a.iaddr

    send(s, f"bootm {a.kaddr} {rd} {a.dtaddr}")
    # Confirm the kernel actually took (Verifying Checksum -> decompress -> Linux).
    if wait_for(s, b"Booting kernel", what="bootm handoff", limit_s=15):
        log("bootm handed off to kernel - watch console log for platform bring-up")
        s.close()
        return 0
    s.close()
    return 3


if __name__ == "__main__":
    sys.exit(main())
