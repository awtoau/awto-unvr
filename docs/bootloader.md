# Bootloader — reverse-engineered

Ghidra headless + manual carving of `mtd05`, UNVR sysid 0xea16, firmware 1.3.35.
Partition map and env in [nand-1.3.35.md](nand-1.3.35.md); peripheral summary in
[hardware.md](hardware.md). This file is the structure and the RE method.

**Canonical boot chain, flash TOC and U-Boot env: [nor-boot-chain.md](nor-boot-chain.md)**
(OLD on-device + NEW 5.1.25 side by side, both containers reconciled). The tables below
are the RE-evidence view for this unit; where they overlap, nor-boot-chain.md wins.

✅ = verified by instruction decoding or byte comparison. ❓ = inferred.

## `mtd05` is the whole boot chain

`mtd00` (`al_boot`) and `mtd01` (`device_tree`) are **100% 0xFF — erased and
unused** ✅, confirmed independently by two passes. Everything the SoC boots
lives in SPI-NOR. That also explains `load_fdt = cp.b $fdtaddr …`: `fdtaddr`
points into memory-mapped NOR, not NAND.

### Annapurna TOC at 0x80000 ✅

Magic `0x070c070c`, 8 entries. Per-image headers use magic `0x000b9ec7` with
size/load/entry at +0x28/+0x30/+0x38.

| Name | Type | Offset | Size | Note |
|---|---|---|---|---|
| `preboot` | 0x0d | 0x000000 | 0x080000 | ✅ **RESOLVED** — entry is `off 0x000000 size 0x080000` (earlier misread as off 0x80000 size 0); the region 0..0x80000 holds S2 + al_boot + stage3, incl. the payload at 0x21000. See [uboot-update-path.md](uboot-update-path.md#toc-correction) / [nor-boot-chain.md](nor-boot-chain.md) §1 |
| `dt` | 0x02 | 0x081000 | 0x07000 | FDT magic at 0x81048 ✅ |
| `dt_pro` | 0x02 | 0x088000 | 0x08000 | ✅ |
| `dt_ai` | 0x02 | 0x090000 | 0x08000 | ✅ |
| `dt_bt` | 0x02 | 0x098000 | 0x08000 | ✅ |
| `uboot` | 0x05 | 0x0a1000 | 0x11f000 | ends exactly at mtd5 end ✅ |
| `uboot-env` | 0x07 | 0x1c0000 | 0x10000 | = mtd6 exactly ✅ |
| `uboot-re` | 0x08 | 0x1d0000 | 0x230000 | overlaps mtd7/8/9 — flagged ❓ |

**Four device trees**, all `model = "Annapurna Labs Alpine V2 UBNT"`, separated
by board-cfg string: `one nas v5.0` (**this unit**), `one nas pro v2.0`,
`one nas ai v3.0`, `one nas bt v1.0`. A **multi-model build** selecting one by
sysid at runtime.

## Three code stages, two architectures ✅

| Stage | Offset | Size | ISA | Base | Established by |
|---|---|---|---|---|---|
| S2 SPI loader | 0x00000 | 25012 | ARM A32 → Thumb-2 | **0xF2200020** | string-pointer scoring: 147 hits vs 13 for runner-up |
| al_boot preboot | 0x21000 | 0x4A6B0 | ARM A32 | **0x01000000** | movw/movt reconstruction, 2723/2863 constants in `0x010xxxxx` |
| **U-Boot** | 0xA1048 | 0xA8640 | **AArch64** | **0x01100000** | its own `.quad TEXT_BASE` |

The 32-bit code is Annapurna's preboot, **not** a U-Boot SPL — the published
source has no `CONFIG_SPL*` for any Alpine target, and `ALPINE_V2_64` selects
`ARM64`.

Decoding evidence:

```
S2      ee115f10 mrc p15,0,r5,c1,c0,0 ; e3855a01 orr r5,r5,#0x1000 ; f57ff06f isb sy
        +0x160:  f44f 3200 mov.w r2,#0x20000            <- Thumb-2
preboot e59f3074 ldr r3,[pc,#116] -> 0x0100007C
        per-CPU stack table at 0x0100006c = image offset 0x6c   <- self-consistent
U-Boot  1400000a b 0x1100028 ; .quad 0x01100000 ; .quad 0x000a8640
        d5384241 mrs x1,currentel ; d51ec000 msr vbar_el3,x0
```

`msr cntfrq_el0` reads **50000000** — byte-for-byte `COUNTER_FREQUENCY` from
`include/configs/alpine_v2_64_ubnt_nas.h` ✅.

## Ghidra results ✅

| Image | Processor / base | Functions | Instructions | Strings |
|---|---|---|---|---|
| U-Boot | `AARCH64:LE:64:v8A` @0x1100000 | 1686 | 108,948 | 3573 |
| preboot | `ARM:LE:32:v8` @0x1000000 | 345 | 41,934 | 737 |
| S2 | `ARM:LE:32:v7` @0xF2200020 | 79 | 6,233 | 0 (pure loader) |

**Method worth reusing:** the Annapurna HAL leaves `__func__` strings in
`.rodata`, so a rename pass keyed on identifier-string references recovered
**383 U-Boot functions by real name** — `al_eth_register@0x01155450`,
`al_eth_init@0x01154fc4`, `al_nand_init@0x01120044`,
`al_spi_claim_bus@0x0111fba8`, `al_gpio_init@0x01121638`,
`board_eth_init@0x01104d10`, `al_eth_retimer_init@0x01104c18`.

## Peripheral address map ✅ — code and FDT agree

`AL_PBS_REGFILE_BASE` is a literal in the binary; the kernel half of the GPL drop
defines it as `AL_SB_PBS_BASE + 0x28000`. FDT `al-pbs` = 0xfd8a8000 ⇒
**`AL_SB_PBS_BASE = 0xfd880000`, `AL_SB_BASE = 0xfd800000`**.

**Full MMIO / address map: see [hardware.md](hardware.md#mmio-and-address-map)** — the
one authoritative table (all bases/sizes/compatibles cross-checked to live.dts +
iomem). The RE finding above (`AL_SB_PBS_BASE = 0xfd880000`, `AL_SB_BASE =
0xfd800000`, derived from the `AL_PBS_REGFILE_BASE` literal + `AL_SB_PBS_BASE +
0x28000`) is what pins those bases; SoC SRAM `0xf2200000` (S2 link base) is a measured
value, not in FDT.

**Clocks in the stored DTB are placeholders** — `sbclk`/`nbclk`/`cpuclk` =
0xf4240 (1 MHz), `refclk` = 0x16e3600. U-Boot overwrites them at boot from
strapping via `ft_board_setup_clock(…, al_globals.bootstraps.sb_clk_freq)`.

## Ethernet configuration ✅

`ethprime = al_eth1` — **port 1 is primary**.

| | Port 1 (RJ45) | Port 2 (SFP+) |
|---|---|---|
| mode | `rgmii` | `auto-detect-auto-speed` |
| PHY | ext, **MDIO addr 4**, MDC **1.0 MHz**, auto-neg out-of-band | **none** (`phy exist No` at runtime) |
| SerDes | — | grp **3**, lane **0**, ref-clock **156.25 MHz**, TX+RX inverted, SSC off |
| lane0 TX | — | amp 7, drivers 0x1f, post_emph 7, pre_emph 1 |

`10g-serial`: `dac = enabled`, `dac-length = 3`, and **`auto-neg`,
`link-training`, `fec` all DISABLED**, `force-1000base-x = enabled`.

`retimer { type = "br410"; i2c-bus 1; i2c-addr 0x56; channel B; exist =
"disabled" }` — supported in code (`al_eth_lm_retimer_br410_config`, `_ds25_`,
`_br210_`) but **not fitted**.

PHY drivers compiled in: Generic, AR8021, AR8031/AR8033, AR8035, RTL8211B/E/DN/F.
Published config sets `CONFIG_AR8033_SEL_1P8` ⇒ **AR8033 likely** ❓; the runtime
kernel reports `Atheros 8031`. Multi-board build, selected by PHY ID at runtime.

Ports 0 and 3 disabled. LED `sfp_1g` on gpio 0x13/2.

**UART baud is not compiled in** — `CONFIG_BAUDRATE = al_bootstrap_uart_baud_rate_get()`
and `CONFIG_SYS_NS16550_CLK = al_bootstrap_sb_clk_get()`, both read from PBS
strapping at runtime. No divisor exists in the binary to read out. Console is
`ttyS0,115200` from `loadbootargs`.

## Upstream match — right tree, wrong build ✅

Base is denx **U-Boot v2015.07**, proven by byte-identical git blob SHAs on four
untouched files (`README`, `common/main.c`, `lib/vsprintf.c`,
`drivers/mtd/nand/nand_base.c`) against tag `v2015.07`; v2015.04 and v2015.10
differ. Banner: `U-Boot 2015.07-alpine_db-2.21-HAL (Dec 16 2020 - 05:54:51 +0800)`.

Matches exactly against `include/configs/alpine_v2_64_ubnt_nas.h`:
`CONFIG_SYS_TEXT_BASE 0x01100000`, `COUNTER_FREQUENCY 50000000`,
`CONFIG_LOADADDR 0x08000000`, `loadaddr_payload 0x08000004`,
`loadaddr_dt 0x04078000`, `loadaddr_rootfs_chk 0x07000000`,
`nand_pt_addr_kernel 0x00300000` size `0x01000000`, `CONFIG_SYS_DTT_BUS_NUM 4`.

**In the binary, absent from all published source** (`grep -rlF` = 0 files):

```
ALPINE_UBNT_NAS_ALL>          (source has "ALPINE_UBNT_NAS> ")
Unabled to match any board, sysid: %0x%0x
No boardinfo matched, skip sysid setup
ubnthal.sysid   multiboot   slowfan   fanspeed
dt_bt   "one nas bt"   multi_dt
al_cmos_rgmii_dll_failure_clear
```

And the boot command differs:

```
source CONFIG_BOOTCOMMAND: … run bootargsnand; run bootnand
binary  bootcmd=           … run loadbootargs; run multiboot
```

Jenkins path `ubnt_unvr_all-161-2020-12-16-05-54-21` corroborates ("unvr_**all**")
and matches the kernel's build time — same CI run, same user `dio`.

## The GPL drop cannot build this binary ✅

Three independent reasons:

1. **All 271 `.S` files stripped** — including plain upstream
   `arch/arm/cpu/armv8/start.S`, which is nobody's IP. Reads as a broken export
   filter, not deliberate redaction.
2. **The entire Annapurna HAL is absent.** Only 2 of hundreds of `al_hal_*`
   files remain; the rest is `-include $(HAL_TOP)/file_list_base.mk` pointing
   outside the tree, and `HAL_TOP` is defined nowhere.
   `AL_PBS_REGFILE_BASE` is *used* in five files and *defined* in none.
3. **No `*_nas_all` defconfig**, no sysid board-matching code, no `multi_dt`.

**Recoverable:** `fabianishere/udm-kernel` ships `drivers/soc/alpine/HAL/` —
the real `HAL_TOP`, version 2.6, with `file_list_base.mk`,
`platform/alpine_v2/include/al_hal_iomap.h` and the exact
`al_init_l2cache_aarch{32,64}.S` the Makefile names. `delroth/alpine_hal` is the
same tree (imported from the UDM GPL drop), and adds `ddr/` — the DDR init
absent from every kernel-side copy. That plus vanilla `.S` files is near-complete.

Second independent vendor snapshot: `SVoxel/R9000` (Netgear, AL-514) carries the
same Annapurna U-Boot forked from **v2015.01** — keeps its `.S` files, HAL
equally stripped, no UBNT targets. Diffing it against the UNVR 2015.07 tree
separates Annapurna BSP from Ubiquiti changes.

## `al_boot` is two vendors' code, neither published ✅

**(a) Annapurna al-boot v2.10.0** — build path
`/project/users/barak/tasks/al-boot-release/…/alpine-v2-al-boot-v2.10.0.tmp/`,
runs on CVOS. Prints `agent_wakeup v2.10`.

**(b) A Ubiquiti-commissioned contractor stage 3** — build path
`/home/winder/projects/data/customers/ubiquiti/multi_dt/preboot_v2/stage3/`.
This is the `multi_dt` logic that picks one of the four DTBs. Functions:
`stg3_early_init`, `stg3_board_init`, `dt_based_init`, `dt_based_init_pcie`,
`thermal_sensor_trim_init`, `power_down_secondary_cpus`, `exec_via_agent`,
`dram_clear`.

Runtime banner: `Stage 3 version: 2.22.0 / Commit 6088bc3 / CVOS bac1d52 /
HAL 61afa9c / Build Sep 8 2020 11:40:22`.

The S2 stage at offset 0 (Thumb-2 @0xF2200000, zero strings) is the boot-ROM
SPI loader — touches only SPI, UART, PBS, I2C, GPIO. Also unpublished.

Source names these only as TOC object IDs (`AL_FLASH_OBJ_ID_PRE_BOOT`,
`_PRE_BOOT_V2`, `_STG2`) that U-Boot validates but never builds.

## Verification mechanisms that DO exist

The headline "boots unsigned" stands — image handling is legacy-only
(`Legacy image found`, `Verifying Checksum`, `Bad Data CRC`; **no** "Verifying
Hash Integrity", no FIT, no `bootsign`/`dobootm`/`fit_index`). But saying "no
verification at all" is wrong; `mtd05` contains:

```
%s: failed to authenticate EEPROM's content
RSA decryption failed!
signature decryption does not match the hashed capabilities
eFuse hashed modulues does not match the EEPROM hashed modulus
bootsecure / ## Error: Secure boot command not specified
unsigned/broken board %08x %02x%02x, skip
```

The RSA block is Annapurna preboot stage 3 verifying an I2C EEPROM capability
blob against an **eFuse-burned modulus hash** — real signature checking, over SoC
capabilities, not over the kernel. `bootsecure` is U-Boot's CLI-lockdown var,
inert at `bootdelay=2`. None gate the boot image.

## Open

- ~~TOC `preboot` row contradicts the payload at 0x21000~~ **RESOLVED** — the entry is
  `off 0x000000 size 0x080000` (see the TOC table above and
  [uboot-update-path.md](uboot-update-path.md#toc-correction)); the region contains the
  payload. A `2ND_TOC` object ID also exists in the name table; not chased.
- `0xF2200000` as SRAM is the measured link base, not a named region.
- Why `cvos_tags = 0x01000000` coincides with the preboot link base.
- ~~Exact fitted 1G PHY part~~ **RESOLVED** — chip is **AR8033** (marking `AR8033-AL1A`,
  U51, photo); the `at803x` driver mislabels it "Atheros 8031" (shared PHY ID). See
  [components.md](components.md) / [chips/ar8033.md](chips/ar8033.md).
