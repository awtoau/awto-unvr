# Discriminate: does a NAK (non-responding address) BEHIND the mux wedge the DW
# controller, or is ch0 specifically bad? Fresh stock U-Boot (watchdog reset armed
# by caller). On known-good ch3: read existing adt7475 0x2e (should ACK 0x75), then
# read NONEXISTENT 0x55 (NAK). If 0x55 wedges (deselect/main-bus fail after) => the
# fault is DW NAK-handling behind the mux (software). If 0x55 NAKs cleanly and the
# bus survives => ch0 is specifically bad.
proc r {c} { send $c; catch {puts ">> $c\n[expect "ALPINE_UBNT_NAS_ALL>" 6]"} }
set ok 0
for {set i 0} {$i < 1200} {incr i} { send_raw ESC; if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set ok 1; break } }
if {!$ok} { puts "NO-STOCK"; return }
puts "=== ch3: existing then nonexistent ==="
r "i2c mw 0x71 0 8 1"
r "i2c md 0x2e 3d 1"
r "i2c md 0x55 0 1"
r "i2c mw 0x71 0 0 1"
r "i2c md 0x21 0 1"
puts "=== DONE ==="
