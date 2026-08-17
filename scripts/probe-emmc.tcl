# Probe the live Fedora for the eMMC (Samsung KLM4G1FE3B). Read-only.
# Run with:  ./dev.py console-tcl scripts/probe-emmc.tcl
# Assumes an already-logged-in root shell on the console (log in first,
# e.g. ./dev.py console-send root  then  console-send <pw>). Prompt has ']' which
# mini_jimtcl treats as command-subst, so we sync on output-only markers (MK.x).
send {echo READY"."GO}
expect "READY.GO" 15

send {ls -ld /dev/mmcblk* 2>&1; echo MK"."A}
set a [expect "MK.A" 8]; puts "== /dev/mmcblk* ==$a"
send {ls /sys/class/mmc_host/ 2>&1; echo MK"."B}
set b [expect "MK.B" 8]; puts "== mmc_host ==$b"
send {lsblk -o NAME,SIZE,TYPE,MODEL,TRAN 2>&1; echo MK"."C}
set c [expect "MK.C" 8]; puts "== lsblk ==$c"
send {dmesg | grep -iE 'mmc|sdhci|dw_mmc|klm|emmc' | tail -25; echo MK"."D}
set d [expect "MK.D" 10]; puts "== dmesg mmc ==$d"
puts "PROBE-DONE"
