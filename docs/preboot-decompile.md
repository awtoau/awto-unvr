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

Reset/entry chain into the reversed region. Ledger:
[preboot-alboot-names.sym](nor-reference/preboot-alboot-names.sym) /
[-names.md](nor-reference/preboot-alboot-names.md) (addresses below are the
**corrected** carve — see [preboot-coverage.md](preboot-coverage.md) "Carve
correction"):

1. `s_reset_tail` (`FUN_010005a8`) → calls `s_stage3_orchestrator`, then
   `halt_baddata` (never returns; control has left via the U-Boot jump
   inside).
2. `s_stage3_orchestrator` (`FUN_01002e8c`) — **stage3 DRAM/SoC-init
   orchestrator**:
   - `s_memcpy(0x1000, &DAT_01029158, 0x5e18)` — copies the embedded
     AArch64 resume-agent (20,492 B) to **physical `0x1000`**
     (`preboot-coverage.md` "AArch64 resume-agent")
   - ISB/DSB
   - `set_vectors(&DAT_01049000, 0xffff0000)` — vector base + MMU/SCTLR
   - `s_sctlr_bits_set(1,1,1)` (`FUN_0100036c`) — SCTLR: I-cache(0x1000) +
     align(4) + branch-pred(0x800), all enabled
   - `s_nb_acf_misc_wr_pos_set(0xf0070000, 0)` / `s_ccu_cluster_snoop_enable(0xf0090000, 1)`
     — **not agent-mailbox pokes** (corrected below)
   - `stg3_board_init()` (`FUN_01001108`, `__func__`-confirmed) — TOC scan +
     multi_dt switch (below) — **not `stg3_early_init`**, correcting an
     earlier mislabel; see `preboot-alboot-names.md`'s "Correction" note
   - `s_soc_fabric_port_init()` (`FUN_01002800`) — SoC/PCIe/ethernet fabric
     bring-up (table-driven)
   - `s_boot_app_load_exec()` (`FUN_01002a88`) — **loads U-Boot** (below)
   - returns `_DAT_fbff4150 == 0x31415926` — DDR-ready magic (pi digits),
     `struct shared_parameters` at `AL_PBS_INT_MEM_SRAM_BASE + 0x150`; main
     core polls it. Unrelated to the `0xf0070000`/`0xf0090000` pokes above.
3. `stg3_print_banner` (`FUN_0100063c`) — **Stage 3 banner** printer: `Stage 3
   version: %s`, `Commit ID: %s`, `CVOS commit ID: %s`, `HAL commit ID:
   %s`, `Press CTRL-F to print full trace`.

CPU/cache/MMU primitives (reproduce or skip in a mainline port — all standard
ARMv8-A32 SCTLR/cache-maintenance, no board secrets):
- `FUN_01000084` — read cache-type / CLIDR level.
- `FUN_010000b8` — clean/invalidate D-cache by set/way (loops CSSELR/CCSIDR).
- `s_sctlr_bits_set` (`FUN_0100036c`) — SCTLR bit compose (I/C/A/Z).
- `set_vectors` (`FUN_01000578`) — set vector base + MMU enable/disable, SCTLR M/TE bits.
- `FUN_01000614` — CPACR: enable CP10/CP11 (NEON/VFP), `|0xff00000`.

### DRAM/SoC init — what a mainline port must know

- **DDR training is NOT in this payload as open code.** It is delegated to the
  Annapurna CVOS **agent** ("agent_wakeup v2.10", `exec_via_agent`). This payload
  **polls `_DAT_fbff4150` for `0x31415926`** to learn DDR is up. DRAM size/timings come
  from the agent, not from a table here.
- `0xf0070000`/`0xf0090000` are NOT agent mailbox regs — per the actual
  decompile (`preboot-alboot-names.md`):
  - `0xf0070000` = `AL_NB_SERVICE_BASE`; `s_nb_acf_misc_wr_pos_set` toggles
    `NB_GLOBAL.acf_misc` bit 30
    (`NB_GLOBAL_ACF_MISC_WR_POS_DEV_AFTER_DEV_DIS`) — a fabric
    write-posting/ordering knob, unrelated to DDR handshake.
  - `0xf0090000` = the CCU; `s_ccu_cluster_snoop_enable` writes
    `speculation_ctrl_register_v1_v2` (`+4=7`) and `slaves[4]`/`slaves[5]`
    `.snoop_control_register` (`+0x4000`/`+0x5000 = 1`) — enabling ACE
    snoop requests from the two CPU-cluster CCU slave ports. **This is the
    same three offsets/values as U-Boot proper's later, DT-gated
    `al_ccu_init_inlined`** ([uboot-ccu-coherency.md](nor-reference/uboot-ccu-coherency.md)):
    al_boot does an unconditional early CCU-snoop enable, then **tears it
    back down** (`s_ccu_cluster_snoop_disable` / `s_nb_acf_misc_wr_pos_clear`,
    called from `s_boot_app_load_exec` right before the U-Boot jump) — so
    U-Boot inherits snoop *disabled* and must re-enable it itself, which is
    exactly what its `io_coherency`-gated init does. Relevant to the #97 CCU
    investigation: this is a genuine, deliberate early/late pairing, not a
    duplicate or a leftover.
- Porting consequence: a mainline U-Boot/ATF port on AL-324 can either keep the
  Annapurna DDR agent/al-boot doing DRAM bring-up (this is what ships), or
  re-implement the AL-324 DDR PHY sequence directly — the algorithm is open
  (`al_hal_ddr_init_alpine_v2.c`, 5,608 lines, BSD-style license,
  `delroth-alpine_hal/ddr/src/`), and the per-unit config it needs is fully
  decoded ([ddr-config-reverse.md](ddr-config-reverse.md) §6-7: EEPROM SPD +
  impedance, plus the live `ddr_pll_freq` strap via the `bootstrap` U-Boot
  command / `scripts/read-ddr-bootstrap.py`). Everything downstream (MMU,
  cache, multi_dt, U-Boot load) is reproducible too.
- `s_soc_fabric_port_init` (`FUN_01002800`) — fabric/PCIe/ethernet SoC config, driven by const tables at
  `DAT_010290a0..`; sets per-port endpoint config. References `soc_board_cfg
  ethernet port %d`, `exec_via_agent`, `ep_ports`.

### U-Boot handoff — `s_boot_app_load_exec` (`FUN_01002a88`)

- Reads image size via SPI read-fn `(*DAT_01049d8c)(0x40000, &sz)` then loads
  application **from SPI off 0x40000 to 0x1100000** (`Loading application to
  %08X ... u-boot`), size-checks (`< 0x100000` guard), then transfers control.
- Confirms the TOC `uboot` object (load/entry 0x1100000) is what runs next. This
  is the proprietary→GPL boundary.

## multi_dt DTB selection — GAP CLOSED

Question was: what input drives DTB selection, and how does it map to an index
(prior RE inferred EEPROM board id but had not disassembled the switch in the NEW
preboot). **Now confirmed in decompiled C AND disassembly.**

Location: inside `stg3_board_init` (`FUN_01001108`, `__func__`-confirmed via
`s_stg3_board_init_0102913c` at 5 call sites). The `stg3_early_init`
`__func__` string (`0x0102912c`, 16 B before `stg3_board_init`'s own string)
belongs to a single error path inside `thermal_sensor_trim_init`
(`FUN_01000b30`), not to this switch. Decompiled read + switch:

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

**Corrected 2026-08-26** (`preboot-alboot-names.md`): this table previously used
the uncorrected (+4) carve addresses AND had `stg3_early_init`/`stg3_board_init`
swapped (`0x0100110c`/`0x01002460` old-addressing). Re-verified against the
current decompile — `stg3_board_init`'s `__func__` string is confirmed at 5
call sites inside the multi_dt-switch function itself; `dram_clear`'s
`__func__` string is confirmed inside the DDR-size-check/iodma function.
Corrected addresses below are the current carve (`container[0x21004:0x6b6b4]`,
load `0x01000000`).

| VA | name | d | role |
|---|---|---|---|
| 0x010005a8 | `s_reset_tail` | R | reset entry tail → `s_stage3_orchestrator`, then `halt_baddata` |
| 0x01002e8c | `s_stage3_orchestrator` | R | AArch64 resume-agent memcpy, MMU/cache, `stg3_board_init`, fabric, U-Boot load; polls DDR-ready `0x31415926` |
| 0x01001108 | `stg3_board_init` | h | **TOC scan + sysid(NOR 0x1F000C)/hwrev(0x1F0010) read + multi_dt DTB switch + obj load** (§multi_dt) |
| 0x0100245c | `dram_clear` | h | DDR-size check, "clearing physical memory", iodma init |
| 0x01000b30 | `thermal_sensor_trim_init` | h | OTP/thermal trim + SRAM-agent memcpy/invoke + CPU-resume-regs validity check. **Open**: this function's own decompile embeds *three* different `__func__`-shaped strings across its error paths (`thermal_sensor_trim_init`, and separately `stg3_early_init` for one "cpu resume regs invalid" branch) — plausibly several small source functions got inlined into one Ghidra-recovered body; needs a source-level (not decompile-level) resolution to pick a single name. |
| 0x01002dec | `exec_via_agent` | m | calls `thermal_sensor_trim_init`, dispatches stage-3 to the resuming secondary core or runs it directly via `s_reset_tail` |
| 0x0100063c | `stg3_print_banner` | R | prints Stage 3 version/Commit/CVOS/HAL banner |
| 0x01002800 | `s_soc_fabric_port_init` | R | PCIe/eth SoC config, table-driven (`DAT_010290a0..`) |
| 0x01002a88 | `s_boot_app_load_exec` | R | SPI read → load app to 0x1100000, transfer control |

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

## Boot ROM / PBS SRAM layout — fact-check vs. unsourced pasted claims (2026-08-26)

Cross-checked an external, uncited pasted analysis against
`delroth-alpine_hal/platform/alpine_v2/include/al_hal_iomap.h` (**this SoC's own
platform header, alpine_v2**) and our decompiles. Our S2/al_boot decompiled code never
references the boot ROM addresses/entry points directly (it runs after the ROM has
already handed off) — those are confirmed via the HAL header, not via a decompiled
call site.

- **Boot ROM base `0xFBFF0000`, size `0x4000` (16 KiB) — CONFIRMED.**
  `AL_PBS_INT_MEM_BASE 0xfbff0000UL` (line 120), `AL_PBS_INT_MEM_BOOT_ROM_BASE =
  AL_PBS_INT_MEM_BASE + 0x0` (line 343), `AL_PBS_INT_MEM_SRAM_BASE = AL_PBS_INT_MEM_BASE
  + 0x4000` (line 344) — the boot ROM occupies exactly `0xfbff0000..0xfbff4000` and the
  PBS-internal SRAM (`ddr_ready` in [hardware.md](hardware.md)) begins right after it.
- **Alternate mapping `0xFD8A0000` — CONFIRMED.** `AL_SB_BASE 0xfc000000` (line 70) +
  `AL_SB_PBS_OFF 0x01880000` (line 161) → `AL_SB_PBS_BASE 0xfd880000`;
  `AL_PBS_BOOT_ROM_BASE = AL_SB_PBS_BASE + 0x20000` (line 303) = **`0xfd8a0000`** exactly.
  (`AL_SB_PBS_BASE 0xfd880000` is also the base of [hardware.md](hardware.md)'s whole
  PBS-peripheral table.)
- **Third mapping `0xFFFF0000` — UNVERIFIED as a boot-ROM alias**, but a `0xffff0000`
  high vector table is real and in-use elsewhere in this boot chain: al_boot calls
  `set_vectors(&DAT_01049000, 0xffff0000)` right before jumping into stage-3
  (`preboot-alboot-decompiled.c:1705`). Not proven to be the *ROM's* vectors specifically.
- **"Entry points" `0xFFFF0021`/`0xFFFF12F9` — CONFIRMED as HAL constants, not literally
  ROM entry points.** alpine_v2 HAL: `BOOT_ROM_NAND_READ_FUNC_PTR 0xffff0021`,
  `BOOT_ROM_XMODEM_RECIEVE_FUNC_PTR 0xffff12f9` (lines 349-350) — real ROM service
  routine pointers (NAND read / XMODEM receive callable from later stages), not a single
  reset vector. Not referenced anywhere in our decompiled S2/al_boot.
- **PBS SRAM internal layout — matches [hardware.md](hardware.md)'s `ddr_ready` row,
  MOSTLY CONFIRMED:**
  - `+0x100` = `SRAM_DEV_INFO_ADDRESS` (`0xfbff4100`, HAL line 358) — pasted "boot ROM
    device info at +0x100" **CONFIRMED**.
  - `+0x120` = `SRAM_CPU_RESUME_ADDRESS` (`0xfbff4120`, HAL line 357) — pasted "CPU
    resume structure at +0x120" **CONFIRMED**.
  - `+0x150` = `magic_num == 0x31415926` / `ddr_size` (`shared_parameters`,
    `PBS_INT_MEM_SHARED_PARAMS_OFFSET 0x0150`) — pasted "DDR init shared params at
    +0x150, magic 0x31415926" **CONFIRMED**, already established above and in
    [ddr-config-reverse.md](ddr-config-reverse.md).
  - `+0x200` = `SRAM_AGENT_ADDRESS` (`0xfbff4200`, HAL line 356) — real HAL term, and
    genuinely written by al_boot: `FUN_010129d8(&SUB_fbff4200, &LAB_0102ef70, 0x15fc)`
    (`preboot-alboot-decompiled.c:748`, memcpy 5,628 B into it). Likely the source of
    the pasted text's "agent" framing — but this "agent" is a data blob copied into PBS
    SRAM, unrelated to the AArch64 resume-agent discussed in
    [preboot-coverage.md](preboot-coverage.md) (that one goes to phys `0x1000`, not here).
  - **`LAB_0102ef70` blob DISASSEMBLED, arch + purpose CONFIRMED (2026-08-26).** File
    offset `0x2ef70` in the corrected 304,816 B al_boot carve (`tmp/alboot-payload.bin`,
    same file — carve recipe above), 5,628 B, runs from PBS SRAM at `0xfbff4200`
    (position-independent, matches copy-to-fixed-address design). **ARM A32 at entry,
    interworks to Thumb-2 via `BLX`** (confirmed both directions: ARM `blx #0xfbff4588`
    switches into a clean Thumb region; Thumb `blx #0xfbff42f4` / `blx #0xfbff437c` call
    back into ARM functions already identified from the A32 pass — consistent call
    graph both ways). Ghidra headless run **on this blob standalone**
    (`--arch arm32 --base 0xfbff4200 --entry 0xfbff4200 --preboot`) recovered **38
    functions**, artifacts staged: `docs/nor-reference/preboot-alboot-sram-agent-decompiled.c`
    / `-disassembly.asm`. (A naive same-mode linear sweep — capstone, ARM-only —
    decodes 90%+ of words as valid instructions purely because every SoC address
    literal in this blob's pools starts with a `0xf0`-`0xff` byte, which is ARM's
    unconditional-instruction encoding space; the Ghidra run with proper interworking
    is the trustworthy source, not that raw percentage.)
  - **Confirms the CPU-resume-setup hypothesis, with exact register hits — this is
    Annapurna's `agent_wakeup v2.10` secondary-CPU wake/resume stub** (banner string
    already known to be inside al_boot, `nor-boot-chain.md`; not traced to a specific
    one of the 38 functions — see `preboot-alboot-sram-agent-names.md`'s note. Full
    ledger: [preboot-alboot-sram-agent-names.sym](nor-reference/preboot-alboot-sram-agent-names.sym)
    / [-names.md](nor-reference/preboot-alboot-sram-agent-names.md)):
    - `nb_cpu_resume_addr_set` (`FUN_fbff4d54`, `-disassembly.asm:839`) writes
      **`al_nb_cpun_config_status.resume_addr_l`
      (off `0x28`) and `.resume_addr_h` (off `0x2c`)** — `movw r2,#0x2028` +
      `str r4,[r0,#0x2c]` off a base computed as `NB_SERVICE_BASE + 0x2000 + cpu*0x100`
      (the `cpun_config_status[cpu]` array, `al_hal_nb_regs_v1_v2.h:368-411`,
      `AL_NB_SERVICE_BASE = AL_NB_BASE(0xf0000000) + 0x70000 = 0xf0070000`). Called in a
      **loop over all 4 CPUs** (`nb_cpu_resume_setup_all`, `FUN_fbff48a0`, cpu index
      0..3) from the entry chain.
    - `nb_cpu_power_ctrl_set` (`FUN_fbff4d2c`) writes **`.power_ctrl` (off `0x20`)** the
      same way (`movw r3,#0x2020`), called with value `3` (park/wait) from the per-CPU
      dispatch loop (`s_cpu_wake_dispatch_loop`, `FUN_fbff48fc`) and `0` (run) once a
      wake target is set.
    - Did **NOT** find a literal/offset hit for `rvbar_low`/`rvbar_high` (off `0x48`/
      `0x4c`) anywhere in this blob — either RVBAR is fixed in hardware to this SRAM
      entry point on the AL-324 (so it never needs runtime programming) or it's set
      elsewhere (al_boot's own ARM body, not chased here). Not confirmed either way;
      flagging as open rather than assuming.
    - **Writes the resume-valid magic itself**: `nb_cpu_resume_setup_all`
      (`FUN_fbff48a0`) ends with
      `*(u32*)0xfbff4120 = 0xf0e1d2c4` (`SRAM_CPU_RESUME_ADDRESS`, literal pool pair
      around `fbff48ec-fbff48f8` in the disassembly) and zeroes 7 more words of a
      small struct there (`CPU resume structure`, matches the pasted-text framing
      already cross-checked above). `0xc4 > 0xc2` — this exact value satisfies the
      caller's own validity check in `thermal_sensor_trim_init`
      (`preboot-alboot-decompiled.c:673`), i.e. **this agent blob is the thing that
      makes the magic valid**; before it runs, `_DAT_fbff4120` is whatever cold-boot
      SRAM contents happen to be.
    - Per-CPU **GIC Distributor init**: `nb_gic_dist_init` (`FUN_fbff4d88`), called for
      both `id=0` and `id=1` from `s_nb_gic_dist_init_all` (`FUN_fbff4e0c`), selects
      base offset `0x200000` vs `0x9000` off
      `AL_NB_BASE` — **exactly** `AL_NB_GIC_DIST_BASE(id)`'s ternary
      (`al_hal_iomap.h:188`, `AL_NB_GIC_MAIN` vs the per-cluster GIC). Sets interrupt
      priority bytes to `0x80808080` and an enable/clear mask to `0xffffffff`. A raw
      literal-pool scan of the blob independently turned up the absolute GICv2
      Distributor register addresses `0xf0200080/0100/0180/0400/0800/0c00`
      (`ICDISR/ICDISER/ICDICER/ICDIPR/ICDIPTR/ICDICFR` off `AL_NB_GIC_DIST_BASE_MAIN
      0xf0200000`), confirming the same block from the data side.
    - Also does an **EL3→NS/Hyp handoff**: `s_scr_ns_set_and_jump`/`s_smc_el3_transition`
      (`FUN_fbff4fa0`/`FUN_fbff4ff4`) read/write
      `SCR` (Secure Configuration Register, NS bit), execute `smc #0` (also standalone in
      `hvc_call`/`FUN_fbff4f8c`) and `hvc #0`,
      and touch `NSACR`/`ACTLR` (`s_actlr_nsacr_smp_setup`, `FUN_fbff5058`, SMP-alike bit
      `0x80000000`) — this
      isn't just SRAM/GIC housekeeping, it's actively switching security state before
      handing a woken CPU to its target exception level. Not fully mapped (which
      64-bit coprocessor pair it's programming via `MRRC/MCRR p15,1` — plausibly
      `VTTBR` — wasn't chased further; low priority next to the resume-address/GIC
      finds).
    - Also contains a **printf-family debug backend**: `s_itoa`
      (`FUN_fbff4628`, itoa/number-format), `s_vprintf_format` (`FUN_fbff4740`,
      `%d/%x/%X/%p/%u/%c/%s` format-string
      parser), `s_uart_tx_raw`/`s_uart_putc_crlf` (`FUN_fbff4f08`/`FUN_fbff4ed8`, UART TX
      with CR→CRLF translation) — logs
      to the same UART al_boot's main body uses.
    - `l2ctlr_smp_park_sync` (`FUN_fbff42f4`, the ARM function called from the Thumb
      wake-dispatch with
      param `1`) is the **SMP-coherency-before-caches** sequence: writes an
      implementation-defined CP15 "peripheral system" register, then bit `0x82`/
      `0x200082` (depending on the param) into **L2CTLR** (`opc1=1,CRn=c9,CRm=c0,opc2=2`
      — Cortex-A15/A17 L2 Control Register) — guarded by a two-CPU handshake
      (`*DAT_fbff434c=1; while(*DAT_fbff4350!=2);`) so only one CPU touches the
      shared L2 config. Textbook Cortex-A15-family secondary-CPU bring-up (enable
      SMP/coherency before turning on D-cache), not AL-specific.
  - **`FUN_0100016c`** (called right before `(*(code *)&SUB_fbff4200)()` at
    `preboot-alboot-decompiled.c:749`) — checked: `ICIALLU` (Invalidate Entire
    Instruction cache, `mcr p15,0,r0,c7,c5,0`) + `ISB` + `DSB`, both `SY`. **Confirmed
    cache-invalidate-and-barrier before executing freshly-`memcpy`'d code from SRAM** —
    exactly what's needed after writing code bytes and before jumping into them on
    ARMv7 (I-cache may hold stale/no data for the new address; barriers order the
    writes before the fetch). No surprises here.
  - **"+0x000 = Stage-2 image offset" — REFUTED / not this SRAM.** S2 links and runs at
    `0xF2200000` (`s2_sram`, a separate 256 KiB window — [hardware.md](hardware.md)), not
    at `0xfbff4000+0`. Nothing in the HAL or our decompiles puts S2 inside the 4 KiB PBS
    SRAM block.

## Open / not chased

- Exact AL-324 DDR PHY timings — **RESOLVED**: SPD/impedance decoded from the live I²C
  EEPROM ([ddr-config-reverse.md](ddr-config-reverse.md) §6, `scripts/decode-ddr-records.py`).
  The one field the EEPROM can't give (`al_bootstrap.ddr_pll_freq`, the running strap —
  EEPROM only bounds it ≤1866 MT/s) now has tooling to read it live: U-Boot `bootstrap`
  command + `scripts/read-ddr-bootstrap.py` (ddr-config-reverse.md §7).
- `FUN_01012b08` used as both memcmp and RSA-verify entry — same primitive; not
  split into named sub-ops here.
- S2 0xf22000fc jumptable (pointer-dispatch) not reconstructed.
