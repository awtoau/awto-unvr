# AL-324 SoC GPIO / pinmux map (UNVR ea16)

- 48 direct SoC lines = 6× PL061 banks × 8. Global pin `P` → bank `P/8`, line `P%8`.
- Each ball is muxed by the **MUIO pinmux** (`pbs_mux_sel`): 4 bits/ball, `0`=GPIO, `1-4`=peripheral.
- **The per-ball map is NOT in any source** — not stock U-Boot, not stock Linux GPL, not UNVR
  userland. It lives in the flash board-cfg DTB (`/soc/board-cfg/pinctrl_init`+`gpio_init`)
  and, definitively, in the live `pbs_mux_sel` registers. AL-324 ballmap is NDA.
- Table below = **live read** from our U-Boot on woomera (2026-08-18), `scripts/read-muio-mux.tcl`.
  `chip_id`@0xfd8a815c = 0x00010000 → dev_id 1 = Alpine V2 ✓.

## Bank register bases

| bank | lines (global) | PL061 base | mux_sel reg |
|------|----------------|-----------|-------------|
| gpio0 | 0-7   | 0xfd887000 | pbs_mux_sel_0 0xfd8a8138 |
| gpio1 | 8-15  | 0xfd888000 | pbs_mux_sel_1 0xfd8a813c |
| gpio2 | 16-23 | 0xfd889000 | pbs_mux_sel_2 0xfd8a8140 |
| gpio3 | 24-31 | 0xfd88a000 | pbs_mux_sel_3 0xfd8a8144 |
| gpio4 | 32-39 | 0xfd88b000 | pbs_mux_sel_4 0xfd8a8220 |
| gpio5 | 40-47 | 0xfd897000 | pbs_mux_sel_5 0xfd8a8224 |

PL061 per bank: dir `base+0x400` (1=out), data (all 8) `base+0x3FC`.

## Live mux read (2026-08-18)

```
mux_sel_0 = 0x33000000   mux_sel_1 = 0x33333333   mux_sel_2 = 0x00403333
mux_sel_3 = 0x01002244   mux_sel_4 = 0x00000000   mux_sel_5 = 0x00000000
```
mux func codes on this board: `0`=GPIO, `1`=ETH-LED, `2`=UART2(RPS), `3`=NAND flash bus, `4`=SGPO.

## Per-ball map

| pin | bank.line | mux | function | notes |
|-----|-----------|-----|----------|-------|
| 0  | 0.0 | GPIO | SFP 25G speed LED | al_eth, active-low (`gpio-0 "25g"`) |
| 1  | 0.1 | GPIO | **spare** | pull-H |
| 2  | 0.2 | GPIO | **spare** | pull-H |
| 3  | 0.3 | GPIO | **spare** | out-low |
| 4  | 0.4 | GPIO | **spare** | pull-H |
| 5  | 0.5 | GPIO | **spare** | pull-H |
| 6  | 0.6 | 3 | NAND flash data | — |
| 7  | 0.7 | 3 | NAND flash data | — |
| 8-15 | 1.0-1.7 | 3 | **NAND flash data bus** | whole bank — NOT spare |
| 16 | 2.0 | 3 | NAND flash | — |
| 17 | 2.1 | 3 | NAND flash | — |
| 18 | 2.2 | 3 | NAND flash | — |
| 19 | 2.3 | 3 | NAND flash CS/WP | — |
| 20 | 2.4 | GPIO | **spare** | pull-H |
| 21 | 2.5 | 4 | SGPO | bay-activity LED shift-reg |
| 22 | 2.6 | GPIO | **spare** | — |
| 23 | 2.7 | GPIO | **spare** | — |
| 24 | 3.0 | 4 | SGPO | — |
| 25 | 3.1 | 4 | SGPO | — |
| 26 | 3.2 | 2 | UART2 RX | RPS RS-232 (ttyS2, MAX3221) |
| 27 | 3.3 | 2 | UART2 TX | RPS RS-232 |
| 28 | 3.4 | GPIO | **spare** | pull-H |
| 29 | 3.5 | GPIO | **spare** | pull-H |
| 30 | 3.6 | 1 | ETH LED | — |
| 31 | 3.7 | GPIO | ulogo_blue LED | active-low |
| 32 | 4.0 | GPIO | **spare** | pull-H |
| 33 | 4.1 | GPIO | rps_prnt | RPS-present sense (input) |
| 34 | 4.2 | GPIO | 12v_lp | RPS 12V sense (input) |
| 35 | 4.3 | GPIO | **spare** | pull-H |
| 36 | 4.4 | GPIO | **spare** | pull-H |
| 37 | 4.5 | GPIO | ulogo_white LED | active-low |
| 38 | 4.6 | GPIO | reset button | active-low, pull-H |
| 39 | 4.7 | GPIO | **spare** | pull-H |
| 40 | 5.0 | GPIO | **spare** | H |
| 41 | 5.1 | GPIO | **spare** | H |
| 42 | 5.2 | GPIO | hdd force-power-on-wa | out-high (bay power workaround) |
| 43 | 5.3 | GPIO | **spare** | H |
| 44 | 5.4 | GPIO | **spare** | L |
| 45 | 5.5 | GPIO | **spare** | L |
| 46 | 5.6 | GPIO | **spare** | L |
| 47 | 5.7 | GPIO | **spare** | L |

## Spare GPIO (GPIO-mode, no known consumer)

21 lines: **1,2,3,4,5, 20,22,23, 28,29, 32,35,36,39, 40,41,43,44,45,46,47**.
- Pulled-H spares (1,2,4,5,20,28,29,32,35,36,39,40,41,43) = candidates for **SW1/SW2 / straps**
  (a wired button reads low when pressed). Identify empirically: read the bank while a button
  is pressed (`scripts/read-muio-mux.tcl` pattern, or `gpio status -a` in U-Boot).
- Low spares (44-47) = likely truly unconnected.

## Re-read live

U-Boot: `md.l 0xfd8a8138 4` (pins 0-31 mux), `md.l 0xfd8a8220 2` (32-47), `gpio status -a` (dir/val).
Linux: `busybox devmem 0xfd8a8138` … per reg; `busybox devmem <base+0x400>` dir, `+0x3FC` data.

## Other boards / userland (checked, no extra SoC-line info)

- Stock Linux GPL (4.1.37): no board DTS, only board-cfg parser + bindings — map is in flash DTB.
- Stock U-Boot GPL: `dt_based_init_pinctrl`/`_gpio` (`board/annapurna-labs/alpine_ubnt/board.c:419/489`)
  parse `/soc/board-cfg/pinctrl_init`+`gpio_init` from the flash DTB — data not in source.
- UNVR userland: ulogo = `ulogo_ctrl` LED-class (`/sys/class/leds/.../pattern`); only raw-gpio poke
  is `ubnt-bt.sh` BT-reset via **dynamic** expander lookup (PCA9575 @0x20 line 8) — no hardcoded
  PL061 numbers.
- MUIO alt-function map (what each ball *can* be): delroth HAL
  `drivers/pbs/al_hal_muio_mux_map.h` (`_al_muio_mux_iface_pins[]`).
