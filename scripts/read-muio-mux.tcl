# Read the AL-324 MUIO pinmux select registers live from our chainloaded
# U-Boot. Ground truth for which of the 48 SoC balls are GPIO (nibble==0) vs
# a peripheral function (1-4). 4 bits/ball, 8 balls/reg. See docs/gpio-map.md.
#   mux_sel_0..3 contiguous 0xfd8a8138 (pins 0-31); mux_sel_4/5 0xfd8a8220 (32-47).
#   chip_id 0xfd8a815c: dev_id field==1 => Alpine V2.
send "\r"
expect "unvr#"
send "md.l 0xfd8a815c 1\r"
expect "fd8a815c:"
expect "unvr#"
send "md.l 0xfd8a8138 4\r"
expect "fd8a8138:"
expect "unvr#"
send "md.l 0xfd8a8220 2\r"
expect "fd8a8220:"
expect "unvr#"
