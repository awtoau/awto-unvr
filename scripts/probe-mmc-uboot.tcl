# Probe the eMMC via the vendor U-Boot USB path — the definitive test.
# The UNVR family reaches its eMMC over a USB bridge (vendor `bootemmc` = `usb start`),
# and Linux showed 0 devices on the xHCI bus. This runs the vendor path at U-Boot:
# if `usb start` enumerates a storage device, the bridge+eMMC are populated/reachable;
# if not, the bridge is unpopulated on this NAND SKU. Read-only — no flashing.
# Run at (or into) the U-Boot prompt:  ./dev.py console-tcl scripts/probe-mmc-uboot.tcl
# Reboot woomera first (power-cycle: Linux `reboot` hangs, #51); this catches U-Boot.

proc catch_uboot {} {
    for {set i 0} {$i < 120} {incr i} {
        send_raw ESC
        if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { return }
    }
    error "did not reach the U-Boot prompt (is woomera rebooting?)"
}
catch_uboot
puts "AT-UBOOT"

send "usb start";   set a [expect "ALPINE_UBNT_NAS_ALL>" 25]; puts "== usb start ==$a"
send "usb tree";    set b [expect "ALPINE_UBNT_NAS_ALL>" 10]; puts "== usb tree ==$b"
send "usb storage"; set c [expect "ALPINE_UBNT_NAS_ALL>" 10]; puts "== usb storage ==$c"
send "usb info";    set d [expect "ALPINE_UBNT_NAS_ALL>" 10]; puts "== usb info ==$d"
puts "USB-PROBE-DONE"
