# Dump stock U-Boot's WORKING eth state for comparison against our hanging eth.
# Flow: reset from our U-Boot -> ESC-spam to catch stock at ALPINE_UBNT_NAS_ALL>
# -> dump adapter coherency/routing cfg (config space, SAFE) BEFORE eth use ->
# activate stock's eth with a real tftp (loads to 0x10000000, does NOT chainload)
# -> dump adapter cfg + UDMA M2S ring state AFTER a working TX/RX.
# UDMA BAR assumed 0xfe000000 (same device 1c36:0001; BAR0 dumped to confirm).
# tftpd must serve u-boot-chainload.bin; firewalld trusted. Box left at stock.
# serverip below is THIS dev host's IP and drifts with its DHCP lease (standalone
# script, no dev.py command drives it, so it can't get an injected value like
# scripts/uboot-test.tcl's $SERVERIP does) - if tftpboot retries forever, check
# `ip -4 addr` on this host and update serverip.
send "reset"
set ok 0
for {set i 0} {$i < 3000} {incr i} { send_raw ESC; if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set ok 1; break } }
if {!$ok} { puts "NO-STOCK (not caught in window)"; return }
send "setenv ipaddr 192.168.25.140";   expect "ALPINE_UBNT_NAS_ALL>" 6
send "setenv serverip 192.168.25.145"; expect "ALPINE_UBNT_NAS_ALL>" 6
puts "=== STOCK eth 00:01.0 BEFORE eth use ==="
send "pci display 0.1.0 0x10 4";  catch {expect "ALPINE_UBNT_NAS_ALL>" 8}
send "pci display 0.1.0 0x110 2"; catch {expect "ALPINE_UBNT_NAS_ALL>" 8}
send "pci display 0.1.0 0x220 1"; catch {expect "ALPINE_UBNT_NAS_ALL>" 8}
send "pci display 0.1.0 0x240 1"; catch {expect "ALPINE_UBNT_NAS_ALL>" 8}
puts "=== STOCK: activate eth via WORKING tftp ==="
send "tftpboot 0x10000000 u-boot-chainload.bin"; catch {expect "Bytes transferred" 40}
catch {expect "ALPINE_UBNT_NAS_ALL>" 8}
puts "=== STOCK eth 00:01.0 AFTER working tftp (adapter cfg + UDMA ring) ==="
send "pci display 0.1.0 0x110 2"; catch {expect "ALPINE_UBNT_NAS_ALL>" 8}
send "pci display 0.1.0 0x220 1"; catch {expect "ALPINE_UBNT_NAS_ALL>" 8}
send "md.l 0xfe000200 1"; catch {expect "ALPINE_UBNT_NAS_ALL>" 8}
send "md.l 0xfe001034 1"; catch {expect "ALPINE_UBNT_NAS_ALL>" 8}
send "md.l 0xfe00103c 2"; catch {expect "ALPINE_UBNT_NAS_ALL>" 8}
send "md.l 0xfe00104c 1"; catch {expect "ALPINE_UBNT_NAS_ALL>" 8}
puts "=== DONE (box at stock prompt; no chainload) ==="
