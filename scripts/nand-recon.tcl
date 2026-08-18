# READ-ONLY NAND/boot recon at the U-Boot prompt, then boot back to Fedora.
# Answers before any flash: where does the kernel load from (bootcmd +
# nand_pt_addr_kernel), and what does the NAND layout look like.
# Run at the U-Boot prompt (catch it first): ./dev.py console-tcl scripts/nand-recon.tcl
proc run {cmd} {
    puts "\n===== $cmd ====="
    send $cmd
    puts [expect "ALPINE_UBNT_NAS_ALL>" 20]
}
send_raw CR
expect "ALPINE_UBNT_NAS_ALL>" 8

run "printenv bootcmd"
run "printenv bootnand"
run "printenv nand_pt_addr_kernel"
run "printenv nand_pt_addr_dtb"
run "printenv nand_pt_size_kernel"
run "nand info"
run "nand device"
run "mtdparts"
puts "\nRECON-DONE — booting back to Fedora"
send "boot"
