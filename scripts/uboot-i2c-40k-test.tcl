# Test the software/timing hypothesis: does behind-mux i2c work at LOWER speed?
# Reset→catch stock U-Boot, force 40 kHz, select mux ch3, read adt7475 device+company
# ID (0x3d=0x75, 0x3e=0x41). If it reads clean at 40k (fails at 100k) => timing fault,
# fix = i2c clock-frequency / sbclk in the DTB. adt7475 is register-addressable + present.
proc r {c} { send $c; catch {puts ">> $c\n[expect "ALPINE_UBNT_NAS_ALL>" 6]"} }
# external watchdog reset is armed by the caller; ESC-catch the stock prompt.
set ok 0
for {set i 0} {$i < 1200} {incr i} { send_raw ESC; if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set ok 1; break } }
if {!$ok} { puts "NO-STOCK"; return }
puts "=== fresh stock; 100k baseline then 40k ==="
r "i2c dev 0"
r "i2c speed"
r "i2c mw 0x71 0 8 1"
r "i2c md 0x2e 3d 2"
r "i2c mw 0x71 0 0 1"
puts "--- now 40 kHz ---"
r "i2c speed 40000"
r "i2c mw 0x71 0 8 1"
r "i2c md 0x2e 3d 2"
r "i2c mw 0x71 0 0 1"
puts "=== DONE ==="
