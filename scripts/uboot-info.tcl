# In-depth, READ-ONLY U-Boot info dump. Run at the U-Boot prompt:
#   ./dev.py console-tcl scripts/uboot-info.tcl > docs/hw-reference/uboot-info-<date>.md
# Every command here is informational (no erase/write/dhcp/boot). Captures the
# full stock env, board info, NAND/NOR/USB/PCI/GPIO/clock state.
proc run {cmd} {
    puts "\n===== $cmd ====="
    send $cmd
    puts [expect "ALPINE_UBNT_NAS_ALL>" 25]
}
send_raw CR
expect "ALPINE_UBNT_NAS_ALL>" 8

# All read-only. sf probe = detect NOR (JEDEC read, no write); pci enum = config-
# space walk U-Boot already did at boot; gpio status / i2c bus / nand bad = reads.
run "version"
run "bdinfo"
run "printenv"
run "env info"
run "coninfo"
run "help"
run "nand info"
run "nand device"
run "nand bad"
run "mtdparts"
run "sf probe"
run "mmc list"
run "i2c bus"
run "pci enum"
run "pci"
run "dm tree"
run "clocks"
run "gpio status -a"
run "date"
run "usb tree"
puts "\nUBOOT-DUMP-DONE"
