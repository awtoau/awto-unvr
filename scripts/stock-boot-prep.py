#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Prepare the stock-boot Debian rootfs that sits on woomera's SSD (#381).

  ./dev.py stock-boot-prep           # chroot-prepare /var/lib/stock4/root on the box

- Target: /var/lib/stock4/root (pristine 5.1.25 rootfs, copied there by the tar-over-ssh step in docs).
- Installs iperf3 fio smartmontools ethtool sysstat from archive.debian.org, adds our ssh key,
  host keys and a static 10G address, masks the UniFi services, adds a 40 min reboot watchdog timer.
- Remote script runs under `set -eu` with a trap that unmounts the bind mounts; it refuses to run if
  the target tree is missing (an empty $R once wrote into Fedora's /etc).
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from _box import MAC_10G  # noqa: E402
from _net import LAN_GATEWAY, WOOMERA_DATA_IP  # noqa: E402
from _repo import make_log  # noqa: E402

log = make_log("stock-boot-prep")
# One apt run over the LAN plus package unpack: ~60 s expected; 300 s stall bound for the whole script.
SSH_TIMEOUT_S = 300
PUBKEY = Path.home() / ".ssh" / "id_ed25519.pub"

REMOTE = r"""
set -eu
R=/var/lib/stock4/root
test -x "$R/sbin/init" || { echo "FATAL: $R/sbin/init missing"; exit 1; }
case "$R" in /var/lib/stock4/root) ;; *) echo "FATAL: R wrong"; exit 1;; esac
mkdir -p "$R"/dev "$R"/proc "$R"/sys "$R"/run "$R"/tmp "$R"/boot
chmod 1777 "$R/tmp"
cleanup() { for m in dev/pts dev proc sys; do umount "$R/$m" 2>/dev/null || true; done; }
trap cleanup EXIT
mount --bind /dev "$R/dev"; mount -t devpts devpts "$R/dev/pts"
mount --bind /proc "$R/proc"; mount --bind /sys "$R/sys"
chmod 1777 "$R/tmp"
cp -L /etc/resolv.conf "$R/etc/resolv.conf.new"
[ -L "$R/etc/resolv.conf" ] && rm "$R/etc/resolv.conf"
mv "$R/etc/resolv.conf.new" "$R/etc/resolv.conf"
printf 'deb https://archive.debian.org/debian/ bullseye main contrib non-free\n' > "$R/etc/apt/sources.list"
printf 'deb https://archive.debian.org/debian-security bullseye-security main contrib non-free\n' >> "$R/etc/apt/sources.list"
for f in $(grep -rl 'ubnt-dpkg' "$R/etc/dpkg" "$R/etc/apt" 2>/dev/null || true); do mv "$f" "$f.disabled"; done
C="chroot $R /usr/bin/env PATH=/usr/sbin:/usr/bin:/sbin:/bin"
$C apt-get -o Acquire::Check-Valid-Until=false update -qq
DEBIAN_FRONTEND=noninteractive $C apt-get -o Acquire::Check-Valid-Until=false install -y -qq --no-install-recommends iperf3 fio smartmontools ethtool sysstat
$C /usr/bin/iperf3 --version | head -1
$C /usr/bin/fio --version
mkdir -p "$R/root/.ssh"; chmod 700 "$R/root/.ssh"
cat /tmp/stock4.pub > "$R/root/.ssh/authorized_keys"; chmod 600 "$R/root/.ssh/authorized_keys"
$C /usr/bin/ssh-keygen -A >/dev/null
mkdir -p "$R/etc/systemd/network"
printf '[Match]\nMACAddress=@MAC@\n\n[Network]\nAddress=@IP@/24\nGateway=@GW@\nDNS=@GW@\n' > "$R/etc/systemd/network/10-awto-10g.network"
cd "$R/lib/systemd/system"
n=0
for f in unifi*.service ucore*.service ucs-*.service uid-agent.service uos-agent.service udapi*.service udcd*.service ulp*.service ubnd.service uhwd.service ustd*.service sfpd.service ds.service ds-monitor.service ms.service msp.service msr.service mst.service msx-monitor.service ai-feature*.service ble-http-transport.service gw-ip-monitor.service nginx.service postgresql*.service avahi-daemon.service earlyoom.service rsync.service ubnt-dpkg-*.service ubnt-neighbor.service ubnt-sta-ht.service uled-bootup-check.service mdadm*.service cryptdisks*.service; do
  [ -e "$f" ] || continue; ln -sf /dev/null "$R/etc/systemd/system/$f"; n=$((n+1))
done
echo "masked $n units"
printf '[Unit]\nDescription=stock-kernel test watchdog: reboot after 40 min (back to Fedora)\n\n[Timer]\nOnBootSec=40min\n\n[Install]\nWantedBy=timers.target\n' > "$R/etc/systemd/system/awto-stock-watchdog.timer"
printf '[Unit]\nDescription=reboot (stock-kernel test watchdog)\n\n[Service]\nType=oneshot\nExecStart=/bin/systemctl reboot --force\n' > "$R/etc/systemd/system/awto-stock-watchdog.service"
mkdir -p "$R/etc/systemd/system/timers.target.wants"
ln -sf /etc/systemd/system/awto-stock-watchdog.timer "$R/etc/systemd/system/timers.target.wants/awto-stock-watchdog.timer"
rm -f "$R/etc/resolv.conf"; ln -s /run/systemd/resolve/stub-resolv.conf "$R/etc/resolv.conf"
echo "prep done: $(ls $R/etc/systemd/system | wc -l) entries in etc/systemd/system"
"""


def main() -> int:
    if not PUBKEY.exists():
        log(f"missing {PUBKEY}")
        return 2
    subprocess.run(
        ["scp", "-q", str(PUBKEY), f"root@{WOOMERA_DATA_IP}:/tmp/stock4.pub"],
        check=True,
    )
    r = subprocess.run(
        ["ssh", f"root@{WOOMERA_DATA_IP}", "bash -s"],
        input=REMOTE.replace("@MAC@", MAC_10G)
        .replace("@IP@", WOOMERA_DATA_IP)
        .replace("@GW@", LAN_GATEWAY),
        text=True,
        capture_output=True,
        timeout=SSH_TIMEOUT_S,
    )
    log((r.stdout + r.stderr)[-2500:])
    print((r.stdout + r.stderr)[-1500:])
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
