# From our awto-nas#: SP805-reset -> box reboots -> auto-chainloads our U-Boot ->
# ONE clean ping to test the DMA fix on a FRESH UDMA (no stacked wedged state).
# SP805 @0xfd88c000: Lock@0xC00, Load@0x000, Control@0x008 (INTEN|RESEN=0x3).
# serverip below is THIS dev host's IP and drifts with its DHCP lease (standalone
# script, no dev.py command drives it) - if the ping fails, check `ip -4 addr` on
# this host and update serverip.
send_raw CR; expect "awto-nas#" 8
send "mw 0xfd88cc00 0x1acce551"; expect "awto-nas#" 6
send "mw 0xfd88c000 0x00002000"; expect "awto-nas#" 6
send "mw 0xfd88c008 0x00000003"
puts "SP805-ARMED — waiting for auto-chainload back to awto-nas#"
if {[catch {expect "awto-nas#" 75}]} { puts "NO-UNVR (auto-chainload didn't complete)"; return }
send "setenv ipaddr 192.168.25.140"; expect "awto-nas#" 6
send "setenv serverip 192.168.25.145"; expect "awto-nas#" 6
puts "=== single ping (fresh UDMA) ==="
send "ping 192.168.25.145"
catch {expect "awto-nas#" 20}
puts "=== PING DONE ==="
