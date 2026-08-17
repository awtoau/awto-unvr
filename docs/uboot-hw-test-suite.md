# Modern U-Boot as a full hardware test suite (no-Linux bring-up shell)

Requirement (owner): the new U-Boot must **drive every peripheral on the board** and
provide a **complete no-Linux test suite** — power a drive bay on/off, blink every LED,
read the reset switch, read the RPS power monitor over I²C (voltage/current), measure
temps/fans, exercise DRAM/flash/network, etc. This is the bring-up + diagnostics tool
(and the thing that makes overclock/DDR-margin work safe).

Feature-parity target = vendor 2015.07 commands **plus** everything we RE'd this
session that the vendor never exposed.

## Hardware inventory → required U-Boot capability

Sources: [components.md](components.md), [gpio-switches-leds.md](gpio-switches-leds.md),
[rps-subsystem.md](rps-subsystem.md), [hardware.md](hardware.md),
[dt-gaps-hardware-of-record.md](dt-gaps-hardware-of-record.md).

| Subsystem | Hardware | U-Boot needs | Status / source |
|---|---|---|---|
| **Console** | uart0 `0xfd883000` (DW 8250, 3.3 V TTL header) | ns16550 driver | mainline ✅ |
| **2nd console / RPS UART** | uart2 `0xfd885000` → MAX3221 (U122) → `JB4` | ns16550 instance; **usable as a console** (owner request); 115200 8N1 for RPS JSON, 9600 for the LCD/MCU frame | mainline driver ✅, DT/config work |
| **Bay power on/off** | PCA9575 **@0x21** pins 0-3 = bay1-4 **pwren** (gpio 480-483) | i2c + pca953x GPIO; a `bay` command (on/off/status per bay) | vendor has none — **new** |
| **Bay presence** | @0x21 pins 4-7 (gpio 484-487), active-low | GPIO read | **new** |
| **Bay fault LEDs** | @0x21 pins 12-15 (gpio 492-495) | GPIO write | **new** |
| **Bay activity LEDs** | SGPO (74VHC595 `UB20`), sgpo lines 16-23 | vendor `sgpo` cmd equivalent | port from vendor |
| **Front LEDs** | `ulogo_blue` gpio 31, `ulogo_white` gpio 37 (PL061) | PL061 GPIO + `led` cmd | mainline gpio ✅ |
| **SFP LEDs** | PCA9575 @0x20 pin2 = SFP 1G link (gpio 498); gpio0.0 = "25g" | GPIO | **new** |
| **Reset switch** | gpio4.6 (gpio 38), active-low, PL061 | GPIO read + a `button`/test cmd | **new** (vendor reads it only in Linux) |
| **SW1 / SW2** | 2 tactile buttons, **GPIO unknown** (probe) | GPIO read once mapped | open |
| **RPS present / 12 V load** | gpio 33 (`rps_prnt`), gpio 34 (`12v_lp`) | GPIO read | **new** |
| **RPS power monitor** | INA230/ISL28022/INA237 class @ 0x40-0x49 on **`i2c_gen` @0xfd894000** (bus disabled in our DT!) | enable the 2nd DW i2c; INA/ISL driver → **V / I / power / OC** readout | **new — owner priority** |
| **Fan + board temps** | ADT7475 @ i2c-4 0x2e (behind PCA9546 @0x71 ch3) | i2c mux + adt7475 cmd (RPM read, PWM set) | vendor `slowfan` env only |
| **SoC die temp** | thermal @0xfd860a00 | `thermal_get` equivalent | port from vendor |
| **RTC** | S-35390A @0x30 (mux ch0) | i2c + `date` (vendor U-Boot has **no** `date`) | **new** |
| **Identity EEPROM** | AT24C64 @0x57 (2-byte addr) | i2c read + decode (sysid/MAC/serial) | partial in vendor |
| **SFP module** | Finisar FTLX8571D3BCL, EEPROM 0x50/0x51 (mux ch1) | i2c read + SFF-8079/8472 decode (DDM: temp, Vcc, TX/RX power) | **new** |
| **SFP retimer** | `br410` (per vendor `sfpd`), raw i2c writes | optional | **new** |
| **DRAM** | 4 GiB DDR4 (Samsung K4A8G16 ×4) | `mtest`, **DDR BIST / margins / shmoo / ECC** via the open HAL | see [uboot-ddr-port.md](uboot-ddr-port.md) |
| **NOR** | MX25U25635F, DW SSI @0xfd882000 | `sf` (mainline designware_spi) | mainline ✅ |
| **NAND** | MT29F8G08, al-nand @0xfa100000 | `nand` — needs the AL NAND driver ported | port from vendor |
| **SATA** | 2× AHCI (`1c36:0031`) behind internal PCIe | PCIe enum + AHCI + `scsi`/`sata` (**the vendor U-Boot can't read SATA — we fix this**) | port PCIe glue |
| **Ethernet 1G** | al_eth + AR8033 PHY (MDIO addr 4) | al_eth port + `mdio`/`mii`, ping/tftp | port from vendor |
| **Ethernet 10G** | SFP+ via SerDes, no PHY | serdes cfg + link mgmt | port from vendor |
| **USB** | ASM1042A xHCI (ext PCIe) | PCIe + xhci-pci + `usb` | mainline ✅ (needs PCIe) |
| **Watchdog / reset** | SP805 @0xfd88c000 | `reset` via SP805 (validated) | trivial |
| **Debug enable** | `cpus_secure @0xf0070008` bit0 DBGEN (RW, unlocked) | **default DBGEN on** at board init + a cmd to toggle | **new — owner request** |
| **Clocks / PLL** | CPU PLL `setup_0 @0xfd860d40` | `cpu_set_speed` (down) + **VCO raise for overclock #29** | port + extend |
| **Pinmux** | MUIO mux (73 ifaces) | `muio_mux` equivalent | port from vendor |
| **Boot instance / TOC** | flash TOC A/B/C/D | `flash_contents_*` equivalent | port from vendor |

## Proposed test-suite commands (`ubnt`/`hwtest` namespace)

- `hwtest all` — run every non-destructive check, print PASS/FAIL table.
- `bay <n> on|off|status` · `bay scan` (presence) · `bay led <n> on|off`
- `led list|on|off` (ulogo, SFP, bay fault/activity via SGPO)
- `btn` — read reset switch + SW1/SW2 + RPS present/12 V-load
- `pwr` — **RPS/ORing monitor: rail voltage, current, power, OC flags** (needs `i2c_gen`)
- `temp` — SoC die + ADT7475 temps; `fan [pwm]` — RPM read / PWM set
- `sfp` — module ID + SFF-8472 DDM (temp/Vcc/TX/RX power)
- `ddr test|margins|ecc` — BIST/shmoo (see uboot-ddr-port.md)
- `net test` — MDIO/PHY status, link, ping both ports
- `stor test` — NOR/NAND/SATA identify + read-back
- `dbg on|off|status` — DBGEN/NIDEN/SPIDEN (`0xf0070008`)

Rules: every test **non-destructive by default**; anything that writes flash or changes
power state requires an explicit flag. Print a machine-parsable PASS/FAIL line so
`console-tcl` scripts can drive the whole suite from the host.

## Gating work

1. **`i2c_gen @0xfd894000` must be enabled** — the RPS power monitor lives there and our
   DT disables it (also blocks the Linux-side read). Highest-value unblock.
2. PCIe enumeration glue → unlocks SATA + USB.
3. SGPO + AL-NAND + al_eth + serdes ports from the vendor tree.
4. DDR HAL integration (open source, see uboot-ddr-port.md) → BIST/margins.
