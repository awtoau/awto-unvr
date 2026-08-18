# STOCK U-Boot i2c behind-mux test: does the vendor firmware read the RTC/adt7475
# through the PCA9546 mux (0x71)? If yes -> hardware+mux fine, the behind-mux
# timeout is a mainline-7.1 regression (Linux + our U-Boot). Keep stock busy so
# its ~50 s prompt-watchdog doesn't autoboot. Restores mux to 0x00 at the end.
proc r {c} { send $c; catch {puts ">> $c\n[expect "ALPINE_UBNT_NAS_ALL>" 6]"} }
send_raw CR
expect "ALPINE_UBNT_NAS_ALL>" 8
r "i2c dev 0"
r "i2c probe"
r "i2c mw 0x71 0 1 1"
r "i2c md 0x30 0 8"
r "i2c mw 0x71 0 8 1"
r "i2c md 0x2e 3d 4"
r "i2c mw 0x71 0 0 1"
puts "=== STOCK-I2C-DONE ==="
