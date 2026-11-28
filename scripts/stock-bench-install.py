#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Install the on-box stock benchmark into the stock-boot Seagate rootfs (#381).

  ./dev.py stock-bench-install        # mount Seagate p1 on woomera, install unit + script, enable ssh

- Copies scripts/stock-bench.py to /usr/local/sbin; unit awto-stock-bench.service runs it after network.
- Watchdog service now resets with SysRq-b: `systemctl reboot --force` hangs the stock kernel on this board.
- Dev host must run `iperf3 -s -p 5702` on the 10G NIC before the boot; see stock-bench.py.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from _net import WOOMERA_DATA_IP  # noqa: E402
from _repo import make_log  # noqa: E402

log = make_log("stock-bench-install")
HOST_NIC = "enp7s0f0"  # dev host NIC on the box subnet (enp8s0 10G is gone: 1G only, iperf capped)
# mount + a few small file writes; 60 s stall bound.
SSH_TIMEOUT_S = 60

REMOTE = r"""
set -eu
P=/dev/disk/by-id/ata-ST6000VX001-2BD186_WCT3LCS1-part1
test -b "$P"
mkdir -p /mnt/stock4
mountpoint -q /mnt/stock4 || mount "$P" /mnt/stock4
R=/mnt/stock4
trap 'umount /mnt/stock4' EXIT
test -f "$R/STOCK4"
install -m 755 /tmp/stock-bench.py "$R/usr/local/sbin/stock-bench.py"
cat > "$R/etc/systemd/system/awto-stock-bench.service" <<'U'
[Unit]
Description=stock-kernel benchmark, then SysRq-b back to Fedora (#381)
After=network-online.target systemd-networkd.service
Wants=systemd-networkd.service

[Service]
Type=simple
Environment=AWTO_LOG_HOST=@HOST@
Environment=AWTO_IPERF_PORT=5702
ExecStart=/usr/bin/python3 /usr/local/sbin/stock-bench.py
U
mkdir -p "$R/etc/systemd/system/multi-user.target.wants"
ln -sf /etc/systemd/system/awto-stock-bench.service "$R/etc/systemd/system/multi-user.target.wants/awto-stock-bench.service"
ln -sf /lib/systemd/system/ssh.service "$R/etc/systemd/system/multi-user.target.wants/ssh.service"
printf '[Unit]\nDescription=reboot (stock-kernel test watchdog)\n\n[Service]\nType=oneshot\nExecStart=/bin/sh -c "sync; echo b > /proc/sysrq-trigger"\n' > "$R/etc/systemd/system/awto-stock-watchdog.service"
test -x "$R/usr/bin/python3" && echo "python3 ok"
ls "$R/usr/bin/fio" "$R/usr/bin/iperf3" "$R/usr/bin/udevadm" "$R/bin/udevadm" 2>&1 | tail -4
echo installed
"""


def host_ip() -> str:
    out = subprocess.run(
        ["ip", "-4", "-o", "addr", "show", HOST_NIC],
        capture_output=True,
        text=True,
        check=True,
    ).stdout
    return out.split()[3].split("/")[0]


def main() -> int:
    subprocess.run(
        [
            "scp",
            "-q",
            str(HERE / "stock-bench.py"),
            f"root@{WOOMERA_DATA_IP}:/tmp/stock-bench.py",
        ],
        check=True,
    )
    r = subprocess.run(
        ["ssh", f"root@{WOOMERA_DATA_IP}", "bash -s"],
        input=REMOTE.replace("@HOST@", host_ip()),
        text=True,
        capture_output=True,
        timeout=SSH_TIMEOUT_S,
    )
    out = (r.stdout + r.stderr)[-2000:]
    log(out)
    print(out)
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
