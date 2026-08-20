# ESC-spam to interrupt stock U-Boot's autoboot before it re-boots whatever
# (possibly broken) kernel bootcmd currently points at. The countdown here is
# only ~2s (not the ~50s some older docs assume), so poll fast: 0.1s per
# iteration, ~10 ESC sends/sec, to have a real chance of landing inside it.
# Start this BEFORE the power-cycle, not after - it just idles harmlessly
# (bytes into a dead line) until the prompt appears and starts listening.
puts "CATCH-LOOP-STARTED"
set found 0
for {set i 0} {$i < 2000} {incr i} {
    send_raw ESC
    if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 0.1}] == 0} { set found 1; break }
}
if {$found} { puts "AT-STOCK-UBOOT" } else { puts "NO-STOCK-UBOOT — autoboot may have already fired" }
