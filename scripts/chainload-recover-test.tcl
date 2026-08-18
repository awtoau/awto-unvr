# Chainload our U-Boot (watchdog reset armed by caller), then test the DesignWare
# i2c HARDWARE SDA-stuck-recovery on the wedged ch0 bus.
#   DW regs @ fd880000: IC_ENABLE 0x6c, IC_ENABLE_STATUS 0x9c,
#   IC_COMP_PARAM_1 0xf4, IC_COMP_VERSION 0xf8.
#   IC_ENABLE[3] = SDA_STUCK_RECOVERY_ENABLE (v2.00a+); IC_ENABLE_STATUS[3]=recovered.
proc r {c} { send $c; catch {puts ">> $c\n[expect "unvr#" 8]"} }
# ESC-catch stock
set ok 0
for {set i 0} {$i < 1200} {incr i} { send_raw ESC; if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set ok 1; break } }
if {!$ok} { puts "NO-STOCK"; return }
send "setenv ipaddr 192.168.25.140"; expect "ALPINE_UBNT_NAS_ALL>" 6
send "setenv serverip 192.168.25.145"; expect "ALPINE_UBNT_NAS_ALL>" 6
send "tftpboot 0x1100000 u-boot-chainload.bin"; catch {expect "Bytes transferred" 30}
expect "ALPINE_UBNT_NAS_ALL>" 6
send "go 0x1100000"
if {[catch {expect "unvr#" 15}]} { puts "NO-UNVR"; return }
puts "=== at our U-Boot; DW IP id + wedge + hw recovery ==="
r "i2c dev 0"
r "md.l 0xfd8800f8 1"
r "md.l 0xfd8800f4 1"
r "i2c mw 0x71 0 1 1"
r "i2c md 0x30 0.0 1"
r "md.l 0xfd88006c 1"
r "mw.l 0xfd88006c 0x9"
r "md.l 0xfd88009c 1"
r "md.l 0xfd88006c 1"
r "i2c md 0x21 0 1"
puts "=== DONE ==="
