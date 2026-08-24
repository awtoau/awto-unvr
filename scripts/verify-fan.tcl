send "reset"
if {[catch {expect "awto-nas#" 50}]} { puts "NO-UBOOT"; exit 1 }
send "fan"
puts "READ0:[expect {awto-nas#} 8]"
send "fan 200"
puts "SET:[expect {awto-nas#} 8]"
send "fan"
puts "READBACK:[expect {awto-nas#} 8]"
puts "DONE"
