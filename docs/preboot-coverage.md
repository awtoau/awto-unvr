# Preboot decompile — byte-level coverage verification

Goal: prove every byte of each proprietary preboot blob is classified as **code
(disassembled)** or **known data (with reason)** — no undiscovered code, no unreached
gaps. Static only.

Method: Ghidra 12.2 headless (`scripts/ghidra-analyse.py`), correct carve from the AL
TOC, entry seeded, verified-code gaps disassembled, then a byte accounting
(`scripts/ghidra/CoverageReport.java`) + gap classification
(`scripts/measure-preboot-coverage.py`, capstone control-flow test). Re-run to
convergence (0 code gaps). Numbers below are from the final exports in
`docs/nor-reference/`.

## Carve correction (was wrong) ✎

- Prior al_boot decompile loaded **435,892 B @ file 0x21000** — 4 bytes early AND
  ~130 KB too long (it swept in the TOC @0x80000 + the `dt` DTB as junk "code").
- **Correct al_boot payload** = `01-uboot.bin[0x21004 : 0x6b6b4]` = **304,816 B**
  (0x4a6b0, the u32 length prefix @0x21000), load **0x01000000**, 4-byte trailer
  `a3 34 a4 03` @0x6b6b4. (`ddr-config-reverse.md` said "305,328" — arithmetic slip;
  the bytes give 0x4a6b0 = 304,816.)
- S2 = `01-uboot.bin[0x0 : 0x6114]` region, whole carved blob **25,044 B**, load
  **0xF2200000**. Byte-identical to `docs/nor-reference/s2-loader-stage2_v2.22.3-25044B.bin`.

## S2 stage2_loader (Thumb-2 + A32, 25,044 B) — 100% classified

| class | bytes | % | notes |
|---|---|---|---|
| code | 16,790 | 67.0% | 82 functions, 0 decompile failures |
| defined data | 5,053 | 20.2% | strings, pointers, lookup tables Ghidra auto-typed |
| undefined (data) | 3,201 | 12.8% | **all data — 0 code gaps** (below) |

- **Mixed mode (was mis-analysed as pure Thumb):** header 0x0-0x20 (data, `"S2"` +
  size@0x0c=0x61b4); **A32 entry stub 0x20-0x98** (`mrc/orr #0x1000/mcr` SCTLR I-cache,
  `isb`); **Thumb-2 body from 0x98**. Two code gaps the auto-analyzer missed, now
  disassembled: `0xf220005c` (A32, MPIDR core-id helper), `0xf22014b4` (Thumb wrapper).
- Undefined breakdown (`< 0xf2204892` = code region): header 32 B, function-pointer
  table `0xf220004c` (12 B → 0xf22081d8/91d8/a1d8), and ~40 × `00bf` Thumb-NOP
  alignment pads. `>= 0xf2204892` (data region, 3,025 B): `__func__`/error strings,
  the impedance/odt/freq lookup tables, `al_ddr_init_cfg` @0xf2205eb0, i2c-cfg
  @0xf2205df8, SPD/eeprom buffers @0xf220606c/609c. All data.
- **0 undiscovered code.** Verified reimplementable — see DDR ledger below.

## al_boot payload (ARM A32, 304,816 B) — 100% classified

| class | bytes | % | notes |
|---|---|---|---|
| A32 code | 168,476 | 55.3% | 377 functions, 0 decompile failures |
| defined data | 25,898 | 8.5% | strings/pointers Ghidra auto-typed |
| undefined | 110,442 | 36.2% | fully attributed below — **0 A32 code gaps** |

Undefined attribution with **reference (xref) class** — every region checked for an
instruction reference (literal-pool word, movw+movt pair, pointer table). "Referenced
by NONE" is the red-flag column; resolved below (`scripts/measure-preboot-coverage.py` +
segment map + pool/movw-movt scan):

| region | bytes | class | referenced-by | name / evidence |
|---|---|---|---|---|
| blob57 @0x010346a0 | 57,340 | **IDENTIFIED**: `al_serdes_25g_fw[]`, an 8051 program | `FUN_01021b60` @0x01021b60 | **RESOLVED** - see [blob57-identified.md](blob57-identified.md). sha256 matches `delroth-alpine_hal/.../al_hal_serdes_25g_fw.h` byte for byte, 0 mismatches. The earlier "Referenced-by: NONE (abs)" was a FALSE NEGATIVE: the scan used 0x0103469d (where the preceding .rodata string ends; the next 3 bytes are alignment pad), so the real `movw #0x46a0`/`movt #0x103` pair was missed. "0xff-record structure" was also wrong - no 0xff run of length >=4 exists. NOT boot-critical: single call site, group-E only, and every board DTB sets group E = `skip`, so the loader never runs on this hardware. |
| zero-pad / BSS | 30,219 | padding | NONE (expected) | 549 runs `00`; 20 KB block @0x010441eb = BSS/scratch; rest inter-fn alignment |
| AArch64 resume-agent @0x01029157 | 20,493 | code (AArch64) | embedded payload (computed ptr; no abs ref) | `stp/msr cpacr_el1/dsb`; 66 AArch64 functions (below); secondary-CPU bring-up, position-independent, copied to the A57s |
| data tables / literal pools | 2,138 | data | referenced (PC-rel pools inside fns) | const tables + `ldr [pc,#imm]` pools |
| string fragments | 249 | data | log/printf pool loads | `.rodata` strings not auto-typed; the referenced ones are already in *defined data* |

- **Referenced-by-NONE resolution:** only two spans — blob57 (embedded foreign-processor
  firmware, proven not-ARM-code) and zero/BSS padding. Neither is undiscovered ARM code
  and neither is a stale mis-carve (contrast the *old* pass, whose 130 KB of "junk" WAS
  a mis-carve: the TOC @0x80000 + `dt` DTB, referenced by nothing in al_boot because they
  are not part of al_boot at all — the correct carve excludes them).
- The defined-data region (25,898 B, incl. the ~26 KB `.rodata` string block) is
  referenced by construction — Ghidra typed it because code points at it.

- **Undiscovered A32 code found + closed (16 gaps):** reset/exception-vector code at
  0x01000000 (Ghidra sets no entry on a raw BinaryLoader import), CPU/cache primitives,
  and functions reached only via pointer tables (e.g. 0x01024e4c, a 396 B function).
  Seeded entry + disassembled the capstone-verified code gaps → **0 code gaps**.
- The `.rodata` string region (0x0102e164-0x0103469d, ~26 KB) is mostly in *defined
  data* (Ghidra typed the referenced strings), incl. the AArch64 agent's self-test
  strings and the al-boot build path.

### AArch64 resume-agent (embedded sub-payload, 20,492 B)

Carved 0x01029158-0x0102e164, analysed as AArch64. **66 functions**, code 15,396 B
(75.1%); remaining 5,048 B = its own literal pools + zero padding + jump tables (largest
is a 2,024 B zero run). Secondary-A57 CPU bring-up / resume agent — not on the DDR path;
not required for a mainline SMP/PSCI port. From al_boot's A32 view it is a data payload;
listed here as code because it decompiles cleanly as AArch64. Artifacts:
`docs/nor-reference/preboot-alboot-a64agent-decompiled.c` /
`-disassembly.asm`.

**Fact-check vs. an unsourced pasted claim ("AArch64 DDR margin-testing agent at
0xF2200000") — REFUTED, 2026-08-26:**
- Runtime placement is proven by al_boot's own caller: `preboot-alboot-decompiled.c:1702`
  — `FUN_010129d8(0x1000, &DAT_01029158, 0x5e18)`. `FUN_010129d8` is memcpy (same
  call shape used elsewhere for stack-buffer copies, e.g. line 3091/4765). Source
  `&DAT_01029158` = this agent's own start VA. Destination is **physical `0x1000`**,
  not `0xF2200000` and not PBS SRAM.
- `0xF2200000` (`s2_sram`, [hardware.md](hardware.md) preboot table) runs the S2
  `stage2_loader v2.22.3` — a **completely different blob**: A32 entry stub + **Thumb-2**
  body (not AArch64), 82 fns, the real DDR-PHY trainer (`al_ddr_init` etc, see below).
  It never contains this AArch64 agent.
- The a64-agent's own decompile has **zero refs to DDR/PHY registers** (no
  `0xf0080000`/`0xf0088000` memctl/PUB-PHY hits; grepped the full 3,161-line decompile).
  What it does touch: `DC_ISW`/`DC_CISW` (D-cache by set/way), `WaitForEvent`/
  `WaitForInterrupt`/`SendEvent`, MPIDR-based core-id (`FUN_01029248`), and two words read
  from PBS SRAM at **`DAT_fbff40f8`/`DAT_fbff40fc`** (a jump-target function pointer pair,
  called via `(*pcVar12)()` — `preboot-alboot-a64agent-decompiled.c:271-336`) — classic
  **PSCI `CPU_ON`/resume trampoline** shape, not DDR margin-testing. `0xfbff40f8` sits
  just below the HAL's own `SRAM_DEV_INFO_ADDRESS` (`0xfbff4100` for Alpine V2 —
  `delroth-alpine_hal/platform/alpine_v2/include/al_hal_iomap.h:358`).
- Verdict: the pasted claim conflates two real, separate mechanisms — S2 really does
  train DDR at `0xF2200000` (Thumb-2/A32, not AArch64); this AArch64 agent really is
  copied out of al_boot at runtime, but to physical `0x1000`, and it does CPU
  bring-up/resume, not DDR work. "Advanced Ethernet SRAM" and "DDR margin-testing agent"
  are not supported by any decompiled reference — omitted as unverified/likely wrong.

## Verdict — is it true 100%?

- **Classification: 100%.** Every byte of both primary blobs is disassembled code,
  defined data, or attributed undefined-data with a reason. **Zero undiscovered ARM
  code** in S2 or al_boot.
- **What resists full *decode*:** `blob57` (57,343 B = 18.8% of al_boot). Proven
  not-ARM-code and unreferenced; a firmware image for another processor (candidate:
  SerDes PHY microcode), so it is classified as data but its internal opcodes are not
  decoded here. Nothing else resists.

## S2 DDR-init — reimplementable, named call graph

- Ledger: `docs/nor-reference/preboot-s2-names.sym` (+ `.md` with confidence/rationale),
  `scripts/build-s2-name-ledger.py`. Applied to the primary artifact
  `docs/nor-reference/preboot-s2-disassembly.asm` (named headers, `bl` targets, inline
  resolved `__func__`/error strings) by `scripts/annotate-preboot.py`.
- **DDR-init subgraph (from `al_ddr_init` + orchestrator): 56 functions, 100% named —
  31 confirmed (string/curated), 25 speculative (`s_` prefix), 0 bare.**
- Confirmed spine: `ddr_bringup_orchestrator`(0xf22003d8) → `al_ddr_cfg_init` →
  `ddr_freq_change_according_to_spd` → `ddr_init_spd_get` → `al_ddr_spd_parse` →
  `al_ddr4_spd_parse`; `ddr_build_addrmap`; `set_dram_impedance_ctrl_from_eeprom`;
  `dram_voltage_gpio`; then `al_ddr_init`(0xf2201a90, 8,392 B) calling
  `al_ddr_phy_wait_for_init_done`, `al_ddr_phy_vt_calc_disable`,
  `al_ddr_mode_register_set`, `al_ddr_ctrl_wait_for_normal_operating_mode`,
  `al_pll_init/freq_set/channel_div_set`, `al_i2c_perform_write`, `s_udelay`,
  `s_al_err_printf`. Field-level SPD→cfg mapping already in `ddr-s2-parser-analysis.md`.
- **DDR register names:** flat `.sym` labels do NOT attach — the HAL pokes uMCTL2
  (0xf0080000) / PUB PHY (0xf0088000) through a handle pointer (`str rX,[r7,#off]`),
  not absolute addresses, so there is no `DAT_0xf008…` to rename. Register meaning is
  captured field-by-field in `ddr-s2-parser-analysis.md` §STRUCTS instead. Symbols
  generated (ddrc/ddrphy/nbpll, cached in tmp/ghidra-in) for any blob that uses
  absolute access.

## Reproduce

```
scripts/analyse-unvr-firmware.py sources/UNVR-5.1.25.bin --extract   # -> tmp/sections/01-uboot.bin
# carve al_boot: 01-uboot.bin[0x21004:0x6b6b4] -> tmp/alboot-payload.bin (304,816 B)
scripts/ghidra-analyse.py tmp/alboot-payload.bin --name alboot --arch arm32 --base 0x1000000 \
    --entry 0x1000000 --disasm-gaps tmp/alboot-gaps.txt --preboot --sym-dir tmp/ghidra-in
scripts/ghidra-analyse.py docs/nor-reference/s2-loader-stage2_v2.22.3-25044B.bin --name s2 \
    --arch arm32 --base 0xf2200000 --entry 0xf2200020 --disasm-gaps tmp/s2-gaps.txt \
    --preboot --sym-dir tmp/ghidra-in
scripts/measure-preboot-coverage.py <out>/coverage.txt <blob> <base>   # gap classify
scripts/build-s2-name-ledger.py                                         # names
scripts/annotate-preboot.py <out> <blob> <base> --names <sym>           # named .asm/.c
```
