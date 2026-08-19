# Reset (box auto-chainloads our U-Boot) -> enumerate PCI -> read SMCC 0x110 on
# 00:01.0 (eth) BEFORE -> snoopfix -> read AFTER. Expect low bits 0x3
# (SNOOP_OVR|SNOOP_EN) set post-fix. #74.
send "reset"
if {[catch {expect "unvr#" 50}]} { puts "NO-UBOOT-AFTER-RESET"; exit 1 }
puts "AT-OUR-UBOOT"
send "pci enum"
expect "unvr#" 12
send "pci display.l 0.1.0 0x110 1"
puts "BEFORE:[expect {unvr#} 8]"
send "snoopfix"
puts "SNOOPFIX:[expect {unvr#} 12]"
send "pci display.l 0.1.0 0x110 1"
puts "AFTER:[expect {unvr#} 8]"
puts "DONE"
