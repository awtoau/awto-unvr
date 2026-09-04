# Catch a BOOTING/cycling box at stock U-Boot (sustained ESC spam), then tftp our
# U-Boot to 0x1100000 and `go`, stopping at awto-nas#. No SP805 reset — use this when
# the box is already rebooting on its own (vs uboot-test.tcl which resets from our
# awto-nas#). tftpd must serve u-boot-chainload.bin; firewalld must allow tftp.
# $SERVERIP is injected by cmd_chainload (scripts/_net.py detect_server_ip(), fresh
# each run): run via `./dev.py chainload scripts/chainload-catch.tcl`, not standalone
# (running this directly leaves $SERVERIP unset).
set ok 0
for {set i 0} {$i < 2400} {incr i} { send_raw ESC; if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set ok 1; break } }
if {!$ok} { puts "NO-STOCK (box not booting through stock in the window)"; return }
send "setenv ipaddr $IPADDR";   expect "ALPINE_UBNT_NAS_ALL>" 6
send "setenv serverip $SERVERIP"; expect "ALPINE_UBNT_NAS_ALL>" 6
send "tftpboot 0x1100000 u-boot-chainload.bin"; catch {expect "Bytes transferred" 40}
expect "ALPINE_UBNT_NAS_ALL>" 6
send "go 0x1100000"
# Our own U-Boot also autoboots (CONFIG_BOOTDELAY=2) - nothing sets the
# CANARY that would make it stay at the prompt on its own, so a passive
# expect races that 2s countdown and loses about half the time (falls
# through to Linux instead). Actively spam CR to interrupt it, same
# pattern as the stock ESC-catch above.
set ok2 0
for {set i 0} {$i < 20} {incr i} {
    send_raw CR
    if {[catch {expect "awto-nas#" 1}] == 0} { set ok2 1; break }
}
if {!$ok2} { puts "NO-UNVR (go failed / tftp failed)"; return }
puts "=== LIVE at awto-nas# — DMA-fix build on hardware ==="
