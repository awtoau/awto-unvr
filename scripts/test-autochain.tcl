# At stock U-Boot: reset and wait — NO ESC spam. Stock bootcmd should auto-chainload
# our U-Boot (bootcount + fallback), landing at unvr#.
send_raw CR
expect "ALPINE_UBNT_NAS_ALL>" 8
send "reset"
if {[catch {expect "unvr#" 50}]} { puts "FAIL - did not reach our U-Boot"; exit 1 }
puts "AUTO-CHAINLOAD-OK (landed at our U-Boot, no ESC)"
