# Reset -> auto-chainload our U-Boot -> read SMCC 0x110 on 00:01.0 WITHOUT running
# snoopfix. If board_late_init applied it, this reads ...3 straight after boot.
send "reset"
if {[catch {expect "unvr#" 50}]} { puts "NO-UBOOT"; exit 1 }
send "pci enum"
expect "unvr#" 12
send "pci display.l 0.1.0 0x110 1"
puts "AUTO-SMCC:[expect {unvr#} 8]"
puts "DONE"
