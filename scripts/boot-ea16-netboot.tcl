# Netboot the plain (non-Fedora) 7.1 ea16 build - self-contained embedded
# initramfs, no NAND flash, no dependency on the (currently module-mismatched)
# SSD rootfs. Assumes we're already sitting at the stock U-Boot prompt
# (bootdelay=-1, no reset needed).
# $IPADDR/$SERVERIP are injected by `./dev.py console-tcl` (scripts/_net.py,
# serverip detected fresh each run).
send_raw CR
expect "ALPINE_UBNT_NAS_ALL>" 8
send "setenv ipaddr $IPADDR"; expect "ALPINE_UBNT_NAS_ALL>" 6
send "setenv serverip $SERVERIP"; expect "ALPINE_UBNT_NAS_ALL>" 6
send "tftpboot 0x02000000 uImage-unvr-ea16-7.1"
expect "Bytes transferred" 30
send "tftpboot 0x04078000 alpine-v2-ubnt-unvr-ea16-7.1.dtb"
expect "Bytes transferred" 15
send "bootm 0x02000000 - 0x04078000"
expect "Starting kernel" 30
puts "\nSTARTING-KERNEL-OK"
expect "login:" 90
puts "\nEA16-LOGIN-REACHED"
