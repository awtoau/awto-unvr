# DDR config reverse — where ea16's DDR "what" actually lives

Goal: recover THIS board's (`sysid 0xea16`, UNVR4) DDR `al_ddr_init_cfg` values that the
open GPLv2 Annapurna DDR HAL needs. Result below **overturns the prior hypothesis** that
DDR is trained by a CVOS "agent" fed a config struct from the main core.

Evidence base: carved blobs + Ghidra decompile in `docs/nor-reference/`, vendor GPL
u-boot `board/annapurna-labs/`, kernel HAL `urnvr-kernel-4.19.152/.../HAL/ddr/`.
Marks: ✅ confirmed (byte/disasm evidence) · ⚠ needs live HW read · ✎ correction to prior RE.

---

## TL;DR

- **DDR is trained by the S2 first-stage `stage2_loader v2.22.3`** (Thumb-2, link base
  `0xf2200000`, mask-BootROM-loaded) — the carved
  `docs/nor-reference/s2-loader-stage2_v2.22.3-25044B.bin`. ✅✎
- The DDR "what" is **read at RUNTIME**, not stored as a per-board table:
  - `org`/`tmg`/`addrmap` from the **JEDEC SPD EEPROM over I2C**. ✅
  - `impedance_ctrl` (ODT/DIC/ROUT) from an **I2C EEPROM override block** (magic `0xCC`);
    absent → hardcoded stage2 defaults. ✅
- **No sysid switch selects DDR params.** The only per-sysid branch in the boot chain is
  DTB selection (al_boot `stg3_early_init`). DDR board-specificity = the physical SPD chip
  + EEPROM impedance bytes on this board. ✅
- **No config struct is handed core→agent.** The `0xf0070000`/`0xf0090000` writes are
  nb-service + CCU fabric config done AFTER DDR is up, not a DDR mailbox. ✎
- Core→core handoff after training = `struct shared_parameters { u32 magic=0x31415926;
  u64 ddr_size; }` at SRAM **`0xfbff4150`**. al_boot Stage-3 only *polls* it and reads
  `ddr_size`; it does not train. ✅
- **Not recoverable from any NOR blob:** exact SPD contents (CL/tRCD/tRP/tRAS/tRC/tRFC/tFAW/
  speed-bin) — they live on the separate on-board SPD I2C chip → must be read from HW. ⚠

---

## 1. Handoff mechanism — who writes what, where

Two proprietary code blobs inside NOR `preboot` (container `01-uboot.bin`, TOC obj
`preboot` @0x0, split at img-hdr `0x20000`):

| blob | ISA / link | role |
|---|---|---|
| **S2 stage2_loader v2.22.3** | Thumb-2 @ `0xf2200000` (SRAM) | **the DDR trainer** — SPD read, `al_ddr_init`, impedance |
| al_boot Stage-3 payload | ARM A32 @ `0x01000000` | downstream: reads `ddr_size`, fabric/PCIe/eth, loads U-Boot |

### Boot order (corrected)
1. BootROM → **S2 stage2_loader** trains DDR (§2), writes `shared_parameters` to SRAM.
2. S2 loads al_boot payload (`0x01000000`) and jumps.
3. al_boot `FUN_01002e90` runs with DRAM **already up**: pokes nb-service/CCU, multi_dt
   DTB select, fabric bring-up, loads U-Boot. Returns `_DAT_fbff4150 == 0x31415926`
   (just a validity check of the handoff, not a DDR-ready poke it issued).

### The shared-params handoff (SRAM), ✅
- GPL source is explicit: `board/annapurna-labs/common/shared_params.{h,c}`
  ```c
  #define SHARED_PARAMS_MN 0x31415926
  struct shared_parameters { uint32_t magic_num; uint64_t ddr_size; };
  ptr = (AL_PBS_INT_MEM_SRAM_BASE + PBS_INT_MEM_SHARED_PARAMS_OFFSET);
  ```
- Addresses: `AL_PBS_INT_MEM_SRAM_BASE = 0xfbff4000`, `SHARED_PARAMS_OFFSET = 0x150`
  → struct base **`0xfbff4150`** (kernel `platform/alpine_v2/include/al_hal_iomap.h`).
- Confirmed in S2 literal pool: `0xfbff4000` (`DAT_f220029c`), `0xfbff4100`
  (`SRAM_DEV_INFO_ADDRESS`, `DAT_f220039c`), `0xfbff410c` (`DAT_f2200290`).
- al_boot consumers, ✅:
  - `FUN_01002f08` → `return _DAT_fbff4150 == 0x31415926;`
    (`preboot-alboot-decompiled.c:1604`).
  - `FUN_01002460` (`stg3_board_init`) reads `ddr_size` lo/hi at struct `+8`/`+0xc`
    (`local_50/uStack_4c`), then loops "clearing physical memory" and the
    `%s: DDR size not supported!` guard (`preboot-alboot-decompiled.c:1197-1207`).
  - For ea16, `ddr_size = 4 GiB` (matches live U-Boot `DRAM: 4 GiB`).

### The `0xf0070000` / `0xf0090000` writes are NOT a DDR mailbox, ✎
- `FUN_01002e90`: `FUN_010274e8(0xf0070000,0)` = nb-service; `FUN_01027508(0xf0090000,1)`
  = CCU/io_coherency. Both run after the DRAM handoff check path; they configure the SoC
  fabric, not DDR.
- al_boot's `al_ddr_cfg_init` (`FUN_01021f14`, args `0xf0070000,0xf0080000,0xf0088000`)
  only builds a HAL **handle over the already-trained controller** (rev-id switch
  `0x120120→V2 / 0x250231→V3`, else `Unknown DDR rev`) for the size sanity check. It does
  **no training** (`preboot-alboot-decompiled.c:14828`). Same call the GPL `cmd_ddr.c`
  uses for live readback.

**Conclusion:** there is no "populate a struct, wake an agent, agent trains DDR" flow from
the main core. The main core is a consumer. The trainer is S2, and it sources params from
hardware (SPD + EEPROM), not from a blob table.

---

## 2. The trainer: S2 `stage2_loader v2.22.3` — DDR path

`docs/nor-reference/s2-loader-stage2_v2.22.3-25044B.bin` (0x61d4 B, Thumb-2).
Prior RE (`preboot-decompile.md`) labelled this "stringless S2 SPI loader, NOT
stage2_loader v2.22.3" — **incorrect** ✎. It carries the banner and the full DDR stack:

Banner + DDR strings (file offsets, from `strings -t x`):
```
0x4892 stage2_loader v2.22.3
0x48de al_ddr_init(%d) succeeded after %d attempts!
0x4948 ddr_freq_change_according_to_spd
0x4969 ddr_init_dimm_params_get
0x4982 %s: ddr_init_spd_get failed!
0x49a0 al_ddr_spd_parse failed!
0x4aea SPD I2C Address: %02x
0x4b12 ddr_init_read_spd failed!
0x4b3f set_dram_impedance_ctrl_from_eeprom
0x4d8a al_ddr4_spd_parse   0x4d78 al_ddr3_spd_parse   0x4d9c al_ddr_spd_parse
0x4d59 al_ddr_spd_compute_cas_latency
0x505b al_ddr_cfg_init     0x5033 al_ddr_mode_register_set
0x5157 al_ddr_ctrl_dfi_init  0x513c al_ddr_phy_vt_calc_disable
0x53a3 Write Leveling Error … 0x54bd DRAM Vref Error … (full training error set)
```

### 2a. `org` / `tmg` / `addrmap` ← SPD over I2C, ✅
Recovered call chain: `ddr_init_read_spd` → `al_ddr_spd_parse` → `al_ddr4_spd_parse`
→ `al_ddr_spd_compute_cas_latency` + `..compute_cas_write_latency_ddr4`
→ `ddr_freq_change_according_to_spd` → `al_ddr_init`.
- I2C SPD address printed at runtime (`SPD I2C Address: %02x`); the DDR4 SPD supplies
  density, device width, ranks, speed bin, CL support map, tRCD/tRP/tRAS/tRC/tRFC/tFAW.
- Guards seen in strings: `%s: 4 bit chips not supported`, `%s: invalid DDR device width
  field`, `DDR clock is faster than the DIMM can support`, `%s: couldn't find supported
  CAS latency!` — standard JEDEC SPD parse, not board-table lookup.

**These exact numbers are NOT in the blob.** They come off the SPD chip. To populate
`al_ddr_init_cfg.org`/`.tmg`/`.addrmap` for ea16, read the SPD live
(`scripts/read-ddr-spd.py`). ⚠

### 2b. `impedance_ctrl` ← I2C EEPROM override block, ✅ (defaults byte-exact)
`set_dram_impedance_ctrl_from_eeprom` = `FUN_f2200a58` (`preboot-s2-decompiled.c:864`).
Builds a default impedance struct, then optionally overrides from an EEPROM block that
must start with magic `0xCC` (`if (*(buf+0xe) != 0xCC) return;` keeps defaults).

**Hardcoded defaults, rank≥2 path (our board = 2 ranks), byte-exact from decompile:**
| struct field (stage2 layout, `param_3+off`) | value | likely al_ddr_init_cfg_impedance_ctrl |
|---|---|---|
| `[0]` | 1 | `dic` |
| `[1]` | 5 (rank≥2) / 3 | `odt` |
| `[2]` | 2 (rank≥2) / 0 | `odt_dyn` |
| `+0x04` | `0x1928` | wr/rd odt or vref pair |
| `+0x06` | `0x0b0d` | " |
| `+0x08` | `0x0d0d` | " |
| `+0x0a` | `0x0707` | " |
| `+0x0c..+0x1a` | 5,10,5,10,4,8,1,2 | `wr_odt_map[]`/`rd_odt_map[]` |
| `+0x1c` | `0x0a0a` | phy_rout pair |
| `+0x1e` | `0x0808` | phy_odt pair |
| `+0x20` | 1 (0x100 if EEPROM byte`+0xf`==1) | flag |

Confidence **medium** on the field mapping (stage2's internal struct layout ≠ kernel HAL
`al_hal_ddr_init.h` offsets; values are byte-exact, the *names* are inferred). These are
SoC-family DDR4 defaults, not ea16-unique.

**EEPROM override decode tables (byte-exact from blob) — stored-value → HAL enum index:**
```
DRAM_ODT  (10): (0,0)(2,1)(4,2)(6,3)(8,4)(12,5)(1,6)(5,7)(3,8)(7,9)   → al_ddr_odt
ODT_DYN    (5): (0,0)(2,1)(4,2)(1,3)(0x99,4)                          → al_ddr_odt_dyn
DRAM_ROUT  (3): (6,0)(7,1)(5,2)                                       → al_ddr_dic (RZQ6/7/5)
PHY_ODT   (15): ohms 200,133,100,77,66,56,50,44,40,36,33,30,28,26,25 → al_ddr_phy_odt
PHY_ROUT  (11): ohms 80,68,60,53,48,44,40,37,34,32,30                 → al_ddr_phy_rout
```
Error strings gate each: `invalid DRAM ODT/ODT_DYN/ROUT from EEPROM`, `invalid PHY
ROUT/ODT from EEPROM` (`FUN_f2200a58`). The override block also carries a DRAM-voltage
GPIO selector (`FUN_f2200d10`: `dram voltage gpio pin out of range`, `spd dram voltage
support`, 1.25 V check).

**ea16 actual impedance = defaults UNLESS this board's I2C EEPROM carries a `0xCC` block.**
Unknown from NOR (the impedance EEPROM is a separate I2C device). ⚠ Read live, or read
back the trained PHY registers (`al_ddr_cfg_init` + PHY IMPD regs) — that's the ground
truth the vendor programmed.

---

## 3. Per-sysid conditioning — NONE for DDR, ✅

- The only sysid switch in the chain is DTB selection: al_boot `stg3_early_init`
  (`FUN_0100110c`) reads sysid 2 B BE from SPI-NOR `0x1F000C`, `switch` → DTB instance
  (ea16→0/`dt`, ea20→`dt_pro`, ea21→`dt_ai`, ea1a→`dt_bt`, ea30→`dt_hd`).
  See `preboot-decompile.md` §multi_dt.
- S2 DDR path has **no sysid read and no per-board table**; it keys entirely off the SPD
  chip + EEPROM. So "would another UBNT board get different DDR values?" — yes, but because
  it has a different SPD/EEPROM soldered, not because the blob picks a row. There is no row
  to dump.

---

## 4. Control DTB `dt_hd-NEW-5.1.25.dtb` — no DDR params, ✅

- Decompiled with `dtc`. `dt_hd` = board-cfg id `"alpine_v2_ubnt one nas hd"` (the HD/Pro
  variant, sysid 0xea30) — NOT our board. Our board is `dt`
  (`"alpine_v2_ubnt one nas v5.0"`, `tmp/dtb-current/dtb00-*.dts`).
- **`memory { reg = <0 0 0 0>; }` in every DTB** — DRAM base/size is patched at runtime
  from `shared_parameters.ddr_size`, never a DTB constant. No `ddr`/`memory-controller`
  timing node exists in any of the 5 DTBs.
- `/soc/board-cfg` holds pinctrl/gpio/**serdes**/ethernet/pcie/sgpo — board-specific, but
  **no DDR/i2c-bus/PLL DDR fields**. (Full ea16 board-cfg captured in
  `tmp/dtb-current/dtb00-0x081048-26208B.dts:1192-1629`.)

---

## 5. Other board-specific config found in the handoff

- **SerDes** (ea16, from `dt` board-cfg, byte-exact in DTS): grp0 `pcie_g2x2_pcie_g2x2`
  100 MHz lanes 0,2; grp1/grp2 `sata` 100 MHz 4-lane (tx override amp=7 units=0x1f
  post_emph=6 on lanes 1,3); grp3 `10gbe` 156.25 MHz lane0 (tx amp=7 units=0x1f
  post_emph=7 pre_emph=1); grp4 `skip`. Consumed by al_boot `al_serdes_init`
  (`dt_based_init_serdes_group`).
- **Ethernet:** port1 rgmii ext_phy addr 0x4 mdc-mdio 1 MHz; port2 10g-serial (SFP, i2c-id
  2, serdes grp3 lane0, dac len 3, force-1000base-x); port0/3 disabled.
- **PCIe:** ep-ports; port0 gen2 x1 enabled, others disabled.
- **PLL (from GPL `board_cfg.h`):** SB PLL chans ETH0/1 ref-clk 25 MHz; SerDes R2L/L2R
  100 MHz (commented out — PLL bypass, 100 MHz ref assumed). NB PLL (DDR clock) comes from
  `al_bootstrap` (PBS regfile), not board_cfg.
- **I2C:** stage2 SPD read uses `al_i2c_perform_write`/`al_i2c_read` on the general I2C
  (`AL_I2C_GEN_BASE`); impedance/caps EEPROM on I2C too (distinct from the SPI-NOR sysid
  read). SPD I2C address printed at runtime, not fixed in blob.
- **Identity EEPROM (NOR mtd04, our unit):** `UBNT` magic block @0x8000 (sysid `ea16`,
  Device ID `0b101d`, MAC `74:ac:b9:41:a8:11`), TlvInfo @0xd000 (P/N `113-02832-29`,
  date 20200524), ssh-rsa host key @0xe000. No DDR fields.

---

## 6. Mapping to `al_ddr_init_cfg` — status per field

| member | source | ea16 status |
|---|---|---|
| `org.data_width` | SPD (4×x16 = 64-bit) | ✅ 64-bit (BOM) / ⚠ confirm from SPD |
| `org.ranks` | SPD | ⚠ 2 (from live 2-bank split) — SPD authoritative |
| `org.ddr_type / ddr_device` | SPD | ✅ DDR4 / x16 (marking) |
| `org.ecc_is_supported` | SPD/BOM | ✅ 0 (no ECC device) |
| `tmg.*` (CL/CWL/tRCD/tRP/tRAS/tRC/tRFC/tFAW/ddr_freq) | **SPD only** | ⚠ NOT in blob — read SPD live |
| `addrmap.*` | derived from density/width/ranks | ⚠ derive after SPD |
| `impedance_ctrl.*` | EEPROM `0xCC` block, else defaults | ✅ defaults byte-exact (§2b) / ⚠ live PHY readback for actual |
| `misc.training_en` etc | stage2 fixed | ✅ training_en=1 (blob does full training) |
| `ddr_cfg` (bases/rev) | fixed | ✅ nb `0xf0070000`, ctrl `0xf0080000`, phy `0xf0088000`, rev V2 |

---

## 7. Open / unrecoverable

- **Exact SPD image (CL, tRCD, tRP, tRAS, tRC, tRFC, tFAW, speed bin) — the bulk of
  `tmg`/`org`/`addrmap`.** Lives on the on-board SPD I2C chip, not in any NOR blob. Read
  live: `scripts/read-ddr-spd.py`. This is the single biggest remaining item.
- Whether ea16's I2C impedance EEPROM carries a `0xCC` override block (→ actual
  `impedance_ctrl` ≠ defaults). Read I2C EEPROM, or read back trained PHY IMPD regs.
- Exact stage2 impedance struct layout (offsets in §2b are observed, not header-matched).
- No further gain from reversing the DDR *algorithm*: it is the open GPLv2
  `al_hal_ddr_init_alpine_v2.c` (5608 lines) already in the kernel tree — same "how".

## 8. Cross-check vs live readback
- `ddr_size` handoff (4 GiB) matches live `DRAM: 4 GiB` and the 2-bank DT split
  (3 GiB@0 + 1 GiB@0x200000000, the DRAM-remap hole for the 3–4 GiB MMIO window; single
  `alpine-mc` @0xf0080000, programmed by `al_addr_map_dram_remap_set`).
- Recommended: chainload U-Boot → `al_ddr_cfg_init(0xf0070000,0xf0080000,0xf0088000,&h)`
  → dump MR0..MR6 + PHY IMPD/VREF regs. That readback is authoritative for the actual
  `impedance_ctrl` and running frequency the vendor stage2 set (§2b confirms the trainer
  path but not the runtime EEPROM override result).
