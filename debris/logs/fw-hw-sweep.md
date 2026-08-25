# UNVR firmware userland — undocumented hardware / debug sweep

Target: `sources/UNVR-5.1.25.bin` rootfs (squashfs @15242534, unsquashed to scratch).
Cross-refs: GPL `UNVR-1.3.35-GPL/{u-boot,linux-arm64-unvr-4.1.37-ubnt}`, delroth-alpine_hal,
`urnvr-kernel-4.19.152`. Board = `alpine_v2_64_ubnt_nas` (ea16). 2026-08-17.
Method: `scripts/fw-hw-sweep.py` (per-file strings→pattern scan) + targeted objdump/grep.
`confirmed` = string/reg/disasm cited; `inferred` = deduced, needs a live check.

Legend for evidence: `<file>` + the matching string, or `<hal-header>:<line>` for registers.

---

## 0. NEW undocumented HARDWARE access (the core deliverable)

| daemon/bin | hardware | interface | protocol/how | evidence | significance |
|---|---|---|---|---|---|
| **`/usr/sbin/sfpd`** (pkg `ubnt-sfp-handler`, systemd `sfpd.service`) | **SFP+ module** (DOM/EEPROM + a raw-I2C device, likely the SFP **retimer**) | `/dev/i2c-%d` via `libi2c.so` (`open_i2c_device`,`set_i2c_slave`,`i2c_smbus_{read,write}_byte_data`); **ETHTOOL_GMODULEEEPROM** for module EEPROM; **GPIO sysfs** | reads SFP module EEPROM via ethtool ioctl; does raw `i2c write to device %02x at register %02x data %02x` (register-poke a module-side chip = retimer/PHY); drives link speed/**FEC**; runs hooks `/usr/lib/ubnt/hooks/sfp/{linkup,linkchange}-bottom` | `strings usr/sbin/sfpd`: `i2c_smbus_write_byte_data`,`/dev/i2c-%d`,`[%s] i2c write to device %02x at register %02x data %02x`,`ETHTOOL_GMODULEEEPROM`,`ETHTOOL_GLINKSETTINGS` | **NEW daemon, never documented.** A second I2C hardware toucher besides the known set. The raw-I2C register writes are the undocumented part (candidate = the `br410` retimer named in dt-gaps C3). |
| **`sfpd`** | **`sfp-slot-3` and `sfp-slot-4` platform devices** (vendor-kernel driver, not in mainline) | sysfs `/sys/devices/platform/sfp-slot-{3,4}/{present,rx_los,tx_fault}` | polls module present / RX-LOS / TX-fault | `strings sfpd`: `/sys/devices/platform/sfp-slot-4/rx_los` (+slot-3, tx_fault, present) | **TWO sfp-slot platform nodes** referenced (3 and 4), but board doc lists 1 SFP+ cage. Either shared-family code (more slots on Pro/UXG) or the ea16 kernel instantiates both — a `sfp-slot` platform driver we have not cataloged. Grep found no such node in our DTS. *inferred* — confirm on live sysfs. |
| **`sfpd`** | **PCA9575@0x21 lines gpio 488/489/490** | `/sys/class/gpio/gpio{488,489,490}/value` | read/write during SFP insert/link | `strings sfpd` gpio488/489/490/value (alongside known 480/482/508/510/511) | **gpio 488–490 are undocumented** — they fall in the @0x21 expander gap (484–487 present, 492–495 fault). Our map leaves 488–491 unmapped; sfpd uses 488/489/490. *confirmed used, function inferred* (SFP-related). |

Everything else that touches hardware is **already documented**: rpsd→RPS(ttyS2+gpio33/34),
ubnthal→AT24C64, infctld/recoveryd→gpio38 reset btn, ustd/libuled→ADT7475+SGPO+pca9575 LEDs,
hci-device-up→(BT, unpopulated). No SECOND serial daemon: only `sfpd` (ethtool/i2c, not serial)
and `rpsd` (ttyS2). Literal `/dev/ttyS*` appears only in `hci-device-up` (BT, absent). Nothing
uses `/dev/mem`/`devmem`/`/dev/watchdog`/`/dev/mtd` from a vendor daemon at runtime (watchdog =
systemd+wdctl; mtd = mtd-utils + fwupdate).

`ustate` (Go, Unifi storage/accessory gRPC) references **storage-expansion** hardware:
`StorageExpansionThermals`, `indexedFan`, `PSU_CAPABILITY_{POWER_METER,CHARGE_CTRL}`,
`PeripheralHandler`/`PeripheralState`, `psu%d present but hwmon %s not found`. This is the
external drive-enclosure / PSU accessory path (hwmon-based). Applies to expansion units, not the
ea16 mainboard itself — noted, not a new on-board interface. *inferred, family code.*

---

## 1. JTAG / SWD / debug-port unlock  (TOP PRIORITY)

**Headline: JTAG/SWD invasive debug on the A57s is gated by an MMIO register that resets to
DISABLED but is plain RW and (on this board) is NOT locked by a fuse — it can be flipped from the
U-Boot console we already have.** JTAG is NOT on the pin-mux, so the unpopulated SoC-top-edge
2-row header is a dedicated debug connector, consistent with SWJ-DP (JTAG or 2-pin SWD).

### 1a. Debug-authentication register (the unlock)
- **`cpus_secure` @ `0xf0070008`** — northbridge nb-service block `0xf0070000` (confirmed base,
  hardware.md) + `struct al_nb_regs.global` @0x0 + `cpus_secure` @0x8.
  - bit0 **DBGEN** (invasive/halting debug + JTAG), bit1 **NIDEN** (non-invasive/trace),
    bit2 **SPIDEN** (secure invasive), bit3 **SPNIDEN** (secure non-invasive).
  - **Reset 0x0, Access RW** → at power-on invasive debug is OFF, but any code (U-Boot `mw`, a
    kernel poke, ATF) can set DBGEN=1 to bring the A57 DAP live.
  - Evidence: `delroth-alpine_hal/include/sys_fabric/al_hal_nb_regs_v3.h:616-636`
    (`NB_GLOBAL_CPUS_SECURE_DBGEN (1<<0)` … `SPNIDEN (1<<3)`), struct layout lines 18/532-534.
- **CoreSight CTI block @ `0xf0070400`** (`struct al_nb_coresight` @0x400) — CTI DBGEN/NIDEN
  select masks (`NB_CORESIGHT_CTI_1_TODBGENSEL_MASK`, header :1592-1602). Cross-trigger routing
  for the debug signals.

### 1b. The secure-boot debug lock (is it set here?)
- **`wr_once_dbg_dis_ovrd_reg` @ `0xfd8a81e0`** — PBS regfile `0xfd8a8000` + `0x1e0`. Comment:
  *"This register can be written only once. Use in the secure boot process."* bit0 =
  `WR_ONCE_DBG_DIS_OVRD`. Evidence: `al_hal_pbs_regs.h:428, 2298-2300`.
  - This is the latch secure boot uses to permanently kill debug. Our board runs unsigned env
    (`bad CRC, using default environment`) and is not in a locked secure-boot state → **likely
    never written**, i.e. debug is not fuse-disabled. *inferred — verify by reading it live.*

### 1c. JTAG/SWD is NOT pin-muxed → dedicated header
- The AL-324 MUIO pin-mux (`al_hal_muio_mux.h`, **73** `AL_MUIO_MUX_IF_*` interfaces) has entries
  for NOR/NAND/SRAM/SATA-LEDs/ETH-LEDs/ETH-GPIO/**UART_1/2/3**/GPIO — but **no JTAG/SWD/TEST/
  TRACE/DAP** interface. Evidence: `grep -cE AL_MUIO_MUX_IF_` = 73; targeted grep for JTAG/TEST/
  DEBUG/SWD/DAP = 0 hits (`al_hal_muio_mux.h`, `cmd_muio_mux.c`).
  → JTAG/SWD are on the SoC's dedicated debug pads, not shared with functional IO. The unpopulated
  **2-row header at the SoC top edge** (components.md JTAG-candidate) is therefore the plausible
  physical SWJ-DP tap (multi-pin JTAG or 2-pin SWDIO/SWCLK). *inferred; needs a macro + continuity.*

### 1d. Actionable path to a live TAP
1. At the **U-Boot console** (we have ttyS0): `mw.l 0xf0070008 0xf` sets DBGEN|NIDEN|SPIDEN|SPNIDEN.
   `md`/`mw` are stock in this U-Boot; `muio_mux` is a registered ubnt command
   (`cmd_muio_mux.c:174 U_BOOT_CMD`). This asserts SoC-level debug-enable before booting Linux.
2. Read `0xfd8a81e0` bit0 first — if 0, no secure-boot debug lock.
3. Physically probe the 2-row header for SWDIO/SWCLK (SWD) or TCK/TMS/TDI/TDO (JTAG).
- No firmware in the GPL/rootfs is seen WRITING DBGEN (grep of u-boot/HAL/kernel = only header
  defs). So debug-enable is left at reset (off) — we set it ourselves. That is the good case
  (nothing forces it off either). *confirmed for GPL sources; ATF/preboot is closed-blob, unknown.*

---

## 2. Power-saving / DVFS

- **No cpufreq/DVFS driver for Alpine.** `drivers/soc/alpine/` has HAL + pbs only; no cpufreq
  policy, no `cpu_set_speed`, no PLL down-clock in the kernel or in any userland daemon. CPU runs
  fixed ~1.7 GHz. PM = ARM **WFI idle + PSCI** (`arm,psci-0.2`, smc) only; no suspend/resume.
  Evidence: `find drivers/soc/alpine drivers/cpufreq` → no alpine cpufreq; hardware.md PSCI rows.
- **No al_thermal-driven throttle**: `al_thermal` isn't built (hardware.md #44), so no thermal
  zones and no CPU throttle. Fan control is the only thermal actuator (ADT7475 via userland PID,
  known).
- **Disk power**: known `ubnt-hdd-pwrctl` (gpio pwren) + per-bay load switches; staggered spin-up.
  No new userland spin-down knob found beyond smartd/hdparm (stock).
- Net: this platform has essentially **no dynamic power management** — a gap, not a hidden feature.

## 3. Reset paths

- **SP805 watchdog** ×4 @ `0xfd88c000` (`sp805_wdt`, known). Userland: systemd + `wdctl` only;
  no vendor daemon kicks a raw `/dev/watchdog`.
- **Fabric software reset @ `0xf007003c`** — nb global `fabric_software_reset`
  (`al_hal_nb_regs_v3.h:22`, struct +0x3c). MMIO SoC-fabric reset.
- **Per-CPU software reset (ANPA)** — `ANPA_GLOBAL_CPUS_SOFTWARE_RESET_LEVEL_CPU_PORESET`
  (`al_hal_sys_fabric_utils_v3.c:18`) — power-on-reset individual A57s from the anpa block.
- **Reboot action** = kernel `reboot=warm reboot=warm` cmdline (hardware.md); U-Boot `Boot: warm`.
- **Factory reset**: `infctld` (reset btn gpio38, 5–119 s hold) → `/sbin/reset2defaults` →
  writes `/boot/reset2defaults` + `/persistent/system/reset_reason`, runs reset hooks
  (`usr/lib/ubnt/hooks/system/reset-top/10-led-reset`). Known (gpio-switches-leds.md); MMIO reset
  registers above are the new detail.
- No reset triggered over serial/GPIO by any daemon beyond the known reset button + rpsd sense.

## 4. Firmware-update mechanism

- **`/sbin/fwupdate`** (pkg `ubnt-tools`, multi-call binary; siblings `fwsplit`/`fwinfo`/
  `fwextract`/`recv_image`).
- **Signature-verified**: OpenSSL **RSA + SHA1** — `EVP_sha1`,`EVP_VerifyFinal`,
  `-s … RSA public Key file`, `ERROR: Bad FW Image Signature`, `DEBUG: VerifyUpdate/Init/Final
  failed`. Image is rejected on bad signature. *confirmed.* (Public key is supplied as a file / TOC
  member; no private key present — see §6.)
- **Flash writes**: operates on `/proc/mtd` + **`/dev/mtdblock%d`** (also handles
  `/dev/mmcblk0p4` and `/dev/nvme0n1p4` for other SKUs). NAND/NOR partitions written by index.
- **Can target the bootloader**: `-t <target partition>` selectable, partition name **`uboot`**
  present in strings (default target rootfs). → a signed update **can rewrite U-Boot**, not just
  kernel/rootfs. *confirmed strings; A/B TOC instance handling present in the fw image format
  (fwsplit/fwinfo), not fully walked.*
- Helpers: `xz-inplace`,`zstd-inplace`,`fsync` — in-place decompress before flash.

## 5. Debug / backdoor / secrets

### Accounts / creds
- **`/etc/shadow`**: `root:$5$OFpXW38NQA/RG$cM3...` — root has a **set SHA256-crypt password**
  (not `!`/`*`). This is the shipped hash; typically overwritten by the user's UniFi console
  password on setup, but as-shipped it is a fixed hash (crackable offline; not attempted).
- **Second UID-0 account `ui`** in `/etc/passwd` (`ui:x:0:0::/root:/bin/bash`) — but in
  `/etc/shadow` it is **`ui:!`** (locked, no password). So no login as `ui`, but any process/PAM
  path that selects `ui` runs as root. *note.*
- No telnet/dropbear/adb/getty-on-extra-tty backdoor. Console: `serial-getty@ttyS0` (login on the
  UART console we use), stock `debug-shell.service` (systemd emergency root shell on tty9, not
  enabled by default).

### Signing / keys
- **No private key and no leaked signing key** in the rootfs — only stock CA bundles
  (`ca-certificates/mozilla/*`, gnupg keyring). The fw-verify **public** key is not a plaintext
  `.pem` in the tree (embedded in the binary or a TOC member); the corresponding **private** key is
  not present (as expected). No API tokens / hardcoded passwords surfaced in the vendor daemons.
- No "engineering mode" / "factory unlock" string or key-combo in the vendor tools (`libubnt`,
  `ubnt-systool`, `ubnt-tools`); the `factory`/`engineering` grep hits are stock (smartctl,
  tcpdump, hdparm) not UBNT unlocks.
- No debug env var that drops a shell in the vendor userland. (U-Boot has the usual `md/mw` which
  is itself the most powerful "debug backdoor" here — see §1d.)

## 6. Cross-version deltas

- **U-Boot is IDENTICAL across generations** — every captured boot (1.3.35 … 5.1.25) shows the
  same `2015.07-alpine_db-2.21-HAL`, built **Dec 16 2020** (hardware.md boot table). The GPL
  U-Boot tree (1.3.35) therefore describes the bootloader running under 5.1.25 too. → no
  version-specific JTAG/debug change in the bootloader; the §1 unlock applies to all versions.
- **Kernel**: 1.3.35 = `4.1.37-ubnt`; 1.4.9+ = `4.19.152`. The debug-auth registers (cpus_secure,
  pbs dbg-dis) are SoC HAL constants, unchanged.
- Rootfs cross-version diff **not performed** — only 5.1.25 extracted. The other bins
  (`sources/UNVR-{1.4.9,2.3.14,3.1.16,4.1.22}.bin`) are on disk; to find a stripped debug service
  or default cred present in an older userland, extract each and diff `/etc/{passwd,shadow}`,
  `/lib/systemd/system/`, and the vendor `sbin`/`usr/sbin` sets. Flagged as follow-up.

## 7. Other Alpine-V2 / UNVR-family shared code

- **The GPL U-Boot builds one binary for the whole family** — defconfigs present:
  `alpine_v2_64_ubnt_{nas,nas_pro,nas_hd,nas_ai,udm_pro_se,udc,uxgp,uxgp_lte,uxg_hd,plus,plus_rm,
  rm,u6wall,u6wall_pro}`. ea16 = `…_nas`. All share `board/annapurna-labs/alpine_ubnt` incl. the
  **UDM-Pro LCD/MCU over UART2** (`alpine_db/lcd.c`, HD44780 frame — matches libuled `mcu0`, known
  absent on ea16) and the 6-port RPS PSE profile (known, rpsd).
- **No other DEVICE firmware image on disk** to mine — `/mnt/2tb/unvr-port-refs` holds HAL/kernel
  source + community trees (`linux-alpine-v2`, `unifi-unvr-emu`, `UNVR-diy-os`, `UNVR-NAS`), and
  `sources/` holds only UNVR `.bin`s. To compare a device that DOES populate the LCD/MCU or a
  different debug posture, worth fetching (via playwrong/gh): **UDM-Pro** and **UDM-Pro-SE**
  firmware (same SoC, LCD populated), and **UNVR-Pro** (backplane/2nd expander @0x29, extra SFP
  slots — would confirm the `sfp-slot-3/4` two-slot question in §0). Not downloaded this session.

---

## Confirmed-vs-inferred summary
- **confirmed**: sfpd daemon + its i2c/ethtool/gpio/sfp-slot access; DBGEN/NIDEN/SPIDEN/SPNIDEN
  register + address; pbs wr-once debug-disable register + address; fabric/anpa reset registers;
  JTAG absent from MUIO mux; fwupdate RSA/SHA1 verify + mtdblock write + `uboot` target; root hash
  set / `ui` UID-0 locked; U-Boot identical across versions; family defconfig list.
- **inferred (needs live/physical check)**: two physical SFP slots vs shared code; that the
  wr-once debug latch is unwritten here (→ debug unlockable); the SoC-top header = SWJ-DP tap;
  sfpd gpio488–490 function; ATF/preboot doesn't disable debug (closed blob).
