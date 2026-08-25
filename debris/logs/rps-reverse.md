# RPS/PSE reverse — findings (rpsd, UNVR-5.1.25)

Binary: `tmp/fw-rootfs/sbin/rpsd` (aarch64, stripped, 181 KB).
Sections: `.rodata` vaddr==fileoff 0x1d728 (len 0x50be); `.data.rel.ro` vaddr **0x36b88**, fileoff 0x26b88 (len 0x1148); `.data` vaddr 0x383e8, fileoff 0x283e8.
- vaddr->fileoff: rodata identity; `.data.rel.ro`/`.data` subtract 0x10000.
- **Doc correction:** doc says `.data.rel.ro` vaddr==0x26b88 — wrong, it's 0x36b88 (fileoff 0x26b88). This is why the earlier pointer-to-0x20024 search failed to line up.

## Data model (3 layers, all confirmed by pointer-walk)

1. **Driver descriptors** (`.data.rel.ro`): `rps_pse` (name@f0x27980, struct fields at f0x27970) and `rps_pse_rps_pro` (f0x27b18). Each: `{fn 0xd930, fn 0xdce0, name, desc, ..., port_template_ptr, port_count}`. `rps_pse` template @ DATA f0x28b50, count=**6**.
2. **Board table** (`.data.rel.ro`): maps a board-id group -> a board-specific config. 4 entries found:
   | group (sysids) | group@rodata | config@DATA |
   |---|---|---|
   | ea39 | f0x1ffe8 | f0x28550 |
   | ea4c | f0x1fff0 | f0x28690 |
   | ea15,ea19,ea2c,ea32 | f0x1fff8 | f0x287d0 |
   | **ea16,ea1a,ea20,ea51,ea67** | **f0x20010** | **f0x28990** |
   - **Doc correction:** our group starts at file **0x20010** (ea16 first). 0x20024 is its `0xffff` terminator, not the array start. Board-table entry at DRO f0x27868 = `{&group@0x20010, 1, &config@0x38990, 1}`.
3. **Board config** = array of phy-port structs; each port has a pin sub-table.

## DELIVERABLE 1 — ea16 rps_pin_* -> gpio map

### ea16 board config (config@f0x28990) — the AUTHORITATIVE ea16 map
- **One RPS port only** (phy-1). UART = **`/dev/ttyS2`** (NOT /dev/ttyRPS1).
- Pins use driver **`gpiolib-sysfs`** (direct SoC PL061 GPIO), NOT a PCA953x/PCA9575 expander.
- Pin entry = 0x40 bytes: `{id, name_ptr, driver_ptr, encoded(gpio<<32|addr), flags, 0, hwrev_gate, 0}`.
  For `gpiolib-sysfs`, hi32 of encoded = **absolute Linux/SoC gpio number**; lo32=0.
- 4 pin entries = 2 unique pins x 2 hwrev variants (both variants same gpio):

| rps_pin | driver | gpio (dec) | flags | hwrev gate | dir |
|---|---|---|---|---|---|
| RPS_PIN_RPS_PRNT | gpiolib-sysfs | **33** (0x21) | 0x5 | ≥0x0b1011 / ≤0x0b1010 | in (RPS present) |
| RPS_PIN_12V_LP   | gpiolib-sysfs | **34** (0x22) | 0x1 | ≥0x0b1011 / ≤0x0b1010 | in (12V load/limit) |

- Board hwrev 0x0777 -> low variant, but both variants give the same gpio, so: **RPS_PRNT=gpio33, 12V_LP=gpio34**.
- **Major doc correction:** ea16 does NOT drive the full 54V_EN/12V_EN/SW/OC/PSU_PG/guard/oring pin set, and does NOT use PCA9575 expander pins for RPS. The UNVR only *senses* RPS present + 12V load over two SoC GPIOs and talks to the RPS PHY over ttyS2. gpio 33/34 = the pins previously logged as SW1/SW2 in gpio-switches-leds.md — same SoC lines.

### Full pin set = the rps_pse driver TEMPLATE (config@f0x28b50, phy-1..6) — belongs to the RPS/USP-RPS product, not UNVR
This is where the 13-pin-per-port set lives (pca953x expander). Recorded for reference; NOT the ea16 board.
Per-port pin sub-table (phy-1 @ DATA f0x29d90); pin entry encoded = `(pin<<32 | i2c_addr)` for driver `gpio-pca953x`:

| rps_pin (phy-1) | driver | i2c addr | pin | flags |
|---|---|---|---|---|
| RPS_PIN_54V_EN | gpio-pca953x | 0x23 | 8 | 0x100 |
| RPS_PIN_12V_EN | gpio-pca953x | 0x20 | 12 | 0x100 |
| RPS_PIN_54V_LP | gpio-pca953x | 0x20 | 2 | 0x2 |
| RPS_PIN_12V_LP | gpio-pca953x | 0x20 | 3 | 0x2 |
| RPS_PIN_PD_PRNT | gpio-pca953x | 0x20 | 0 | 0x1 |
| RPS_PIN_RPS_PRNT | gpio-pca953x | 0x20 | 1 | 0x104 |
| RPS_PIN_54V_SW | gpio-pca953x | 0x24 | 0 | 0x1 |
| RPS_PIN_12V_SW | gpio-pca953x | 0x22 | 8 | 0x1 |
| RPS_PIN_54V_OC | gpio-pca953x | 0x23 | 0 | 0x5 |
| RPS_PIN_12V_OC | gpio-pca953x | 0x22 | 0 | 0x5 |
| RPS_PIN_54V_PSU_PG | gpio-pca953x | 0x24 | 10 | 0x1 |
| RPS_PIN_12V_OUT_OC_ORING | gpiolib-sysfs | (soc gpio 10) | - | 0x5 |
| RPS_PIN_54V_OUT_OC_ORING | gpio-pca953x | 0x27,bus0xb | 8 | 0x5 |
- Global Linux gpio for pca953x pins would be `expander_base + pin` once i2c_gpioexp_find_base resolves the addr's base — but these addresses (0x22,0x23,0x24) are the RPS-product expanders, not the UNVR's (@0x20/0x21/0x29). N/A for ea16.

## DELIVERABLE 3 — ttyRPS UART protocol (CONFIRMED from disasm)

### Baud / line settings — `rps_drv_uart_common_configure` @ .text 0x12790
- tcgetattr, then cfsetospeed+cfsetispeed to speed value; default (when caller passes NULL speed ptr) = **0x1002 = B115200**. -> **115200 baud**.
- c_cflag: clear CSIZE|CSTOPB|PARENB|PARODD (mask 0x7ffffe8f), set 0x8b0 = CS8|CREAD|CLOCAL -> **8N1, no parity, 1 stop**.
- c_lflag &= ~(ICANON|ECHO|ISIG...) ; c_iflag cleared (no IXON/flow) -> **raw, no flow control**.
- c_cc[VTIME]=0x0a (1.0 s inter-byte read timeout); tcflush(fd, TCIOFLUSH=2); tcsetattr(fd, TCSANOW).
- Called via ops-table fn pointer (uart-common driver), not a direct bl.

### Frame format — it is **line-delimited JSON**, NOT a binary frame w/ checksum
- TX (`rps_port_do_command` @ .text 0x82b0): builds a JSON request object, `json_dumps()`, then appends a single **`\n` (0x0a)** at `str[strlen]` (0x835c-0x8360), then writes buffer to the port fd. Frame = `<json-object>\n`.
- RX: reads bytes from UART; splits the buffer with `strtok_r` on delimiter **`"\r\n"`** (rodata 0x1dad8); each line `json_loads()`-parsed; response correlated by `"status"` field (rodata 0x1d958) and a request-id. Retry loop = 5 attempts (loop counter 0x5 @ 0x83e8).
- Errors: "Failed to write to RPS UART" (write fn @0x7f00/0x7f64), "select() timeout %ld sec. from RPS UART" (read fn @0x8200), "Reading from serial port error." (0x1da70), "unknown uart message: %s" (parser @0x8824) where message is a raw string.
- No start byte / length / checksum — it is a JSON-RPC-style text protocol over 115200 8N1. (Contrast the LCD-MCU `a0 0a 08 <cmd> 0b b0` binary frame — different bus.)
- ea16 uses **`/dev/ttyS2`** for this (from its board config), not /dev/ttyRPS1.

## DELIVERABLE 2 — ORing power-monitor IC

### From rpsd binary (CONFIRMED) — these are the parts rpsd knows
The rps_pse driver port template (config@f0x28b50, phy-1..6) lists TWO hwmon power monitors per port, all on **i2c bus 0xb (11)**:
| chip | i2c addr (phy1/2/3) | role |
|---|---|---|
| **hwmon-isl28022** (Renesas/Intersil ISL28022 bidirectional precision power monitor) | 0x44 / 0x45 / 0x46 | V/I/P monitor |
| **hwmon-ina230** (TI INA230 I2C current/power monitor w/ alert) | 0x40 / 0x41 / 0x42 | V/I/P + over-power alert |
- phy-1 pin sub-table tail also lists **hwmon-ina237** @0x49 and a second **hwmon-ina230** @0x43 (bus2). 
- rpsd reads them via raw i2c register R/W: fns `hwmon_read_i2c_regs`/`hwmon_write_i2c_regs`, `rps_drv_i2c_read/write`; log fmts `i2c:%d-0x%x failed reading register 0x%x`, `i2c:0x%x reg:0x%x val:0x%x`.
- The `oring_12v/54v_*_crit` alert is written through hwmon-attr/i2c-register helpers (`rps_set_oring12v/54v_power_crit[_enable]` @ .text 0xa8e8/0xa950, `__rps_set_default_oring_power_crit`). The attr->register map lives in the kernel ina230/isl28022 driver, not in rpsd. Over-power-limit = the monitor's power-limit/mask-enable + alert-limit registers (INA230 regs 0x06/0x07; INA237 0x0B/0x0C). power_crit register offset itself is NOT hard-coded in rpsd.
- **INA230/INA237 measure the ORed output V/I/P and raise the OC/over-power crit alert -> this IS the "dedicated ORing controller/monitor" of the doc.** ISL28022 is the bidirectional current monitor. High confidence on part family; addresses/bus confirmed from the descriptor.
- **Caveat for ea16:** the ea16 board config (f0x28990) header lists **no hwmon-*** entry — the UNVR's minimal RPS profile does not instantiate an oring hwmon in rpsd. The ISL28022/INA230 set belongs to the 6-port RPS-product profile. So on ea16, rpsd does present/load sensing + UART only; the on-board power monitors are likely driven by the kernel/other daemon, not rpsd's oring path.

### From photos (RPS.jpg) — physical chips at the RPS block
- **U122 = TI MAX3221C** (marking `MA3221C 9BKG4 CPK0` + TI logo, 16-pin TSSOP) = **RS-232 transceiver** -> the RPS PHY UART is level-shifted to RS-232 on the `JB4` connector. (Correction to the coordinator's guess: U122 is NOT the power monitor.)
- **U1** = ~16-pin QFN (~3-4 mm) ringed by sense resistors (R247/R249/R259/R260/R262/R250) and wired toward SW1/SW2 — analog front-end / power-monitor or hot-swap-controller class. **Marking not legible** (matte laser etch, no raking light).
- **U48** = ~10-pin QFN — **marking not legible**. Size matches an INA230/ISL28022 (10-pin) candidate but unconfirmed.
- **D500** = DPAK/SOT-223 3-terminal power part (marking `R03 / 8K01`) near Q536/Q537 — part of the ORing/protection FET path.
- Could not read a legible power-monitor part number off the board photos; the authoritative IDs are the binary's ISL28022/INA230/INA237.

## DELIVERABLE 4 — physical RPS IN connector (`JB4`) — from RPS.jpg + 225150.jpg

- Silk **`JB4`** = the large black right-angle **through-hole** connector. Contacts are **gold bent-blade** fingers, each soldered into a large plated through-hole (high-current capable).
- **Count ≈ 15-16 contacts**, single physical row, footprint uses a **staggered/2-column via** pattern (allows bigger copper pads per pin). All blades are **uniform size** — no visually distinct "big power blade vs small signal pin"; high-current rails are handled by **ganging several adjacent identical pins**.
- Rail identification (from board context, partial):
  - **54 V + 12 V power + GND** = the majority of the uniform blades (multiple ganged pins each) — they route into the FET/inductor/bulk-cap power section (Q536/Q537/Q14/Q59/Q590, L5/L7/L9, bulk electrolytics).
  - **UART (RS-232)** = a small group of pins on the side nearest **U122 (MAX3221)** — TX/RX at RS-232 levels (this is `/dev/ttyS2`).
  - **Sense/logic** = `RPS_PIN_RPS_PRNT` (present) and `RPS_PIN_12V_LP` (load) — SoC gpio 33/34 (see Deliv.1); plus PG lines per the pin set.
  - Exact per-pin blade->rail assignment needs continuity probing; not resolvable from images alone.
- Nearby (RPS.jpg): **`JB5` = `BP DCOUT1`** 4-pin (2x2) backplane-power output connector, silk **`HDD4_PWROFF`** — the ORed DC feeds the HDD backplane through it. FET bank Q536/Q537/Q14/Q59/Q590 + VR5 DC-DC form the ORing/hot-swap + regulation between `JB4` (RPS in) and `JB5` (backplane out). `FD1` (fuse/diode) and `RPS POWER ON` LED are on the input side (225150.jpg).

## Scripts
- `scripts/rps_walk.py` — ELF pointer-array walker (board table / descriptor / pin-table extraction).
- `scripts/crop_rps.py` — photo region cropper for IC-marking / pin-count reads.
