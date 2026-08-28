# Redeploy a completely fresh Fedora rootfs onto the SSD, driven from a live
# Linux session: catch stock U-Boot (the caller already armed the SP805
# watchdog over SSH before invoking this) -> netboot the installer kernel
# (enhanced initramfs: mkfs.ext4+wget+tar) -> reformat sda2 -> stream the
# fresh rootfs tar + fresh module tree over HTTP -> unmount. Leaves the box
# sitting at the installer shell (no final reboot here - see
# docs/fedora-on-ssd.md for the next phase: flash the matching kernel into
# NAND, then power-cycle to boot the new persistent system).
#
# $SERVERIP and $HTTPPORT are injected by the calling dev.py command (this
# dev host's IP + the port serving fedora-rootfs-ea16.tar/modules-*.tar from
# tmp/rootfs-deploy/). Installer uImage+DTB must already be staged in
# tmp/tftp/ under the plain (non-fedora) ea16 7.1 names.

set found 0
for {set i 0} {$i < 400} {incr i} {
    send_raw ESC
    if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set found 1; break }
}
if {!$found} { puts "NO-STOCK-UBOOT"; exit 1 }
puts "AT-STOCK-UBOOT"

send "setenv ipaddr 192.168.25.140"; expect "ALPINE_UBNT_NAS_ALL>" 6
send "setenv serverip $SERVERIP"; expect "ALPINE_UBNT_NAS_ALL>" 6

# 22.8 MB installer uImage: same order of magnitude as the 18.5 MB gzip
# fedora image netboot.py budgets 40s for at typical tftp throughput here.
send "tftpboot 0x02000000 uImage-unvr-ea16-7.1"
if {[catch {expect "Bytes transferred" 40}]} { puts "TFTP-KERNEL-FAIL"; exit 2 }
expect "ALPINE_UBNT_NAS_ALL>" 6

# DTB is ~16 KB: sub-second at any real link speed, 15s is generous margin.
send "tftpboot 0x04078000 alpine-v2-ubnt-unvr-ea16-7.1.dtb"
if {[catch {expect "Bytes transferred" 15}]} { puts "TFTP-DTB-FAIL"; exit 2 }
expect "ALPINE_UBNT_NAS_ALL>" 6

send "setenv bootargs 'console=ttyS0,115200 sysid=ea16 ubnthal.sysid=ea16 reboot=warm rw iommu.passthrough=1 pci=pcie_bus_perf'"
expect "ALPINE_UBNT_NAS_ALL>" 6

send "bootm 0x02000000 - 0x04078000"
# Kernel decompress + al_* module load + PCI/SATA enum banner: bring-up
# kernel boots to a shell in a few seconds normally; 40s is generous margin.
if {[catch {expect "serial shell on ttyS0" 40}]} { puts "NO-INSTALLER-SHELL"; exit 3 }
puts "AT-INSTALLER-SHELL"

send "export PS1='@@D@@'; echo READY"
if {[catch {expect "READY" 6}]} { puts "NO-SHELL-READY"; exit 4 }

# Interface name isn't guaranteed (no systemd predictable-naming in this
# minimal initramfs) - discover it instead of hardcoding.
send {IFACE=$(ip -o link show | awk -F': ' '$2!="lo"{print $2; exit}'); echo IFACE=$IFACE}
if {[catch {expect "IFACE=" 6}]} { puts "NO-IFACE"; exit 5 }

# DHCP over LAN is normally sub-5s; 20s is 4x margin.
send {udhcpc -i $IFACE -n -q 2>&1; echo DHCP_DONE}
if {[catch {expect "DHCP_DONE" 20}]} { puts "DHCP-TIMEOUT"; exit 5 }

# AHCI async probe typically resolves in a few seconds; poll 20x1s (~4x
# margin over that) then fail rather than hang indefinitely.
send {for i in $(seq 1 20); do grep -q sda2 /proc/partitions && break; sleep 1; done; grep -q sda2 /proc/partitions && echo SDA2_OK || echo SDA2_MISSING}
if {[catch {expect "SDA2_OK" 25}]} { puts "NO-SDA2"; exit 6 }
puts "SDA2-PRESENT"

# mkfs.ext4 on a ~931GB partition with lazy_itable_init (e2fsprogs default)
# is normally under 10s; 60s is generous margin, not a real multi-minute op.
send "mkfs.ext4 -F -L unvr-root /dev/sda2; echo MKFS_RC=\$?"
if {[catch {expect "MKFS_RC=0" 60}]} { puts "MKFS-FAIL"; exit 7 }
puts "MKFS-OK"

send "mkdir -p /mnt/root && mount /dev/sda2 /mnt/root; echo MOUNT_RC=\$?"
if {[catch {expect "MOUNT_RC=0" 10}]} { puts "MOUNT-FAIL"; exit 8 }

# 1.5 GB rootfs tar over LAN: even at a conservative 20 MB/s that's ~75s;
# 180s gives ~2.4x margin given this box's LAN has shown flakiness this
# session (see #163 clone retries).
send "wget -O - http://$SERVERIP:$HTTPPORT/fedora-rootfs-ea16.tar | tar -x -C /mnt/root; echo WGETROOTFS_RC=\$?"
if {[catch {expect "WGETROOTFS_RC=0" 180}]} { puts "ROOTFS-STREAM-FAIL"; exit 9 }
puts "ROOTFS-STREAMED"

# 21 MB module tree tar: trivial at any real link speed, 60s generous.
send "mkdir -p /mnt/root/lib/modules && wget -O - http://$SERVERIP:$HTTPPORT/modules-7.1.8-dirty.tar | tar -x -C /mnt/root/lib/modules; echo WGETMOD_RC=\$?"
if {[catch {expect "WGETMOD_RC=0" 60}]} { puts "MODULES-STREAM-FAIL"; exit 10 }
puts "MODULES-STREAMED"

send "echo woomera > /mnt/root/etc/hostname; sync; echo HOSTNAME_SET"
if {[catch {expect "HOSTNAME_SET" 10}]} { puts "HOSTNAME-FAIL"; exit 11 }

send "umount /mnt/root; sync; echo UMOUNT_RC=\$?"
if {[catch {expect "UMOUNT_RC=0" 15}]} { puts "UMOUNT-FAIL"; exit 12 }

puts "DEPLOY-COMPLETE"
