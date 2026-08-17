# Test the serial console at a higher baud at the U-Boot prompt. Two-phase,
# because U-Boot switches its own console the instant `setenv baudrate` runs:
#
#   PHASE 1 (tio @115200):  ./dev.py console-tcl scripts/baud-test.tcl
#     catches U-Boot, runs `setenv baudrate 1000000`; U-Boot prints
#     "## Switch baudrate to 1000000 bps and press ENTER ..." then listens at 1M.
#   Reconnect the console at the new baud:
#     ./dev.py console-stop
#     UNVR_CONSOLE_BAUD=1000000 ./dev.py console
#   PHASE 2 (tio @1M):
#     ./dev.py console-send --raw --expect ALPINE_UBNT_NAS_ALL CR
#     A clean prompt at 1M == PASS. Revert: at 1M `setenv baudrate 115200`, then
#     reconnect at 115200 (do NOT saveenv until proven, or you lock the new baud).
#
# Best CLEAN rate from the 500 MHz UART clock is 1,953,125 (=500e6/16, exact);
# 1,000,000 is +0.8% (works fine, round number). Chainload-safe: env only.
# Requires woomera at/into U-Boot (power-cycle: Linux `reboot` hangs, #51).

proc catch_uboot {} {
    for {set i 0} {$i < 120} {incr i} {
        send_raw ESC
        if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { return }
    }
    error "no U-Boot prompt (power-cycle woomera?)"
}
catch_uboot
puts "AT-UBOOT"
send "setenv baudrate 1000000"
expect "press ENTER" 5
puts "BAUD-SET-1M — now: ./dev.py console-stop ; UNVR_CONSOLE_BAUD=1000000 ./dev.py console ; then send CR"
