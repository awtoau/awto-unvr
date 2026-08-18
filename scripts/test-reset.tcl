# at stock U-Boot: chainload ours (BOOTDELAY=-1 -> lands at unvr#), then test `reset`.
send_raw CR
expect "ALPINE_UBNT_NAS_ALL>" 8
send "setenv ipaddr 192.168.25.140";   expect "ALPINE_UBNT_NAS_ALL>" 8
send "setenv serverip 192.168.25.145"; expect "ALPINE_UBNT_NAS_ALL>" 8
send "tftpboot 0x1100000 u-boot-chainload.bin"; expect "Bytes transferred" 30
expect "ALPINE_UBNT_NAS_ALL>" 8
send "go 0x1100000"
if {[catch {expect "unvr#" 15}]} { puts "NO-OUR-PROMPT"; exit }
puts "AT-OUR-UBOOT-PROMPT (no autoboot)"
send "reset"
if {[catch {expect "Boot: warm" 25}]} { puts "RESET-FAILED (hung?)"; exit }
puts "RESET-WORKED (SP805 -> SoC reset -> stock U-Boot)"
