# UNVR proprietary preboot — function-level decompile

Scope: the closed-source parts of the boot chain (no GPL source exists). U-Boot
proper is GPL (UNVR-1.3.35-GPL) and out of scope. Decompiler: Ghidra 12.2 headless.

Builds on: [nor-boot-chain.md](nor-boot-chain.md), [bootloader.md](bootloader.md),
[uboot-update-path.md](uboot-update-path.md). Cross-check
[nor-reference/preboot-dt-selection.asm](nor-reference/preboot-dt-selection.asm).

## Decompiler fix (was the blocker)

- Ghidra 12.2 requires **JDK 21** (`application.java.min=21`). System default JDK
  is **25** → decompiler/analysis hangs.
- Only real alternate JDK present was 17 (rejected by 12.2); the 21-openjdk dir was
  an empty stub.
- **Fix:** installed Temurin JDK **21** and set
  `JAVA_HOME_OVERRIDE=/usr/lib/jvm/java-21-temurin-jdk` in
  `<ghidra>/support/launch.properties` (both installs on this box).
- **Verified working:** headless import + auto-analysis + decompile-all of the
  435,892-B al_boot payload → **317 functions, 0 decompile failures**, ~20 s
  wall. S2 blob → 78 functions. Real data-flow C (coprocessor cache ops, recovered
  `switch` on sysid, RSA loops). JDK 25 previously hung indefinitely on the same input.
- Runner: `scripts/ghidra-decompile.py` (wraps analyzeHeadless + `ExportAll.java`,
  reused from awto-2000). Log `tmp/logs/ghidra-decompile.log`.

## Targets, arch, load — confirmed from container

TOC (`scripts/parse-al-toc.py` on `01-uboot.bin`, magic `0x070c070c` @0x80000):

| Obj | type | container off | notes |
|---|---|---|---|
| preboot | 0x0d | 0x00000 (size 0x80000) | S2 + al_boot payload (below) |
| dt / dt_pro / dt_ai / dt_bt / dt_hd | 0x02 | 0x81000..0xac000 | 5 DTBs, multi-DT index = type high nibble (0..4) |
| uboot | 0x05 | 0xac000, load/entry 0x1100000 | GPL U-Boot, out of scope |

Two proprietary code blobs inside `preboot`:

- **S2 first-stage** — container 0x00000, 25,044 B, magic `"S2"`, **stringless**
  boot-ROM SPI loader. Thumb-2, link base **0xF2200000** (SRAM). Carved:
  `docs/nor-reference/s2-loader-stage2_v2.22.3-25044B.bin` (filename is a misnomer —
  this is the S2 SPI loader, NOT the `stage2_loader v2.22.3` banner code).
- **al_boot payload** — img-hdr (magic `0x000b9ec7`) @container 0x20000, payload
  @container **0x21000**, size **0x6a6b4 (435,892 B)**, **ARM A32, load 0x01000000**.
  Contains BOTH banners: Annapurna al-boot v2.10.0 (`stage2_loader v2.22.3`,
  `agent_wakeup v2.10`) AND contractor `Stage 3 v2.22.0` (multi_dt). Re-carve:
  `container[0x21000:0x21000+0x6a6b4]` → verified byte-exact at VA 0x0100167c.

Confirmation method: img-hdr payload-size @+0x28, load/entry @+0x30/+0x38; dispatch
opcode at file-off 0x167c == `e3097308` (`movw r7,#0x9308`), matching
preboot-dt-selection.asm at VA 0x0100167c.

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
   - `FUN_0100110c()` — stage3 board init + multi_dt (below)
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

Location: inside `FUN_0100110c` (**stg3 board init**; references `stg3_early_init`,
`dt_based_init_pcie`). Decompiled read + switch:

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

### `FUN_01002f3c` — RSA capability signature authentication

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

## S2 first-stage (Thumb-2 @0xF2200000) — for completeness

- 78 functions, zero strings. Boot-ROM-loaded SPI loader; touches only SPI/UART/
  PBS/I2C/GPIO. Loads the al_boot payload (container 0x21000) to 0x01000000 and
  jumps. Entry region `0xf2200098`; a jumptable at 0xf22000fc Ghidra could not
  fully recover (dispatch by loaded function pointers). Not on the critical path
  for a mainline port (BootROM+S2 stay in mask ROM / factory NOR).

## Open / not chased

- Exact AL-324 DDR PHY timings (owned by the CVOS agent, not in these blobs).
- `FUN_01012b08` used as both memcmp and RSA-verify entry — same primitive; not
  split into named sub-ops here.
- S2 0xf22000fc jumptable (pointer-dispatch) not reconstructed.
