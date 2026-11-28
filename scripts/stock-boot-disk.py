#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Turn woomera's unused Seagate (ata5, `single`) into the stock-kernel boot disk (#381). DESTROYS `single`.

  ./dev.py stock-boot-disk --i-accept-wipe     # detach `single`, GPT + ext4, copy the prepared Debian rootfs

- Dan 2026-11-28: the disk is unused (160 KiB, empty) - use it for the stock boot instead of the SSD.
- Refuses unless the by-id serial is WCT3LCS1 and /srv/single holds nothing but the empty dir.
- Detaches `single`: timers, mount, fstab, NFS export, SMB share (backups next to each file, *.pre-stock4).
- p1 64 GiB ext4 (no orphan_file/metadata_csum_seed: the 4.19 kernel must mount it), p2 = rest, unformatted.
- Source rootfs: /var/lib/stock4/root (`./dev.py stock-boot-prep`). Marker file /STOCK4 tells the init.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from _net import WOOMERA_DATA_IP  # noqa: E402
from _repo import make_log  # noqa: E402

log = make_log("stock-boot-disk")
SERIAL = "WCT3LCS1"
# mkfs of 64 GiB + copy of 2.4 GB at HDD speed (~150 MB/s) ~ 1 min; 300 s stall bound.
SSH_TIMEOUT_S = 300

REMOTE = r"""
set -eu
D=/dev/disk/by-id/ata-ST6000VX001-2BD186_WCT3LCS1
test -b "$D"
[ "$(lsblk -dno SERIAL "$D")" = "WCT3LCS1" ] || { echo "FATAL: serial mismatch"; exit 1; }
test -d /var/lib/stock4/root/sbin
# nothing but the empty top dir on single
n=$(find /srv/single -mindepth 1 2>/dev/null | wc -l)
[ "$n" = 0 ] || { echo "FATAL: /srv/single not empty ($n entries)"; exit 1; }
for t in awto-filehash@single.timer awto-dedupe@single.timer; do systemctl disable --now $t 2>&1 | tail -1 || true; done
systemctl stop awto-filehash@single.service awto-dedupe@single.service 2>/dev/null || true
cp -n /etc/fstab /etc/fstab.pre-stock4
sed -i 's|^\(UUID=d09b43c3-e8bf-4932-9d7b-9bb7933dc2b3 .*\)$|# detached for the stock-kernel disk (#381): \1|' /etc/fstab
cp -n /etc/exports.d/awto.exports /etc/exports.d/awto.exports.pre-stock4
sed -i 's|^\(/srv/single .*\)$|# detached (#381): \1|' /etc/exports.d/awto.exports
cp -n /etc/samba/smb.conf /etc/samba/smb.conf.pre-stock4
python3 - <<'PY'
import re
p='/etc/samba/smb.conf'
s=open(p).read()
s2=re.sub(r'\n\[single\]\n(?:[ \t]+.*\n|\n)*?(?=\n\[|\Z)', '\n', s)
open(p,'w').write(s2)
PY
testparm -s >/dev/null 2>&1 || { cp /etc/samba/smb.conf.pre-stock4 /etc/samba/smb.conf; echo 'FATAL: smb.conf invalid, restored'; exit 1; }
systemctl daemon-reload
umount /srv/single 2>&1 || true
mountpoint -q /srv/single && { echo "FATAL: still mounted"; exit 1; }
exportfs -ra
systemctl reload smb 2>&1 | tail -1 || true
wipefs -a "$D"
sfdisk "$D" <<'SF'
label: gpt
size=64GiB, type=0FC63DAF-8483-4772-8E79-3D69D8477DE4, name=stock4-root
type=0FC63DAF-8483-4772-8E79-3D69D8477DE4, name=free
SF
udevadm settle
P=${D}-part1
test -b "$P"
mkfs.ext4 -q -F -L stock4-root -O ^orphan_file,^metadata_csum_seed "$P"
mkdir -p /mnt/stock4
mount "$P" /mnt/stock4
rsync -aHAX --numeric-ids /var/lib/stock4/root/ /mnt/stock4/
touch /mnt/stock4/STOCK4
test -x /mnt/stock4/sbin/init
echo "PARTUUID: $(lsblk -no PARTUUID "$P")"
df -h /mnt/stock4 | tail -1
umount /mnt/stock4
lsblk -o NAME,SIZE,FSTYPE,LABEL,PARTUUID "$D"
"""


def main() -> int:
    if "--i-accept-wipe" not in sys.argv:
        print(__doc__)
        return 2
    r = subprocess.run(
        ["ssh", f"root@{WOOMERA_DATA_IP}", "bash -s"],
        input=REMOTE,
        text=True,
        capture_output=True,
        timeout=SSH_TIMEOUT_S,
    )
    out = (r.stdout + r.stderr)[-3000:]
    log(out)
    print(out)
    return r.returncode


if __name__ == "__main__":
    sys.exit(main())
