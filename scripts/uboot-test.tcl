# `./dev.py uboot-test` drives this: put the FRESH build on the box and stop for
# hands-on testing. From our awto-nas#: arm SP805 watchdog (SoC reset) -> ESC-catch
# stock U-Boot -> tftp our U-Boot to 0x1100000 -> go -> STOP at awto-nas#. No
# auto-tests — the live prompt is the user's to drive (i2c scan / ping / serdes /
# ddr bist). SP805 @0xfd88c000: Lock@0xC00, Load@0x000, Control@0x008 (INTEN|RESEN).
# $SERVERIP is injected by cmd_uboot_test (scripts/_net.py detect_server_ip(),
# fresh each run - a hardcoded serverip goes stale the moment this dev host's
# DHCP lease drifts, and looks exactly like a TFTP TX hang). Not standalone-safe:
# running this .tcl directly (not via ./dev.py uboot-test) leaves $SERVERIP unset.
#
# $COLD (also injected by cmd_uboot_test, default 0): set to 1 when the box
# isn't already at awto-nas# - cmd_uboot_test power-cycles it first (a genuine
# cold reset, not the SP805 warm-reset this script otherwise uses to get back
# to stock), so this skips straight to the stock-U-Boot ESC-catch loop below
# instead of requiring an already-live awto-nas# session to arm SP805 from.
if {![info exists COLD]} { set COLD 0 }
if {!$COLD} {
    send_raw CR
    if {[catch {expect "awto-nas#" 8}]} { puts "NOT-AT-UNVR (get the box to our awto-nas# or stock, then re-run)"; return }
    send "mw 0xfd88cc00 0x1acce551"; expect "awto-nas#" 6
    send "mw 0xfd88c000 0x00002000"; expect "awto-nas#" 6
    send "mw 0xfd88c008 0x00000003"
    puts "SP805-ARMED — catching stock U-Boot"
} else {
    puts "COLD START (already power-cycled) — catching stock U-Boot"
}
# Waits for: stock's prompt after the SP805 reset (the power-cycle already
# happened when $COLD). Expected: reset -> preboot -> banner -> bootdelay=2
# prompt; every automated catch-uboot.py caller budgets 60s for exactly this
# (ram-boot-deploy.py, uboot-bench-check.py), so 60 iterations x 1s matches
# the number already validated elsewhere. The old 1200 was 20 MINUTES - a
# dead reset is indistinguishable from a hang for that whole time.
# On expiry: say NO-STOCK and return, rather than typing into whatever is there.
set ok 0
for {set i 0} {$i < 60} {incr i} { send_raw ESC; if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { set ok 1; break } }
if {!$ok} { puts "NO-STOCK (SP805 reset didn't reach stock in ~60s; a power-cycle may be needed)"; return }
send "setenv ipaddr $IPADDR";     expect "ALPINE_UBNT_NAS_ALL>" 6
send "setenv serverip $SERVERIP"; expect "ALPINE_UBNT_NAS_ALL>" 6
# 0x1100000 is STOCK's own load/entry (nor-boot-chain.md:56) AND our
# CONFIG_TEXT_BASE - we must RUN there but cannot tftp there, as that
# overwrites the executing bootloader (#265: the write was discarded and
# stock's bootcmd then chainloaded NAND, which prints the same prompt).
# Stage elsewhere, copy, jump - same shape as flash-awto-uboot.py.
send "tftpboot 0x02000000 u-boot-chainload.bin"; catch {expect "Bytes transferred" 30}
expect "ALPINE_UBNT_NAS_ALL>" 6
send "cp.b 0x02000000 0x1100000 \$filesize"; expect "ALPINE_UBNT_NAS_ALL>" 10
send "go 0x1100000"
# Our own U-Boot also autoboots (CONFIG_BOOTDELAY=2) - nothing sets the
# CANARY that would make it stay at the prompt on its own, so a passive
# expect races that 2s countdown and loses about half the time (falls
# through to Linux instead). Actively spam CR to interrupt it, same
# pattern as the stock ESC-catch above.
set ok2 0
for {set i 0} {$i < 20} {incr i} {
    send_raw CR
    if {[catch {expect "awto-nas#" 1}] == 0} { set ok2 1; break }
}
if {!$ok2} { puts "NO-UNVR (go failed — check the tftp'd image)"; return }
# WHICH U-Boot answered? The NAND copy at 0x1300000 prints the same awto-nas#
# prompt, and stock's bootcmd loads it to the same 0x1100000 we tftp to - so
# matching the prompt alone passed for a month while running an old build
# (#265). `version` echoes the banner, which carries the ident build-uboot
# stamped in (#258). No match = we are NOT on the fresh image; say so.
if {[info exists WANTSHA]} {
    send "version"
    if {[catch {expect "awto-$WANTSHA" 6}]} {
        puts "WRONG-UBOOT (prompt answered but banner is not awto-$WANTSHA —"
        puts "  the NAND copy won the race, or the tftp'd image never ran. #265)"
        return
    }
    puts "verified: running awto-$WANTSHA"
}
puts "=== LIVE at awto-nas# — fresh build on hardware, box is yours to test ==="
