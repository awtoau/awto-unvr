# Does a DRAM word survive a warm reset? (gates BOOTCOUNT_RAM + memory canary)
# At stock U-Boot: write magic, reset, re-catch, read it back.
send_raw CR
expect "ALPINE_UBNT_NAS_ALL>" 8
send "mw 0x10000000 0xb0075709"; expect "ALPINE_UBNT_NAS_ALL>" 8
send "md 0x10000000 1";          expect "ALPINE_UBNT_NAS_ALL>" 8
puts "MAGIC-WRITTEN"
send "reset"
set found 0
for {set i 0} {$i < 400} {incr i} {
    send_raw ESC
    if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set found 1; break }
}
if {!$found} { puts "NO-RECATCH"; exit 1 }
puts "RECAUGHT-AFTER-RESET"
send "md 0x10000000 1"; expect "ALPINE_UBNT_NAS_ALL>" 8
puts "DRAM-SURVIVE-CHECK-DONE (look for b0075709 above = survived)"
