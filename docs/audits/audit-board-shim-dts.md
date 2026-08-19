# Source-of-truth audit — board.c / HAL shim / DTS / defconfig

Adversarial audit of the UNVR U-Boot port. Every magic constant, register
offset, bitmask and config value cross-checked against the vendor kernel,
the delroth HAL, `pcie-al-internal.c`, the ARM SP805 driver, and repo docs.

Date: 2026-08-19. Scope + sources per the audit brief.

**Headline:** the DMA-coherency-critical path (#74 AXI-snoop fix) is a
**byte-for-byte match** against `pcie-al-internal.c`. No fabricated constants of
the al_eth ring-size class were found. All hard register facts (SP805, DDR
reg-names, adt7475, UART/GPIO bases, PCA9575 LED map) match source-of-truth. The
two i2c deviations (400 kHz, sda-hold 300 ns) are deliberate and CI-guard-enforced.

---

## Full cross-check table

| our symbol/value | our file:line | source-of-truth value | source file:line | VERDICT | fix |
|---|---|---|---|---|---|
| **#74 AXI-snoop fix** |||||
| `AL_SMCC 0x110` | alpine.c:173 | `AL_ADAPTER_SMCC 0x110` | pcie-al-internal.c:60 | **match** | — |
| `AL_SMCC_BUNDLE 0x20` | alpine.c:174 | `AL_ADAPTER_SMCC_BUNDLE_SIZE 0x20` | pcie-al-internal.c:61 | **match** | — |
| `AL_SMCC_SNOOP 0x3` (OVR\|EN) | alpine.c:175 | `SNOOP_OVR BIT(0)\|SNOOP_EN BIT(1)=0x3` | pcie-al-internal.c:62-65 | **match** | — |
| `AL_APP_CONTROL 0x220` | alpine.c:176 | `AL_ADAPTER_APP_CONTROL 0x220` | pcie-al-internal.c:76 | **match** | — |
| `AL_APP_LO16 0x3ff` | alpine.c:177 | `AL_ADAPTER_APP_CONTROL_LO16 0x03ff` | pcie-al-internal.c:77 | **match** | — |
| `AL_SLOT_THRESH 5` | alpine.c:178 | `AL_INTERNAL_SLOT_THRESHOLD 5` | pcie-al-internal.c:85 | **match** | — |
| `AL_VENDOR 0x1c36` | alpine.c:179 | `PCI_VENDOR_ID_ANNAPURNA_LABS 0x1c36` | pcie-al-internal.c:88 | **match** | — |
| SM1-3 loop `i=1;i<4` writes SM0 val | alpine.c:189-191 | `for i=1..3 write val` | pcie-al-internal.c:153-159 | **match** | — |
| APP_CONTROL `(v&0xffff0000)\|0x3ff` | alpine.c:194 | identical RMW | pcie-al-internal.c:167 | **match** | — |
| filter: **vendor only** | alpine.c:212 | vendor **AND** `PCI_FUNC==0` **AND** root-bus | pcie-al-internal.c:120,132,135 | **mismatch (looser)** | add func-0 + root-bus guard (see MUST-FIX #2) |
| **SP805 watchdog reset** |||||
| `SP805_WDT_BASE 0xfd88c000` | alpine.c:307 | `wdt0 @ 0xfd88c000` | ea16.dts:164 / hardware.md:232 | **match** | — |
| Lock `0xC00 = 0x1ACCE551` | alpine.c:310 | `WDTLOCK 0xC00, UNLOCK 0x1ACCE551` | sp805_wdt.c:53-54 | **match** | — |
| Load `0x000` | alpine.c:311 | `WDTLOAD 0x000` | sp805_wdt.c:40 | **match** | — |
| Control `0x008 = 0x3` (INTEN\|RESEN) | alpine.c:312 | `WDTCONTROL 0x008, ENABLE_MASK=INT(1<<0)\|RESET(1<<1)=0x3` | sp805_wdt.c:44-48 | **match** | — |
| Load value `0x100` (=256) | alpine.c:311 | arbitrary small (LOAD_MIN=1) | sp805_wdt.c:41 | **match** (valid) | comment "~2 counts" is wrong — 256 counts ×2-expiry; harmless |
| **adt7475 fan** |||||
| PWM duty regs `0x30-0x32` | alpine.c:273,282 | stock `slowfan` regs 0x30-0x32 | i2c-map.md:27 / datasheet PWMxCUR | **match** | — |
| PWM config regs `0x5c-0x5e` | alpine.c:281 | stock `slowfan` regs 0x5c-0x5e (PWMxCONFIG) | i2c-map.md:27 | **match** | — |
| config value `0xe8` = manual | alpine.c:281 | PWMxCONFIG BHVR[7:5]=111=manual (0xe8=1110_1000) | ADT7475 datasheet | **match** | — |
| adt7475 addr `0x2e`, bus 2/ch3 | alpine.c:265 | `hwmon@2e` mux ch3 | ea16.dts:307-315 / i2c-map.md:16 | **match** | verify DM busnum on box |
| **S-35390A RTC** |||||
| addr `0x30`, bus 1/ch0 | alpine.c:124 | `rtc@30` mux ch0 | ea16.dts:295-303 / i2c-map.md:13 | **match** | verify DM busnum on box |
| STATUS1 flags POC 0x01/BLD 0x02/24H 0x40/RESET 0x80 | alpine.c:99-102 | S-35390A datasheet STATUS1 | (datasheet) | **match** (LSB-first via s35_rev, correct) | — |
| **HAL plat shim** |||||
| reg accessors → `readl/writel` (barriered) | plat_services.h:42-52 | vendor uses `__raw_readl` (unbarriered) | vendor al_hal_plat_services.h:80-88 | **match** (stronger, safe) | — |
| `al_reg_read32_relaxed`→`readl_relaxed` | plat_services.h:46 | vendor: read-relaxed absent, write only | vendor:87 | **match** (added, harmless) | — |
| `al_data_memory_barrier`=`dsb sy` | plat_services.h:80-83 | vendor `dmb()` | vendor:231 | **match** (stronger) | — |
| `al_local_data_memory_barrier`=`dmb ish` | plat_services.h:85-88 | vendor `dmb()` | vendor:235 | **match** (Device access ordered regardless of domain) | — |
| `al_smp_data_memory_barrier`=`dmb ish` | plat_services.h:90-93 | vendor: **empty no-op** | vendor:233 | **match** (stronger, harmless) | — |
| `al_assert` non-fatal print | plat_services.h:63-68 | sample: `exit(-1)` | sample plat_services.h:210-218 | **match** (intended: live diag, must not wedge console) | — |
| `al_phys_addr_t = uint64_t` | plat_types.h:29 | LPAE 64-bit phys | (HAL convention) | **match** | — |
| little-endian `#error` guard | plat_services.h:105-107 | sample identical guard | sample:269-271 | **match** | — |
| **DTS — memory-controller** |||||
| nb `0xf0070000`/0x10000 | dts:141 | `AL_NB_SERVICE_BASE +0x70000` | uboot-ddr-port.md:150 / ea16.dts:127 | **match** | — |
| ctrl `0xf0080000`/0x8000 | dts:142 | `AL_NB_DDR_CTL_BASE +0x80000` | uboot-ddr-port.md:151 / ea16.dts:151 | **match** | — |
| phy `0xf0088000`/0x8000 | dts:143 | `AL_NB_DDR_PHY_BASE +0x88000` (inside mc 64K window) | uboot-ddr-port.md:152 | **match** | — |
| reg-names `nb/ctrl/phy` | dts:144 | consumed by al_ddr ofnode lookup | uboot-ddr-port.md:162 | **match** (non-overlapping: 70000+10000=80000, +8000=88000, +8000=90000) | — |
| **DTS — serdes** |||||
| serdes PMA `0xfd8c0000`/0x2400 | dts:161 | `serdes @ 0xfd8c0000` size 0x2400 | ea16.dts:145-148 / hardware.md:239 | **match** | — |
| pcs 2nd reg `0xfe120000`/0x10000 | dts:162 | **no source** — self-flagged unverified in the node | dts:154-158 | **fabricated-no-source** (honestly flagged) | confirm PCS base on HW or drop (diag-only, cannot wedge boot) |
| **DTS — UART / GPIO / timer** |||||
| uart0 `0xfd883000` int SPI17 | dts:104-106 | `serial@1883000` +0xfc000000, SPI17 | alpine-v2.dtsi:161-164 / hardware.md:223 | **match** | — |
| uart2 `0xfd885000` int SPI19 | dts:120-123 | `serial@1885000`, SPI19 | alpine-v2.dtsi:181-184 / hardware.md:225 | **match** | — |
| gpio0-5 bases fd887/888/889/88a/88b/897000 | dts:213-252 | identical bases | ea16.dts:206-264 | **match** | — |
| pl061 GPIODIR `0x400` (1=out), data addr-masked `1<<(off+2)` | pl061.c:19,26-29 | PL061 TRM; stock al_gpio.c | (ARM PL061 TRM) | **match** | — |
| timer PPI 13/14/11/10 type 8 (LOW) | dts:67 | vendor dtsi armv8-timer | alpine-v2.dtsi:95 | **match** | — |
| **DTS — i2c (deliberate deviations)** |||||
| clock-frequency `400000` | dts:269 | vendor kernel `100000`; **repo canonical 400000** | check-dts-shared.py:22 / ea16.dts:283 | **deliberate** (guard-enforced, rated-max) | — |
| i2c-sda-hold-time-ns `300` | dts:273 | mainline dropped; **live.dts 0x12c=300; repo canonical 300** | check-dts-shared.py:23 | **deliberate** (guard-enforced; RTC-wedge fix #78/#86) | — |
| PCA9546 mux `@0x71` ch0/ch3 | dts:317-345 | `i2c-mux@71` same topology | ea16.dts:288-317 | **match** | — |
| AT24C64 `@0x57` pagesize 32 | dts:304-308 | 24C64 DDR-blob EEPROM | i2c-map.md:11,36 | **match** | — |
| **DTS — LED / hog map** |||||
| led d1-d4 → i2c_gpio1 lines 12-15 | dts:370-373 | fault-led lines 12-15 | ea16.dts:706-727 | **match** | — |
| led white → gpio4.5, blue → gpio3.7 | dts:375-376 | ulogo_white gpio4.5 / blue gpio3.7 | ea16.dts:683-689 | **match** | — |
| led sfp → i2c_gpio0 pin2 | dts:374 | SFP LED pin 2 | i2c-map.md:25 | **match** | — |
| hdd-pwren hog lines 0-3 output-high (4 nodes) | dts:295-298 | single hog gpios 0-3 (Linux) | ea16.dts:342-347 | **match** (split: U-Boot hog takes only first gpio — correct workaround) | — |
| **defconfig / MMU** |||||
| TEXT_BASE `0x01100000` | defconfig:3 | chainload entry `go 0x1100000` | alpine.c:5 | **match** | — |
| SP addr `0x05000000` | defconfig:5 | above TEXT+MALLOC, below memtest | (derived) | **match** | — |
| ENV_SIZE `0x10000` | defconfig:8 | env partition size 0x10000 | ea16.dts:560-568 | **match** | — |
| MEMTEST 0x10000000-0x40000000 | defconfig:65-66 | above SP 0x05000000 | (derived) | **match** | — |
| SYS_BOOTM_LEN `0x4000000` (64MB) | defconfig:91 | > ~57MB uncompressed Image | defconfig:85-86 | **match** | — |
| BOOTCOMMAND canary `0xb0075709 @0x10000000` | defconfig:32 | 16-aligned sentinel, itest.l-safe | defconfig:27-30 | **match** | — |
| kernel@0x02000000 + DTB@0x04078000 | defconfig:32 | — | — | **WATCH** | possible decompression overlap — MUST-FIX #1 |
| MMU device range 0xC0000000..0x100000000 NGnRnE | alpine.c:52-58 | covers fb/fd/f0 MMIO + fe PCIe window | hardware.md MMIO map | **match** | — |
| MMU DRAM banks 0/0xC0000000 + 0x200000000/0x40000000 | alpine.c:46-64 | 3GB@0 + 1GB@8GB | ea16.dts:64-70 / dts:71-75 | **match** | — |
| **board.c — A57 debug enable** |||||
| `AL_CPUS_SECURE 0xf0070008` = 0xf | alpine.c:32-33,85 | nb-service +0x8 cpus_secure, bits DBGEN/NIDEN/SPIDEN/SPNIDEN | docs/unvr-access-research.md #48 | **unverified** (cited to #48, not re-derived here) | low risk: RW, resets to 0; confirm offset against a register map |

---

## MUST-FIX shortlist (ranked: boot-wedge / brick-reset / DMA-corruption first)

1. **[VERIFY — could wedge boot] Kernel-decompress vs DTB load overlap.**
   `BOOTCOMMAND` (defconfig:32) loads the gz uImage to `0x02000000` and the DTB to
   `0x04078000`, then `bootm`. bootm's LOADOS (gunzip, up to `SYS_BOOTM_LEN`=64 MB)
   runs **before** FDT relocation. The ~57 MB decompressed arm64 Image spans
   `0x02000000..~0x05700000`, which **contains** the DTB at `0x04078000`. If the
   uImage `ih_load` places the kernel anywhere in `[0x02000000, 0x04078000]`, the
   DTB is clobbered before bootm relocates it → boot failure with a garbage FDT.
   Action: read the uImage `ih_load`; if it overlaps, move the DTB above
   `~0x05800000` (or load the kernel higher). Cannot confirm statically — flag on box.

2. **[DMA-scope divergence — currently benign] Snoop fix looser than source.**
   `al_pcie_snoop_fix` (alpine.c:203-224) filters on **vendor 0x1c36 only**;
   `pcie-al-internal.c` also requires `PCI_FUNC(devfn)==0` (line 120) and
   `pci_is_root_bus` (line 132). On this HW all 6 internal AL devices are func-0 on
   bus 0, so the end state matches (board comment: "SMCC 0x110 -> 0x3 on all 6").
   Risk only if a multifunction AL device appears on bus 0 — then our code writes
   SMCC/APP_CONTROL to non-func-0 config space the vendor deliberately skips.
   Action: add `if (PCI_FUNC(bdf)) continue;` + a root-bus check for exact parity.

3. **[diag-only — cannot wedge boot] serdes "pcs" 2nd reg 0xfe120000 unverified.**
   dts:162 second reg (`0xfe120000`/0x10000, name "pcs") has **no source** and is
   self-flagged unverified in the node comment (dts:154-158). Only consumed by the
   `serdes` command. Action: confirm the PCS base on HW or drop the reg.

4. **[cosmetic] reset_cpu comment "RESEN fires in ~2 counts"** (alpine.c:305) is
   wrong — Load is `0x100`=256, and SP805 resets on the *second* expiry (2×256).
   Value is correct and fast (µs); only the comment misleads.

5. **[cosmetic/verify] A57 debug-enable offset 0xf0070008** (alpine.c:32) cited to
   #48 but not re-derived against a register map in this audit. RW, resets to 0 →
   low risk. Confirm the cpus_secure offset if a datasheet/regmap is available.

---

## Summary

- **Mismatches: 1** (snoop-fix filter looser than source — currently benign, MUST-FIX #2).
- **Fabricated-no-source: 1** (serdes "pcs" 0xfe120000 — honestly self-flagged, diag-only).
- **Deliberate deviations (not defects): 2** (i2c 400 kHz, sda-hold 300 ns — both CI-guard-enforced).
- **Unverified (cited, not re-derived): 1** (A57 debug-enable offset 0xf0070008).
- **Everything else: match**, including the entire #74 AXI-snoop path (byte-for-byte vs pcie-al-internal.c), SP805 regs, adt7475, DDR reg-names, UART/GPIO bases, PCA9575 LED map, HAL barriers/accessors.
- **No al_eth-class fabricated constant found** in this scope.
- **Top boot risk is MUST-FIX #1** (kernel-decompress/DTB overlap) — verify on box.
