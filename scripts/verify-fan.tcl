send "reset"
if {[catch {expect "unvr#" 50}]} { puts "NO-UBOOT"; exit 1 }
send "fan"
puts "READ0:[expect {unvr#} 8]"
send "fan 200"
puts "SET:[expect {unvr#} 8]"
send "fan"
puts "READBACK:[expect {unvr#} 8]"
puts "DONE"
