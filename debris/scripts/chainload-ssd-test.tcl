# Chainload OUR U-Boot via TFTP (stock U-Boot's SCSI can't read the SSD, so we
# can't ext4load it — tftp it instead), then test OUR OWN PCIe(ECAM)+AHCI:
# does our enumeration find the SATA SSD stock U-Boot misses?
# Entered ALREADY at the stock U-Boot prompt (caught separately via watchdog+ESC).
#   ./dev.py console-tcl scripts/chainload-ssd-test.tcl
send_raw CR
expect "ALPINE_UBNT_NAS_ALL>" 8
send "setenv ipaddr 192.168.25.140";   expect "ALPINE_UBNT_NAS_ALL>" 8
send "setenv serverip 192.168.25.145"; expect "ALPINE_UBNT_NAS_ALL>" 8

# our u-boot.bin -> 0x1100000 (its TEXT_BASE; free after stock U-Boot relocated high)
send "tftpboot 0x1100000 u-boot-chainload.bin"; expect "Bytes transferred" 30
expect "ALPINE_UBNT_NAS_ALL>" 8
send "go 0x1100000"
if {[catch {expect "unvr#" 20}]} { puts "NO-OUR-UBOOT — go did not reach our prompt"; exit 2 }
puts "AT-OUR-UBOOT"

# --- OUR OWN enumeration: ECAM host -> assign BARs -> class-based AHCI-PCI ---
# i2c + gpio: confirm the pca9575 bay-power hog fired (drives powered)
send "i2c dev 0";      expect "unvr#" 8
send "i2c probe";      expect "unvr#" 12
send "gpio status -a"; expect "unvr#" 12
# our own PCIe(ECAM) enum + AHCI scan (drives now powered)
send "pci enum";  expect "unvr#" 20
send "scsi scan"; expect "unvr#" 45
send "scsi info"; expect "unvr#" 10
send "scsi part"; expect "unvr#" 15
send "ls scsi 0:2 /boot"; expect "unvr#" 15
puts "SSD-TEST-DONE"

# --- back to Fedora ---
send "reset"
puts "RESET-SENT"
