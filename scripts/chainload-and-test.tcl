# ESC-catch stock (caller arms watchdog), chainload our U-Boot, then: read the
# adt7475 monitor on mux ch3, and enumerate PCIe (internal special + external xHCI).
# $SERVERIP is injected by cmd_chainload (scripts/_net.py detect_server_ip(), fresh
# each run). Not standalone-safe: running this directly (not via ./dev.py chainload)
# leaves $SERVERIP unset.
proc r {c} { send $c; catch {puts ">> $c\n[expect "awto-nas#" 10]"} }
set ok 0
for {set i 0} {$i < 1200} {incr i} { send_raw ESC; if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set ok 1; break } }
if {!$ok} { puts "NO-STOCK"; return }
send "setenv ipaddr 192.168.25.140";   expect "ALPINE_UBNT_NAS_ALL>" 6
send "setenv serverip $SERVERIP"; expect "ALPINE_UBNT_NAS_ALL>" 6
send "tftpboot 0x1100000 u-boot-chainload.bin"; catch {expect "Bytes transferred" 30}
expect "ALPINE_UBNT_NAS_ALL>" 6
send "go 0x1100000"
if {[catch {expect "awto-nas#" 15}]} { puts "NO-UNVR"; return }
puts "=== adt7475 monitor (ch3) ==="
r "i2c dev 0"
r "i2c mw 0x71 0 8 1"
r "i2c md 0x2e 3d 2"
r "i2c md 0x2e 20 10"
r "i2c mw 0x71 0 0 1"
puts "=== PCIe: internal (special) + external (xHCI) ==="
r "pci enum"
r "pci"
puts "=== DONE ==="
