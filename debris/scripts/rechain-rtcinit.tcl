send_raw CR
catch {expect "unvr#" 4}
send "reset"
set ok 0
for {set i 0} {$i < 1200} {incr i} { send_raw ESC; if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set ok 1; break } }
if {!$ok} { puts "NO-STOCK"; return }
send "setenv ipaddr 192.168.25.140";   expect "ALPINE_UBNT_NAS_ALL>" 6
send "setenv serverip 192.168.25.145"; expect "ALPINE_UBNT_NAS_ALL>" 6
send "tftpboot 0x1100000 u-boot-chainload.bin"; catch {expect "Bytes transferred" 30}
expect "ALPINE_UBNT_NAS_ALL>" 6
send "go 0x1100000"
catch {puts [expect "unvr#" 25]}
puts "=== DONE ==="
