# From OUR U-Boot: SP805-reset -> catch stock U-Boot (LAST ESC catch) -> set stock
# bootcmd to auto-chainload our u-boot.bin (tftp+go), falling back to the Fedora
# NAND boot if tftp is down. Env-only (NOR binary untouched); revert = restore bootcmd.
send_raw CR
expect "unvr#" 8
send "mw 0xfd88cc00 0x1acce551"; expect "unvr#" 6
send "mw 0xfd88c000 0x00002000"; expect "unvr#" 6
send "mw 0xfd88c008 0x00000003"; expect "unvr#" 6
set found 0
for {set i 0} {$i < 400} {incr i} {
    send_raw ESC
    if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set found 1; break }
}
if {!$found} { puts "NO-STOCK-UBOOT"; exit 1 }
puts "AT-STOCK-UBOOT"
# Fedora NAND boot preserved as a fallback (the current bootcmd value)
send {setenv bootfedora 'nand read 0x02000000 0x1300000 0x1200000; nand read 0x04078000 0x2800000 0x20000; bootm 0x02000000 - 0x04078000'}
expect "ALPINE_UBNT_NAS_ALL>" 8
send {setenv bootchain 'setenv ipaddr 192.168.25.140; setenv serverip 192.168.25.145; if tftpboot 0x1100000 u-boot-chainload.bin; then go 0x1100000; fi'}
expect "ALPINE_UBNT_NAS_ALL>" 8
send {setenv bootcmd 'run bootchain; run bootfedora'}
expect "ALPINE_UBNT_NAS_ALL>" 8
send "saveenv"; expect "done" 15
puts "AUTOCHAIN-SET"
# exercise it: reset -> stock should auto-chainload ours, landing at unvr#
send "reset"
if {[catch {expect "unvr#" 40}]} { puts "DID-NOT-REACH-OUR-UBOOT (check)"; exit 2 }
puts "LANDED-AT-OUR-UBOOT-NO-ESC"
