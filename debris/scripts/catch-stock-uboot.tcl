# Catch the stock U-Boot prompt after a reset: spam ESC through the ~2 s autoboot
# window until ALPINE_UBNT_NAS_ALL> appears. Trigger the reset separately (watchdog
# over SSH) right after launching this. Then read its gpio help so we know the syntax.
for {set i 0} {$i < 800} {incr i} {
    send_raw ESC
    if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} {
        puts "\n=== AT-STOCK-UBOOT ==="
        send "gpio"
        catch {puts [expect "ALPINE_UBNT_NAS_ALL>" 4]}
        exit 0
    }
}
puts "NO-STOCK (never caught prompt)"
