# Verify, at the awto-nas# prompt, the three U-Boot claims that were closed on
# code inspection alone (#81 env persistence, #89 per-port MAC from NOR,
# #110 HSSP gen.* offsets).
#
# Copyright (C) 2026 Awto / Daniel Tyrrell <dan@awto.au>
# Co-authored with Claude (Anthropic).
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Run AFTER ./dev.py uboot-test has landed the fresh build at awto-nas#:
#   ./dev.py console-tcl scripts/uboot-env-mac-verify.tcl
# Everything it prints lands in the shared console log; read it there.
#
# Timeouts: every command below is a NOR read/erase/write or a register read.
# The slowest is `saveenv` - a 64 KB sector erase + program on a MX25U25635F,
# spec'd at 0.7 s typ / 2 s max sector erase plus ~0.4 s page programming, so
# ~2.4 s worst case; 6 s is ~2.5x that. The rest are sub-millisecond register
# or SPI reads, and 6 s is orders above their worst case - they are bounded by
# the prompt round-trip, not the operation. On expiry expect throws and the
# script aborts at the failing step, which names itself in the log.
set P "awto-nas#"
set T 6

# The prompt is not sticky (autoboot countdown). Re-assert it first.
send_raw CR
expect $P $T

puts "===== uboot-env-mac-verify START ====="

# --- #89: per-port MAC, both ports, read from NOR at probe -----------------
# al_eth_hwaddr_commit() prints "al_ethN: MAC .. (NOR base+N-1)" at PROBE, so
# it has already run by the time we are at the prompt; `net list` confirms what
# the uclass ended up with. NOR base+0 is the 1G port, +1 the 10G.
send "echo === 89 nor identity blob at 0x1f0000 ==="; expect $P $T
send "sf probe 0:0"; expect $P $T
send "sf read 0x1000000 0x1f0000 0x10"; expect $P $T
send "md.b 0x1000000 0x10"; expect $P $T
send "echo === 89 ethernet devices ==="; expect $P $T
send "net list"; expect $P $T
send "printenv ethaddr"; expect $P $T
send "printenv eth1addr"; expect $P $T

# --- #110: HSSP gen.version now read at group+0x100, not group+0x00 --------
# Pre-fix this read the reserved window. A plausible non-zero version (the HAL
# packs major/minor/date into it) is the go/no-go. NOTE #239/#228: the INDIRECT
# reads (signal_detect) have no settling wait and 0xFF may be garbage - only
# gen.version is a direct read and therefore trustworthy here.
send "echo === 110 serdes gen.version ==="; expect $P $T
send "serdes status"; expect $P $T

# --- #81: env persistence across a real reset ------------------------------
# Write a canary, save, and print it back. The reset half is a separate run:
# after this, power-cycle and re-enter the prompt, then `printenv awto_canary`.
send "echo === 81 env persistence ==="; expect $P $T
send "setenv awto_canary uboot-cluster-20260904"; expect $P $T
send "saveenv"; expect $P $T
send "printenv awto_canary"; expect $P $T
send "printenv bootcmd"; expect $P $T

puts "===== uboot-env-mac-verify END ====="
puts "Now power-cycle, re-enter awto-nas#, and printenv awto_canary."
puts "A surviving awto_canary is the #81 proof; its absence is the disproof."
