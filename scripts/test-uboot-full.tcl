proc t {cmd to} { puts "\n===== $cmd" ; send $cmd ; puts [expect "unvr#" $to] }
send_raw CR
expect "unvr#" 8
t "version" 12
t "bdinfo" 12
t "pci enum" 20
t "pci" 12
t "i2c dev 0" 8
t "i2c probe" 12
t "i2c md 0x57 0.2 20" 10
t "i2c md 0x20 0 6" 8
t "i2c md 0x21 0 6" 8
t "gpio status -a" 15
t "scsi scan" 45
t "scsi info" 10
t "scsi part" 12
t "ext4ls scsi 0:2 /boot" 12
t "dm tree" 15
t "clocks" 12
t "part list scsi 0" 12
puts "\n===== DONE"
