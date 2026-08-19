# Undo linux-to-autochain.tcl: stock U-Boot's bootcmd currently auto-chainloads
# into our custom awto-nas# U-Boot (via tftp u-boot-chainload.bin + go). Revert
# it to the direct-NAND Fedora boot that was already verified working for days
# before that override (docs/fedora-on-ssd.md PERSISTENT, 2026-08-17) — kernel
# and DTB are untouched in NAND, this only restores the env pointing at them.
#
# Catches stock BEFORE its bootcmd fires (so it never runs "run bootchain"),
# reads the current value first (logged, for the record), writes the minimal
# replacement, reads it back to confirm, then test-boots it right here so we
# don't leave the box in an unverified state pending a future reset.
#   ./dev.py console-tcl scripts/revert-autochain-to-fedora.tcl
send_raw CR
set found 0
for {set i 0} {$i < 400} {incr i} {
    send_raw ESC
    if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set found 1; break }
}
if {!$found} { puts "NO-STOCK-UBOOT — bootchain may have already fired; aborting, nothing changed"; exit 1 }
puts "AT-STOCK-UBOOT"

send "printenv bootcmd"
expect "ALPINE_UBNT_NAS_ALL>" 8
puts "\n^^ CURRENT bootcmd (before change, for the record) ^^\n"

send "setenv bootcmd 'nand read 0x02000000 0x1300000 0x1200000; nand read 0x04078000 0x2800000 0x20000; bootm 0x02000000 - 0x04078000'"
expect "ALPINE_UBNT_NAS_ALL>" 8
send "setenv bootargs 'console=ttyS0,115200 root=PARTUUID=dcdc291e-9956-48cd-9d7c-48219877881a rootfstype=ext4 rw rootwait selinux=0 panic=15 reboot=cold'"
expect "ALPINE_UBNT_NAS_ALL>" 8
send "saveenv"
expect "done" 15
puts "SAVEENV-DONE"

send "printenv bootcmd"
expect "ALPINE_UBNT_NAS_ALL>" 8
puts "\n^^ NEW bootcmd (after change — verify it reads nand read .../bootm) ^^\n"
send "printenv bootargs"
expect "ALPINE_UBNT_NAS_ALL>" 8
puts "\n^^ NEW bootargs (verify PARTUUID=dcdc291e...) ^^\n"

puts "TEST-BOOTING NOW to confirm the reverted env actually reaches Fedora..."
send "boot"
expect "Starting kernel" 60
puts "\nSTARTING-KERNEL-OK"
expect "login:" 150
puts "\nFEDORA-LOGIN-REACHED — revert confirmed end-to-end"
