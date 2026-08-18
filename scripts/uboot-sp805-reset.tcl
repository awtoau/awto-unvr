# Force a SoC reset from OUR U-Boot (which has no sysreset driver) by arming the
# SP805 watchdog directly via mw. Recovers a box stuck at unvr#.
# SP805 @0xfd88c000: Load@0x000, Control@0x008 (INTEN|RESEN=0x3), Lock@0xC00.
send_raw CR
expect "unvr#" 8
send "mw 0xfd88cc00 0x1acce551"; expect "unvr#" 6
send "mw 0xfd88c000 0x00002000"; expect "unvr#" 6
send "mw 0xfd88c008 0x00000003"; expect "unvr#" 6
puts "SP805-ARMED — expecting SoC reset -> stock U-Boot"
if {[catch {expect "ALPINE_UBNT_NAS_ALL" 25}]} { puts "NO-RESET (may need power-cycle)"; exit 1 }
puts "RESET-OK — at stock U-Boot, will autoboot Fedora"
