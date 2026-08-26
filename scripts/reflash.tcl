# Reflash woomera's Fedora kernel+DTB into NAND (stock kernel @0x300000 kept as recovery).
# Run:  ./dev.py console-tcl scripts/reflash.tcl
# Prereqs:
#   - gzip uImage + DTB staged in the tftp dir as the names below (mkuimage.py --gzip).
#   - woomera rebooting NOW: this catches U-Boot by spamming ESC, then flashes.
# Offsets match scripts/flash-nand.py. Chainload-safe: NOR U-Boot binary untouched, env only.

# --- catch the U-Boot prompt: send ESC, poll for the prompt, up to ~120 tries ---
# --- LAN config: EDIT for your setup (device = ipaddr, tftp host = serverip) ---
# serverip is THIS dev host's IP and drifts with its DHCP lease - no injection point
# here (standalone script, not driven by a dedicated dev.py command). If tftpboot
# retries forever ("Retry count exceeded"), check `ip -4 addr` on this host and
# update serverip below - see scripts/_net.py detect_server_ip() for the pattern
# used where a driving dev.py command CAN inject it fresh.
set ipaddr   192.168.25.140
set serverip 192.168.25.145

proc catch_uboot {} {
    for {set i 0} {$i < 120} {incr i} {
        send_raw ESC
        if {[catch {expect "ALPINE_UBNT_NAS_ALL>" 1}] == 0} { return }
    }
    error "did not reach the U-Boot prompt (is woomera rebooting?)"
}
catch_uboot
puts "AT-UBOOT"

# --- network ---
send "setenv ipaddr $ipaddr";     expect "ALPINE_UBNT_NAS_ALL>"
send "setenv serverip $serverip"; expect "ALPINE_UBNT_NAS_ALL>"

# --- kernel: tftp 0x02000000 -> NAND 0x1300000, span 0x1200000 ---
send "tftpboot 0x02000000 uImage-unvr-ea16-7.1-fedora-gz"; expect "Bytes transferred" 60
send "nand erase 0x1300000 0x1200000";                     expect "OK"
send "nand write 0x02000000 0x1300000 0x1200000";          expect "OK" 60

# --- dtb: tftp 0x04078000 -> NAND 0x2800000, block 0x40000 ---
send "tftpboot 0x04078000 alpine-v2-ubnt-unvr-ea16-7.1-fedora.dtb"; expect "Bytes transferred" 30
send "nand erase 0x2800000 0x40000";                               expect "OK"
send "nand write 0x04078000 0x2800000 0x40000";                    expect "OK"

# --- bootcmd + persist (env write only; NOR U-Boot binary untouched) ---
send "setenv bootcmd 'nand read 0x02000000 0x1300000 0x1200000; nand read 0x04078000 0x2800000 0x20000; bootm 0x02000000 - 0x04078000'"
expect "ALPINE_UBNT_NAS_ALL>"
send "saveenv"; expect "done" 15
puts "REFLASH-DONE — 'boot' or power-cycle to run the fixed kernel"
