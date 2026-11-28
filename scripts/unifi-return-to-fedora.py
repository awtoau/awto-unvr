#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Get woomera back to Fedora from a UniFi OS boot with no console (#383). Stdlib only.

  python3 unifi-return-to-fedora.py install /run/ubnt   # on woomera, stick rootfs mounted at /run/ubnt
  python3 unifi-return-to-fedora.py run                 # what the unit runs inside UniFi OS

- run: wait 90 s, save diagnostics to /var/log/awto-diag-*/ on the stick, then mount the SSD's Fedora root,
  rename /boot/awto/unifi -> unifi.disabled, sync, reboot.
- awto-uboot then cannot load the profile and falls back to /boot/uImage (Fedora).
- Re-stage the profile afterwards with ./dev.py boot-next stage unifi ...
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import time

SSD_PARTUUID = "dcdc291e-9956-48cd-9d7c-48219877881a"  # Fedora root on the SSD (p2)
PROFILE = "boot/awto/unifi"
MNT = "/run/awto-ssd"
LOG = "/var/log/awto-return.log"
# SSD link-up is ~3 s after boot (dmesg ata7); 30 s bounds a slow probe, then give up and log.
WAIT_S = 30
# UniFi services restart every ~7 s; 90 s lets postgres/mongo/sfpd/unifi-core show their failure.
DIAG_DELAY_S = 90
DIAG_CMDS = {
    "dmesg": "dmesg",
    "mtd": "cat /proc/mtd",
    "lsmod": "lsmod",
    "systemd-failed": "systemctl --failed --no-pager",
    "ssh": "systemctl status ssh --no-pager -l; journalctl -u ssh --no-pager -n 40",
    "network": "ip -br a; networkctl --no-pager",
    "ubnthal": "cat /proc/ubnthal/system.info; ls -R /proc/ubnthal",
    "journal-errors": "journalctl -b -p err --no-pager -n 300",
}
UNIT = "awto-return-to-fedora.service"
SBIN = "/usr/local/sbin/unifi-return-to-fedora.py"

UNIT_TEXT = f"""[Unit]
Description=Return woomera to Fedora (#383)
After=local-fs.target systemd-udev-settle.service

[Service]
Type=oneshot
TimeoutStartSec=150
ExecStart=/usr/bin/python3 {SBIN} run

[Install]
WantedBy=multi-user.target
"""


def log(msg: str) -> None:
    with open(LOG, "a") as f:
        f.write(f"{time.strftime('%Y-%m-%dT%H:%M:%S%z')} {msg}\n")


def diag() -> None:
    out = f"/var/log/awto-diag-{time.strftime('%Y%m%d-%H%M%S')}"
    os.makedirs(out, exist_ok=True)
    for name, cmd in DIAG_CMDS.items():
        with open(f"{out}/{name}.txt", "w") as f:
            subprocess.run(
                cmd, shell=True, stdout=f, stderr=subprocess.STDOUT, check=False
            )
    os.sync()
    log(f"diagnostics in {out}")


def run() -> int:
    time.sleep(DIAG_DELAY_S)
    diag()
    dev = f"/dev/disk/by-partuuid/{SSD_PARTUUID}"
    t0 = time.monotonic()
    while not os.path.exists(dev):
        if time.monotonic() - t0 > WAIT_S:
            log(f"SSD {dev} not found after {WAIT_S}s: giving up")
            return 1
        time.sleep(1)
    os.makedirs(MNT, exist_ok=True)
    subprocess.run(["mount", "-t", "ext4", dev, MNT], check=True)
    try:
        src = f"{MNT}/{PROFILE}"
        if not os.path.isdir(src):
            log(f"{src} absent: nothing to do")
            return 0
        if os.path.exists(src + ".disabled"):
            shutil.rmtree(src + ".disabled")
        os.replace(src, src + ".disabled")
        os.sync()
        log(f"renamed {src} -> {src}.disabled; rebooting")
    finally:
        subprocess.run(["umount", MNT], check=False)
    subprocess.run(["systemctl", "reboot"], check=False)
    return 0


def install(root: str) -> int:
    with open(__file__) as f:
        body = f.read()
    os.makedirs(f"{root}/usr/local/sbin", exist_ok=True)
    with open(f"{root}{SBIN}", "w") as f:
        f.write(body)
    os.chmod(f"{root}{SBIN}", 0o755)
    with open(f"{root}/etc/systemd/system/{UNIT}", "w") as f:
        f.write(UNIT_TEXT)
    wants = f"{root}/etc/systemd/system/multi-user.target.wants"
    os.makedirs(wants, exist_ok=True)
    link = f"{wants}/{UNIT}"
    if not os.path.lexists(link):
        os.symlink(f"/etc/systemd/system/{UNIT}", link)
    return 0


if __name__ == "__main__":
    if len(sys.argv) >= 2 and sys.argv[1] == "run":
        sys.exit(run())
    if len(sys.argv) >= 3 and sys.argv[1] == "install":
        sys.exit(install(sys.argv[2]))
    print(__doc__)
    sys.exit(2)
