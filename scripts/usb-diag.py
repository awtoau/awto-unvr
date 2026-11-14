#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""USB port/device diagnostics on woomera over SSH (#360). Log: tmp/logs/usb-diag.log.

./dev.py usb-diag status        # root hubs, devices, USB disks, xHCI PORTSC, recent dmesg
./dev.py usb-diag rebind        # xhci_hcd unbind/bind (controller re-init)
./dev.py usb-diag pci-reset     # PCI remove + rescan of the xHCI function
./dev.py usb-diag watch         # stream USB kernel messages until Ctrl-C
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _box
from _repo import make_log

log = make_log("usb-diag")
XHCI = "0001:01:00.0"  # ASMedia ASM1042A on the external PCIe port (#360)
# One SSH round trip + sysfs reads finish in ~2 s; a PCI rescan adds <1 s. 4x -> 10 s.
CALL_TIMEOUT_S = 10

STATUS = r"""
cat /proc/uptime; lsusb -t; lsusb
lsblk -d -o NAME,TRAN,SIZE,MODEL,SERIAL | awk 'NR==1 || $2=="usb"'
for f in /sys/kernel/debug/usb/xhci/*/ports/*/portsc; do echo "$f: $(cat $f)"; done
for p in /sys/bus/usb/devices/*-0:1.0/usb*-port*; do
  echo "$(basename $p) state=$(cat $p/state) over_current=$(cat $p/over_current_count)"; done
dmesg | grep -iE 'usb [0-9]|uas|\[sd[e-z]\]|disconnect|over-current|xhci' | tail -n 15
"""
REBIND = f"""
echo {XHCI} > /sys/bus/pci/drivers/xhci_hcd/unbind; echo {XHCI} > /sys/bus/pci/drivers/xhci_hcd/bind
udevadm settle
"""
PCI_RESET = f"""
echo 1 > /sys/bus/pci/devices/{XHCI}/remove; echo 1 > /sys/bus/pci/rescan
udevadm settle; lspci -s {XHCI}
"""


def remote(ip: str, script: str, *, stream: bool = False) -> int:
    argv = _box.ssh_argv(
        ip, cmd=["bash", "-s"], batch=True
    )  # script on stdin: no ssh re-quoting
    if stream:
        with subprocess.Popen(
            argv, stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True
        ) as p:
            p.stdin.write(script)
            p.stdin.close()
            for line in p.stdout:
                log(line.rstrip())
        return p.returncode
    try:
        r = subprocess.run(
            argv, input=script, capture_output=True, text=True, timeout=CALL_TIMEOUT_S
        )
    except subprocess.TimeoutExpired:
        log(f"FAIL: ssh call exceeded {CALL_TIMEOUT_S}s")
        return 1
    for line in (r.stdout + r.stderr).splitlines():
        log(line)
    return r.returncode


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("action", choices=["status", "rebind", "pci-reset", "watch"])
    a = ap.parse_args()
    ip = _box.require()
    log(f"== usb-diag {a.action} on {ip}")
    if a.action == "watch":
        return remote(
            ip,
            "dmesg -w | grep --line-buffered -iE 'usb [0-9]|uas|sd[e-z]|disconnect|over-current'",
            stream=True,
        )
    if a.action == "rebind":
        rc = remote(ip, REBIND)
    elif a.action == "pci-reset":
        rc = remote(ip, PCI_RESET)
    else:
        rc = 0
    return rc or remote(ip, STATUS)


if __name__ == "__main__":
    sys.exit(main())
