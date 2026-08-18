# Exercise every meaningful command in our U-Boot on woomera and capture each
# output block. Run at the `unvr#` prompt (our chainloaded U-Boot). Drains the
# line first (leading CR) so stale bytes from a prior session don't concatenate.
# Each `puts [expect ...]` prints that command's full output. See docs/uboot-hw-test-suite.md.
proc run {cmd to} {
    puts "\n===== $cmd ====="
    send $cmd
    if {[catch {puts [expect "unvr#" $to]} e]} { puts "  <TIMEOUT/ERR: $e>" }
}
send_raw CR
expect "unvr#" 8
run "version" 12
run "coninfo" 12
run "i2c dev 0" 12
run "i2c probe" 12
run "i2c speed" 12
run "led list" 12
run "gpio status -a" 20
run "clk dump" 20
run "dm tree" 20
run "pci enum" 20
run "pci" 20
run "scsi scan" 30
run "scsi info" 12
run "part list scsi 0" 12
run "ls scsi 0:2 /boot" 15
run "printenv" 15
puts "\n===== DONE ====="
