# ZFS pool `p3` (USB SSDs) — read-only inspection

## Pool layout (zdb -l, read-only, 2026-11-28)
- name `p3`, pool GUID 14567611814353409266, on-disk v5000, label txg 6182223 (newest uberblock txg 6189093)
- last imported by hostname `truenas`, hostid 139646277
- vdev tree: ONE top-level **raidz1**, 5 children, ashift 12, asize 5000989245440 (children use partuuid paths)
- present on woomera (2 of 5): guid 16412366758258455131 = Samsung 860 EVO M.2 S5GENJ0NB03022H part1; guid 9324376859468008244 = Samsung 860 EVO S3YBNB0M621787E part1
- missing (3): 3318863734986173148, 3850581713542755871, 14683741380490037633
- raidz1 needs >=4 of 5 members: **import impossible until 2 more members are online**, regardless of kernel module
- features_for_read: hole_birth, embedded_data, vdev_zaps_v2 — all supported by OpenZFS 2.4
- `sde3` (boot SSD, S21BNXAG501763W) is the TrueNAS `boot-pool`, unrelated

## Userland
- `scripts/zfs-userland-stage.py` copies zdb/zpool/zfs + lib closure (incl. Debian ld.so) from the extracted TrueNAS rootfs (`tmp/truenas-install/rootfs`, OpenZFS 2.4.0); rsync to woomera `/opt/zfs`
- run: `/opt/zfs/lib/ld-linux-aarch64.so.1 --library-path /opt/zfs/lib /opt/zfs/sbin/zdb -l <by-id part1>`

## Kernel module
- OpenZFS 2.4.4 META Linux-Maximum 7.2, master 7.3; woomera kernel is 7.4 (not supported upstream yet, not tried)
- Not needed until all required members are online; then import via TrueNAS profile (6.18 + matching kmod) or port module to 7.4
