# Locate any USB storage bridge on woomera (the family's eMMC is "over USB xHCI").
# Read-only.  Run with:  ./dev.py console-tcl scripts/probe-usb.tcl
# Assumes an already-logged-in root shell (see scripts/probe-emmc.tcl header).
send {echo READY"."GO}
expect "READY.GO" 15

send {ls /sys/bus/usb/devices/ 2>&1; echo MK"."A}
set a [expect "MK.A" 8]; puts "== usb devices ==$a"
send {cat /sys/bus/usb/devices/*/product /sys/bus/usb/devices/*/manufacturer 2>/dev/null; echo MK"."B}
set b [expect "MK.B" 8]; puts "== usb product/mfr ==$b"
send {ls -l /dev/sd* 2>&1; echo MK"."C}
set c [expect "MK.C" 8]; puts "== /dev/sd* ==$c"
send {cat /proc/scsi/scsi 2>&1; echo MK"."D}
set d [expect "MK.D" 8]; puts "== scsi ==$d"
send {dmesg | grep -iE 'xhci|usb-storage|Direct-Access|Mass Storage|scsi [0-9]|new .*speed USB' | tail -30; echo MK"."E}
set e [expect "MK.E" 10]; puts "== dmesg usb/scsi ==$e"
puts "PROBE-DONE"
