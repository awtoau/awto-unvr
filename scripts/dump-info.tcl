# Catch the box at STOCK U-Boot and DUMP every info command. Read-only — no
# tftp/go/saveenv. printenv first (the suspect env). Simple expect form only.
set ok 0
for {set i 0} {$i < 2400} {incr i} { send_raw ESC; if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set ok 1; break } }
if {!$ok} { puts "NO-STOCK (box not booting through stock in the window)"; return }
proc r {c} { send $c; catch {puts ">>> $c"; puts [expect "ALPINE_UBNT_NAS_ALL>" 8]} }
r "printenv"
r "version"
r "bdinfo"
r "mii device"
r "i2c dev 0"
r "i2c probe"
r "pci"
puts "=== DUMP DONE ==="
