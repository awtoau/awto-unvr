# Test the datasheet-mandated s35390a init: write RESET (STATUS1 B7=1) as the FIRST
# access on ch0. Per the S-35390A datasheet, after POC/BLD the chip is in "indefinite
# status" until RESET is written — reads (which we tried before) fail, but the RESET
# WRITE may still be accepted. 0xC0 = RESET(0x80)|24H(0x40), matching Linux rtc-s35390a.
# Chainload our U-Boot first (watchdog reset armed by caller).
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
puts "=== at our U-Boot; s35390a RESET-write test on ch0 ==="
r "i2c dev 0"
r "i2c mw 0x71 0 1 1"
r "i2c mw 0x30 0.0 c0 1"
r "i2c md 0x30 0.0 1"
r "i2c mw 0x71 0 0 1"
r "i2c md 0x21 0 1"
puts "=== DONE ==="
