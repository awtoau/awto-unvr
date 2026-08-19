# Ghidra RE methodology — UNVR / AL-324 (Alpine V2)

Mechanical, repeatable, token-cheap procedure to decompile the UNVR boot chain and
drivers consistently. Target SoC: Annapurna Labs **Alpine V2 / AL-324** (Cortex-A57
cluster, ARMv8-A). 64-bit code (U-Boot proper, kernel) and a 32-bit-ARM proprietary
preboot (S2 + al_boot). **Do not touch the device** to follow this doc — it is
static RE of carved blobs only.

Companion docs (owned elsewhere, do not edit here): [nor-boot-chain.md](nor-boot-chain.md),
[preboot-decompile.md](preboot-decompile.md), [bootloader.md](bootloader.md).

## Reused from awto-2000 (known-working Ghidra setup on this box)
- **JDK/launch fix**: `JAVA_HOME_OVERRIDE` in `<ghidra>/support/launch.properties`
  (confirmed present, value below).
- **`ExportAll.java`** exporter — trimmed to a project-neutral copy at
  `scripts/ghidra/ExportAll.java` (awto-2000's carried a per-project TSV header).
- **`SetupMemory.java`** memory-block + label pattern — reworked into
  `scripts/ghidra/SetupAlpineMemory.java` (MMIO map baked in) and the label logic
  in `ApplyAlRegs.java` (`shouldLabel`: replace null/`DAT_`/DEFAULT only).
- **Pipeline shape** (preScript setup → analyse → postScript apply/export, one
  runner) — mirrored in `scripts/ghidra-analyse.py`.
- awto-2000 uses an SVD (`SetupMemory.java` parses `STM32F103.svd`); **no SVD exists
  for the AL-324** (§4) so register names come from the AL HAL/driver headers instead.

---

## 1. Setup (once)

- **Ghidra**: `/home/dan/tools/ghidra_12.2_DEV` (v12.2, `application.java.min=21`).
- **JDK — the blocker**: 12.2 needs **JDK 21**. JDK 25 (system default) **hangs the
  decompiler indefinitely**; JDK 17 is **rejected** by 12.2 (min=21). Both present on
  this box: `/usr/lib/jvm/java-17-temurin-jdk`, `/usr/lib/jvm/java-21-temurin-jdk`.
  Use **21**.
- **Fix (already applied out-of-tree)** in `<ghidra>/support/launch.properties`:
  `JAVA_HOME_OVERRIDE=/usr/lib/jvm/java-21-temurin-jdk`. Verify with
  `grep JAVA_HOME_OVERRIDE /home/dan/tools/ghidra_12.2_DEV/support/launch.properties`.
- **Headless vs GUI**: default to **headless** (`support/analyzeHeadless`) — driven by
  `scripts/ghidra-analyse.py`; reproducible, no per-session state, cache-friendly.
  Open the GUI only to eyeball a hard function; do NOT hand-edit exported C.
- **Project dir**: `tmp/ghidra-proj/` (gitignored, throwaway). Exports →
  `tmp/ghidra-out/`. Curated decompiled artifacts that are evidence live under
  `docs/nor-reference/` (checked in). Generated C is READ-ONLY output — never edit it.

---

## 2. Loading a raw binary

Never guess base/entry — derive from the Annapurna container TOC.

- **TOC parse**: `scripts/parse-al-toc.py <container>.bin` (TOC magic `0x070c070c`
  @0x80000). Prints each object's name/type/offset/size and, when the per-image
  header (magic `0x000b9ec7`) is present, its **payload size @+0x28, load @+0x30,
  entry @+0x38**. U-Boot payload = image-header **+0x48** (prior RE).
- **Processor/variant** (Ghidra language ID):
  - 64-bit U-Boot proper / kernel → **`AARCH64:LE:64:v8A`**
  - 32-bit preboot al_boot payload → **`ARM:LE:32:v8`** (A32)
  - S2 first-stage → **`ARM:LE:32:v8` in THUMB** (Thumb-2; set TMode=1 / disassemble
    as Thumb at entry). Link base **0xF2200000** (SRAM), entry region `0xf22000098`.
- **Confirmed load/entry (from container, preboot-decompile.md — for cross-check, not
  to re-derive)**:
  | blob | arch | base/load | source |
  |---|---|---|---|
  | al_boot payload | ARM A32 | **0x01000000** | img-hdr @container 0x20000, payload @0x21000, size 0x6a6b4 |
  | S2 first-stage | Thumb-2 | **0xF2200000** | carved `docs/nor-reference/s2-loader-stage2_v2.22.3-25044B.bin` |
  | U-Boot proper | AArch64 | **0x1100000** | TOC `uboot` obj load/entry |
  | kernel (uImage) | AArch64 | per FIT/uImage header | (unsigned; see nor-boot-chain.md) |
- Runner: `scripts/ghidra-analyse.py <blob> --name N --arch {aarch64|arm32} --base 0x…`
  Loader = `BinaryLoader`, `-loader-baseAddr`.

---

## 3. Memory map — create blocks BEFORE analysis

Why first: auto-analysis follows pointers; if a load/store targets unmapped space it
either drops the reference or fabricates junk code. Map RAM + **every** MMIO region,
mark MMIO **volatile + non-executable**, fill gaps, THEN analyse.

- **Mechanical**: `scripts/ghidra-analyse.py … ` runs
  `scripts/ghidra/SetupAlpineMemory.java` as a **preScript**. Map table is baked into
  that script; **keep it in sync with the authoritative map in
  [hardware.md](hardware.md#mmio-and-address-map)**. RAM → R/W/X, non-volatile; MMIO
  → R/W, **volatile, non-exec**. Idempotent (skips blocks that overlap the loaded
  image). `--preboot` adds the SoC-fabric regions (s2_sram, agent mailboxes,
  ddr_ready — see the preboot-only table in hardware.md).
- Source of truth for the map: **[hardware.md](hardware.md#mmio-and-address-map)**,
  verified from `docs/hw-reference/20260816-104601/live.dts` (`reg=`/`compatible=`) +
  `iomem.txt` (Linux `/proc/iomem`). Do not re-table it here.

### AL-324 address map

**MMIO / address map: see [hardware.md](hardware.md#mmio-and-address-map)** — the one
authoritative table (DRAM, PBS peripherals, fabric, PCIe/ECAM, IO-fabric EPs,
preboot-only regions). RE-relevant notes only below:

- `otp_efuse` @`0xfd896000` is **inferred** (not in DT/iomem); size 0x1000 chosen to
  cover the preboot eFuse modulus-hash at `0xfd89608c`. Keep it non-volatile-read; OTP.
- **Standard-IP gotchas** (matter when applying register structs): UART is `ns16550a`
  (DesignWare 8250), **NOT PL011** — `al_hal_uart_regs.h` is 8250-style; use the 8250
  layout. GPIO is genuine `arm,pl061` — `al_hal_gpio_regs.h` matches PL061
  (`gpiodata[0x100]`@0x0, `gpiodir`@0x400); the AL HAL wraps the PL061 block.
- Where iomem and DT disagree on presence (uart3, wdt1..3, timers — DT-disabled),
  trust **DT** for block layout; iomem lists only what a live driver claimed.

---

## 4. Register naming — mechanical

**No CMSIS-SVD ships for the AL-324** (SVDs exist mainly for MCU-class ARM parts;
app-class SoCs like Alpine V2 do not publish one). The machine-readable register
description is the **Annapurna HAL / driver headers** — `struct al_<periph>_regs {}`
for the block layout, and `#define AL_<periph>_<reg>_<field>_SHIFT/_MASK` for
bitfields. Partial generic SVDs (DesignWare APB / ARM PrimeCell) exist upstream but
are not needed here — the HAL structs cover the same blocks with the AL naming the
firmware actually uses.

### Register sources (which header names which block)
- **HAL (gold)**: `/mnt/2tb/unvr-port-refs/delroth-alpine_hal/` — 58 `*_regs.h`.
  Per-peripheral headers listed in the §3 table. Cross-fabric: `include/sys_fabric/`
  (ccu, nb, anpa), `include/io_fabric/al_hal_unit_adapter_regs.h`, `include/udma/`.
- **U-Boot / kernel (UBNT)**: `/mnt/2tb/unvr-port-refs/UBNT-source-code/…` (al_* +
  u-boot) and `urnvr-kernel-4.19.152/drivers/net/ethernet/al/` (+`internal/`) — same
  blocks, sometimes newer field defines; use to **cross-check** the HAL.
- **Mainline** (`dw-i2c`, `pcie-designware`, `ahci`, `8250_dw`, `pl061`) — for the
  standard IP blocks when a field is undocumented in the AL header.
- **Merge rule**: if two sources define the same block, prefer the HAL struct for
  layout; if a field offset/width disagrees, **log the discrepancy in a comment** and
  trust the source whose struct's `/* 0xNN */` anchors are self-consistent. Never
  silently pick one.

### Procedure
1. **Generate** per-peripheral labels from a HAL struct + its base address:
   ```
   scripts/gen-al-reg-symbols.py <HAL_regs.h> --struct al_i2c_regs \
       --base 0xfd880000 --prefix i2c0 --out tmp/ghidra-in
   ```
   Emits `tmp/ghidra-in/i2c0.sym` (`<label>\t0x<abs_addr>`, one per register) and
   `i2c0.h` (a Ghidra-parseable struct with `/* 0xoff */` anchors). Field offsets are
   computed from types + array counts and **cross-checked against the header's own
   `/* 0xNN */` anchors** (a mismatch is logged — catches a mis-sized field).
   Repeat per instance (i2c0/i2c1, gpio0..5, uart0..3) with each base from §3.
2. **Bitfield equates** (optional): `--defines --prefix AL_I2C` emits
   `AL_I2C.equ.tsv` of the *directly-numeric* `#define`s (SHIFT amounts, plain
   masks). Defines built from other macros (`(0x1 << AL_..._SHIFT)`) are **skipped** —
   they need C-preprocessor evaluation; not worth it, the SHIFTs alone read fine.
3. **Apply** in Ghidra (postScript, after analysis): `scripts/ghidra/ApplyAlRegs.java`
   takes any number of `*.sym` (→ labels) and `*.equ.tsv` (→ equates) args. Labels
   only overwrite null/`DAT_`/DEFAULT symbols (never a curated name). Equates are
   **heuristic** (attached to every scalar operand == value, guarded `value >= 8` to
   avoid painting 0/1/2 everywhere) — a readability aid, verify at call sites.
   `ghidra-analyse.py --sym-dir tmp/ghidra-in` globs and applies all of them.

### Struct-overlay alternative (best readability, §6)
Instead of flat labels, parse `tmp/ghidra-in/<prefix>.h` via Ghidra
"File → Parse C Source" (or `DataTypeManager`), then apply the struct type at the
peripheral base. Decompiled C then reads `i2c0->status` not `*(int*)0xfd880070`. Use
for the peripheral you are actively reversing; labels are the cheap default for all.

---

## 5. Code vs data discipline

- **Decompile-first**: run auto-analysis, read the decompiled C, and let Ghidra find
  code. **Do not pre-declare data** across the image — a data definition placed over
  real code permanently blocks disassembly there.
- **MMIO already handled**: §3 marks peripheral blocks volatile/non-exec, so the
  analyzer won't try to execute a register window or invent a function in it.
- **Spotting mis-typed regions**:
  - a "function" that is one `undefined` blob or bytes-as-string → likely data in a
    code block; a run of `??`/`DAT_` amid real instructions → likely code marked data.
  - Fix: `Clear Code Bytes` then `Disassemble` (D) for code; `Clear` then define the
    right type for data. In headless, prefer re-running with a corrected memory map
    over scripting one-off type fixes.
- **Literal pools (ARM)**: `ldr rX,[pc,#imm]` pools are data *inside* code; Ghidra
  usually gets these, but for a flat blob use `scripts/arm-xref.py <bin> <VA…>` to
  resolve pool words / `movw`+`movt` pairs to targets before trusting a load.
- **Genuinely overlaid data** (a jump table in the middle of a function): let Ghidra
  keep it as data within the function body; don't split the function. If two meanings
  truly share bytes, use a Ghidra **overlay block**.
- **Jump tables / switch on ARM**: the AL preboot uses `ldrls pc,[pc,r3,lsl#2]`
  (bounded jump table). Ghidra recovers most as `switchD_*`. If a switch shows as an
  unresolved indirect branch: select the branch, "Create Jump Table" / mark the table
  extent; confirm against the disasm (e.g. the sysid table at VA `0x01001720`, 27
  slots, idx = sysid−0xea16 — preboot-decompile.md).

---

## 6. Structs for readability

Define structs from the decompile + GPL headers and apply them so C reads cleanly.

- **Per-peripheral register blocks**: from the generated `<prefix>.h` (§4). Apply at
  the block base.
- **Board / identity structs** (from GPL source + preboot RE — see
  `identity-partitions.md`, `docs/nor-reference/`):
  - `board_info` / `ubnt_hal` board-table entry — from the U-Boot board table
    (`scripts/dump-uboot-board-table.py`).
  - EEPROM / identity layout — `eeprom-fieldmap.json`; sysid at NOR `0x1F000C` (2 B
    BE), hw-rev at `0x1F0010` (4 B BE).
  - `multi_dt` selection table — sysid→instance→DTB (preboot-decompile.md table).
  - RSA capabilities blob (0x100 B) + modulus (0x100 B) — preboot EEPROM parser.
- **Naming conventions**:
  - registers: `<periph><inst>_<reg>` (e.g. `i2c0_status`) — matches gen script.
  - struct types: `<periph>_regs`; instances typed, not labelled, when overlaid.
  - functions: keep Ghidra `FUN_<va>` until identified; then a semantic name; note
    banner/`__func__`-derived names in **bold** in the companion docs.
  - globals: `g_<name>`; MMIO pokes get the register label, not a `DAT_`.
- **Do not invent purpose for noise** — uninitialised-register "params" and dead reg
  spills are artifacts (awto-2000 README §recurring-bugs). Name them as such.

---

### Iterative naming loop (off a persistent ledger)

Don't name one-shot — converge the call graph by looping off a ledger file.

1. **Ledger** = `<blob>-names.sym` (`name\t0xADDR`, applied by `ApplyAlRegs.java`
   like the register `.sym`) + a `<blob>-names.md` (addr, name, confidence, 1-line
   rationale). This is the document every pass works off and grows.
2. **Seed (confirmed)**: map each `__func__`/error string → the `FUN_` that
   references it (via its `DAT_` pointer). Ground-truth names.
3. **Speculate (context)**: for each bare `FUN_`, infer from caller (helper of
   `al_ddr_init` → `ddr_*`), registers poked (uMCTL2 → `ddr_ctrl_*`, PUB PHY →
   `ddr_phy_*`), strings touched. Prefix speculative `s_`.
4. **Re-apply + iterate**: re-run with `--sym-dir` including the names ledger;
   each newly-named fn gives context to name its neighbours. Loop until the target
   subgraph has no bare `FUN_`. The ledger is the durable, reviewable output.

## 7. Mechanical / token-cheap workflow

One command per blob; cache and reuse register symbols across blobs.

```
# 0. (once) generate register symbols for the peripherals you care about
scripts/gen-al-reg-symbols.py .../al_hal_i2c_regs.h  --struct al_i2c_regs  --base 0xfd880000 --prefix i2c0
scripts/gen-al-reg-symbols.py .../al_hal_uart_regs.h --struct al_uart_regs --base 0xfd883000 --prefix uart0
scripts/gen-al-reg-symbols.py .../al_hal_gpio_regs.h --struct al_gpio_regs --base 0xfd887000 --prefix gpio0
#    (repeat per instance; all land in tmp/ghidra-in/)

# 1. confirm load/entry from the container (never guess)
scripts/parse-al-toc.py sources/…container.bin

# 2. analyse + label + export in one headless run
scripts/ghidra-analyse.py alboot.bin --name alboot --arch arm32 --base 0x1000000 \
    --preboot --sym-dir tmp/ghidra-in
#   -> tmp/ghidra-out/decompiled.c , disassembly.asm ; log tmp/logs/ghidra-analyse.log
```

Pipeline inside `ghidra-analyse.py` (analyzeHeadless):
`preScript SetupAlpineMemory.java [preboot]` → auto-analysis →
`postScript ApplyAlRegs.java <syms>` → `postScript ExportAll.java <out>` →
`-deleteProject`.

- **Cache/reuse across binaries**: `tmp/ghidra-in/*.sym` and `*.equ.tsv` are
  target-independent (address-based) — generate once, apply to every blob that maps
  the same peripherals. The MMIO map in `SetupAlpineMemory.java` is likewise shared.
- **Per-blob only**: base/entry (from TOC), arch, and `--preboot`.
- **Timeout**: `ghidra-analyse.py --timeout` default 1200 s = 1.25× the observed
  worst case (435 KB ARM32 → ~20 s per preboot-decompile.md; larger 64-bit images
  budgeted higher). On expiry Ghidra is killed and the operation+elapsed logged.
- Simpler existing runner (preboot agent's): `scripts/ghidra-decompile.py` (import +
  ExportAll only, no memory/reg step) — use `ghidra-analyse.py` for the full pipeline.

## Scripts added by this doc
- `scripts/gen-al-reg-symbols.py` — HAL `struct al_*_regs` → Ghidra `.sym` labels +
  `.h` struct + numeric-`#define` equates. **Tested**: i2c/uart/gpio offsets match
  header anchors exactly (con@0x00, status@0x70, comp_param_1@0xF4; uart ctr@0xFC;
  gpio gpiodir@0x400).
- `scripts/ghidra-analyse.py` — full one-command headless pipeline.
- `scripts/ghidra/SetupAlpineMemory.java` — AL-324 RAM+MMIO blocks (preScript).
- `scripts/ghidra/ApplyAlRegs.java` — apply `.sym` labels + `.equ.tsv` equates (postScript).
- `scripts/ghidra/ExportAll.java` — decompile-all exporter (postScript; from awto-2000).

## Uncertain / low-confidence
- The `.java` scripts are adapted from awto-2000's proven `SetupMemory.java` /
  `ExportAll.java` and use the same Ghidra APIs, but were **not re-run headless in
  this session** (no analysis was launched). The **Python** parser/pipeline IS tested
  (offsets verified above; both scripts `py_compile` clean).
- Equate application is heuristic (§4) — scalars are matched by value, not by
  data-flow; over- and under-matches are possible. Verify at call sites.
- `otp_efuse`/eFuse block size (0x1000) is inferred to cover `0xfd89608c`; not an
  explicit iomem entry.
- The `DRAM0` covering block (0..3 GiB) **auto-skips** whenever the loaded image sits
  inside it (always, since images load at `0x1xxxxxx`) — the image block covers the
  working set; stray DRAM refs land in MMIO/fabric blocks or can be mapped ad-hoc.
  `DRAM1` (above 4 GiB) always maps.
- PCIe/ECAM window sizes taken from DT `reg=`; the `pcie_ext0_mem` BAR window size is
  the iomem span and may over-cover unused BAR gaps.
