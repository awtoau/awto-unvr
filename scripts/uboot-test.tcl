# `./dev.py uboot-test` drives this: put the FRESH build on the box and stop for
# hands-on testing. From our unvr#: arm SP805 watchdog (SoC reset) -> ESC-catch
# stock U-Boot -> tftp our U-Boot to 0x1100000 -> go -> STOP at unvr#. No
# auto-tests — the live prompt is the user's to drive (i2c scan / ping / serdes /
# ddr bist). SP805 @0xfd88c000: Lock@0xC00, Load@0x000, Control@0x008 (INTEN|RESEN).
send_raw CR
if {[catch {expect "unvr#" 8}]} { puts "NOT-AT-UNVR (get the box to our unvr# or stock, then re-run)"; return }
send "mw 0xfd88cc00 0x1acce551"; expect "unvr#" 6
send "mw 0xfd88c000 0x00002000"; expect "unvr#" 6
send "mw 0xfd88c008 0x00000003"
puts "SP805-ARMED — catching stock U-Boot"
set ok 0
for {set i 0} {$i < 1200} {incr i} { send_raw ESC; if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set ok 1; break } }
if {!$ok} { puts "NO-STOCK (SP805 reset didn't reach stock; a power-cycle may be needed)"; return }
send "setenv ipaddr 192.168.25.140";   expect "ALPINE_UBNT_NAS_ALL>" 6
send "setenv serverip 192.168.25.145"; expect "ALPINE_UBNT_NAS_ALL>" 6
send "tftpboot 0x1100000 u-boot-chainload.bin"; catch {expect "Bytes transferred" 30}
expect "ALPINE_UBNT_NAS_ALL>" 6
send "go 0x1100000"
if {[catch {expect "unvr#" 15}]} { puts "NO-UNVR (go failed — check the tftp'd image)"; return }
puts "=== LIVE at unvr# — fresh build on hardware, box is yours to test ==="
