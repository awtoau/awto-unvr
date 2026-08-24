# Full READ-probe i2c scan of the CURRENT bus (run `i2c dev <N>` first, or pass
# nothing to scan the active bus). Reads 1 byte from every address 0x08-0x77 and
# reports which ACK. Catches devices U-Boot's `i2c probe` SKIPS: the DW adapter
# has no SMBus quick-write, so probe silently skips 0x00-02 / 0x28-2f / 0x40-4f.
# A read-probe hits every address, so an undocumented chip anywhere shows up.
send_raw CR
expect "awto-nas#" 5
set found {}
for {set a 8} {$a < 0x78} {incr a} {
    set h [format %02x $a]
    send "i2c md 0x$h 0 1"
    set r [expect "awto-nas#" 3]
    if {[string first "Error" $r] < 0 && [string first "timed out" $r] < 0} {
        lappend found $h
    }
}
puts "SCAN-FOUND: $found"
