# AL-324 overclock, eFuse caps, errata, recovery — static RE

Scope: how CPU + DRAM frequency are set on the Annapurna Alpine V2 / AL-324 (quad
Cortex-A57 r1p3, MIDR `0x411fd073`), the register levers to raise them, the eFuse
capability gate, A57 errata bits, and boot/debug backdoors. Recipe only — nothing
applied. This unit: sysid 0xea16 (UNVR4), 4 GB DDR4.

Evidence tags: **[P]** proven from code/live capture · **[I]** inferred (consistent
but not directly observed on this unit). Device serial NOT touched (in use) — no live
PLL register read; current freqs come from DT fixups + banner, not a register dump.

Sources: delroth `alpine_hal` (`al_hal_pll*`, `al_hal_bootstrap*`, `al_bootstrap.c`,
`al_hal_iomap.h` V2, `al_hal_otp.h`, `ddr/`), GPL U-Boot `board/annapurna-labs/
alpine_ubnt/board.c`, live `hw-reference/20260816-104601/`, `preboot-alboot-decompiled.c`.

## Headline

- **CPU is clocked well below its ceiling. [P]** Cores run **1.7 GHz**; the SoC's own
  strap table allows CPU PLL up to **2.7 GHz** on Alpine V2. ~1 GHz of headroom in the
  vendor's own bin table, before even challenging the marking.
- **DRAM is below ceiling too. [P]** DDR4-**1866** (NB PLL 933.33 MHz); strap table
  allows NB PLL up to **1300 MHz = DDR4-2600**; DDR controller enum goes to DDR4-3200.
- **Overclock levers (found, register-exact):**
  - CPU — rewrite **CPU PLL `setup_0` @ `0xfd860d40`** (V2 format) to a higher
    feedback scalar, set RELOCK, poll lock. Runtime-capable (HAL `al_pll_freq_set`
    path; preboot already reprograms PLLs live). **[P]**
  - DRAM — raise **NB PLL `setup_0` @ `0xfd860c40`**, but only viable **at boot before
    DDR training** (needs full PHY retrain by the closed CVOS agent); cleanest via the
    **bootstrap strap** NB_PLL_FREQ field or by patching the agent's `ddr_freq`. **[P/I]**
- **eFuse caps gate nothing overclock-relevant, and are not unlockable. [P]** The
  RSA-2048/eFuse "capabilities" blob gates optional SoC feature flags, is signed
  against a SHA-256 modulus hash fused at `0xfd89608c`, and its failure path is
  non-fatal. CPU/DDR clocks are set by strap+PLL, entirely outside this gate. Forging
  an expanded caps blob needs Annapurna's private key → not unlockable.

## Clock architecture (confirmed)

Three independent PLLs, base = `AL_SB_RING_BASE(0xfd860000) + 0xb00 + id*0x100`: **[P]**

| PLL | id | base | drives |
|---|---|---|---|
| SB (south bridge) | 0 | `0xfd860b00` | I/O fabric, slow I/O, crypto boost ch |
| **NB (north bridge)** | 1 | `0xfd860c00` | **DRAM + system fabric** (coupled) |
| **CPU** | 2 | `0xfd860d00` | A57 cores, caches, local GIC |

- Ref clock = **100 MHz** (DTS `refclk`; strap PLL_REF_CLK_FREQ bit19=1). **[P]**
- Alpine V2 PLLs are **REV2** → use the V2 register/format (`al_pll_freq_set_v2`). **[P]**
- V2 output freq formula (`al_pll_freq_get_v2`): **[P]**
  `f = (pre × post × ref) / (refdiv × outdiv)`, ref=100 MHz.
- Cores/DRAM are fed from **PLL channel 0**; on this part channel 0 = ÷1
  (cpu_pll_freq == PLL output). **[P]**

### V2 `setup_0` register (per-PLL, offset +0x40 from base)

| bits | field | meaning |
|---|---|---|
| [7:0] | FB_DIV255 | feedback **post** scalar |
| [8] | FB_PRE23 | pre scalar: set→3, clear→2 |
| [21:16] | OUT_DIV | output divider |
| [29:24] | REF_IN_DIV | reference divider |
| [31] | RELOCK | write 0 then 1 to latch + relock |

Other per-PLL regs: `setup_6` @ +0x58 (bit31 BYPASS, bit12 ENABLE, bit28 POWER_DOWN),
`status_7` @ +0x1c (bit? PLL_LOCK + FSM state + REVISION_ID), `ch_pair[0].cfg` @ +0x20
(channel 0/1 dividers: [10:0] div value, bit12 relock-req, bit15 ref-clk-bypass). **[P]**

## 1. CPU overclock

**Current:** 1.7 GHz [P] (`hardware.md`: U-Boot `Device ID = a324`, "Quad A57 @ 1.7GHz").
BogoMIPS 116.66 is timer-derived (arch_timer 58.33 MHz), NOT the core clock — ignore it.

**Ceiling (SoC strap table, `al_bootstrap_cpu_pll_freq_get`, dev ≤ V2): [P]**

| strap [3:0] | CPU freq | | strap | CPU freq |
|---|---|---|---|---|
| 0x2 | 1.4 GHz | | 0x9 | 2.2 GHz |
| 0x3 | 1.5 GHz | | 0xA | 2.3 GHz |
| 0x4 | 1.6 GHz | | 0xB | 2.4 GHz |
| **0x5** | **1.7 GHz ← now** | | 0xC | 2.5 GHz |
| 0x6 | 1.8 GHz | | 0xD | 2.6 GHz |
| 0x7 | 1.9 GHz | | **0xE** | **2.7 GHz ← table max** |
| 0x8 | 2.1 GHz | | 0xF | 2.0 GHz |

Note: 2.7 GHz is the vendor bin table max; A57 r1p3 on 28 nm typically rates ~2.0–2.5 GHz.
Treat 1.8–2.0 GHz as the low-risk target, above that per the "challenge the marking" test.

### CPU PLL register table (CPU PLL @ `0xfd860d00`)

| reg | addr | field | now (inferred) | change to | how |
|---|---|---|---|---|---|
| setup_0 | `0xfd860d40` | post/pre/out/ref | `0x01030111` {ref1,pre3,post17,out3}=1700 | see targets | write new, then set bit31 |
| status_7 | `0xfd860d1c` | PLL_LOCK | — | poll=1 after relock | read |
| setup_6 | `0xfd860d58` | BYPASS b31 | 0 | set→bypass while switching | optional safety |

**setup_0 target values (ref=100 MHz, verified against `al_pll_freq_map_v2_100`): [P]**

| target | {ref,pre,post,out} | setup_0 (RELOCK clear) | +RELOCK |
|---|---|---|---|
| 1800 | {1,3,18,3} | `0x01030112` | `0x81030112` |
| 1900 | {1,3,19,3} | `0x01030113` | `0x81030113` |
| 2000 | {1,3,20,3} | `0x01030114` | `0x81030114` |
| 2100 | {1,2,21,2} | `0x01020015` | `0x81020015` |
| 2200 | {1,2,22,2} | `0x01020016` | `0x81020016` |
| 2300 | {1,2,23,2} | `0x01020017` | `0x81020017` |
| 2400 | {1,2,24,2} | `0x01020018` | `0x81020018` |

### Procedure (mirrors `al_pll_freq_set_v2`) [P]

1. `setup_6 &= ~BYPASS` (bit31) — ensure not bypassed.
2. write `setup_0` = target (RELOCK clear).
3. write `setup_0` |= `RELOCK` (bit31).
4. poll `status_7` PLL_LOCK until set (HAL timeout budget µs-scale; derive from relock
   spec, not a round number).

### WHERE to apply — three options

- **Runtime (kernel/U-Boot), simplest [P]:** three MMIO writes above. **Caveat:** all 4
  cores run *from* this PLL; the VCO relock stalls/glitches the core clock. HAL claims
  the freq-set path is glitch-free, but for a VCO (not just channel-div) change do the
  safe dance: `setup_6 |= BYPASS` (cores drop to 100 MHz ref) → change `setup_0` →
  clear BYPASS + RELOCK → poll lock. Channel-div-only change (`ch_pair[0].cfg`) is the
  documented glitch-free path but can't raise freq here (ch0 already ÷1). **[P]**
- **Preboot / boot ROM via strap (persistent) [I]:** the boot ROM programs the CPU PLL
  from bootstrap bits [3:0] at reset. `boot_strap` @ `0xfd8a8110` is a **read-only
  reflection** of the physical straps — writing it post-boot does nothing. Persisting an
  OC through the strap means either physical strap-pin rework or an OTP strap-override
  fuse (irreversible; exact OTP word not mapped here — do not blow blind).
- **DDR agent / al_boot patch (persistent, software) [I]:** inject the runtime writes
  above into the closed al_boot/CVOS bring-up so cores come up fast from reset. Highest
  effort, cleanest result.

**Proof the PLL-reprogram path is real, not theoretical:** shipping preboot
`FUN_01003f7c` = `pll_fixup_crypto_boost` reprograms an **SB PLL** channel to 600 MHz
(`0xfd860b00`, "PLL freq not suitable for 600MHz"), and calls `al_pll_channel_div_set`
in several places. Same HAL, same registers. **[P]**

## 2. DRAM overclock

**Current:** DDR4-**1866**, NB PLL = **933.33 MHz** {ref1,pre2,post28,out6}. **[P]**
Confirmed two ways: DT `nbclk`=`0x37a18808` (933.33 MHz), and U-Boot writes
`arm,armv8-timer clock-frequency = ddr_pll_freq/(1+scale)` = 58.33 MHz with scale=15 ⇒
ddr_pll_freq=933.33 MHz (`board.c` `ft_board_setup`). DDR data rate = 2× clock.

**Ceiling (strap NB_PLL_FREQ [6:4], `al_bootstrap_ddr_pll_freq_get`, ≥V2): [P]**

| strap [6:4] | NB PLL | DDR4 rate |
|---|---|---|
| 0x1 | 1066.67 MHz | DDR4-2133 |
| 0x2 | 666.67 MHz | DDR4-1333 |
| 0x3 | **1300 MHz** | **DDR4-2600 (strap max)** |
| **0x4** | **933.33 MHz ← now** | **DDR4-1866** |
| 0x6 | 1200 MHz | DDR4-2400 |
| 0x7 | 800 MHz | DDR4-1600 |

DDR controller enum (`enum al_ddr_freq`) supports up to **DDR4-3200**, but NB PLL also
clocks the **system fabric** — raising it raises fabric clock in lockstep (a real
constraint, not just a DRAM-margin question).

### NB PLL register table (NB PLL @ `0xfd860c00`)

| target | {ref,pre,post,out} | setup_0 @ `0xfd860c40` |
|---|---|---|
| 1866 (now) | {1,2,28,6} | `0x0106001c` |
| 2133 | {1,2,26,5} | `0x0105001a` |
| 2400 | {1,2,24,4} | `0x01040018` |
| 2600 | {1,2,26,4} | `0x0104001a` |

### WHERE — boot only, not runtime

- **DRAM cannot be re-clocked live.** DDR is under active refresh/traffic; changing NB
  PLL requires a full PHY **retrain** (write-leveling, gate training, read/write eye) —
  the sequence in `al_hal_ddr_init_alpine_v2.c`, owned by the closed CVOS agent, not in
  the recoverable blobs. **[P]**
- **Lever:** raise the **strap NB_PLL_FREQ** (0x4→0x6 for DDR4-2400, →0x3 for 2600). The
  agent derives `cfg.tmg.ddr_freq` (the `AL_DDR_FREQ_*` enum) from `bootstrap.ddr_pll_freq`,
  so a higher strap makes it **train at the higher rate automatically** — no timing table
  edit needed. **[I]** (strap change itself needs pin rework or OTP override, as above.)
- **Retiming implications:** all DDR4 timing/ODT/DIC come from the JEDEC tables in the HAL
  keyed by `ddr_freq`; the agent re-derives them at the new enum. Risk is signal-integrity
  on the actual DIMM/board at 2400+, and 4 GB parts binned for 1866 may not train at 2600.
  Retrain will simply fail (agent won't post the `0x31415926` DDR-ready magic) rather than
  corrupt silently — a failed OC bricks the boot, recoverable via strap revert.

## 3. eFuse capability flags

Mechanism (from `preboot-decompile.md` + preboot C, cross-checked): **[P]**

- Object = **SoC "capabilities" blob** (0x100 B) + **RSA-2048 signature**, read from an
  **I2C EEPROM** (`FUN_01003234`).
- Root of trust = **SHA-256 of the RSA modulus, burned in eFuse/OTP @ `0xfd89608c`**
  (32 B, to `0xfd8960ac`; OTP base `0xfd896000`, word 0x23). `eeprom_preload_parser`
  (`FUN_01002f3c`) hashes the EEPROM modulus, compares to the fused hash, then RSA-verifies
  the caps signature.
- Effect: **enables/denies SoC capability flags in preboot only.** Pass/fail paths set
  capability state + log; **neither aborts OS boot** (kernel is unsigned).

What the flags gate — **not decodable to a bit list** from these blobs (caps content is in
the I2C EEPROM, not the firmware; no descriptive strings). By mechanism they are Annapurna's
per-SKU feature enables (candidates: SSM/crypto accel, PCIe/SATA port counts, unit enables —
possibly a clock-bin cap). **[I]**

**Unlockable?** No. The blob is RSA-2048 signed against a modulus whose hash is fused;
forging an expanded caps set needs Annapurna's **private** key (the on-NOR EEPROM private
key @`0xe004` is a *different* key, never read by this path). **Crucially, CPU/DDR clock is
NOT behind this gate** — overclock needs nothing from here. **[P]**

## 4. CPU errata / chicken-bits

- **No CPUACTLR / L2CTLR / IMP_DEF (S3_1_c15) writes in the al_boot A32 payload.** [P]
  Searched the decompile + disassembly. `al_sys_fabric_core_aarch64_setup`
  (`FUN_01025bcc`) only does sys-fabric reset-deassert + AArch64 mode selection, no
  errata register pokes.
- A57 r1p3 errata chicken-bits (CPUACTLR_EL1 = `S3_1_C15_C2_0`, e.g. #832075, #852523,
  prefetch/store-streaming disables) are therefore applied **by the boot ROM or the
  secure monitor/ATF stage**, which are outside the recoverable NOR blobs. **[I]**
- Linux side: dmesg shows only generic ARM64 workarounds ("EL2 vector hardening") — the
  kernel adds nothing A57-model-specific here. **[P]**
- Consequence for OC: no chicken-bit is being toggled by recoverable firmware that a port
  must reproduce; but any CPU OC should keep whatever CPUACTLR the boot ROM sets (don't
  clear it) since those are correctness fixes, not perf throttles.

## 5. Recovery / debug backdoors

All strap-driven, from `boot_strap` @ `0xfd8a8110` (read-only reflection): **[P]**

| field | bits | values (relevant) |
|---|---|---|
| CPU_PLL_FREQ | [3:0] | clock bin (table above) |
| NB_PLL_FREQ | [6:4] | DDR clock bin |
| SB_PLL_FREQ | [8:7] | 0→bypass, 1→3 GHz (V2) |
| BOOT_ROM | [14] | 0=disabled, 1=enabled |
| **BOOT_DEVICE** | [17:15] | 0=**UART CLI**, 1=**UART 2 Mbaud DL**, 6=UART 1 Mbaud, 5=SPI mode3, 7=SPI mode0 |
| **DEBUG_MODE** | [18] | **0=ENABLED**, 1=disabled (boot-ROM debug) |
| PLL_REF_CLK | [19] | 0=25 MHz, 1=100 MHz (now=1) |
| CPU_EXIST | [21:20] | 0=1 core,1=2,3=4 (now=3) |

- **UART boot-ROM download mode exists** (BOOT_DEVICE 0/1/4/6) — set strap → boot ROM
  accepts an image over UART (2 Mbaud). This is the primary bare-metal recovery path if
  NOR is bricked. **[P]** (physical strap change or OTP override to select it.)
- **Boot-ROM debug mode** is a strap bit (DEBUG_MODE). **[P]**
- **JTAG-enable / secure-boot-disable:** governed by **OTP secure-boot fuses**
  ("Secure Boot enforcement, secure key" per `al_hal_otp.h`), not a strap. If the
  secure-boot fuse is unblown (likely on this consumer part — the caps path is the only
  RSA use, and it doesn't verify the kernel), JTAG/debug are open by default. Exact OTP
  bit not mapped here — read OTP words 0–30 via `al_otp_read_word` (regs `0xfd896000`)
  to enumerate before assuming. **[I]**
- OTP is writable (`al_otp_write_enable`/`write_word`, "Warning!!!! Irreversible!!!") —
  a strap-override or fuse flip is physically possible but one-way; out of scope to apply.

## Register cheat-sheet (all [P] addresses)

| what | addr |
|---|---|
| SB_RING base | `0xfd860000` |
| SB PLL setup_0 | `0xfd860b40` |
| NB PLL setup_0 / status_7 / ch0 | `0xfd860c40` / `0xfd860c1c` / `0xfd860c20` |
| CPU PLL setup_0 / status_7 / ch0 / setup_6 | `0xfd860d40` / `0xfd860d1c` / `0xfd860d20` / `0xfd860d58` |
| thermal sensor | `0xfd860a00` |
| PBS regfile base | `0xfd8a8000` |
| bootstrap reg | `0xfd8a8110` |
| OTP regs base | `0xfd896000` |
| eFuse caps modulus-hash | `0xfd89608c`..`0xfd8960ac` |

## Open / not chased

- No live PLL register dump (device in use) — current `setup_0` values are inferred from
  the freq map, not read back. First safe step: `devmem 0xfd860d40` / `0xfd860c40` /
  `0xfd8a8110` to confirm the exact current strap + PLL config before any write.
- Exact OTP word map (strap-override, secure-boot, JTAG fuses) not enumerated.
- Caps blob bit meanings (I2C EEPROM content) not dumped.
- A57 CPUACTLR value set by boot ROM/ATF not recovered (not in NOR blobs).
