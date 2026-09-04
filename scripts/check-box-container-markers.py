#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Check the LIVE box for stale container markers that fool systemd (#164, #217).

`scripts/build-fedora-rootfs.py:verify_not_container_flavored()` catches these
in a freshly-built installroot. It cannot see a rootfs that was built BEFORE
that guard existed and is still running - which is exactly what happened: a
`/run/.containerenv` dated 2026-08-16, left by the podman-based build dropped
in c712df1 (2026-08-23), survived on the SSD for weeks.

Why it hides: `/run` gets a tmpfs early in boot, so the on-disk file is
invisible to everything afterwards, including `ls /run/.containerenv`. But
PID1 reads it in its first mount-table pass, BEFORE that tmpfs is mounted, and
caches "container" for its whole lifetime. `systemd-detect-virt` - a fresh
process, running after the tmpfs is up - reports "none", so the two disagree
and the real state is only visible via `systemctl show --property=Virtualization`.

Consequence: systemd disables .swap units as a class ("Operation on or unit
type of dev-zram0.swap not supported on this system"), plus anything else
gated on ConditionVirtualization=!container.

Detection is via a bind mount of / , which exposes what is underneath /run.

Run: ./dev.py check-box-container   (or via ./dev.py gate)
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
import _box  # noqa: E402

# Files systemd's detect_container() consults. Only a container runtime ever
# writes these; on a bare-metal rootfs any of them is a defect.
MARKERS = (
    "run/.containerenv",
    ".dockerenv",
    "run/systemd/container",
    "run/host/container-manager",
)

# Bind-mount / somewhere, so what is UNDER /run's tmpfs becomes visible.
PROBE = (
    "set -e; M=$(mktemp -d); mount --bind / $M; "
    + "; ".join(f'test -e "$M/{m}" && echo "MARKER {m}"' for m in MARKERS)
    + " ; umount $M; rmdir $M; "
    "echo VIRT=$(systemctl show --property=Virtualization --value)"
)


def main() -> int:
    ip = _box.locate()
    out = ""
    if ip:
        _box.flush_failed_neighbours(ip)
        out = subprocess.run(
            _box.ssh_argv(ip, cmd=["sh", "-c", PROBE]),
            capture_output=True,
            text=True,
            check=False,
        ).stdout
    if "VIRT=" not in out:
        # Not reachable is not a failure - the box is often at the U-Boot
        # prompt or powered off during a gate run.
        print("box-container: SKIP (box not reachable)")
        return 0

    markers = [
        ln.split(None, 1)[1] for ln in out.splitlines() if ln.startswith("MARKER ")
    ]
    virt = next(
        (
            ln.split("=", 1)[1].strip()
            for ln in out.splitlines()
            if ln.startswith("VIRT=")
        ),
        "",
    )

    if markers or virt:
        for m in markers:
            print(f"  STALE container marker on the live rootfs: /{m}")
        if virt:
            print(f"  systemd reports Virtualization={virt} on bare metal")
        print(
            "\nBOX CONTAINER MARKERS (#164, #217): PID1 caches 'container' from a\n"
            "marker read before /run's tmpfs mounts, and disables .swap units and\n"
            "anything with ConditionVirtualization=!container.\n"
            "Fix, then reboot:\n"
            "  ./dev.py ssh -- 'M=$(mktemp -d); mount --bind / $M; "
            "rm -f $M/run/.containerenv; umount $M; rmdir $M'"
        )
        return 1

    print("box-container: OK (no stale markers, Virtualization empty)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
