# DDR config reverse — where ea16's DDR "what" actually lives

Goal: recover THIS board's (`sysid 0xea16`, UNVR4) `al_ddr_init_cfg` values that the open
GPLv2 Annapurna DDR HAL needs. Result **overturns the prior hypothesis** that DDR is
trained by a CVOS "agent" fed a config struct from the main core.

Evidence base: carved blobs + Ghidra decompile in `docs/nor-reference/`, vendor GPL u-boot
`board/annapurna-labs/`, kernel HAL `urnvr-kernel-4.19.152/.../HAL/ddr/`.
Raw offsets/dumps: [tmp/logs/ddr-config-reverse.md](../tmp/logs/ddr-config-reverse.md).
Marks: ✅ confirmed (byte/disasm evidence) · ⚠ needs live HW read · ✎ correction to prior RE.

---

## TL;DR

- **DDR is trained by the S2 first-stage `stage2_loader v2.22.3`** (Thumb-2, link base
  `0xf2200000`, mask-BootROM-loaded) — carved as
  `docs/nor-reference/s2-loader-stage2_v2.22.3-25044B.bin`. `al_ddr_init` = `FUN_f2201a90`,
  0xf2201a90..0xf2203b58 = **8,392 B** of inlined Thumb-2. ✅✎
- The DDR "what" is **read at RUNTIME from an I²C EEPROM**, not stored as a per-board table:
  - `org`/`tmg`/`addrmap` from a **256-byte JEDEC SPD image**. ✅
  - `impedance_ctrl` (ODT/ODT_DYN/DIC/PHY ROUT/PHY ODT) from a **20-byte override block**
    (magic `0xCC`); absent → hardcoded stage2 defaults. ✅
- **The EEPROM records live at 16-bit offsets** (default base **0x400**) behind a magic-`0xAA`
  pointer record. ✅ This is why a plain `i2cdump 0x57` looked like garbage — see §2c.
- **No sysid switch selects DDR params.** The only per-sysid branch is DTB selection in
  al_boot. DDR board-specificity = the EEPROM contents + bootstrap straps on this unit. ✅
- Core→core handoff after training = `struct shared_parameters { u32 magic=0x31415926;
  u64 ddr_size; }` at SRAM **`0xfbff4150`**. al_boot Stage-3 only *reads* it. ✅
- **EEPROM RESOLVED (2026-08-17):** live 0x57 dumped + decoded through the vendor algorithm
  → full `al_ddr_init_cfg` for ea16 ([ddr-s2-parser-analysis.md](ddr-s2-parser-analysis.md)).
  DDR4-1866 CL13, 4 GiB, x16, 1 rank. Only `al_bootstrap.ddr_pll_freq` (running-point strap)
  remains live-open, SPD-bounded to ≤1866. ⚠

---

## 1. Handoff mechanism — who writes what, where

Two proprietary blobs inside NOR `preboot` (container `tmp/sections/01-uboot.bin`, TOC obj
`preboot` @0x0):

| blob | ISA / link | role |
|---|---|---|
| **S2 `stage2_loader v2.22.3`** | Thumb-2 @ `0xf2200000` (SRAM), NOR 0x0, 25,044 B | **the DDR trainer** — bootstrap parse, NB PLL, SPD read, `al_ddr_init`, impedance |
| al_boot Stage-3 payload | ARM A32 @ `0x01000000`, NOR 0x21004, 305,328 B | downstream: reads `ddr_size`, fabric/PCIe/eth, loads U-Boot |

### Boot order (corrected) ✎
1. BootROM → **S2 stage2_loader** trains DDR (§2), writes `shared_parameters` to SRAM.
2. S2 loads the al_boot payload to `0x01000000` and jumps.
3. al_boot `FUN_01002e90` runs with DRAM **already up**: pokes nb-service/CCU, multi_dt DTB
   select, fabric bring-up, loads U-Boot. Its `_DAT_fbff4150 == 0x31415926` return is a
   validity check of the handoff, not a DDR-ready poke it issued.

### The shared-params handoff (SRAM) ✅
GPL source is explicit — `board/annapurna-labs/common/shared_params.{h,c}`:
```c
#define SHARED_PARAMS_MN 0x31415926
struct shared_parameters { uint32_t magic_num; uint64_t ddr_size; };
ptr = (AL_PBS_INT_MEM_SRAM_BASE + PBS_INT_MEM_SHARED_PARAMS_OFFSET);
```
`AL_PBS_INT_MEM_SRAM_BASE = 0xfbff4000`, `SHARED_PARAMS_OFFSET = 0x150` → **`0xfbff4150`**
(kernel `platform/alpine_v2/include/al_hal_iomap.h`).

| off | field | writer | reader |
|---|---|---|---|
| +0x00 | `magic_num` = 0x31415926 | S2 `FUN_f22044b8` | al_boot `FUN_01002f08` (`preboot-alboot-decompiled.c:1604`), U-Boot `shared_params_valid` |
| +0x04 | `al_ddr_init` retry count (u8) — vendor extension over the GPL struct ✅ | S2 `FUN_f22003d8` | — |
| +0x08 | `ddr_size` (u64, bytes) | S2 `FUN_f22003d8` | al_boot `stg3_board_init` (`:1197-1207`), U-Boot |

al_boot's fallback when the magic is absent is **0x20000000 (512 MiB)** ✅. For ea16
`ddr_size = 4 GiB`, matching live U-Boot `DRAM: 4 GiB`.

Neighbouring SRAM regions used by the same chain: `SRAM_DEV_INFO_ADDRESS 0xfbff4100`,
`SRAM_CPU_RESUME_ADDRESS 0xfbff4120`, `SRAM_AGENT_ADDRESS 0xfbff4200`.

### The `0xf0070000` / `0xf0090000` writes are NOT a DDR mailbox ✎
- `FUN_01002e90`: `FUN_010274e8(0xf0070000,0)` = `AL_NB_SERVICE_BASE`;
  `FUN_01027508(0xf0090000,1)` = CCU/io_coherency. SoC fabric config, after DRAM is up.
- al_boot's `al_ddr_cfg_init` (args `0xf0070000,0xf0080000,0xf0088000`) only builds a HAL
  **handle over the already-trained controller** (rev switch `0x120120→V2 / 0x250231→V3`,
  else `Unknown DDR rev`) for the size sanity check — no training
  (`preboot-alboot-decompiled.c:14828`). Same call the GPL `cmd_ddr.c` uses for readback.
- The `aarch64_resume_agent` / `agent_wakeup v2.10` strings in al_boot are the **CPU resume**
  agent (`0xfbff4120`), unrelated to DDR. ✎

**Conclusion:** there is no "populate a struct, wake an agent, agent trains DDR" flow. The
main core is a consumer. The trainer is S2, and it sources params from hardware.

### al_boot payload carve is off by 4 bytes ✎ ✅
The S2 reads a u32 length at container **0x21000** (= `0x0004a6b0`) and copies from
**0x21004** to `0x01000000` (S2 disasm 0xf220021e..0xf2200232). Real payload =
`01-uboot.bin[0x21004 : 0x6b6b4]`, 305,328 B, + 4 B trailing checksum `a3 34 a4 03`.
Existing `preboot-alboot-*.{c,asm}` were loaded at 0x21000, so **every `FUN_`/`DAT_` VA in
them and in `preboot-decompile.md` is 4 too high** (e.g. `al_ddr_cfg_init` = 0x01021f10).
The instruction decode is still valid — A32 stays 4-byte aligned and PC-relative loads are
shift-invariant. The old "payload 0x6a6b4" came from the image-header field @+0x28, which is
length + 0x20000; the extra 128 KiB carved in was the TOC and the `dt` DTB.

---

## 2. The trainer: S2 `stage2_loader v2.22.3` — DDR path

Prior RE (`preboot-decompile.md`) labelled this "stringless S2 SPI loader, NOT stage2_loader
v2.22.3" — **incorrect** ✎. It carries the banner and the full DDR stack:

```
0x4892 stage2_loader v2.22.3          0x48de al_ddr_init(%d) succeeded after %d attempts!
0x4948 ddr_freq_change_according_to_spd  0x4969 ddr_init_dimm_params_get
0x4982 %s: ddr_init_spd_get failed!    0x49a0 al_ddr_spd_parse failed!
0x4aea SPD I2C Address: %02x           0x4b12 ddr_init_read_spd failed!
0x4abc ddr_init_spd_read_early_init_paramsa failed!   0x4aa2 invalid early init info!
0x4b3f set_dram_impedance_ctrl_from_eeprom
0x4d78/4d8a/4d9c al_ddr3_spd_parse / al_ddr4_spd_parse / al_ddr_spd_parse
0x4d59 al_ddr_spd_compute_cas_latency  0x5033 al_ddr_mode_register_set
0x5157 al_ddr_ctrl_dfi_init            0x513c al_ddr_phy_vt_calc_disable
0x53a3 Write Leveling Error … 0x54bd DRAM Vref Error … (full training error set)
```

Key functions (VA = 0xf2200000 + file offset):

| VA | role |
|---|---|
| 0xf2200124 | main — bootstrap/PLL/UART, DDR, TOC load of al_boot |
| 0xf22002c8 | `al_bootstrap_parse(0xfd8a8000, &bs)` + NB PLL init (0xfd860c00) |
| 0xf2200360 | `eeprom_read(off, len)` → buf `0xf220606c + off` |
| **0xf22003d8** | **DDR bring-up orchestrator** |
| 0xf2200628 | `ddr_freq_change_according_to_spd` — SPD get + PLL retry ladder |
| 0xf2200816 | addrmap builder (col/bank/bg/row/cs index tables) → cfg+0x44 |
| 0xf220093c | `ddr_init_spd_get` — early-init record + SPD scan 0x50→0x58 |
| **0xf2200a58** | **`set_dram_impedance_ctrl_from_eeprom`** |
| 0xf2200d10 | DRAM-voltage GPIO |
| 0xf2200dcc | JEDEC CRC-16 |
| 0xf2201140 / 0xf2200f30 | `al_ddr4_spd_parse` / `al_ddr3_spd_parse` |
| 0xf22013e8 | `al_ddr_spd_parse` — CRC check + DDR3/DDR4 dispatch |
| 0xf22018f4 | `al_ddr_cfg_init(nb, ctrl, phy, cfg)` |
| **0xf2201a90** | **`al_ddr_init(cfg)`**, 8,392 B |
| 0xf22044d0 | `i2c_eeprom_read(addr, off, len, dst)` — 2-byte offset when off > 0xff |

`al_ddr_init` is retried up to **1000** times; the successful attempt count is stored at
`0xfbff4154`. ✅

### 2a. `org` / `tmg` / `addrmap` ← JEDEC SPD over I²C ✅

`al_ddr_spd_parse` (0xf22013e8) validates JEDEC CRC-16 over bytes 0..0x7D or 0..0x74
(selected by bit 7 of byte 0), CRC at 0x7E/0x7F, second block CRC at 0xFE/0xFF over
0x80..0xFD — then dispatches on byte 2: `0x0B` → DDR3, `0x0C` → DDR4. ✅

Recovered SPD-byte → cfg-field map ✅:

| cfg field | SPD source | S2 evidence |
|---|---|---|
| `org.data_width` | byte 13 bits 2:0 | 0=32b, 1=64b, 2=16b (`enum al_ddr_data_width`) |
| `org.ranks` | `((byte12 >> 3) & 7) + 1`, × dimms | 0xf2201164 `ubfx r1,r1,#3,#3; adds r1,#1` |
| `org.dimms` | 1 or 2, from the aux-address probe | 0xf2200a00 |
| `org.rdimm` | byte 3 module_type ∈ mask 0x132 | 0xf22011a4 |
| `org.udimm_addr_mirroring` | bit0 of byte 0x88 | 0xf22011ac |
| `org.ecc_is_supported` | `byte13 & 0x18` (bus-width extension) | 0xf22011b6 |
| `org.ddr_type` | byte 2 (0x0B→0, 0x0C→1) | 0xf2201440 |
| `org.ddr_device` | byte 12 bits 2:0 (x4 rejected: "4 bit chips not supported") | 0xf220114e `tbb` |
| `tmg.cl` / `cwl` | CAS support map + JEDEC tables | 0xf2200ed8 / 0xf2200e74 |
| `tmg.t_*_ps` | MTB/FTB timing bytes | inside `al_ddr4_spd_parse` |
| `addrmap` | col/bank/bg/row/cid bit counts | 0xf2200816 → cfg+0x44 |

Guards seen in strings: `%s: invalid DDR device width field`, `DDR clock is faster than the
DIMM can support.`, `%s: couldn't find supported CAS latency!` — standard JEDEC parse, not
a board-table lookup.

`tmg.ref_clk_freq_mhz` and `tmg.ddr_freq` come from the **bootstrap straps**, not SPD ✅:
`ref_clk_freq_mhz = al_bootstrap.ddr_pll_freq / 1e6`;
`tCK_ps = 1e9 / (ddr_pll_freq / 1000)` → enum:

| tCK ps | enum | rate |
|---|---|---|
| 1875, or no match | 1 | 1066 |
| 1500 | 2 | 1333 |
| 1250 | 3 | 1600 |
| 1090 / 1071 | 4 | 1866 |
| 937…1069 | 5 | 2133 |
| 833…936 | 6 | **2400** |

Matches `enum al_ddr_freq` (`al_hal_ddr_init.h:70`) exactly. If the SPD cannot support the
requested tCK, `ddr_freq_change_according_to_spd` reprograms the NB PLL (0xfd860c00) down the
ladder — targets 933,333,333 / 800,000,000 / 666,666,666 / 533,333,333 Hz ✅ — and retries.

**RESOLVED: ea16 runs DDR4-1866, NOT 2400** ✎ (from the live SPD, [ddr-s2-parser-analysis.md]
(ddr-s2-parser-analysis.md)). The SPD `tCKAVGmin = 1071 ps` (byte 18/125) and the CAS map
tops out at CL14 — i.e. the DIMM declares **1866** as its fastest. The K4A8G165WB-**BCRC**
marking is a 2400 bin, but this SPD **caps the part at 1866**: a 2400 request (tCK 833 ps)
fails the `param_2+1 < tCKmin` gate (`FUN_f2201140`, `preboot-s2-decompiled.c:1480`) and the
loader downshifts the NB PLL. Decoded `tmg`: CL 13, CWL 10, tRAS 34 ns, tRC 47.92 ns,
tRFC1/2/4 350/260/160 ns (JEDEC 8 Gbit). The *running* freq = `al_bootstrap.ddr_pll_freq`
(live read still open) but is SPD-bounded to 1866.

### 2b. `impedance_ctrl` ← I²C EEPROM override block ✅ (byte-exact, header-matched)

`set_dram_impedance_ctrl_from_eeprom` = `FUN_f2200a58` (`preboot-s2-decompiled.c:864`),
called as `FUN_f2200a58(org.dimms, org.ranks, &cfg->impedance_ctrl)`. It writes defaults,
then overrides from the EEPROM block if `buf[base+0x0e] == 0xCC`.

**Struct layout (cfg+0xb8), header-matched** ✅ — the shipping stage2 was built with
**1-byte enums** (`-fshort-enums`) and an older HAL (**no `hv_min`/`hv_max`**), giving a
0x22-byte block that accounts for every observed write:

```
+0x00 dic            +0x01 odt          +0x02 odt_dyn      +0x03 rtt_park (0 = disabled)
+0x04 host_initial_vref = 0x28 (40)     +0x05 vrefdq = 0x19 (25)
+0x06..07 phy_rout_pu[2] = 13, 11       +0x08..09 phy_rout_pd[2] = 13, 13
+0x0a..0b phy_pu_odt[2] = 7, 7
+0x0c..13 wr_odt_map[4] (u16)           +0x14..1b rd_odt_map[4] (u16)
+0x1c..1d phy_rout[2] = 10 (30 Ω)       +0x1e..1f phy_odt[2] = 8 (40 Ω)
+0x20 dqs_res        +0x21 dqsn_res
```

Defaults are selected by **`dimms`**, not ranks ✎ (an earlier draft assumed the rank≥2 path):

| | dimms = 1 (**our board** ⚠) | dimms ≥ 2 |
|---|---|---|
| `dic` | 1 (`AL_DDR_DIC_RZQ7`) | 1 |
| `odt` | 3 (`AL_DDR_ODT_RZQ6`) | 5 (`RZQ12`) |
| `odt_dyn` | 0 (`DIS`) | 2 (`RZQ4`) |
| `wr_odt_map[4]` | {1, 2, 0, 0} | {5, 10, 5, 10} |
| `rd_odt_map[4]` | {0, 0, 0, 0} | {4, 8, 1, 2} |

`base+0x0f` selects the DQS termination pair ✅: `==1` → `dqs_res = PULL_DOWN_500OHM`,
`dqsn_res = PULL_UP_500OHM`; else `dqs_res = PULL_UP_611OHM`, `dqsn_res = PULL_UP_458OHM`.
(Confirms +0x20/+0x21 against `enum al_ddr_dqs_res` / `al_ddr_dqsn_res`.)

**The 20-byte EEPROM table** at `base+0x10` is indexed
`idx = (dimms-1)*10 + (ranks/dimms - 1)*5`, 5 bytes per (dimms, ranks-per-dimm) case ✅:

| table byte | cfg field | decode table (S2 addr) | encoding |
|---|---|---|---|
| +0 | `odt` | 0xf2204c34, 10 entries | RZQ divider 0,2,4,6,8,12,1,5,3,7 → `AL_DDR_ODT_*` |
| +1 | `odt_dyn` | 0xf2204c2a, 5 | 0,2,4,1,**0x99 = Hi-Z** → `AL_DDR_ODT_DYN_*` |
| +2 | `dic` | 0xf2204c48, 3 | RZQ divider 6,7,5 → `AL_DDR_DIC_RZQ6/7/5` |
| +3 | `phy_odt[0..1]` | 0xf2204c4e, 15 | **ohms** 200,133,100,77,66,56,50,44,40,36,33,30,28,26,25 |
| +4 | `phy_rout[0..1]` | 0xf2204c6c, 11 | **ohms** 80,68,60,53,48,44,40,37,34,32,30 |

All five tables match `enum al_ddr_odt` / `al_ddr_odt_dyn` / `al_ddr_dic` /
`al_ddr_phy_odt` / `al_ddr_phy_rout` in the GPL kernel HAL **element-for-element** ✅. The
EEPROM stores human units — RZQ dividers DRAM-side, ohms PHY-side. Error strings gate each
(`invalid DRAM ODT/ODT_DYN/ROUT from EEPROM`, `invalid PHY ROUT/ODT from EEPROM`).

**ea16 actual impedance = defaults UNLESS this unit's EEPROM carries the `0xCC` block.** ⚠
Read it live (§2c) or read back the trained PHY registers.

### 2c. The EEPROM record layout — the actionable finding ✅

Bus: **`i2c-pld`, DesignWare @`0xfd880000` = `AL_I2C_PLD_BASE`, 100 kHz standard mode** ✅
(S2 i²c cfg struct @0xf2205df8 → IC_CON 0x63; DT `i2c-pld ... clock-frequency = <0x186a0>`).
This is Linux **`i2c-0`**; the PCA9546 mux at 0x71 hangs off the same bus but the loader
never touches it. ✎ (an earlier draft said `AL_I2C_GEN_BASE` — wrong; GEN is 0xfd894000).

Device address = `al_bootstrap.i2c_preload_addr` (straps, read from `AL_PBS_REGFILE_BASE`
0xfd8a8000) ❓ not recoverable offline.

Record base = **0x400** by default, or `DEV_INFO[0x0a] | DEV_INFO[0x0b]<<8` when
`DEV_INFO[0x00] != 0` ✅ (`SRAM_DEV_INFO_ADDRESS 0xfbff4100`; `DEV_INFO_EARLY_INIT_ADDR_
LSB/MSB_OFFSET 10/11` in `u-boot/board/annapurna-labs/alpine_ubnt/dev_info_layout.h`).
Offsets > 0xFF use a **2-byte big-endian** EEPROM pointer (`FUN_f22044d0`).

Four magic-guarded records, relative to that base ✅ (byte-exact live values → RESOLVED,
reader fn per record; full layout [ddr-s2-parser-analysis.md](ddr-s2-parser-analysis.md)):

| off | len | magic | reader | content | ea16 live |
|---|---|---|---|---|---|
| +0x00 | 7 | `0xAA` | `FUN_f220093c` | `spd_i2c_addr`, `spd_off` (u16 LE), `aux_i2c_addr`, `aux_off` (u16 LE) | `aa ff 40 04 00 00 00` → addr=strap(0x57), off=0x0440, aux=0⇒dimms 1 |
| +0x0b | 3 | `0xBB` | `FUN_f2200d10` | GPIO pin (0xff=off, else <0x38), polarity | `bb ff 00` → **disabled** |
| +0x0e | 0x16 | `0xCC` | `FUN_f2200a58` | `dqs_sel` byte, then 20-byte impedance table (4 cases×5 B) | `cc 01 04 00 07 38 22 …` |
| +0x24 | 2 | `0xDD` | `FUN_f22003c8` | UART divisor override | absent (0xff) |

Then **256 bytes of SPD** from `(spd_i2c_addr, spd_off)` — for ea16 the SPD lives on the SAME
device (0x57) at `spd_off = 0x0440` (a genuine CRC-valid JEDEC DDR4 image, both CRCs pass). A
1-byte probe at `(aux_i2c_addr, aux_off)` sets `dimms = 1` or `2`; ea16 `aux_i2c_addr=0` ⇒
dimms=1. On i²c failure the loader scans **0x50 → 0x58**, printing `SPD I2C Address: %02x` per
attempt. Our unit prints exactly `SPD I2C Address: 57`, from the `0xAA` record (strap fallback,
`spd_i2c_addr=0xff`).

The 20-byte impedance table is indexed `idx = (dimms-1)*10 + (ranks/dimms-1)*5`; ea16
(dimms=1, ranks=1) uses bytes `04 00 07 38 22` = `{odt=RZQ4/60 Ω, odt_dyn=DIS, dic=RZQ7/34 Ω,
phy_odt=56 Ω, phy_rout=34 Ω}` (decode tables recovered from the .bin, §2b).

**This explains issue #62:** the SPD sits at a **16-bit offset**; `i2cdump` uses 1-byte
addressing and read window 0x0000-0x00FF instead. Readout task in §7.

---

## 3. Per-sysid conditioning — NONE for DDR ✅

- The only sysid switch is DTB selection: al_boot `stg3_early_init` reads sysid 2 B BE from
  SPI-NOR `0x1F000C`, `switch` → DTB instance (ea16→0/`dt`, ea20→`dt_pro`, ea21→`dt_ai`,
  ea1a→`dt_bt`, ea30→`dt_hd`). See `preboot-decompile.md` §multi_dt.
- S2's DDR path has **no sysid read and no per-board table** — it keys entirely off the
  EEPROM + straps. So "would another UBNT board get different DDR values?" — yes, but
  because it has different EEPROM bytes, not because the blob picks a row. **There is no
  row to dump**, for ea16 or any other model.

---

## 4. Control DTBs — no DDR params ✅

`dtc` on the DTBs carved by `scripts/carve-dtb.py`:

| TOC obj | sysid | `board-cfg/id` |
|---|---|---|
| `dt` | **0xea16 (ours)** | `alpine_v2_ubnt one nas v5.0` |
| `dt_pro` | 0xea20 | `alpine_v2_ubnt one nas pro v2.0` |
| `dt_ai` | 0xea21 | `alpine_v2_ubnt one nas ai v3.0` |
| `dt_bt` | 0xea1a | `alpine_v2_ubnt one nas bt v1.0` |
| `dt_hd` (5.1.25 fw only, absent from current NOR) | 0xea30 | `alpine_v2_ubnt one nas hd` |

- **`memory { reg = <0 0 0 0>; }` in every DTB** — DRAM base/size is patched at runtime from
  `shared_parameters.ddr_size`, never a DTB constant.
- The only DDR-adjacent node is
  `memory-controller { compatible = "annapurna-labs,alpine-mc"; reg = <0 0xf0080000 0 0x10000>; }`
  — no timing/ODT/addrmap properties in any DTB.
- `/soc/board-cfg` holds pinctrl/gpio/serdes/ethernet/pcie/sgpo — board-specific, but no
  DDR fields. It is consumed by al_boot/U-Boot **after** DRAM is up, so it structurally
  cannot carry DDR parameters.

---

## 5. Other board-specific config found in the handoff

ea16 `board-cfg`, byte-exact from `tmp/dtb-current/dtb00-0x081048-26208B.dts:1192-1629`:

- **SerDes** — grp0 `pcie_g2x2_pcie_g2x2` 100 MHz lanes 0,2; grp1/grp2 `sata` 100 MHz
  lanes 0-3; grp3 `10gbe` 156.25 MHz lane 0; grp4 `skip`. All `inv-tx-lanes`,
  `inv-rx-lanes`, `ssc = disabled`. TX overrides: SATA grp1/grp2 lanes 1,3 →
  `amp=7 total_driver_units=0x1f post_emph=6 pre_emph=0 slew_rate=0`; 10 GbE grp3 lane0 →
  `amp=7 total_driver_units=0x1f post_emph=7 pre_emph=1`. Consumed by al_boot
  `al_serdes_init` / `dt_based_init_serdes_group`.
- **Ethernet** — port0 disabled; port1 `rgmii`, ext PHY MDIO addr 4, MDC 1.0 MHz,
  out-of-band autoneg; port2 `auto-detect-auto-speed`, `i2c-id=2`, serdes grp3 lane0,
  10G-serial DAC enabled len 3, autoneg/link-training/FEC off, `force-1000base-x`, retimer
  br410 present=disabled at i2c bus 1 addr 0x56 chan B, LED `sfp_1g` on `gpio@20` pin 2;
  port3 disabled.
- **PCIe** — `ep-ports`; port0 gen2 x1 enabled, ports 1-3 disabled.
- **SGPO** — group_mode `two`, sata_mode `active-presence`, clk_setup 0x40 ns, update 1 kHz,
  clk 1 MHz, blink normal; group0 `mode_mask=0xf0 init_val=0xc0`, group1
  `stretch_mask=blink_mask=0xff`, groups 2/3 `mode_mask=0xff`, stretch 512 ms.
- **pinmux** (`pinctrl_init`, phandles resolved) — `if_nand_8`, `if_nand_cs_0`, `if_uart_2`,
  `if_eth_2_led`, `if_gpio31`, `if_sgpo_clk`, `if_sgpo_ds_2`. `gpio_init` = `<3 1 0>`.
- **I²C** — `i2c-pld` @0xfd880000 (bootstrap/SPD/mux bus, = Linux `i2c-0`), `i2c-gen`
  @0xfd894000; both 100 kHz, `i2c-ss-scl-hcnt-raw = 0x855`, `i2c-ss-scl-lcnt-raw = 0xb0b`.
- **PLL** — GPL `alpine_ubnt/board_cfg.h`: SB PLL chans ETH0/1 ref-clk 25 MHz; SerDes
  R2L/L2R 100 MHz commented out (PLL bypass, 100 MHz ref assumed). The **NB PLL is the DDR
  clock** and comes from `al_bootstrap` (PBS regfile 0xfd8a8000), not board_cfg ✅.
- **DRAM voltage GPIO** ✅ — EEPROM `base+0x0b` record (`0xBB`): GPIO pin + polarity, driven
  from the SPD module-voltage field (`FUN_f2200d10`; GPIO regs `0xfd897000 + (pin>>3)*0x1000`,
  with `0xfd897400`/`0xfd897000` for pins 0x28-0x2f). DDR4 here is 1.2 V nominal, so the pin
  may be unpopulated — the record's presence is the test.
- **UART divisor override** ✅ — EEPROM `base+0x24` (`0xDD`): byte `base+0x25` used directly
  instead of deriving from `al_bootstrap.sb_clk_freq`.
- **DDR frequency is capability-gated** ✅ — al_boot `stg3_early_init` compares the running
  DDR frequency against a limit from the **RSA-2048-signed EEPROM capabilities blob** and
  logs `DRAM frequency violation!` (string @al_boot 0x307c0), setting the caps-invalid flag;
  likewise `CPU frequency violation!`. Directly relevant to the overclock issue (#29):
  raising the DDR PLL past the signed limit trips this path.
- **DRAM remap** ✅ — the 3 GiB @0 + 1 GiB @0x200000000 split is
  `al_addr_map_dram_remap_set` in al_boot (the MMIO hole at 3-4 GiB), not two controllers.
- **Identity EEPROM (NOR mtd04, our unit)** — `UBNT` magic block @0x8000 (sysid `ea16`,
  Device ID `0b101d`, MAC `74:ac:b9:41:a8:11`), TlvInfo @0xd000 (P/N `113-02832-29`, date
  20200524), ssh-rsa host key @0xe000. **No DDR fields** — this is SPI-NOR, a different
  device from the I²C EEPROM the stage2 reads.

Diffs vs the other three current DTBs are ethernet/PHY only (pro and ai add a second
`i2c-id=2` port with a 0x18 retimer; ai adds a second PHY at MDIO addr 5). SerDes group
assignment, PCIe and SGPO are identical across all four.

---

## 6. Mapping to `al_ddr_init_cfg` — status per field

Observed top-level offsets in the shipping stage2 ✅: `ddr_cfg` 0x00, `org` 0x10,
`addrmap` 0x44, `tmg` 0x6c, `impedance_ctrl` 0xb8. `org` has **no `cids`** field (the
published header puts it between `ranks` and `dimms`), and cfg+0x104 is a pointer to a
static 0x3c-byte table at 0xf220602c `{1,8,43,1,100,16,15,16,128,15,128,128,15,128,
0xfd883000, 0xfd884000}` ❓ unidentified. A port using the current header must not assume
these offsets.

All EEPROM-sourced fields now **RESOLVED** from the live 0x57 dump (decoded through the vendor
algorithm — [ddr-s2-parser-analysis.md](ddr-s2-parser-analysis.md), `scripts/decode-ddr-records.py`).
Only `ddr_pll_freq` (strap) remains live-open, and it only picks the *running* point on an
SPD-bounded (≤1866) ladder.

| member | source | ea16 status |
|---|---|---|
| `ddr_cfg` (bases/rev) | fixed | ✅ nb `0xf0070000`, ctrl `0xf0080000`, phy `0xf0088000`, rev V2 |
| `org.data_width` | SPD byte 13 | ✅ **64-bit** (byte13&7=3⇒enum1) |
| `org.ranks` | SPD byte 12 bits 5:3, × dimms | ✅ **1** (byte12=0x02) |
| `org.dimms` | aux-address probe | ✅ **1** (aux_i2c_addr=0) |
| `org.ddr_type` / `ddr_device` | SPD bytes 2 / 12 | ✅ **DDR4 / x16** (byte2=0x0c, byte12&7=2) |
| `org.ecc_is_supported` | SPD byte 13 bits 4:3 | ✅ **0** (byte13&0x18=0) |
| `org.rdimm` / `udimm_addr_mirroring` | SPD bytes 3 / 0x88 | ✅ **0 / 0** (UDIMM, byte3=0x02) |
| `tmg.ref_clk_freq_mhz` / `ddr_freq` | `al_bootstrap.ddr_pll_freq` (running), SPD (cap) | ✅ **≤1866** (SPD tCKmin=1071 ps); ⚠ exact running freq needs live `ddr_pll_freq` |
| `tmg.cl / cwl / t_*_ps` | **SPD** | ✅ **CL13, CWL10**; tRAS 34 / tRC 47.92 / tRFC1 350 / tRFC2 260 / tRFC4 160 ns, tFAW 30 / tRRD_S 5.3 / tRRD_L 6.4 / tCCD_L 5.355 ns (@1866) |
| `addrmap.*` | derived from density/width/ranks | ✅ **16 row / 10 col / 2 bank / 1 bg** (`FUN_f2200816` output: col→sys7-13, bank→17-18, row→14-16,19-31) |
| `impedance_ctrl.dic/odt/odt_dyn/phy_rout/phy_odt` | EEPROM `0xCC` block | ✅ **dic=RZQ7(34 Ω), odt=RZQ4(60 Ω), odt_dyn=DIS, phy_rout=34 Ω, phy_odt=56 Ω** (0xCC present) |
| `impedance_ctrl.rtt_park / host_initial_vref / vrefdq / phy_rout_pu/pd / phy_pu_odt / wr_odt_map / rd_odt_map` | stage2 hardcoded | ✅ rtt_park=DIS, vref=40, vrefdq=25, pu={13,11}, pd={13,13}, pu_odt={7,7}, wr={1,2,0,0}, rd={0,0,0,0} |
| `impedance_ctrl.dqs_res / dqsn_res` | EEPROM `0xCC` dqs_sel | ✅ **PULL_DOWN_500 / PULL_UP_500** (dqs_sel=1) |
| `misc.training_en` | stage2 fixed | ✅ 1 (blob does full training) |

Total from the size math (`FUN_f22003d8:439-452`): `ranks(1) << (row+col+bank+bg = 29) × 8 B`
= **4 GiB** ✅ — matches live `DRAM: 4 GiB`.

---

## 7. Open / unrecoverable from the blobs

Structural + per-unit config now known. The EEPROM is dumped
(`docs/nor-reference/ddr-config-eeprom-0x57-8k.bin`) and decoded through the vendor algorithm
(`scripts/decode-ddr-records.py` → `tmp/logs/decode-ddr-records.log`; analysis
[ddr-s2-parser-analysis.md](ddr-s2-parser-analysis.md)). What remains is **one strap**:

| unknown | why it matters | how to get it |
|---|---|---|
| `al_bootstrap.ddr_pll_freq` | picks the *running* point on the freq ladder (SPD bounds it to ≤1866) → exact `tmg.ddr_freq`, `ref_clk_freq_mhz`, and CL/CWL | live PBS read (0xfd8a8000) via `/dev/mem`, or infer from the trained controller (§8) |

`al_bootstrap.i2c_preload_addr` is resolved by the dump itself (records present at 0x57 offset
0x400; `spd_i2c_addr=0xff` ⇒ strap fallback, live console prints `SPD I2C Address: 57`).
Remaining SPL task: feed the decoded `al_ddr_init_cfg` (this doc / analysis doc) into
`alpine_ddr_cfg.c`, then cross-check against the live `al_ddr_cfg_init` readback (§8) before
committing ([uboot-ddr-port.md](uboot-ddr-port.md) §6).

No further gain from reversing the DDR *algorithm*: it is the open GPLv2
`al_hal_ddr_init_alpine_v2.c` (5608 lines) already in the kernel tree — the same "how".

---

## 8. Cross-check vs live readback

`ddr_size` handoff (4 GiB) matches live `DRAM: 4 GiB` and the 2-bank DT split (3 GiB@0 +
1 GiB@0x200000000; single `alpine-mc` @0xf0080000).

Every EEPROM-derived field has an independent live check via
`al_ddr_cfg_init(0xf0070000, 0xf0080000, 0xf0088000, &cfg)` on the trained controller
([uboot-ddr-port.md](uboot-ddr-port.md) §5):

| field | live source |
|---|---|
| `odt` (RTT_NOM), `dic` | MR1 |
| `odt_dyn` (RTT_WR), `cwl` | MR2 |
| `rtt_park` | MR5 |
| `vrefdq` | MR6 |
| `cl`, BL | MR0 |
| `phy_rout` / `phy_odt` / `host_initial_vref` | PUB PHY ZQ + VREF registers |
| `ranks`, `data_width`, `addrmap` | uMCTL2 MSTR + ADDRMAP registers |
| `ddr_freq` | DFI/PLL divider |
| `ddr_size` | `shared_parameters` @0xfbff4150 +0x08 |

Agreement between the EEPROM path and the readback path is the acceptance test before
committing an `alpine_ddr_cfg.c` for the SPL.

---

## 9. Licence note

Nothing here is copied vendor code. The findings are register/EEPROM layout facts and
value→enum tables **already present verbatim in the GPLv2 kernel HAL headers**
(`al_hal_ddr_init.h`, `al_hal_ddr.h`, `al_hal_iomap.h`) and the GPLv2 vendor U-Boot
(`shared_params.h`, `dev_info_layout.h`). The SPD format is JEDEC. The only novel output is
the per-unit EEPROM byte map — data about this board, not vendor code.

## ✅ LIVE-CONFIRMED on ea16 (2026-08-17)

Read `0x57` @ 16-bit offset `0x400` on `i2c-0`:
`i2ctransfer -y 0 w2@0x57 0x04 0x00 r128` →
```
0xaa 0xff 0x40 0x04 00 00 00 00 00 0x08 00   <- 0xAA pointer record (SPD @ ...)
0xbb 0xff 00                                  <- 0xBB DRAM-voltage GPIO
0xcc 0x01 0x04 00 0x07 0x38 0x22 0x04 00 0x07 0x38 0x22 ...  <- 0xCC impedance override
...
0x0c 0x02 0x40 0x21 ...                        <- JEDEC SPD: byte2=0x0C = DDR4  ✅
```
Confirms the reverse end-to-end: records at 16-bit offset base 0x400, magic-guarded,
DDR4 marker present.

### Full 8 KiB dump — 2026-08-17 (#67)

`scripts/read-ddr-spd.py` (16-bit `i2ctransfer` path) dumped the whole 8 KiB of `0x57`
over the serial console → `docs/nor-reference/ddr-config-eeprom-0x57-8k.bin` (tracked).

- **Records at `0x400`** (byte-exact, decoded — full analysis
  [ddr-s2-parser-analysis.md](ddr-s2-parser-analysis.md), decoder
  `scripts/decode-ddr-records.py`):
  - `0400: aa ff 40 04 00 00 00` — `0xAA` pointer: `spd_i2c_addr=0xff`(⇒strap 0x57),
    `spd_off=0x0440`, `aux_i2c_addr=0x00`(⇒dimms=1)
  - `0x40b: bb ff 00` — `0xBB` DRAM-voltage GPIO: `pin=0xff` ⇒ **disabled** (DDR4 fixed 1.2 V)
  - `0x40e: cc 01 04 00 07 38 22 …` — `0xCC` impedance: `dqs_sel=1`, case[0]=`04 00 07 38 22`
  - `0x440: 00 00 0c 02 40 21 …` — SPD image (`byte2=0x0C` = DDR4)
- **The `0x440` record IS a genuine CRC-valid JEDEC DDR4 SPD** ✎ (corrects the earlier
  "Ubiquiti-custom, not JEDEC" note). Both JEDEC CRC-16s pass (block1 `0xfbbc`, block2
  `0x0000`); the S2 reads standard JEDEC offsets (`al_ddr4_spd_parse` = `FUN_f2201140`).
  It only *looks* wrong to a stock decoder because **byte4[3:0] density code = 0** (left
  unset) — the S2 derives density from row/col/bank/bg geometry, not the density code, so a
  `decode_ddr4_spd()` that trusts byte4[3:0] mis-reports 256 Mb. Decode via the S2 parser.
- **Decoded ea16 config** (through the vendor algorithm): DDR4, x16, 1 rank, 64-bit bus,
  16 row / 10 col / 2 bank / 1 bank-group ⇒ **4 GiB** ✅; **DDR4-1866 CL13 CWL10** ⚠
  (SPD-capped, see §2a); impedance from `0xCC` (odt=RZQ4/60 Ω, dic=RZQ7/34 Ω, odt_dyn=DIS,
  phy_rout=34 Ω, phy_odt=56 Ω). Full table in the analysis doc.
- **`0x57` holds DDR config only** — the `0x000` region is `36 1c 36 1c …`, not the
  ubnthal plaintext schema. Board **identity lives in NOR**, not here → settles
  dt-gaps M1 (0x57 is *not* the identity EEPROM).

Remaining for #67: read `al_bootstrap.ddr_pll_freq` (SRAM `0xfbff4150`) via `/dev/mem` to
pin the *running* freq (SPD bounds it to ≤1866). Static config is otherwise resolved →
unblocks the U-Boot DDR SPL (#65).

