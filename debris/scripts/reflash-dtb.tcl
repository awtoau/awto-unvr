# Reflash ONLY the Fedora DTB into NAND @0x2800000 (kernel untouched, no bootcmd
# change, no saveenv — minimal + safe). Caller arms a watchdog reset; this ESC-
# catches stock U-Boot, tftp's the new DTB, erases+writes the dtb block, resets.
proc r {c n t} { send $c; catch {puts ">> $c\n[expect $n $t]"} }
set ok 0
for {set i 0} {$i < 1200} {incr i} { send_raw ESC; if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set ok 1; break } }
if {!$ok} { puts "NO-STOCK"; return }
send "setenv ipaddr 192.168.25.140";   expect "ALPINE_UBNT_NAS_ALL>" 6
send "setenv serverip 192.168.25.145"; expect "ALPINE_UBNT_NAS_ALL>" 6
r "tftpboot 0x04078000 alpine-v2-ubnt-unvr-ea16-7.1-fedora.dtb" "Bytes transferred" 30
expect "ALPINE_UBNT_NAS_ALL>" 6
r "nand erase 0x2800000 0x40000" "OK" 15
expect "ALPINE_UBNT_NAS_ALL>" 6
r "nand write 0x04078000 0x2800000 0x40000" "OK" 15
expect "ALPINE_UBNT_NAS_ALL>" 6
puts "=== DTB written; resetting to Fedora ==="
send "reset"
catch {expect "woomera login" 120}
puts "=== BOOTED ==="
