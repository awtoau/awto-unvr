# Flash the fixed Fedora kernel+DTB into NAND, entering from an ALREADY-AT-PROMPT
# U-Boot (no reset/catch — the box is sitting at the prompt). Same offsets as
# reflash.tcl. Factory kernel @0x300000 (bootnand) untouched = recovery.
#   ./dev.py console-tcl scripts/reflash-now.tcl
set ipaddr   192.168.25.140
set serverip 192.168.25.145

# land on a fresh prompt
send_raw CR
expect "ALPINE_UBNT_NAS_ALL>" 8

send "setenv ipaddr $ipaddr";     expect "ALPINE_UBNT_NAS_ALL>" 8
send "setenv serverip $serverip"; expect "ALPINE_UBNT_NAS_ALL>" 8

# --- kernel: tftp 0x02000000 -> NAND 0x1300000, span 0x1200000 ---
send "tftpboot 0x02000000 uImage-unvr-ea16-7.1-fedora-gz"; expect "Bytes transferred" 60
expect "ALPINE_UBNT_NAS_ALL>" 10
send "nand erase 0x1300000 0x1200000";                     expect "OK" 60
expect "ALPINE_UBNT_NAS_ALL>" 10
send "nand write 0x02000000 0x1300000 0x1200000";          expect "OK" 90
expect "ALPINE_UBNT_NAS_ALL>" 10

# --- dtb: tftp 0x04078000 -> NAND 0x2800000, block 0x40000 ---
send "tftpboot 0x04078000 alpine-v2-ubnt-unvr-ea16-7.1-fedora.dtb"; expect "Bytes transferred" 30
expect "ALPINE_UBNT_NAS_ALL>" 10
send "nand erase 0x2800000 0x40000";                               expect "OK" 30
expect "ALPINE_UBNT_NAS_ALL>" 10
send "nand write 0x04078000 0x2800000 0x40000";                    expect "OK" 30
expect "ALPINE_UBNT_NAS_ALL>" 10

# --- bootcmd already points here; re-set + persist (env write only) ---
send "setenv bootcmd 'nand read 0x02000000 0x1300000 0x1200000; nand read 0x04078000 0x2800000 0x20000; bootm 0x02000000 - 0x04078000'"
expect "ALPINE_UBNT_NAS_ALL>" 8
send "saveenv"; expect "done" 15
expect "ALPINE_UBNT_NAS_ALL>" 8
puts "REFLASH-DONE — new kernel+DTB in NAND; recovery = 'run bootnand'"
