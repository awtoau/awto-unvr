# Is the ch0 "wedge" a STUCK DW CONTROLLER (recoverable) or a physical line hold?
# The U-Boot DW "Timed out waiting for bus" checks IC_STATUS MA/TFE (controller state),
# NOT the physical SDA/SCL. So a stale-FIFO/aborted transfer can look like a wedge.
# Test: wedge ch0, then toggle IC_ENABLE (0x6c: 0 = disable+flush, 1 = enable) and see if
# the MAIN bus (0x21) recovers WITHOUT a SoC reset. If yes -> controller state, fixable.
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
puts "=== DW re-init recovery test ==="
r "i2c dev 0"
r "i2c md 0x21 0 1"
r "i2c mw 0x71 0 1 1"
r "i2c md 0x30 0.0 1"
r "i2c md 0x21 0 1"
r "mw.l 0xfd88006c 0"
r "mw.l 0xfd88006c 1"
r "i2c md 0x21 0 1"
puts "=== DONE ==="
