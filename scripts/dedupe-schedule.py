#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Hourly btrfs dedupe on woomera: systemd timer + duperemove per data filesystem (#372).

  python3 scripts/dedupe-schedule.py plan|apply|status

- Btrfs only dedupes when duperemove runs; this makes that hourly (Dan, 2026-11-18).
- One instance per filesystem: awto-dedupe@mirror / awto-dedupe@single -> /srv/<name>.
- Hashfile per fs on the system SSD: unchanged files are not re-read, so idle passes are cheap.
- Idle IO class + nice 19: dedupe never competes with NFS/SMB; 1 IO thread (2 HDDs, #372).
- Skips its own start while another duperemove runs (e.g. a manual pass): no overlap.
- Log: journalctl -u 'awto-dedupe@*'.
"""

from __future__ import annotations

import argparse
import base64
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import make_log  # noqa: E402

log = make_log("dedupe-schedule")

# mirror is deduped by bees (#372, Dan 2026-11-18); duperemove only covers single.
FILESYSTEMS = ("single",)
SERVICE = "/etc/systemd/system/awto-dedupe@.service"
TIMER = "/etc/systemd/system/awto-dedupe@.timer"
# Hourly cadence: a pass must end before the next trigger. 50 min = 5/6 of the period; on expiry
# systemd kills it and the next hour resumes from the hashfile.
PASS_TIMEOUT = "50min"


def service_text() -> str:
    return f"""# managed by scripts/dedupe-schedule.py (#372)
[Unit]
Description=btrfs dedupe of /srv/%i (duperemove)
ConditionPathIsMountPoint=/srv/%i

[Service]
Type=oneshot
Nice=19
IOSchedulingClass=idle
TimeoutStartSec={PASS_TIMEOUT}
StateDirectory=duperemove
ExecCondition=/bin/sh -c '! pgrep -x duperemove'
ExecStart=/usr/bin/duperemove -rdh --io-threads=1 --cpu-threads=2 \\
    --hashfile=/var/lib/duperemove/%i.hash --exclude='*#recycle*' --exclude='*/.tools' /srv/%i
"""


def timer_text() -> str:
    return """# managed by scripts/dedupe-schedule.py (#372)
[Unit]
Description=hourly btrfs dedupe of /srv/%i

[Timer]
OnCalendar=hourly
Persistent=true

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
        timeout=120,  # one systemctl/dnf call over LAN ssh; dnf install is the longest (~30 s)
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


def plan() -> int:
    log("would install: duperemove (dnf), " + SERVICE + ", " + TIMER)
    log(
        "would enable timers: "
        + ", ".join(f"awto-dedupe@{n}.timer" for n in FILESYSTEMS)
    )
    log("--- service ---\n" + service_text() + "--- timer ---\n" + timer_text())
    return 0


def apply() -> int:
    ssh("rpm -q duperemove >/dev/null 2>&1 || dnf -y install duperemove")
    put(SERVICE, service_text())
    put(TIMER, timer_text())
    ssh("systemd-analyze verify " + SERVICE + " " + TIMER)
    ssh("systemctl daemon-reload")
    for n in FILESYSTEMS:
        ssh(f"systemctl enable --now awto-dedupe@{n}.timer")
    return status()


def status() -> int:
    ssh("systemctl list-timers 'awto-dedupe@*' --no-pager", check=False)
    ssh(
        "for n in " + " ".join(FILESYSTEMS) + "; do echo $n: "
        "$(systemctl is-enabled awto-dedupe@$n.timer) "
        "last=$(systemctl show awto-dedupe@$n.service -p ExecMainStatus --value) ; done",
        check=False,
    )
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("cmd", choices=["plan", "apply", "status"])
    a = ap.parse_args()
    return {"plan": plan, "apply": apply, "status": status}[a.cmd]()


if __name__ == "__main__":
    sys.exit(main())
