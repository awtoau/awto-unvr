# Making our DTS the hardware-of-record — what's configured outside the DT

Reverse-engineered by diffing our ea16 DTS against the live vendor DTB
(`hw-reference/20260816-104601/live.dts`), vendor GPL U-Boot `board.c`, `ubnthal.ko`,
and the 5.1.25 userland. 2026-08-17.

**Architecture fact:** Ubiquiti U-Boot builds a **`/soc/board-cfg` DT subtree at
runtime and consumes it itself** — pinmux, GPIO init, SGPO LED behaviour, SerDes
analog params, per-port eth/SFP/retimer. Linux never sees most of it (al_eth/al_sgpo/
al_serdes are configured by U-Boot before Linux, or run off PCI IDs). So our bare DTS
**boots correctly but is not the hardware description.** To make it the record, fold
the items below in.

## Surprises

- **S1 — MCU + front-panel LCD: code present, NOT populated on UNVR-4.** U-Boot
  `power_init_board()→lcd_init()` writes to a 16×2 HD44780 MCU over **UART2** (frame
  `a0 0a 08 <cmd> 0b b0`); userland `libuled` has `ui::McuLED` → `/sys/class/leds/mcu0`.
  This is the **UDM-Pro** path — **no `mcu0` in the live sysfs/gpio on ea16**. No
  phantom MCU on our board; no DT node needed.
- **S2 — Bluetooth CSR8811 (BT4.2), and our DTS mislabels the UART.** `hci-device-up`
  runs `hciattach` on **`/dev/ttyS1` or `/dev/ttyS3`** with `csr8x11-…psr` firmware;
  `ubnthal` EEPROM has `BtMACAddrCount`/`bt%d.macaddr`. **Our DTS labels uart2/ttyS2 as
  "Bluetooth CSR" — wrong UART** (vendor never uses ttyS2). Either BT sits on ttyS1/S3,
  or UNVR doesn't populate it. **Owner-confirmed: NO Bluetooth on this board.** So the
  CSR8811 is shared-platform code only; **our DTS `ttyS2` "Bluetooth CSR" label is wrong**
  — remove/relabel it (no `bluetooth` node needed).
- **S3 — RPS/PSE is a hidden power-monitor + UART + expander subsystem (Pro).** `rpsd`
  drives 12 V/54 V enable, over-current, `54V_STBY_PG`, battery-guard, and
  `oring12v/54v power_crit` **power monitoring** (dedicated ORing power-monitor IC over
  I²C), via PCA9575 GPIO **plus a per-port RPS UART** (`/dev/ttyRPS1..8`). **Owner-
  confirmed: the RPS connector IS populated on this 4-bay board — NOT Pro-only.** Full
  mechanism / protocol / pins in [rps-subsystem.md](rps-subsystem.md).

## Confirmed gaps in our ea16 DTS (actionable)

| # | Gap | What the vendor sets | DT to add |
|---|---|---|---|
| **C1** | **SGPO LED behaviour** | `sgpo_init`: `group_mode="two"`, `sata_mode="active-presence"`, per-group mode/init/invert/stretch/blink masks + timing (`live.dts:1208-1255`) — consumed by U-Boot AND the `al-sgpo` driver | replicate `sgpo_init` props on our `sgpo` node (else SATA LEDs don't blink/stretch as designed) |
| **C2** | SerDes per-lane TX EQ | `board-cfg/serdes` amp/post/pre-emph per lane: SATA grp1&2 (amp7,post6), 10G grp3 lane0 (amp7,post7,pre1) (`live.dts:1257-1548`) | keep in U-Boot; **document** (needed only if we own SerDes init) |
| **C3** | eth2 SFP mgmt bus + retimer | `port2 i2c-id=2` (SFP DOM/EEPROM bus); `retimer{br410,i2c 1:0x56,ch B,disabled}` (`live.dts:1573-1604`) | `sfp` node on the cage i2c + `sfp=<&sfp>` on the 10G port — only if going mainline phylink |
| **C4** | **SFP+ 1G-link LED** (`sfp_1g`) | PCA9575@0x20 pin2 = global gpio **498**, via al_eth `gpio_spd_1g` (`board.c:787`) — only a *comment* in our DTS | `gpio-leds` child `led-sfp-1g { gpios=<&i2c_gpio0 2 ...> }` |
| **C5** | **PCA9575@0x20 init + straps** | `lines-initial-states=0xd688`, `-vals=0x123`, `baseidx=0x1f0` (`board.c:548`); pins 12/14/15 (gpio 508/510/511)=strap/status/present inputs | initial-states/vals or explicit `gpio-hog`s |
| **C6** | `al-sata-sw-leds` incomplete | vendor `led@0..7` (both AHCI ctrls, 8 ports) each with `fault-led=<0x1ec..0x1ef>` (`live.dts:1666`); ours has `led@0..3`, no fault-led | add `led@4..7` + `fault-led` phandle (or document 4-bay subset) |
| **C7** | `25g` SFP-speed LED | gpio0.0 labelled `25g` (al_eth `gpio_spd_25g`, `board.c:812`), off on 10G | minor — gpio-led or leave |

## Mechanism / identity (the "master key", not DT hardware)

- **M1 — `ubnthal.ko`** reads the **AT24C64 identity EEPROM** (= U5, the `pld_i2c_addr=0x57`
  chip, separate from the SPI-NOR identity block), creates `/proc/ubnthal/*`, exports
  `ubnthal_get_systemid/serialno/…/eth_macaddr`. EEPROM schema: `boardid, boardrevision,
  bomrev, systemid, serialno, hwaddrbase, EthMACAddrCount, BtMACAddrCount, bt%d.macaddr,
  eth%d.macaddr, vendorid, manufid, mfgweek, qrid, regdmn, lcm_count`. Optional DT hook:
  `ubnthal,write-protect` (dmesg logs it missing) to gate EEPROM writes.
- **M2 — pinmux** (`board-cfg/pinctrl_init`): U-Boot muxes pads (`if_nand_*`, `if_uart_2`,
  `if_eth_2_led`, `if_sgpo_*`…) (`board.c:419`). Linux `alpine-pinctrl` node is passive.
  Stays in U-Boot; captured in `live.dts`.
- **M3 — 2nd per-device EEPROM** (power/DDR margins) — unpopulated here ("not valid,
  using defaults", `board.c:1571`).

## Non-gaps (checked, no action)

- i2c `1-0031..37` "dummy" = the S-35390A RTC's 8-address command scheme, not chips.
- PCA9575@0x29 = Pro bays 5-8, EREMOTEIO here (unpopulated) — correctly absent.
- ADT7475 in DTS; `ustdbox` is the userland PID fan controller. SoC-die thermal still
  needs `al_thermal` (#44).

## Ranked action list (DTS as hardware-of-record)

1. **C1** SGPO blink/group config — functional (SATA LEDs).
2. **C5** PCA9575@0x20 initial states + strap inputs.
3. **C4** `sfp_1g` LED node.
4. **C6** complete `sata-leds` (8 + fault-led) or document 4-bay subset.
5. **S2** fix the UART2 "Bluetooth" mislabel; if BT populated, `bluetooth` node on
   ttyS1/ttyS3 — **needs hardware probe**.
6. **C3** SFP DOM i2c / `sfp` node — only if migrating 10G to phylink.
7. **M1** optional `ubnthal,write-protect`.
8. **C2** SerDes TX params — keep in U-Boot; document only.

**Owner-resolved:** BT is **not** populated (remove the wrong `ttyS2` label); the RPS
connector **is** populated (see [rps-subsystem.md](rps-subsystem.md)). Remaining RPS
work: identify the ORing power-monitor IC (I²C addr + marking), finish the `rpsd`
static pin-number walk, and macro-shot the `RPS IN` connector contacts.
