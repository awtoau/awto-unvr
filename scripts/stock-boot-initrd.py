#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Build the diagnostic initrd for booting Ubiquiti's stock 4.19 kernel without a console (#381).

  ./dev.py stock-boot-initrd [OUT]     # default tmp/stock4/initrd

- One-file newc cpio carrying a replacement /init. The kernel unpacks it over the stock embedded
  initramfs, so the stock busybox (static) and tools are used; the stock `ubnt` init scripts never run
  (they can mkfs the user device).
- Finds the stick root by PARTUUID; on success switch_root to its /sbin/init.
- On any failure: writes dmesg/partitions/blkid/USB list to the SSD ESP (/awto-stock4-diag.txt)
  and reboots (U-Boot's one-shot flag is already cleared, so the box returns to Fedora).
"""

from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from _repo import make_log  # noqa: E402

log = make_log("stock-boot-initrd")

STICK_PARTUUID = "1b5bb944-02"  # Cruzer Switch p2, Debian 11 rootfs (5.1.25)
ESP_PARTUUID = "79cd524f-deea-43fe-ba09-1c0dccf82a1f"  # SSD p1 vfat (200 MiB ESP): any kernel can write it; Fedora root stays untouched
# USB enumeration through hub chain is ~3-5 s on 7.4; stock 4.19 is untested here. 30 s = ~6x, then diag + reboot.
ROOT_WAIT_S = 30

INIT = f"""#!/bin/sh
export PATH=/sbin:/bin:/usr/sbin:/usr/bin
mount -t proc proc /proc
mount -t sysfs sysfs /sys
mount -t devtmpfs devtmpfs /dev 2>/dev/null || mdev -s
L=/tmp/awto-stock4-diag.txt
mkdir -p /tmp /root /ssd
echo "awto stock4 init: kernel $(uname -r) up $(cut -d' ' -f1 /proc/uptime)s" > $L
echo "cmdline: $(cat /proc/cmdline)" >> $L
find_part() {{ blkid | grep -i "PARTUUID=\\"$1\\"" | head -1 | cut -d: -f1; }}
ROOT=
i=0
while [ $i -lt {ROOT_WAIT_S} ]; do
    ROOT=$(find_part {STICK_PARTUUID})
    [ -n "$ROOT" ] && break
    sleep 1
    i=$((i+1))
done
echo "stick root: ${{ROOT:-NOT FOUND}} after ${{i}}s" >> $L
if [ -n "$ROOT" ] && mount -t ext4 -o rw $ROOT /root 2>>$L && [ -x /root/sbin/init ]; then
    echo "mounted $ROOT; switch_root" >> $L
    cp $L /root/var/log/awto-stock4-init.txt 2>/dev/null
    mount -n -o move /dev /root/dev 2>/dev/null || umount /dev
    umount /proc /sys 2>/dev/null
    exec switch_root /root /sbin/init
fi
echo "ROOT FAILED, collecting diagnostics" >> $L
{{
  echo "--- dmesg"; dmesg
  echo "--- /proc/partitions"; cat /proc/partitions
  echo "--- blkid"; blkid
  echo "--- usb devices"; for d in /sys/bus/usb/devices/*; do echo "$d $(cat $d/product 2>/dev/null) $(cat $d/speed 2>/dev/null)"; done
  echo "--- scsi"; cat /proc/scsi/scsi 2>/dev/null
}} >> $L 2>&1
S=$(find_part {ESP_PARTUUID})
if [ -n "$S" ] && mount -t vfat -o rw $S /ssd 2>/dev/null; then
    cp $L /ssd/awto-stock4-diag.txt
    sync
    umount /ssd
fi
sync
reboot -f
"""


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
    cpio = newc_entry("init", INIT.encode(), 0o100755, 1)
    cpio += newc_entry("TRAILER!!!", b"", 0, 0)
    return cpio + b"\0" * (-len(cpio) % 512)


def main() -> int:
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else HERE.parent / "tmp/stock4/initrd"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_bytes(build())
    log(f"wrote {out} ({out.stat().st_size} B)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
