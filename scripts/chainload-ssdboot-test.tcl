# At stock U-Boot: chainload ours, let it AUTOBOOT off the SSD (canary not set).
send_raw CR
expect "ALPINE_UBNT_NAS_ALL>" 8
send "setenv ipaddr 192.168.25.140";   expect "ALPINE_UBNT_NAS_ALL>" 8
send "setenv serverip 192.168.25.145"; expect "ALPINE_UBNT_NAS_ALL>" 8
send "tftpboot 0x1100000 u-boot-chainload.bin"; expect "Bytes transferred" 30
expect "ALPINE_UBNT_NAS_ALL>" 8
send "go 0x1100000"
if {[catch {expect "Starting kernel" 70}]} { puts "NO-KERNEL (see transcript)"; } else { puts "KERNEL-STARTING-FROM-SSD" }
if {[catch {expect "login:" 120}]} { puts "NO-LOGIN"; } else { puts "FEDORA-UP-FROM-SSD" }
