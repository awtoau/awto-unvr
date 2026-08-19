# Boot Fedora from OUR custom U-Boot (awto-nas#), reading kernel+DTB off the
# SSD ext4 root via AHCI/SCSI (no NAND driver in this build) — NOT via stock
# U-Boot's NAND bootcmd. Stock U-Boot's env/bootcmd is never touched.
#   ./dev.py console-tcl scripts/boot-fedora-from-nas.tcl
send_raw CR
expect "awto-nas#" 8
send "ext4load scsi 0:2 0x02000000 /boot/uImage-unvr-ea16-7.1-fedora-gz"
expect "awto-nas#" 20
send "ext4load scsi 0:2 0x04078000 /boot/alpine-v2-ubnt-unvr-ea16-7.1-fedora.dtb"
expect "awto-nas#" 10
send "setenv bootargs 'console=ttyS0,115200 root=PARTUUID=dcdc291e-9956-48cd-9d7c-48219877881a rootfstype=ext4 rw rootwait selinux=0 panic=15 reboot=cold'"
expect "awto-nas#" 6
send "bootm 0x02000000 - 0x04078000"
expect "Starting kernel" 30
puts "\nSTARTING-KERNEL-OK"
expect "login:" 150
puts "\nFEDORA-LOGIN-REACHED"
