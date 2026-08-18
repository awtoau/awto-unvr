# Clean behind-mux test in STOCK U-Boot: SoC-reset for a fresh bus, catch stock,
# then select mux ch0 and read ONLY the SFP EEPROM 0x50 (register-addressable) —
# never touch the s35390a 0x30 (it holds the bus if mis-accessed). Tells us whether
# the SFP EEPROM (what al_eth polls) is reachable through the mux at all.
proc r {c} { send $c; catch {puts ">> $c\n[expect "ALPINE_UBNT_NAS_ALL>" 6]"} }
send_raw CR
expect "ALPINE_UBNT_NAS_ALL>" 8
send "reset"
set ok 0
for {set i 0} {$i < 800} {incr i} {
    send_raw ESC
    if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set ok 1; break }
}
if {!$ok} { puts "NO-STOCK"; return }
puts "=== fresh stock, clean ch0 SFP test ==="
r "i2c dev 0"
r "i2c mw 0x71 0 1 1"
r "i2c md 0x50 0 20"
r "i2c mw 0x71 0 0 1"
puts "=== DONE ==="
