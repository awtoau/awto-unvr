#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Poll-based hotplug for the empty SATA ports on woomera (#377).

  ./dev.py sata-hotplug install|uninstall|status|scan

- Ports 2/4/6/8 (ata2,4,6,8) are unmasked but unused; no hotplug detect is proven on them.
- Timer: every 10 s, wildcard-scan the scsi host of each port with no device (COMRESET, ~0.3 s).
- Latency bound: a plugged drive is found within 10 s. Occupied ports are never scanned.
- Needs the unmasked boot (no ahci.mask_port_map=0x5); on a masked boot the ports are DUMMY and the scan is a no-op.
"""

from __future__ import annotations

import argparse
import base64
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import make_log  # noqa: E402

log = make_log("sata-hotplug")

EMPTY_ATA = (2, 4, 6, 8)  # host index = ata number - 1
SERVICE = "/etc/systemd/system/awto-sata-rescan.service"
TIMER = "/etc/systemd/system/awto-sata-rescan.timer"
# Detect latency <= 10 s; scan of one empty port ~0.3 s (dmesg: COMRESET to link-down line).
PERIOD = "10s"


def scan_cmd() -> str:
    return "; ".join(
        f"ls /sys/class/scsi_device | grep -q ^{a - 1}: || "
        f"echo '- - -' > /sys/class/scsi_host/host{a - 1}/scan"
        for a in EMPTY_ATA
    )


def service_text() -> str:
    return f"""# managed by scripts/sata-hotplug.py (#377)
[Unit]
Description=rescan empty SATA ports (poll hotplug)

[Service]
Type=oneshot
TimeoutStartSec=5s
ExecStart=/bin/sh -c "{scan_cmd()}"
"""


def timer_text() -> str:
    return f"""# managed by scripts/sata-hotplug.py (#377)
[Unit]
Description=poll empty SATA ports for new drives

[Timer]
OnBootSec=30s
OnUnitActiveSec={PERIOD}
AccuracySec=1s

[Install]
WantedBy=timers.target
"""


def ssh(cmd: str, check: bool = True) -> str:
    log(f"box$ {cmd[:160]}")
    here = Path(__file__).resolve().parent
    r = subprocess.run(
        [sys.executable, str(here / "ssh-woomera.py"), "--", cmd],
        capture_output=True,
        text=True,
        timeout=30,  # one systemctl call over LAN ssh, expected <2 s
    )
    out = (r.stdout + r.stderr).strip()
    if out:
        log(out[-1500:])
    if check and r.returncode:
        raise SystemExit(f"remote command failed rc={r.returncode}")
    return out


def put(path: str, text: str) -> None:
    b64 = base64.b64encode(text.encode()).decode()
    ssh(f"echo {b64} | base64 -d > {path}")


def install() -> int:
    put(SERVICE, service_text())
    put(TIMER, timer_text())
    ssh(f"systemd-analyze verify {SERVICE} {TIMER}")
    ssh("systemctl daemon-reload")
    ssh("systemctl enable --now awto-sata-rescan.timer")
    return status()


def uninstall() -> int:
    ssh("systemctl disable --now awto-sata-rescan.timer", check=False)
    ssh(f"rm -f {SERVICE} {TIMER}; systemctl daemon-reload")
    return 0


def status() -> int:
    ssh("systemctl list-timers awto-sata-rescan.timer --no-pager", check=False)
    ssh("ls /sys/class/ata_device/; lsblk -dno NAME,SIZE,MODEL,TRAN", check=False)
    return 0


def scan() -> int:
    ssh(scan_cmd())
    ssh("dmesg | grep -E 'ata[0-9]+: ' | tail -6", check=False)
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("cmd", choices=["install", "uninstall", "status", "scan"])
    a = ap.parse_args()
    return {
        "install": install,
        "uninstall": uninstall,
        "status": status,
        "scan": scan,
    }[a.cmd]()


if __name__ == "__main__":
    sys.exit(main())
