# TEST (read-only) whether U-Boot can load kernel+DTB from the SSD ext4 /boot.
# Does NOT change bootcmd or boot — just proves ext4load works on sda2 before we
# commit to SSD-direct boot. Run at the U-Boot prompt.
#   ./dev.py console-tcl scripts/test-ssd-load.tcl
proc run {cmd exp t} {
    puts "\n===== $cmd ====="
    send $cmd
    puts [expect $exp $t]
}
send_raw CR
expect "ALPINE_UBNT_NAS_ALL>" 8
run "scsi init"                                                              "ALPINE_UBNT_NAS_ALL>" 30
run "scsi part"                                                             "ALPINE_UBNT_NAS_ALL>" 15
run "ext4ls scsi 0:2 /boot"                                                 "ALPINE_UBNT_NAS_ALL>" 20
run "ext4load scsi 0:2 0x02000000 /boot/uImage-unvr-ea16-7.1-fedora-gz"     "ALPINE_UBNT_NAS_ALL>" 40
run "ext4load scsi 0:2 0x04078000 /boot/alpine-v2-ubnt-unvr-ea16-7.1-fedora.dtb" "ALPINE_UBNT_NAS_ALL>" 20
puts "\nSSD-LOAD-TEST-DONE"
