# UNVR proprietary preboot — function-level decompile

Scope: the closed-source parts of the boot chain (no GPL source exists). U-Boot
proper is GPL (UNVR-1.3.35-GPL) and out of scope. Decompiler: Ghidra 12.2 headless.

Builds on: [nor-boot-chain.md](nor-boot-chain.md), [bootloader.md](bootloader.md),
[uboot-update-path.md](uboot-update-path.md). Cross-check
[nor-reference/preboot-dt-selection.asm](nor-reference/preboot-dt-selection.asm).

## Decompiler fix (was the blocker)

Setup/JDK fix + methodology: [ghidra.md](ghidra.md) §1. This doc's verification of
that fix: headless import + auto-analysis + decompile-all of the **304,816-B**
al_boot payload → **377 functions, 0 decompile failures**, ~20 s wall. S2 blob →
82 functions. Real data-flow C (coprocessor cache ops, recovered `switch` on
sysid, RSA loops). JDK 25 previously hung indefinitely on the same input.

## Targets, arch, load — confirmed from container

TOC (`scripts/parse-al-toc.py` on `01-uboot.bin`, magic `0x070c070c` @0x80000):

| Obj | type | container off | notes |
|---|---|---|---|
| preboot | 0x0d | 0x00000 (size 0x80000) | S2 + al_boot payload (below) |
| dt / dt_pro / dt_ai / dt_bt / dt_hd | 0x02 | 0x81000..0xac000 | 5 DTBs, multi-DT index = type high nibble (0..4) |
| uboot | 0x05 | 0xac000, load/entry 0x1100000 | GPL U-Boot, out of scope |

Two proprietary code blobs inside `preboot`:

- **S2 first-stage** — container 0x00000, 25,044 B, magic `"S2"`. NOT stringless and
  NOT pure Thumb: A32 entry stub 0x20-0x98, Thumb-2 body from 0x98 (it IS the
  `stage2_loader v2.22.3` DDR trainer). Link base **0xF2200000** (SRAM). Carved:
  `docs/nor-reference/s2-loader-stage2_v2.22.3-25044B.bin`.
- **al_boot payload** — img-hdr (magic `0x000b9ec7`) @container 0x20000, u32 length
  prefix @0x21000, payload @container **0x21004**, size **0x4a6b0 (304,816 B)**, 4-byte
  trailer `a3 34 a4 03` @0x6b6b4, **ARM A32, load 0x01000000**. Contains both banners:
  Annapurna al-boot v2.10.0 (`stage2_loader v2.22.3`, `agent_wakeup v2.10`) AND
  contractor `Stage 3 v2.22.0` (multi_dt). Carve:
  `container[0x21004:0x6b6b4]`. (The img-hdr field @+0x28 = 0x6a6b4 is length+0x20000 —
  the old 435,892 B / 0x21000 carve swept in the TOC + `dt` DTB as junk; see
  [preboot-coverage.md](preboot-coverage.md).)

Confirmation method: img-hdr payload-size @+0x28, load/entry @+0x30/+0x38; length prefix
@0x21000 = 0x4a6b0; dispatch opcode `movw r7,#0x9308` present in the multi_dt switch.

Decompiled artifacts (this doc's evidence):
- `docs/nor-reference/preboot-alboot-decompiled.c` / `-disassembly.asm` (317 fns)
- `docs/nor-reference/preboot-s2-decompiled.c` / `-disassembly.asm` (78 fns)

FUN_ addresses below are VAs; names in **bold** are from `__func__`/banner strings.

## Boot sequence — al_boot payload (VA 0x01000000)

Reset/entry chain into the reversed region:

1. `FUN_010005ac` (reset tail) → calls `FUN_01002e90`, then `halt_baddata` (never
   returns; control has left via the U-Boot jump inside).
2. `FUN_01002e90` — **stage3 DRAM/SoC-init orchestrator**:
   - logs `dram_clear` (`FUN_010129dc(0x1000, "dram_clear", 0x5e18)`)
   - ISB/DSB
   - `FUN_0100057c(&DAT_01049000, 0xffff0000)` — MMU/vectors + SCTLR
   - `FUN_01000370(1,1,1)` — SCTLR: I-cache(0x1000)+align(0x2)+branch-pred(0x800)
   - `FUN_010274e8(0xf0070000, 0)` / `FUN_01027508(0xf0090000, 1)` — poke SoC
     fabric/agent mailbox regs
   - `FUN_0100110c()` — **`stg3_early_init`** (`__func__`): TOC scan + multi_dt (below)
   - `FUN_01002804()` — SoC/PCIe/ethernet fabric bring-up (table-driven)
   - `FUN_01002a8c()` — **loads U-Boot** (below)
   - returns `_DAT_fbff4150 == 0x31415926` — DDR-ready magic (pi digits) written by
     the CVOS agent; main core polls it.
3. `FUN_01000640` — **Stage 3 banner** printer: `Stage 3 version: %s`, `Commit
   ID: %s`, `CVOS commit ID: %s`, `HAL commit ID: %s`, `Press CTRL-F to print
   full trace`.

CPU/cache/MMU primitives (reproduce or skip in a mainline port — all standard
ARMv8-A32 SCTLR/cache-maintenance, no board secrets):
- `FUN_01000088` — read cache-type / CLIDR level.
- `FUN_010000bc` — clean/invalidate D-cache by set/way (loops CSSELR/CCSIDR).
- `FUN_01000370` — SCTLR bit compose (I/C/A/Z).
- `FUN_0100057c` — set vector base + MMU enable/disable, SCTLR M/TE bits.
- `FUN_01000618` — CPACR: enable CP10/CP11 (NEON/VFP), `|0xff00000`.

### DRAM/SoC init — what a mainline port must know

- **DDR training is NOT in this payload as open code.** It is delegated to the
  Annapurna CVOS **agent** ("agent_wakeup v2.10", `exec_via_agent`). This payload
  only pokes agent mailbox regs (`0xf0070000`, `0xf0090000`) and **polls
  `_DAT_fbff4150` for `0x31415926`** to learn DDR is up. DRAM size/timings come
  from the agent, not from a table here.
- Porting consequence: a mainline U-Boot/ATF port on AL-324 must either keep the
  Annapurna DDR agent/al-boot doing DRAM bring-up (this is what ships), or
  re-implement the AL-324 DDR PHY sequence (proprietary, not recoverable here).
  Everything downstream (MMU, cache, multi_dt, U-Boot load) is reproducible.
- `FUN_01002804` — fabric/PCIe/ethernet SoC config, driven by const tables at
  `DAT_010290a0..`; sets per-port endpoint config. References `soc_board_cfg
  ethernet port %d`, `exec_via_agent`, `ep_ports`.

### U-Boot handoff — `FUN_01002a8c`

- Reads image size via SPI read-fn `(*DAT_01049d8c)(0x40000, &sz)` then loads
  application **from SPI off 0x40000 to 0x1100000** (`Loading application to
  %08X ... u-boot`), size-checks (`< 0x100000` guard), then transfers control.
- Confirms the TOC `uboot` object (load/entry 0x1100000) is what runs next. This
  is the proprietary→GPL boundary.

## multi_dt DTB selection — GAP CLOSED

Question was: what input drives DTB selection, and how does it map to an index
(prior RE inferred EEPROM board id but had not disassembled the switch in the NEW
preboot). **Now confirmed in decompiled C AND disassembly.**

Location: inside `FUN_0100110c` = **`stg3_early_init`** (`__func__`-confirmed; NOT
"stg3 board init" — that is `FUN_01002460`; references `dt_based_init_pcie`).
Decompiled read + switch:

```c
// board-id read via SPI-NOR read fn-ptr: (offset, dst, len)
uVar8 = (*(code *)*piVar10)(0x1f000c, &DAT_0104930a, 2);   // 2 bytes @ NOR 0x1F000C
DAT_0104930a = (DAT_0104930a>>8) | (DAT_0104930a<<8);       // byte-swap -> BE sysid
// ("fetch_subsystem_ID", "subsystem_id: 0x%04x")
(*(code *)*piVar10)(0x1f0010, &DAT_0104930c);               // 4 bytes @ NOR 0x1F0010 (hw rev id)
switch (DAT_0104930a) {                                     // switchD_01001718
  case 0xea16: ...   // -> instance 0
  ...
  default: DAT_01049308 = 0xff;   // INVALID error path
}
```

**Selection input (confirmed):** the **board id / sysid** = **2 bytes,
big-endian, read from SPI-NOR flash offset 0x1F000C** (= EEPROM partition mtd04
offset 0x000C; see `eeprom-fieldmap.json`, `identity-partitions.md`). A second read
at 0x1F0010 (4 B, BE) fetches the hw-revision/device id but does **not** drive the
DTB choice. This is a SPI-NOR read (via `al_flash`/SPI read fn-ptr), not I2C.

**Mapping (sysid → DTB instance → TOC object)** — `scripts/decode-preboot-dtmap.py`
on the carved payload; matches the recovered `switch` and the ARM `ldrls pc,[pc,
r3,lsl#2]` table at VA 0x01001720 (idx = sysid−0xea16, 27 slots, default 0xFF):

| sysid | instance | DTB (TOC obj) |
|---|---|---|
| 0xea16 | 0 | dt |
| 0xea1a | 3 | dt_bt |
| 0xea20 | 1 | dt_pro |
| 0xea21 | 2 | dt_ai |
| 0xea30 | 4 | dt_hd |
| all other 0xea17..0xea2f | 0xFF | INVALID (error path) |

This unit is **sysid 0xea16 → instance 0 → DTB `dt`** (UNVR4, per
`identity-partitions.md`). Instance byte stored at `DAT_01049308`; U-Boot later
selects the matching TOC DTB object by that index (type high-nibble).

## EEPROM + RSA/eFuse capability authentication — confirmed preboot-only

Three functions, all in the al_boot payload. This authenticates a **signed SoC
capabilities blob**, and gates **SoC capability flags — NOT the OS kernel** (kernel
is unsigned; see nor-boot-chain.md §5). Confirmed by what the pass/fail paths do:
they set capability state and log, they do not abort OS boot.

### `FUN_01003234` — I2C EEPROM capability parser

- Reads 12-byte (0x0c) records from an **I2C EEPROM** via `FUN_01003464(i2c_bus=
  DAT_01049dbf, addr, 2, dst, len)` (this is I2C, distinct from the SPI-NOR sysid
  read in multi_dt).
- Walks TLV-style encapsulation using `FUN_01012b08` (compare) against marker
  strings: `no opening EEPROM encapsulation`, `failed to find closing EEPROM…`,
  `i2c read %d failed`, `i2c modulues read failed`.
- On success returns the capabilities blob (`param_3`, 0x100 B) and the RSA
  **modulus** (`param_4`, 0x100 B = 2048-bit) read from the I2C EEPROM.
- Error returns: `0xffffffea` (no/short encapsulation), `0xfffffffb` (read fail).

### `FUN_01002f3c` = `eeprom_preload_parser` — RSA capability signature authentication

- Hashes the capabilities blob: `FUN_01009a58` (init) / `FUN_01009c44` (update,
  0x100 B) / `FUN_01009ae4` (final → 32-byte digest). Digest size 32 ⇒ SHA-256.
- Compares the **modulus hash against the eFuse-burned hash** at memory-mapped
  **`0xfd89608c` (32 bytes, to 0xfd8960ac)** — the SoC eFuse/secure region. On
  mismatch: logs `signature decryption does not match the hashed capabilities`
  and returns `0xfffffffb`.
- Then RSA-verifies the signature over the caps (`FUN_01012b08` modexp/verify with
  the modulus); on fail logs `RSA decryption failed!`, retries once, then fails.
- `eeprom_preload_parser` path handles < 0xc leftover records.

### eFuse-modulus check — `eFuse hashed modulus` error

- Emitted in `FUN_01003234` (VA 0x01030f80 string). Fires when the eFuse-stored
  modulus hash ≠ hash of the EEPROM-provided modulus — the device only trusts a
  capabilities blob signed by the key whose modulus was fused at manufacture.
- The word-for-word 32-byte hash compare against eFuse mapping `0xfd89608c` is in
  `FUN_01002f3c` (above); both functions gate on the fused modulus.

**Trust-model confirmation (matches nor-boot-chain.md §5):**
- Root of trust = **eFuse-burned SHA-256 of the RSA modulus** (`0xfd89608c`).
- Authenticated object = **I2C-EEPROM capabilities blob + its RSA-2048 signature**.
- Effect = enable/deny **SoC capability flags** in preboot. No branch here reads,
  hashes, or verifies the OS kernel/uImage. The SPI-NOR EEPROM RSA **private** key
  (@0xe004, see secrets inventory) is unrelated — never read by this path.

## Function name map — significant functions

Annapurna/contractor names recovered from `__func__` strings the AL HAL / contractor stage3 leave
in `.rodata` (log calls pass the enclosing function's `__func__`). Extractor:
`scripts/name-preboot-funcs.py` (reads the full NUL-terminated string from
`tmp/alboot-payload.bin` at each label's rodata VA — defeats Ghidra's ~32-char label
truncation; keeps only clean C-identifier strings). Full machine map (89 auto-named):
`tmp/logs/name-preboot-funcs.log`.

**Coverage (verified 100% — [preboot-coverage.md](preboot-coverage.md)):** on the
**corrected carve** (304,816 B @0x01000000, not the +4/oversized old load) al_boot has
**377 functions**, **119 auto-named** via `__func__` (`preboot-alboot-names.sym`). Every
byte is classified: 55.3% A32 code, 8.5% defined data, 36.2% attributed undefined
(blob57 embedded non-ARM data 57 KB, zero/BSS 30 KB, embedded AArch64 resume-agent
20 KB = 66 fns, tables/pools 2 KB). **0 undiscovered A32 code.** **S2 is NOT stringless
and NOT pure Thumb**: 82 functions, mixed A32 entry stub (0x20-0x98) + Thumb body, full
DDR stack with strings; DDR-init subgraph 100% named (`preboot-s2-names.sym`).

Names in `code` are recovered `__func__`; names in (parens) are functional labels
from hand-RE (no `__func__` logged). "d" = auto-name confidence: h=high (`__func__`
recurs, hits≥4), m=medium (hits 2–3), R=hand-RE.

### Boot orchestration — contractor stage3

| VA | name | d | role |
|---|---|---|---|
| 0x010005ac | (reset_tail) | R | reset entry tail → `FUN_01002e90`, then `halt_baddata` |
| 0x01002e90 | (stg3 orchestrator) | R | DRAM/SoC-init: dram_clear, MMU/cache, board init, fabric, U-Boot load; polls DDR-ready `0x31415926` |
| 0x0100110c | `stg3_early_init` | h | **TOC scan + sysid(NOR 0x1F000C)/hwrev(0x1F0010) read + multi_dt DTB switch + obj load** (§multi_dt) — **corrects** earlier "stg3 board init" label |
| 0x01002460 | `stg3_board_init` | h | DDR-size check, "clearing physical memory", iodma init |
| 0x01000b34 | `dt_based_init` | h | DT-driven init dispatch (thermal trim, `dt_based_init_pcie`) |
| 0x01002df0 | `power_down_secondary_cpus` | m | parks secondary A57 cores |
| 0x01000640 | (stage3 banner) | R | prints Stage 3 version/Commit/CVOS/HAL |
| 0x01002804 | (SoC fabric bring-up) | R | PCIe/eth SoC config, table-driven (`DAT_010290a0..`) |
| 0x01002a8c | (U-Boot loader) | R | SPI read → load app to 0x1100000, transfer control |

### CPU / cache / MMU primitives (ARMv8-A32; reproduce or skip in a port)

| VA | name | d | role |
|---|---|---|---|
| 0x01000088 | (read_cache_type) | R | read cache-type / CLIDR level |
| 0x010000bc | (dcache_clean_inval) | R | clean/invalidate D-cache by set/way |
| 0x01000370 | (sctlr_compose) | R | SCTLR bit compose (I/C/A/Z) |
| 0x0100057c | `set_vectors` | m | VBAR + MMU enable, SCTLR M/TE — confirms hand-RE |
| 0x01000618 | (cpacr_enable_simd) | R | CPACR enable CP10/CP11 (NEON/VFP) |

### Flash / TOC (HAL)

| VA | name | d |
|---|---|---|
| 0x01005504 | `read_toc_obj_hdr` | m |
| 0x01027a3c | `al_flash_obj_id_to_str` | h |
| 0x01027c58 | `al_flash_toc_validate` | m |
| 0x01027e14 | `al_flash_toc_find_id_with_fallback` | h |
| 0x01027f70 | `al_flash_toc_stage2_active_instance_get_with_fallback` | h |
| 0x010280d8 | `al_flash_obj_header_read_and_validate` | h |

### EEPROM / RSA / eFuse capability auth (§RSA)

| VA | name | d | role |
|---|---|---|---|
| 0x01003234 | (i2c caps record reader) | R | reads 12-B TLV records from I2C EEPROM via `FUN_01003464` |
| 0x01002f3c | `eeprom_preload_parser` | m | **RSA-2048 verify + eFuse SHA-256 modulus compare @`0xfd89608c`** (contains the SHA calls) — name recovered |
| 0x01009a58 | (sha256_init) | R | SHA-256 init |
| 0x01009c44 | (sha256_update) | R | SHA-256 update (0x100 B) |
| 0x01009ae4 | (sha256_final) | R | SHA-256 final (32-B digest) |
| 0x01012b08 | (memcmp / rsa_modexp) | R | shared compare / RSA-verify primitive |
| 0x01003464 | (i2c_eeprom_read) | R | I2C read (bus, addr, len, dst, len) |
| 0x01003448 | `i2c_read` | m | preboot I2C read flag helper |

### HW-access HAL — I2C / UART / OTP / timer / PLL / bootstrap / PBS / thermal

| VA | name | d |
|---|---|---|
| 0x01022864 | `al_i2c_perform_write` | h |
| 0x01022ce8 | `al_i2c_read` | m |
| 0x01022f00 | `al_uart_handle_init` | h |
| 0x01022e6c | `al_uart_is_input_available` | m |
| 0x0100d0e0 | `uart_write_byte` | h |
| 0x0100d204 | `uart_read_byte` | m |
| 0x01024fdc | `al_otp_read_word` | h |
| 0x010244b8 | `al_timer_value_get` | h |
| 0x01023ed4 | `al_pll_init` | m |
| 0x01023fd4 | `al_pll_freq_get` | h |
| 0x01022328 | `al_bootstrap_parse` | h |
| 0x010221cc | `al_pbs_axi_timeout_set` | h |
| 0x0102383c | `al_addr_map_dram_remap_set` | m |
| 0x01024968 | `al_thermal_sensor_readout_get` | h |

### CPU sys-fabric bring-up

| VA | name | d |
|---|---|---|
| 0x01025300 | `al_sys_fabric_handle_init` | h |
| 0x01025484 | `al_sys_fabric_cluster_handle_init` | h |
| 0x010255e0 | `al_sys_fabric_cluster_pd_pu_timer_set` | h |
| 0x010257a4 | `al_sys_fabric_core_power_on_reset` | h |
| 0x0102596c | `al_sys_fabric_core_reset_deassert` | h |
| 0x01025bcc | `al_sys_fabric_core_aarch64_setup` | h |

### DDR

| VA | name | d |
|---|---|---|
| 0x01021f14 | `al_ddr_cfg_init` | m |

### SerDes (10G/25G PHY bring-up)

| VA | name | d |
|---|---|---|
| 0x01026184 | `al_serdes_init` | h |
| 0x0101dc0c | `al_serdes_hssp_group_ictl_pma_val_set` | h |
| 0x0101f4bc | `al_serdes_hssp_group_cfg_eth_sgmii_2_5g_mode` | m |
| 0x010194d0 | `al_serdes_25g_reg_read` | m |
| 0x01019be4 | `al_serdes_25g_rx_diag_info_get` | m |
| 0x01019f68 | `al_serdes_25g_rx_leq_fsm_op` | m |
| 0x0101a0b0 | `al_serdes_25g_single_iteration_dosc_set` | m |
| 0x0101a1d8 | `al_serdes_25g_rx_equalization` | h |
| 0x0101af8c | `al_serdes_25g_cdr_is_locked` | m |
| 0x0101bc3c | `al_serdes_25g_qsample_pll_lock_check` | m |
| 0x0101bd6c | `al_serdes_25g_tx_pll_wa_find_window` | h |

### PCIe / UDMA / IOFIC / unit-adapter (I/O fabric)

| VA | name | d |
|---|---|---|
| 0x0100c2f0 | `al_pcie_read_config` | h |
| 0x0100c718 | `al_pcie_master_enable` | h |
| 0x01003b24 | `al_pcie_int_adapter_pd` | h |
| 0x01003e74 | `pci_skip_dev` | m |
| 0x01012eb8 | `al_udma_init` | h |
| 0x0101332c | `al_udma_q_init` | m |
| 0x010134c0 | `al_udma_state_set` | h |
| 0x010040d8 | `al_udma_fast_desc_flags_set` | h |
| 0x01004584 | `al_udma_ring_id_get` | h |
| 0x01013818 | `al_udma_q_set_pointers` | h |
| 0x01013af4 | `al_udma_m2s_packet_size_cfg_set` | h |
| 0x01013c34 | `al_udma_m2s_max_descs_set` | h |
| 0x01013d6c | `al_udma_iofic_s2m_error_ints_unmask` | m |
| 0x01012b84 | `al_iofic_abort_mask_clear` | h |
| 0x01025e24 | `al_unit_adapter_handle_init` | m |

### Misc (hand-RE)

| VA | name | role |
|---|---|---|
| 0x010129dc / 0x0100ceb0 / 0x01010c00 | (log/printf) | AL logging helpers |
| 0x010274e8 / 0x01027508 | (poke fabric / agent-mailbox reg) | writes `0xf0070000` / `0xf0090000` |

## S2 first-stage (A32 entry + Thumb-2 body @0xF2200000) — the DDR trainer

Corrects the old "78 fns, zero strings, stringless SPI loader" note ✎. S2 IS the DDR
trainer (`stage2_loader v2.22.3`) — see [ddr-config-reverse.md](ddr-config-reverse.md),
[ddr-s2-parser-analysis.md](ddr-s2-parser-analysis.md). Coverage verified 100%
([preboot-coverage.md](preboot-coverage.md)).

- **82 functions**, code 67.0%, data 20.2%, undef 12.8% (all data), **0 code gaps**.
- **Mixed mode:** header 0x0-0x20 (data), **A32 entry stub 0x20-0x98** (SCTLR I-cache
  enable, `mrc/mcr`), **Thumb-2 body from 0x98** (memcpy32 @0x98, memset @0xb4, then the
  DDR/TOC code). The A32 stub + `0xf220005c` (MPIDR core-id) + `0xf22014b4` (Thumb
  wrapper) were undiscovered until seeded; now disassembled.
- Loads the al_boot payload (`01-uboot.bin[0x21004:0x6b6b4]`, 304,816 B) to 0x01000000,
  writes `shared_parameters` (magic 0x31415926, ddr_size) to SRAM 0xfbff4150, jumps.
- **DDR-init call graph 100% named** — `docs/nor-reference/preboot-s2-names.{sym,md}`
  (31 confirmed, 25 speculative `s_`, 0 bare over the 56-fn subgraph). Named annotated
  disassembly (the primary artifact): `docs/nor-reference/preboot-s2-disassembly.asm`.

## Coverage verification (100%) — [preboot-coverage.md](preboot-coverage.md)

- Byte accounting via `scripts/ghidra/CoverageReport.java` +
  `scripts/measure-preboot-coverage.py`; iterated to **0 undiscovered code** in both
  blobs (seed entry, disassemble capstone-verified code gaps, re-measure).
- **al_boot 304,816 B:** code 55.3% (377 fns) / data 8.5% / undef 36.2% (all attributed:
  blob57 57 KB non-ARM data, zero/BSS 30 KB, AArch64 agent 20 KB=66 fns, tables 2 KB).
- **S2 25,044 B:** code 67.0% (82 fns) / data 20.2% / undef 12.8% (all data).
- **Only region not decoded-to-meaning:** `blob57` @0x0103469d (57 KB, 18.8% of al_boot)
  — proven not-ARM-code, unreferenced, entropy 6.66; candidate SerDes-25G PHY microcode.
- New pipeline flags: `ghidra-analyse.py --entry/--entry-thumb/--disasm-gaps`;
  `SeedEntry.java`, `DisasmGaps.java`, `CoverageReport.java`; annotation via
  `annotate-preboot.py` + `build-s2-name-ledger.py`.

## Superseded by [ddr-config-reverse.md](ddr-config-reverse.md)

- **DDR training is NOT in a "CVOS agent".** It is `al_ddr_init` inside the **S2 /
  `stage2_loader v2.22.3`** blob (`FUN_f2201a90`, 8,392 B), fed by a JEDEC SPD +
  impedance record read over I²C. The S2 is not stringless.
- **`0xfbff4150` is not a mailbox** — it is `struct shared_parameters`
  {`magic 0x31415926`, `ddr_size` u64} at `AL_PBS_INT_MEM_SRAM_BASE + 0x150`, named in
  the GPL U-Boot (`board/annapurna-labs/common/shared_params.h`). `0xf0070000` is
  `AL_NB_SERVICE_BASE`, `0xf0090000` the CCU.
- **al_boot payload carve is off by 4 bytes.** Real payload = container
  `0x21004 .. 0x6b6b4` (0x4a6b0 B, u32 length prefix at 0x21000), load 0x01000000. The
  existing decompile/disassembly were loaded at 0x21000, so **every `FUN_`/`DAT_` VA in
  this document is 4 too high** (e.g. `al_ddr_cfg_init` = 0x01021f10). The instruction
  decode itself is valid.

## Open / not chased

- Exact AL-324 DDR PHY timings — recoverable: read the SPD + impedance records off the
  I²C EEPROM ([ddr-config-reverse.md](ddr-config-reverse.md) §7).
- `FUN_01012b08` used as both memcmp and RSA-verify entry — same primitive; not
  split into named sub-ops here.
- S2 0xf22000fc jumptable (pointer-dispatch) not reconstructed.
