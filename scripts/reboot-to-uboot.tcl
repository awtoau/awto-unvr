# Force a reset to U-Boot WITHOUT a power-cycle, via the SP805 watchdog.
# Linux `reboot` hangs on this box (#51, no AL-324 restart handler); a hardware
# watchdog reset bypasses that. Arms /dev/watchdog to 1 s and busy-holds the fd
# open (so closing can't disarm it) -> SP805 RESEN fires in ~2 s -> SoC reset.
# Then spams ESC to interrupt U-Boot autoboot. Assumes a logged-in root shell.
# If this reaches the prompt, it also VALIDATES the fix for #51 (watchdog = reboot).
send {python3 -c "import fcntl,struct; f=open('/dev/watchdog','r+b',buffering=0); fcntl.ioctl(f,0xC0045706,struct.pack('I',1)); exec('while True: pass')"}

set found 0
for {set i 0} {$i < 400} {incr i} {
    send_raw ESC
    if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set found 1; break }
}
if {$found} {
    puts "AT-UBOOT — SP805 watchdog reset WORKED (validates #51)"
} else {
    puts "NO-UBOOT — watchdog did not bring up the U-Boot prompt (may need power-cycle)"
}
