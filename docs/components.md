# UNVR mainboard — master component / connector / test-point catalog

Board: **Ubiquiti UNVR**, Annapurna Labs **Alpine V2 / AL-324**, sysid **0xEA16**,
board `alpine_v2_ubnt one nas v5.0`, BOM `113-2832-29`, CM = **MSI**, UL `E248779 94V-0`.

- **Single reconciled index of every physical part, connector and test-point seen** across the
  130-photo vision sweep (`tmp/logs/photo-catalog/batch-0*-findings.md`) + curated docs.
- Mechanism/pin-map lives elsewhere — this is the parts list. Cross-links:
  [hardware.md](hardware.md) (silicon + MMIO), [gpio-switches-leds.md](gpio-switches-leds.md)
  (GPIO/LED/expander map), [rps-subsystem.md](rps-subsystem.md) (RPS), [chips/](chips/) (per-part).
- Status: **confirmed** (marking read or live-probed) · **uncertain** (worn/illegible/inferred) ·
  **unpopulated** (footprint, no part). Photo = best `docs/photos/20260816_<hhmmss>.jpg` frame.

> **Designator gotcha — SoC is U2, not U1.** [hardware.md](hardware.md)'s "Main silicon" table
> calls the SoC "U1" as shorthand; the **physical silk is U2**. **`U1` is a genuinely separate
> chip** — the **LMK00338 clock fan-out buffer** (confirmed) in the SATA/USB corner (see Unidentified).
> Do not conflate them.

---

## SoC & memory

| Ref | Part / verbatim marking | Function | Location (photo · area) | Status | Value / interest |
|---|---|---|---|---|---|
| U2 | Annapurna Labs **AL-324** — `AL32400-1700-A1-E-2BT-8-C`, die `T42B1190-01A 2BT-8 / 1950 TW` | Main SoC (4× A57 @1.7 GHz) | 232439/232501 · board center, heatspreader | confirmed | boot-critical; the target silicon |
| U3, U4, +2 (×4) | Samsung **K4A8G165WB** — `SEC 013 / K4A8G16 / 5WB BCRC` | DDR4 SDRAM, 8 Gb ×16 (**4 GiB total**) | 225226/232437 · around SoC | confirmed | boot-critical; ×4 bank, silk U5048/U5049 adjacent |

## Storage

| Ref | Part / verbatim marking | Function | Location (photo · area) | Status | Value / interest |
|---|---|---|---|---|---|
| U8 | Macronix **MX25U25635FM2I-10G** (SOP-16) | SPI-NOR 32 MiB, 1.8 V | 225318 · near TI-logic cluster | confirmed | boot-critical — SPL+U-Boot+env+identity+recovery. Live JEDEC `C2 25 39` |
| U12 | Micron **MT29F8G08ABBCAH4** — `8RFY / 9WC12 / NQ299` + Micron (BGA) | NAND 1 GiB SLC, 1.8 V | 230038/230232 · by I2C mux U40 | confirmed | boot medium (kernel+rootfs); actively ECC-correcting under 1.3.35 |
| U21 | Winbond **W25X05CL** — `Winbond / 25X05CLN16 / 2000` (SOIC-8) | SPI flash 512 Kbit | 230732 · SATA/USB corner | confirmed | ASM1042A (U20) config/FW store — commodity |
| — | Samsung **KLM4G1FE3B-B001** eMMC 4 GB (BGA) | eMMC boot (other SKUs) | not in photos | uncertain | **populated but hostless on ea16** — USB-eMMC bridge not fitted. Reshoot needed |

## Networking

| Ref | Part / verbatim marking | Function | Location (photo · area) | Status | Value / interest |
|---|---|---|---|---|---|
| U51 | Qualcomm Atheros **AR8033** — `AR8033-AL1A / SLMAAJWJ1 / 1507` (QFN-48) | 1G Ethernet **PHY** (RGMII, RJ45) | 225423 · RJ45 side | confirmed | the "Atheros switch" myth = this PHY; Linux `at803x` labels it "8031". [chips/ar8033.md](chips/ar8033.md) |
| Y8 | 25 MHz crystal — `T250 / Vp2P` (gold can) | AR8033 ref clock | 225426 · by U51 | confirmed ref | PHY clock |
| U20 | ASMedia **ASM1042A** — `asmedia / ASM1042A / C32V03178I / 1946` (QFN) | PCIe→USB 3.0 xHCI host | 225723 · SATA/USB corner | confirmed | carries the internal boot-USB (`/dev/sdq`); PCI `1b21:1142`. [chips/asm1042a.md](chips/asm1042a.md) |
| YA2 | 25 MHz crystal — `T200 / P+16` (metal can) | ASM1042A ref clock | 230718 · by U20 | confirmed | USB clock |
| — | **M-TEK G241035XG** — `M-TEK / G241035XG / 20132` (SMD SOIC-24) | Gigabit LAN magnetics module | 225435 · RJ45 side | confirmed | RJ45 magnetics (not a discrete magjack) |
| D1671, D1690 | dual-diode/TVS arrays (SOP-8) | Ethernet-pair ESD/Bob-Smith | 225433 · RJ45 side | uncertain | commodity ESD |
| R6394 (75 Ω), V1/V2 | Bob-Smith term; V1/V2 footprints | Ethernet common-mode/ESD termination | 225439 · under magnetics | uncertain | V1/V2 read as TVS footprints (batch 0) **or** small crystals `090`/`060` (batch 4) — unresolved |
| D53, D54 | SFP status LEDs (silk `SFP LED`) | SFP link/activity | 225454 · SFP area | confirmed silk | board supports SFP+ cage |

## Power / DC-DC

Point-of-load topology — **no single big PMIC**. Full tree in
[photo-catalog.md](photo-catalog.md#new--notable-parts-not-previously-cataloged).

| Ref | Part / verbatim marking | Function | Location (photo · area) | Status | Value / interest |
|---|---|---|---|---|---|
| VR1 | Richtek-class buck — `[R] AA0B / 0C19` + `R33` (0.33 µH) choke | **SoC-core VRM** (multi-phase) | 225352/232455 · above SoC | confirmed fn | boot-critical core rail; phases sense via R002/R005 |
| UB3 | uPI **uP1708P** — `uP1708P A5A944` (QFN) + `LB1` 6R8 (6.8 µH) | Buck controller | 230449 · BP-control area | confirmed | drives a POL buck rail |
| VR2 | Anpec-class buck — `ADEN / 5F13` + 1R0 (1.0 µH) choke | POL buck | 230759 · mid-board | uncertain part | one of several POL bucks |
| VR4 | buck/LDO (SOT-23-6, mark illegible) + power inductor | Local rail | 225239 · rail zone | uncertain | 12 V→rail POL |
| VR5, VR7, VR8, VR9, VR13 | regulator refs; VR13 = `SRA0G` (SOT-23-6) near USB | POL regulators / load switches | 230718 (VR13) · various | uncertain parts | distributed rails |
| UB16, UB19, UB36 | `C07J` (SOT-23-5) ×3 identical | Per-rail LDOs / load switches | 230107 · underside | uncertain | 3 rails |
| U17 | `AVW` (SOT-23-6) | Load switch / LDO / translator | 230047 · small-signal cluster | uncertain | |
| UB17, UB18 | `ES41 988 ADES` (SOT-23-6) | Load switch / supervisor | 230454 · BP-control area | uncertain | |
| U5055, U5056 | SOP-8 (illegible) + 3× SOT-23-6 | **HDD1 power load-switches** (`HDD1_PWROFF`) | 225734 · HDD1 SATA | uncertain part | per-bay staggered spin-up gating |
| U5075, U5076 | QFN + `C17J` (SOT-23-6) | SFP power / LED driver | 225459 · SFP area | uncertain | |
| Q532–Q540 (Q53x) | DPAK / SOT-23 MOSFETs; Q534 = `D5D0 / P03` | HDD power switching + `MB_DCIN` 12 V path | 225741 (Q534)/225723 · SATA corner | confirmed fn | main-input & per-bay power path |
| Q536, Q537, Q14, Q59, Q590 | ORing FET bank | **RPS ORing / hot-swap** | RPS.jpg · RPS area | inferred | see [rps-subsystem.md](rps-subsystem.md) |
| D500 | `D500 / P03 / 8KQ1` (DPAK FET/diode) | RPS ORing path | 232205 · power/RTC | confirmed | ORing element |
| R002 (2 mΩ), R005 (5 mΩ) | 2512 current-sense shunts | VRM phase current sense | 231327/232455 · SoC VRM | confirmed | power-monitoring |
| F152–F155 | SMD fuses (silk) | Rail protection | 230309 · power zone | confirmed silk | |
| FD1, FD3, FD4, FD5, FD6 | fuse/diode refs | Input protection | 225150 (FD1) · edges | uncertain | |
| J10 | 330 µF radial electrolytic — `330 EFK` | VRM bulk output | 232455 · SoC VRM | confirmed | bulk cap. CapXon 330 µF/6.3 V, 100 µF/47 µF-35 V also on VRM |

## I2C & sensors

Bus map + addresses in [gpio-switches-leds.md](gpio-switches-leds.md); mux in [chips/pca9546a.md](chips/pca9546a.md).

| Ref | Part / verbatim marking | Function | Location (photo · area) | Status | Value / interest |
|---|---|---|---|---|---|
| U40 | TI **TCA9546A** — `PW546A / 9CK G4 / AY5E` (TSSOP-16) | 4-ch I2C mux @0x71 | 230041 · by NAND | confirmed | **4-channel** (not 8); ch0=RTC, ch3=fan |
| U27 | AD **ADT7475ARQZ** — `ADT747 5ARQZ #1950` (TSSOP-16) | Fan/thermal PWM @0x2e | 230131 · fan zone | confirmed | live hwmon; temp1/2/3 + 4 fan. [chips/adt7475.md](chips/adt7475.md) |
| U5 | Atmel **AT24C64** — `ATMLH / 64DM CN / 2002 7P6` (SOIC-8) | Identity EEPROM (I2C) | 225217 · by SoC edge | confirmed | boot-critical — board identity/MAC. (SPD read at 0x57) |
| U5050 | Seiko **S-35390A** — `S353 / 90A / VK0592` (SOIC-8) | I2C RTC @0x30 (mux ch0) | 231551 · RTC cluster | confirmed | live `rtc-s35390a`. [chips/s-35390a.md](chips/s-35390a.md) |
| BAT1 | SII **MS621 / HS621** rechargeable Li coin cell | RTC backup | 231554 · RTC cluster | confirmed | **rechargeable, not CR2032** |
| Y4 | 32.768 kHz crystal — `GP902`? | RTC crystal | 231606 · by RTC | uncertain freq | presumed RTC xtal |

## GPIO / LED / switches

Full expander/line map in [gpio-switches-leds.md](gpio-switches-leds.md); [chips/pca9575.md](chips/pca9575.md).

| Ref | Part / verbatim marking | Function | Location (photo · area) | Status | Value / interest |
|---|---|---|---|---|---|
| UB1 | NXP **PCA9575** — `P 9575PW2 / 2C X263 / TuD1952` (TSSOP-24) | 16-bit I2C GPIO expander | 230555 · BP-control area | confirmed | bay pwr/present/fault (@0x21) |
| U10 | NXP **PCA9575** — `PCA9575PW2 / C TX263 / TuD1952` (TSSOP-24) | 16-bit I2C GPIO expander | 231415 · near SoC | confirmed | SFP-1G-LED + straps (@0x20) |
| — | (DTS `pca9575@0x29`) | Pro bays 5–8 expander | — | **unpopulated** | phantom node; probe EREMOTEIO every boot |
| UB20 | Toshiba **TC74VHC595** — `VHC 595 / 3950 / E4704A` (TSSOP-16) | 8-bit shift register (bay LEDs / SGPO) | 230431 · BP-control area | confirmed | drives SATA activity/presence LEDs |
| UB22 | (TSSOP-16 footprint, no chip) | 2nd 595 (daisy-chain) | 230442 · below UB20 | **unpopulated** | DNP option |
| SW1, SW2 | tactile pushbuttons (no function silk) | Unknown — factory/debug buttons | 231606 · by RTC | confirmed present | **GPIO unknown** (gpio 33/34 are RPS, not these); repurposable input — needs continuity probe |
| — | Front logo LED (`ulogo_blue` gpio31 / `ulogo_white` gpio37) | Status LED | front panel | confirmed | live-controllable on Fedora; not a discrete board part |
| — | `RPS POWER ON` LED (SMD, rear) | RPS rail indicator | 225150 · RPS edge | confirmed | hardwired rail indicator, not GPIO |

## Connectors & headers

| Ref | Description | Function | Location (photo · area) | Status | Value / interest |
|---|---|---|---|---|---|
| **UART console** | 4-pin 0.1″ header — GND/TXD/RXD/3V3, 3.3 V TTL, 115200 8N1 | `ttyS0` console | behind SFP+ cage, mid-PCB | confirmed | **primary console** — use first 3 pins, do NOT wire 3V3. Candidate = 4-pin near `FD5` (231921) |
| **JB4** | `RPS IN` — ~15–16 gold blade contacts, staggered 2-col via | RPS 54 V/12 V/GND + RS-232 UART (ttyS2) | RPS.jpg · rear edge | confirmed | rails ganged; per-blade map needs probe. [rps-subsystem.md](rps-subsystem.md) |
| **MB_DCIN** | white keyed Molex-type, ~4-pin | Main 12 V DC input | 225723 · SATA corner | confirmed | pin count needs straight-on shot |
| **J86** | `BP CONTROL` 2×8 = 16-pin header | Backplane control (presence/LED/power-seq) | 225519 · BP-control area | confirmed | series R on lower row; probe for signal names |
| **J92** | `RST BTN CONN` 2-pin JST, red+black | Reset-button cable | 230512 · reset edge | confirmed | → gpio 38 `KEY_RESTART` |
| **JB5** | `BP DCOUT1` 4-pin (2×2) | Backplane DC-power out (silk `HDD4_PWROFF`) | 232455 · VRM/BP edge | confirmed | |
| BP_DCOUT2 | silk header | Backplane DC out (2nd) | 225706 · power area | confirmed silk | |
| J84 | gold edge-finger connector | Drive/backplane SATA-style edge | 231702 · backplane edge | uncertain | |
| J87 | white 4-pin JST + 4-wire cable | HDD1 caddy LED / thermal / fan | 225739 · HDD1 | uncertain fn | |
| HDD1–HDD4 | SATA metal edge connectors (`1952`) | Drive-caddy data+power | 225550/225734 · bays | confirmed | 4 hot-swap bays |
| J44, J46, J48, J50, J88, J91, JB6, JB7 | misc headers/jumpers (2-pin & board-to-board) | Board-to-board / jumper / test | 225542/230454/230512 · various | uncertain | mostly small; J46 = named BP connector |
| — | microSD/SIM-style card socket edge | unknown | 225723 · SATA corner | uncertain | partially in frame; purpose unread |

## Test points

All silk is **generic `TPnnn`** — **no signal-named pads (no TCK/TDO/TMS/TX/RX/BOOT) anywhere in
130 photos**. Deliberate clusters and the JTAG lead:

| Cluster | TPs | Location (photo · area) | Interest |
|---|---|---|---|
| **★ JTAG-candidate header** | unpopulated **2-row PTH** (~2×7–8) hugging SoC top edge, by silk `C24/C25/C7835/C7836` | 231327/232437/232446/232501 · SoC top edge | **best debug/JTAG/SWD lead** — classic footprint, no signal silk. Needs straight-down macro |
| SoC probe cluster | TP5, TP6, TP34, TP36, TP37, TP39, TP40, TP45, TP46, TP49 (curved gold-pad row) | 231327 · around SoC/VRM | dense probe access at SoC; sits with the header above |
| NAND row | TP93, TP94, TP95, TP96, TP98, TP103 (each + series R) | 225941/225955 · by U12/U40 | 6-pad bus-probe row; low JTAG-likelihood (spread round pads, series R) |
| U1/OSC1/U20 corner | TP184, TP185, TP186, TP187 (4 gold PTH) | 230622/230630 · SATA/USB corner | 4-hole row (once mistaken for header); labeled TP18x |
| EEPROM/SoC edge | TP9, TP10, TP18, TP19, TP20, TP47, TP250 | 225217 · by U5 | probe pads |
| Center cluster | TP287, TP288, TP289, TP290, TP291, TP292 | 230121 · mid-board | 6-pad group (diff-pair/clock/power?) |
| **Secondary header** | 4-pin inline PTH, pin-1 square, near silk `FD5` | 231921 · near U27/PHY | candidate UART/debug/fan header — may be the console header |
| Isolated (generic) | TP3, TP7, TP8, TP11, TP22, TP56, TP57, TP59, TP82, TP99, TP101, TP102, TP107, TP109–111, TP122, TP138, TP229, TP236, TP237, TP248, TP252, TP254, TP286, TP294, TP295, TP619; TPA164–190; TPB6, TPB7; PA165, PA166 | scattered | numeric only; no named signals |

**Power-sequencing silk (named nets, not JTAG):** `HDD1/2/3/4_PWROFF`, `DDR2_PWROFF`, `PWROFF` —
per-drive + DDR staggered gating (231327/232437 SoC-left edge).

## Unidentified / to-chase

| Ref | What / marking | Best guess | Location (photo · area) | Status | Value / interest |
|---|---|---|---|---|---|
| **U1** | 40-pin WQFN 6×6mm (10 pins/side, counted [photos/u1-lmk00338-clock-buffer.jpg](photos/u1-lmk00338-clock-buffer.jpg)) + own osc **OSC1**; NS logo + `9?ARF4G3 / K00338` = LMK00338 ("LM" dropped, legacy National mark) | **National/TI LMK00338** 8-output HCSL clock fan-out buffer (1 ref in → 8 diff pairs, 2 banks of 4). Pin-match: pkg 40-WQFN = datasheet RTA (48-pin was wrong); `OSC1` → OSCin/CLKin; `RA6xx` array bank = series-R on the 8 HCSL diff outputs. NOT a switch/SATA ctrl — topology wrong for both; AL-324 (2 SATA + 2 MACs) needs neither. 2019 codes fit. [sources/lmk00338.pdf] | 230619/225706 · SATA/USB corner | **confirmed** | resolves the "is there a switch?" open item |
| OSC1 | 4-pad SMD can oscillator, mark illegible | U1 (LMK00338) reference clock — single osc feeds the buffer input | 230614 · by U1 | confirmed role | |
| U48 | ~10-pin QFN, marking illegible | RPS ORing power-monitor candidate | RPS area | uncertain | ORing monitor — macro needed. [rps-subsystem.md](rps-subsystem.md) |
| U5052 | `P617A / 03 01 / 948` (MSOP-8/10) | current-sense amp / supervisor / small analog | 225411/230555 · near RTC/shunt | uncertain | not in known list |
| U13 | `64?B / 7604`? (QFN ~16-lead) | small logic/controller | 225407 · by reset edge | uncertain | |
| U16 | `1400 / ACR / AGHS` (SO-8) | small analog/logic | 232437 · by SoC | uncertain | |
| U6 | SOIC-8, code rotated/illegible | EEPROM / sensor / driver | 230219 · near 4-pin PTH | uncertain | flag for close shot |
| U9 | `L25ZZ`? (SOT-23-6) | small regulator/switch | 231927 · by NAND | uncertain | |
| UB14 | `3082?` (small QFN/DFN) | small IC | 232249 · by U8 NOR | uncertain | |
| UB21 | small QFN/DFN, no legible mark | RTC-adjacent (charge/level for MS621?) | 231554 · by RTC | uncertain | |
| U50 | small package, mark unread | PHY-related regulator | 225426 · by U51 | uncertain | |
| — | wide SOP-28-ish, `…4S1G` (vertical) | buffer/flash/translator | 231702 · by V1/V2 | uncertain | |
| U49, U14, U44, U48, U5053, U5054 | TI **YF04E / YE04** "04" family — `YF04E / TI 02K`, `YE04 / 01K G4 / AN6J` | single-gate logic / level-translate | 230543 · logic cluster | confirmed family | glue logic (uncertain exact P/N); U48 double-listed as RPS candidate |
| U11, U15, U46, U5046, U5072, U5073, U5074 | silk refs, bodies unread | — | various | uncertain | low priority |

---

### Ranked chase list

1. **Macro the SoC-top-edge 2-row header** → confirm/deny JTAG/SWD (best lead).
2. ~~Macro U1~~ — **CONFIRMED LMK00338**: 40-WQFN (10 pins/side) + NS-logo `K00338` marking + OSC1 + RA6xx output arrays. Not a switch/SATA ctrl.
3. **Continuity-probe** JB4 per-blade rail map + SW1/SW2 GPIO.
4. Macro `MB_DCIN` pin count; ID `U48` (RPS monitor), `U5052`, `U13`, `U16`.
