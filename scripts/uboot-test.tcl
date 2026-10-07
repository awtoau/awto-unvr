# `./dev.py uboot-test` drives this: put the FRESH build on the box and stop for
# hands-on testing. From our awto-nas#: arm SP805 watchdog (SoC reset) -> catch
# the NAND awto-uboot -> tftp 0x02000000 -> cp.b to 0x1100000 -> go -> STOP. No
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
# Catch the NAND awto-uboot, NOT stock. Stock has no autoboot window under
# the #216 chain - persist-boot.py sets its bootcmd to load NAND 0x1300000
# and `go`, so it never prints "Hit any key" and never shows its prompt.
# The NAND image does (CONFIG_BOOTDELAY=2), and tftp/cp.b/go are all in it
# (CMD_NET, CMD_MEMORY, CMD_GO), so the fresh image is staged from there.
# Waits for: awto-nas# after the reset. Expected: ~12s of ROM/S2/preboot/
# stock, then NAND's 2s countdown; 60 x 1s is ~5x that, matching the budget
# every catch-uboot.py caller already uses.
# On expiry: NO-UBOOT and return, rather than typing into whatever is there.
set ok 0
for {set i 0} {$i < 60} {incr i} { send_raw CR; if {[catch {expect "awto-nas#" 1}] == 0} { set ok 1; break } }
if {!$ok} { puts "NO-UBOOT (reset didn't reach the NAND awto-nas# in ~60s; power-cycle may be needed)"; return }
# Stop the 2s countdown before typing anything: it expired mid-setenv and the
# next expect waited 6s at a Linux console. RAM env only - no saveenv, so a
# reset reverts it and stock's own env is untouched.
# Each step names itself on failure: a bare `expect` raise aborted the script
# with just "tcl error:" and no indication of which command was lost, which
# cost a round of console archaeology to work out.
proc step {cmd needle secs what} {
    send $cmd
    if {[catch {expect $needle $secs}]} {
        puts "STEP-FAILED ($what): '$needle' not seen in ${secs}s after '$cmd'"
        return 0
    }
    return 1
}
if {![step "setenv bootdelay -1" "awto-nas#" 6 "stop autoboot"]} { return }
if {![step "setenv ipaddr $IPADDR" "awto-nas#" 6 "set ipaddr"]} { return }
if {![step "setenv serverip $SERVERIP" "awto-nas#" 6 "set serverip"]} { return }
# 0x1100000 is the RUNNING bootloader's load/entry (nor-boot-chain.md:56)
# AND our CONFIG_TEXT_BASE - must run there, cannot be written there.
# Stage elsewhere, copy, jump: same shape as flash-awto-uboot.py.
# tftp bound: 820KB over a 100Mb link is <1s; 30s is ~30x, and covers the
# ARP retries that precede a transfer on a cold neighbour table.
if {![step "tftpboot 0x02000000 u-boot-chainload.bin" "Bytes transferred" 30 "tftp the image"]} { return }
if {![step "cp.b 0x02000000 0x1100000 \$filesize" "awto-nas#" 10 "copy to TEXT_BASE"]} { return }
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
