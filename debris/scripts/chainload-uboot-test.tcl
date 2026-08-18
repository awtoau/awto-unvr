# Chainload U-Boot test: load OUR u-boot.bin @ 0x1100000 via the stock U-Boot
# and run version/bdinfo/mtest + DDR-register md. Safe: runs from already-trained
# DRAM, NOR untouched; `reset` returns to the stock chain -> Fedora.
#
# Prereqs: logged-in Fedora root shell on the console + /boot/u-boot-chainload.bin
# staged. Run:  ./dev.py console-tcl scripts/chainload-uboot-test.tcl
#
# 1) SP805 watchdog reset -> catch the stock U-Boot prompt (from reboot-to-uboot.tcl;
#    also revalidates #51). 2) scsi init + ext4load ours + go. 3) tests. 4) reset.

# --- 1. watchdog reset, then spam ESC to catch stock U-Boot autoboot ---
send {python3 -c "import fcntl,struct; f=open('/dev/watchdog','r+b',buffering=0); fcntl.ioctl(f,0xC0045706,struct.pack('I',1)); exec('while True: pass')"}
set found 0
for {set i 0} {$i < 400} {incr i} {
    send_raw ESC
    if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set found 1; break }
}
if {!$found} { puts "NO-UBOOT — watchdog did not reach the stock U-Boot prompt"; exit 1 }
puts "AT-STOCK-UBOOT (SP805 watchdog reset works — revalidates #51)"

# --- 2. load our chainload U-Boot off the SSD and jump to it ---
send "scsi init"
expect "ALPINE_UBNT_NAS_ALL>" 25
send "ext4load scsi 0:2 0x1100000 /boot/u-boot-chainload.bin"
expect "ALPINE_UBNT_NAS_ALL>" 25
send "go 0x1100000"
if {[catch {expect "unvr#" 15}]} { puts "NO-OUR-UBOOT — go did not reach our prompt"; exit 2 }
puts "AT-OUR-UBOOT — chainload SUCCESS (our modern U-Boot runs on the hardware)"

# --- 3. tests: identity + board info + memtest + live DDR registers ---
send "version";  expect "unvr#" 10
send "bdinfo";   expect "unvr#" 10
# memtest 16 MiB well above U-Boot, 1 pass (ALT_MEMTEST = thorough)
send "mtest 0x10000000 0x11000000 0 1"; expect "unvr#" 120
# live DDR controller (MSTR...) + PHY head via md (chainload can read them)
send "md 0xf0080000 8"; expect "unvr#" 10
send "md 0xf0088000 8"; expect "unvr#" 10
puts "TESTS-DONE"

# --- 4. return to Fedora ---
send "reset"
puts "RESET-SENT — stock chain -> Fedora"
