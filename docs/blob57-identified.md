# blob57 identified — Annapurna 25G SerDes 8051 microcode, NOT boot-critical

Closes the last opaque region of the preboot RE ([#152](https://github.com/awtoau/awto-unvr/issues/152)),
answers [#246](https://github.com/awtoau/awto-unvr/issues/246), and unblocks
[#46](https://github.com/awtoau/awto-unvr/issues/46).

Static analysis only — no device access, no flash writes. Reproduce with
`scripts/identify-blob57.py`; log `tmp/logs/identify-blob57.log`.

## Answer

**Link-training only — and not even that on this board.** A from-scratch preboot
replacement can omit blob57 entirely.

- blob57 **is** `al_serdes_25g_fw[]`, the Annapurna 25G SerDes PHY firmware — an
  **8051** program for the SerDes complex's internal microcontroller.
- It is loaded **only** by the group-E (25G complex) branch of `al_serdes_init_cores`.
- This board sets SerDes **group E = `skip`**, so the loader is never reached.
- The 10G SFP+ is on **HSSP group D @0xfd8c0c00**, a different block with no
  microcode at all ([#121](https://github.com/awtoau/awto-unvr/issues/121), eac280a).

## Identification — byte-exact

| | |
|---|---|
| extent | al_boot VA `0x010346a0`–`0x0104269c`, file `0x346a0`–`0x4269c` |
| length | **57,340** B (`0xdffc`) |
| sha256 | `96711fa0d5f92f9d8ba85bca68be1f90f90cb4426e3fc0c36c945dd7c229411d` |
| entropy | 6.655 |

Identical sha256, length and entropy to `al_serdes_25g_fw[]` extracted from
`/mnt/2tb/unvr-port-refs/delroth-alpine_hal/drivers/serdes/al_hal_serdes_25g_fw.h`.
**0 mismatching bytes over all 57,340.**

- Target CPU is an **8051**, stated in the HAL:
  `al_hal_serdes_25g_regs.h:218` — "Internal 8051 micro-controller is blocked from
  accessing the internal APB CSR."
- Byte histogram is a textbook 8051 opcode profile, not compressed data — top values
  `f5` (MOV direct,A), `e5` (MOV A,direct), `83`/`82` (DPH/DPL SFRs), `24` (ADD A,#),
  `e4` (CLR A), `f0` (MOVX @DPTR,A), `02` (LJMP). Entropy is flat at ~6.6 across the
  whole blob (6.608 first 44 KB / 6.588 rest): one uniform instruction stream, no
  header, no compressed section.
- Word-reversed, offset 0 reads `02 d5 7f` = `LJMP 0xd57f` — the 8051 reset vector.
  The download register takes each 4-byte group MSB-last.

## The loader — `FUN_01021b60` @ `0x01021b60`

Byte-for-byte the HAL's `al_serdes_25g_fw_init()`
(`delroth-alpine_hal/drivers/serdes/al_hal_serdes_25g_init.c:29`).

```
01021bcc:  movw r1,#0x46a0     ; \ fw = 0x010346a0  (blob57 base)
01021bd4:  movt r1,#0x103      ; /
01021bd8:  movw r2,#0xdffc     ;   fw_size = 57,340
01021b74:  str  r3,[r4,#0x11c] ;   cpu_prog_addr = 0  (auto-increments)
01021bb4:  str  r3,[r4,#0x120] ;   cpu_prog_data = BE32(fw[i..i+3])   <- download loop
```

`r4` = `regs_base + 0x100`; `+0x11c`/`+0x120` are `al_serdes_c_gen.cpu_prog_addr` /
`cpu_prog_data` at struct offsets `0x1c`/`0x20`
(`al_hal_serdes_25g_regs.h:67-69`). Same `+0x100` base as
`al_serdes_25g_group_ictl_pma_val_set`, which writes `gen.ctrl` at `+0x118` (`0x18`).

### Correction to `preboot-coverage.md`

That doc records blob57 as **"Referenced-by: NONE (abs)"** and start `0x0103469d`.
Both are wrong:

- True base is `0x010346a0` — `0x0103469d` is where the preceding `.rodata` string
  `al_serdes_25g_group_ictl_pma_val_set\0` ends; the next 3 bytes are alignment pad.
- The reference exists and is exactly the `movw`/`movt` pair the scan looked for. It
  was missed because the scan used the 3-byte-early boundary.
- `0xdffc` from the loader lands precisely on `0x0104269c`, the documented end —
  independent confirmation of the extent.
- "0xff-record structure" is also wrong: there are **no** `0xff` runs of length ≥4
  anywhere in the blob (`0xff` is 2.2% of bytes, scattered).

## Why it is not on the boot path

Single call site: `preboot-alboot-decompiled.c:23138`, inside `al_serdes_init_cores`,
in the **group E** arm (its errors are `"%s: invalid group E configuration"`,
`"%s: group E configuration failed"`). Guarded by:

```c
if (local_a8 == 1) return 0;   /* AL_SRDS_CFG_SKIP -> return before the download */
FUN_01021b60(local_1dc, local_58, local_54);
```

Matching the HAL's `al_serdes_init.c:1399`:
`if (cfg->grp_cfg[AL_SRDS_GRP_E].mode != AL_SRDS_CFG_SKIP) { al_serdes_25g_fw_init(...); }`

Group-mode enum (`al_hal_serdes_interface.h:110+`): `OFF`=0, **`SKIP`=1**, `OFF_BP`=2,
… `KR`=0x12, `ETH_25G`=0x15 — matching the preboot's `== 1`, `== 0x12`, `== 0x15`
and `< 3` / `> 2` tests exactly.

### This board sets group E = skip

**All five** board DTBs in the NOR container (`dt`, `dt_pro`, `dt_ai`, `dt_bt`,
`dt_hd`) carry the same `serdes` node:

| DT group | HAL group | interface | ref-clock |
|---|---|---|---|
| group0 | A | `pcie_g2x2_pcie_g2x2` | 100 MHz |
| group1 | B | `sata` | 100 MHz |
| group2 | C | `sata` | 100 MHz |
| group3 | D | **`10gbe`** | 156.25 MHz |
| group4 | **E** | **`skip`** | — |

Also in the live board DT: `docs/hw-reference/20260816-104601/live.dts:1496`.
`"skip"` is one of the preboot's own interface-name strings (table at
`0x010321b8`–`0x01032254`: `40gbe`, **`skip`**, `off`, `off_bp`, `pcie_*`, `sata`,
`sgmii`, `sgmii-2.5g`, `10gbe`, `25gbe`).

So `al_serdes_init_cores` returns at the group-E `== 1` test and
**`FUN_01021b60` never executes on this board.** blob57 is dead weight in NOR:
18.8% of al_boot that this hardware never uses.

### Addresses

`AL_SERDES_BASE(idx) = 0xfd8c0000 + (idx < 4 ? idx*0x400 : 0x2000)`
(`delroth-alpine_hal/platform/alpine_v2/include/al_hal_iomap.h:324`)

- group D (10G SFP+) = **0xfd8c0c00** — HSSP, no microcode
- group E (25G complex) = **0xfd8c2000** — the blob's target, unused here

This identifies the previously-unexplained `+0x2000` fifth SerDes window noted in
`hardware.md:254,263-270`: it is `AL_SERDES_BASE(4)`, group E.

## Corroboration

- **UBNT GPL U-Boot**: no `al_serdes_init.c`, no `AL_SRDS_GRP_E`, no `serdes_25g_fw`.
  Never configures group E.
- **UBNT 4.1.37 / urnvr 4.19.152 kernels**: ship the HAL (incl. the fw array) but no
  board-level group-E config; the 25G handle is generic library code keyed off
  `serdes_grp`, which is 3 for this board's eth ports.
- **imbushuo CCR2004 EDK2** (a working AL-324 port): ships **no serdes HAL at all** —
  no `drivers/serdes/`, zero hits for `al_serdes_25g` / `serdes_25g_fw`. It boots this
  SoC family with no 25G firmware download whatsoever.
- **NOR TOC has no `serdes_25g_fw` object.** The preboot has an override path —
  `"Loading SerDes 25G FW to %08X (%u bytes)..."` @`0x01032130`, calling
  `al_flash_obj_data_load(..., 0x1120000)` — but the container's 9 TOC entries are
  `preboot`, `dt`, `dt_pro`, `dt_ai`, `dt_bt`, `dt_hd`, `uboot`, `uboot-en`,
  `uboot-re`. Nothing to load; the embedded blob is the fallback default, and it too
  goes unused.

## Consequence for #46

A from-scratch preboot **can omit blob57**. Nothing in the boot-to-U-Boot path reads
it, and the only consumer (25G group E) is disabled in every board DT variant.

If a future board ever needed the 25G complex, the same firmware is available in the
open HAL (`delroth-alpine_hal/.../al_hal_serdes_25g_fw.h`, also in the urnvr 4.19.152
kernel tree) — so even then the replacement carries no blob the project cannot source
independently.

**No destructive "zero it and boot" test is needed.** The question is settled from the
call graph and the board DT.
