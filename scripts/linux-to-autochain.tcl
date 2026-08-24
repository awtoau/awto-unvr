# From a Linux root shell: SP805 watchdog reset -> catch stock U-Boot -> set stock
# bootcmd to AUTO-CHAINLOAD our u-boot.bin (tftp + go), with NO Fedora/Linux
# fallback (owner: never boot Linux on reset) -> chainload now -> our U-Boot.
# Revert = restore stock bootcmd. Requires host tftpd serving u-boot-chainload.bin.
# If tftp is ever down, reset stops at the stock prompt (recoverable) — never Linux.
send {python3 -c "import fcntl,struct; f=open('/dev/watchdog','r+b',buffering=0); fcntl.ioctl(f,0xC0045706,struct.pack('I',1)); exec('while True: pass')"}
set found 0
for {set i 0} {$i < 400} {incr i} {
    send_raw ESC
    if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set found 1; break }
}
if {!$found} { puts "NO-STOCK-UBOOT"; exit 1 }
puts "AT-STOCK-UBOOT"
send {setenv bootchain 'setenv ipaddr 192.168.25.140; setenv serverip 192.168.25.145; if tftpboot 0x1100000 u-boot-chainload.bin; then go 0x1100000; fi'}
expect "ALPINE_UBNT_NAS_ALL>" 8
send {setenv bootcmd 'run bootchain'}
expect "ALPINE_UBNT_NAS_ALL>" 8
send "saveenv"; expect "done" 15
puts "AUTOCHAIN-SET"
send "run bootchain"
if {[catch {expect "awto-nas#" 40}]} { puts "NO-OUR-UBOOT"; exit 2 }
puts "AT-OUR-UBOOT"
