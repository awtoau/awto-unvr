send_raw CR
expect "unvr#" 8
send "reset"
set found 0
for {set i 0} {$i < 400} {incr i} { send_raw ESC; if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set found 1; break } }
if {!$found} { puts "NO-STOCK"; exit }
send "setenv ipaddr 192.168.25.140";   expect "ALPINE_UBNT_NAS_ALL>" 8
send "setenv serverip 192.168.25.145"; expect "ALPINE_UBNT_NAS_ALL>" 8
send "tftpboot 0x1100000 u-boot-chainload.bin"; expect "Bytes transferred" 30
expect "ALPINE_UBNT_NAS_ALL>" 8
send "go 0x1100000"
if {[catch {expect "unvr#" 15}]} { puts "NO-NEW-PROMPT"; exit }
puts "AT-NEW-UBOOT"
send "gpio status -a"; expect "unvr#" 25
puts "DONE"
