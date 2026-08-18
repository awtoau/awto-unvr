# Boot the freshly-flashed kernel from U-Boot and wait for Fedora login.
# Stays connected through the boot (expect after send) so 'boot' isn't truncated.
#   ./dev.py console-tcl scripts/boot-verify.tcl
send_raw CR
expect "ALPINE_UBNT_NAS_ALL>" 8
send "boot"
expect "Starting kernel" 60
puts "\nSTARTING-KERNEL-OK"
expect "login:" 150
puts "\nFEDORA-LOGIN-REACHED"
