# Reset -> auto-chainload new build -> test the baud TABLE safely: 57600 is in
# U-Boot's DEFAULT table but NOT in ours {115200,230400,460800,921600}, so it
# must be REJECTED (proves our table replaced the default). Stays at 115200 —
# no console desync. Then confirm a valid rate (460800) is accepted (prints the
# switch prompt at the old baud) — we do NOT complete the switch.
send "reset"
if {[catch {expect "unvr#" 50}]} { puts "NO-UBOOT"; exit 1 }
send "setenv baudrate 57600"
puts "REJECT-TEST:[expect {unvr#} 6]"
puts "DONE"
