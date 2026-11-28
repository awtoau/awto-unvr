#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Diagnostic initrd for booting Ubiquiti's stock 4.19 kernel without a console (#381).

  ./dev.py stock-boot-initrd [OUT]      # build tmp/stock4/initrd
  ./dev.py stock-boot-initrd listen     # receive the box's progress over TCP (tmp/logs/stock4-net.log)

- One-file newc cpio carrying a replacement /init. The kernel unpacks it over the stock embedded
  initramfs, so the stock static busybox is used; the stock `ubnt` init scripts never run (they can mkfs).
- Brings up the 10G port (stock kernel has the NIC driver built in) and sends each step plus the kernel
  log to the listener with `nc`, so a failure is visible with no console or disk.
- Loads ui-hdd-pwrctl/al_nand like the stock initramfs, finds the ext4 partition holding /STOCK4 (the
  Seagate made by stock-boot-disk) and switch_roots to it.
- On failure: sends dmesg, partitions, USB list, then `reboot -f` (U-Boot's one-shot flag is already
  cleared, so the box returns to Fedora).
"""

from __future__ import annotations

import socket
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from _box import MAC_10G  # noqa: E402
from _net import WOOMERA_DATA_IP  # noqa: E402
from _repo import make_log  # noqa: E402

log = make_log("stock-boot-initrd")

# SATA link-up is ~1 s after boot on 7.4; stock 4.19 untested here. 30 s = 30x, then diagnostics + reboot.
ROOT_WAIT_S = 30
# 10G optic link-up takes ~4 s on 7.4; 20 s = 5x, then carry on without network.
LINK_WAIT_S = 20
LOG_PORT = 9999
# Timing-channel delays (s) added before the reboot when the stock kernel sees a block device / an ethN / carrier.
D_DISK, D_NIC, D_LINK = 25, 50, 100
HOST_NIC = "enp7s0f0"  # dev host NIC on the box subnet

INIT = r"""#!/bin/sh
export PATH=/sbin:/bin:/usr/sbin:/usr/bin
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev 2>/dev/null || mdev -s
mkdir -p /tmp /probe /newroot
L=/tmp/awto-stock4-diag.txt
: > $L
say() {
    M="$(cut -d' ' -f1 /proc/uptime)s $*"
    echo "$M" >> $L
    echo "$M" | nc -w 2 @LOGHOST@ @PORT@ 2>/dev/null
}
# The stock initramfs loads these (conf/modules) before it touches disks: ui-hdd-pwrctl powers the bays.
# Backgrounded: a vendor module that hangs in the kernel must not hang this init. 5 s = ~10x a module probe.
for m in ui-hdd-pwrctl al_nand; do
    echo "modprobe $m start" >> $L
    ( modprobe $m 2>>$L; echo "modprobe $m rc=$?" >> $L ) &
done
sleep 5
NIC=
for n in /sys/class/net/*; do
    [ "$(cat $n/address 2>/dev/null)" = "@MAC@" ] && NIC=${n##*/}
done
if [ -n "$NIC" ]; then
    ifconfig $NIC @IP@ netmask 255.255.255.0 up
    c=0
    while [ "$(cat /sys/class/net/$NIC/carrier 2>/dev/null)" != 1 ] && [ $c -lt @LINK_WAIT@ ]; do
        sleep 1
        c=$((c+1))
    done
fi
say "stock4 init: kernel $(uname -r) nic=${NIC:-NONE} carrier=$(cat /sys/class/net/$NIC/carrier 2>/dev/null) ifaces=$(ls /sys/class/net | tr '\n' ' ')"
say "modules: $(sed -n 1,3p $L | tr '\n' ' ')"
# Stock-boot disk = the ext4 partition holding the marker file /STOCK4 (scripts/stock-boot-disk.py).
find_root() {
    for d in /dev/sd[a-z][0-9]*; do
        [ -b "$d" ] || continue
        mount -t ext4 -o ro,noload $d /probe 2>/dev/null || continue
        if [ -f /probe/STOCK4 ]; then umount /probe; echo $d; return 0; fi
        umount /probe
    done
    return 1
}
ROOT=
i=0
while [ $i -lt @ROOT_WAIT@ ]; do
    ROOT=$(find_root) && break
    ROOT=
    sleep 1
    i=$((i+1))
done
say "root search: ${ROOT:-NOT FOUND} after ${i}s; partitions: $(tr '\n' ';' < /proc/partitions)"
if [ -n "$ROOT" ] && mount -t ext4 -o rw $ROOT /newroot 2>>$L; then
    say "root mounted rw; switch_root"
    cp $L /newroot/var/log/awto-stock4-init.txt 2>/dev/null
    mkdir -p /newroot/dev
    mount -n -o move /dev /newroot/dev 2>/dev/null || umount /dev
    umount /proc /sys 2>/dev/null
    exec switch_root /newroot /sbin/init
fi
say "ROOT FAILED, sending diagnostics"
# Timing channel for a console-less box: each fact adds a distinct delay before the reboot, so the
# time the box takes to return to Fedora says what the stock kernel saw.
CARRY=$(cat /sys/class/net/$NIC/carrier 2>/dev/null)
[ "$(ls /sys/block | grep -c '^sd')" -gt 0 ] && sleep @D_DISK@
[ "$(ls /sys/class/net | grep -c '^eth')" -gt 0 ] && sleep @D_NIC@
[ "$CARRY" = 1 ] && sleep @D_LINK@
{
    echo "--- dmesg"; dmesg
    echo "--- /proc/partitions"; cat /proc/partitions
    echo "--- /proc/mounts"; cat /proc/mounts
    echo "--- lsmod"; cat /proc/modules
    echo "--- usb"; for d in /sys/bus/usb/devices/*; do echo "$d $(cat $d/product 2>/dev/null) $(cat $d/speed 2>/dev/null)"; done
    echo "--- scsi"; cat /proc/scsi/scsi 2>/dev/null
    echo "--- init log"; cat $L
} 2>&1 | nc -w 5 @LOGHOST@ @PORT@ 2>/dev/null
sync
reboot -f
"""


def host_ip() -> str:
    out = subprocess.run(
        ["ip", "-4", "-o", "addr", "show", HOST_NIC],
        capture_output=True,
        text=True,
        check=True,
    ).stdout
    return out.split()[3].split("/")[0]


def init_script() -> str:
    return (
        INIT.replace("@LOGHOST@", host_ip())
        .replace("@PORT@", str(LOG_PORT))
        .replace("@MAC@", MAC_10G)
        .replace("@IP@", WOOMERA_DATA_IP)
        .replace("@ROOT_WAIT@", str(ROOT_WAIT_S))
        .replace("@LINK_WAIT@", str(LINK_WAIT_S))
        .replace("@D_DISK@", str(D_DISK))
        .replace("@D_NIC@", str(D_NIC))
        .replace("@D_LINK@", str(D_LINK))
    )


def newc_entry(name: str, data: bytes, mode: int, ino: int) -> bytes:
    nm = name.encode() + b"\0"
    hdr = "070701" + "".join(
        f"{v:08x}" for v in (ino, mode, 0, 0, 1, 0, len(data), 0, 0, 0, 0, len(nm), 0)
    )
    out = hdr.encode() + nm
    out += b"\0" * (-len(out) % 4)
    out += data
    out += b"\0" * (-len(data) % 4)
    return out


def build() -> bytes:
    cpio = newc_entry("init", init_script().encode(), 0o100755, 1)
    cpio += newc_entry("TRAILER!!!", b"", 0, 0)
    return cpio + b"\0" * (-len(cpio) % 512)


def listen() -> int:
    """Append every TCP message from the box to tmp/logs/stock4-net.log until killed."""
    path = HERE.parent / "tmp" / "logs" / "stock4-net.log"
    path.parent.mkdir(parents=True, exist_ok=True)
    srv = socket.socket()
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("0.0.0.0", LOG_PORT))
    srv.listen(8)
    log(f"listening on :{LOG_PORT} -> {path}")
    while True:
        conn, addr = srv.accept()
        with conn, path.open("ab") as f:
            f.write(f"\n=== {time.strftime('%F %T')} from {addr[0]}\n".encode())
            while chunk := conn.recv(65536):
                f.write(chunk)
                f.flush()


def main() -> int:
    if len(sys.argv) > 1 and sys.argv[1] == "listen":
        return listen()
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else HERE.parent / "tmp/stock4/initrd"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(build())
    log(f"wrote {out} ({out.stat().st_size} B), log host {host_ip()}:{LOG_PORT}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
