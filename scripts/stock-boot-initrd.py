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
- Probes each ext4 partition for the Debian tree, mounts the SSD's Fedora root, overlays a tmpfs on
  /var/lib/stock4/root and switch_roots to it (the stock run writes nothing but one breadcrumb file).
- On failure: sends dmesg, partitions, USB list, then `reboot -f` (U-Boot's one-shot flag is already
  cleared, so the box returns to Fedora). The stock kernel has no vfat; the file goes to the ext4 root.
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

ROOT_SUBDIR = "/var/lib/stock4/root"  # Debian 11 rootfs (5.1.25) on the SSD root; writes go to a tmpfs overlay
# SATA link-up is ~1 s after boot on 7.4; stock 4.19 untested here. 30 s = 30x, then diagnostics + reboot.
ROOT_WAIT_S = 30
# 10G optic link-up takes ~4 s on 7.4; 20 s = 5x, then carry on without network.
LINK_WAIT_S = 20
LOG_PORT = 9999
# Timing-channel delays (s) added before the reboot when the stock kernel sees a block device / an ethN / carrier.
D_DISK, D_NIC, D_LINK = 25, 50, 100
SIGNATURE_DELAY_S = 40  # proves our /init ran (diagnostic only)
HOST_NIC = "enp8s0"  # dev host 10G NIC; same subnet as the box

INIT = r"""#!/bin/sh
export PATH=/sbin:/bin:/usr/sbin:/usr/bin
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev 2>/dev/null || mdev -s
mkdir -p /tmp /probe /base /ovl /newroot
# Signature delay: the box returning ~@SIG@ s later than a plain failure proves THIS init ran.
sleep @SIG@
L=/tmp/awto-stock4-diag.txt
: > $L
say() {
    M="$(cut -d' ' -f1 /proc/uptime)s $*"
    echo "$M" >> $L
    echo "$M" | nc -w 2 @LOGHOST@ @PORT@ 2>/dev/null
}
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
say "cmdline: $(cat /proc/cmdline)"
find_root() {
    for d in /dev/sd[a-z][0-9]*; do
        [ -b "$d" ] || continue
        mount -t ext4 -o ro,noload $d /probe 2>/dev/null || continue
        if [ -x /probe@SUBDIR@/sbin/init ]; then umount /probe; echo $d; return 0; fi
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
if [ -n "$ROOT" ] && mount -t ext4 -o rw $ROOT /base 2>>$L; then
    cp $L /base/var/log/awto-stock4-diag.txt 2>/dev/null
    sync
    mount -o remount,ro /base 2>>$L
else
    say "rw mount of '$ROOT' failed"
fi
if [ -x /base@SUBDIR@/sbin/init ] \
   && mount -t tmpfs -o size=1500m tmpfs /ovl 2>>$L && mkdir -p /ovl/up /ovl/work \
   && mount -t overlay overlay -o lowerdir=/base@SUBDIR@,upperdir=/ovl/up,workdir=/ovl/work /newroot 2>>$L; then
    say "overlay root ready; switch_root"
    cp $L /newroot/var/log/awto-stock4-init.txt 2>/dev/null
    mkdir -p /newroot/dev
    mount -n -o move /dev /newroot/dev 2>/dev/null || umount /dev
    umount /proc /sys 2>/dev/null
    exec switch_root /newroot /sbin/init
fi
say "ROOT FAILED, sending diagnostics"
# Timing channel for a console-less box: each fact adds a distinct delay before the reboot, so the
# time the box takes to return to Fedora says what the stock kernel saw (docs/stock-boot.md).
CARRY=$(cat /sys/class/net/$NIC/carrier 2>/dev/null)
[ "$(ls /sys/block | grep -c '^sd')" -gt 0 ] && sleep @D_DISK@
[ "$(ls /sys/class/net | grep -c '^eth')" -gt 0 ] && sleep @D_NIC@
[ "$CARRY" = 1 ] && sleep @D_LINK@
{
    echo "--- dmesg"; dmesg
    echo "--- /proc/partitions"; cat /proc/partitions
    echo "--- /proc/mounts"; cat /proc/mounts
    echo "--- usb"; for d in /sys/bus/usb/devices/*; do echo "$d $(cat $d/product 2>/dev/null) $(cat $d/speed 2>/dev/null)"; done
    echo "--- scsi"; cat /proc/scsi/scsi 2>/dev/null
    echo "--- init log"; cat $L
} 2>&1 | nc -w 5 @LOGHOST@ @PORT@ 2>/dev/null
cp $L /base/var/log/awto-stock4-diag.txt 2>/dev/null && sync
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
        .replace("@SUBDIR@", ROOT_SUBDIR)
        .replace("@ROOT_WAIT@", str(ROOT_WAIT_S))
        .replace("@LINK_WAIT@", str(LINK_WAIT_S))
        .replace("@SIG@", str(SIGNATURE_DELAY_S))
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
