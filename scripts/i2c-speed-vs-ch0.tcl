# Is the ch0/s35390a failure a SPEED/timing (code) issue, not the chip?
# Our U-Boot i2c defaults to 400 kHz (s35390a max). The ch3 reads that worked were at
# 100 kHz (stock). Fresh bus (watchdog reset by caller): read adt7475 at 400k, then read
# the s35390a at progressively LOWER speeds. If ch0 responds at a lower speed -> code/timing.
proc r {c} { send $c; catch {puts ">> $c\n[expect "unvr#" 8]"} }
set ok 0
for {set i 0} {$i < 1200} {incr i} { send_raw ESC; if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set ok 1; break } }
if {!$ok} { puts "NO-STOCK"; return }
send "setenv ipaddr 192.168.25.140"; expect "ALPINE_UBNT_NAS_ALL>" 6
send "setenv serverip 192.168.25.145"; expect "ALPINE_UBNT_NAS_ALL>" 6
send "tftpboot 0x1100000 u-boot-chainload.bin"; catch {expect "Bytes transferred" 30}
expect "ALPINE_UBNT_NAS_ALL>" 6
send "go 0x1100000"
if {[catch {expect "unvr#" 15}]} { puts "NO-UNVR"; return }
puts "=== speed vs ch0 test ==="
r "i2c dev 0"
r "i2c speed"
r "i2c speed 100000"
r "i2c mw 0x71 0 1 1"
r "i2c md 0x30 0.0 1"
r "i2c mw 0x71 0 0 1"
r "i2c md 0x21 0 1"
puts "=== DONE ==="
