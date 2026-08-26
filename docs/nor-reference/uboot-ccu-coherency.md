# Vendor U-Boot CCU/coherency mechanism — full decompile + GPL cross-reference

Upgrades the single-function spot-check in issue #97 (comment 2026-08-26T00:42:49Z)
to a full headless-Ghidra decompile of **both** vendor U-Boot builds plus a
real GPL-source cross-reference, per user direction. Static RE only, no
device touched, per `docs/ghidra.md`.

## 1. What was decompiled

Both blobs: AArch64, `AARCH64:LE:64:v8A`, load/entry `0x1100000` (per
`docs/ghidra.md` §2, `docs/nor-boot-chain.md` §1/§4). Ghidra 12.2 / JDK 21,
headless pipeline (`scripts/ghidra-analyse.py`), default `--timeout 1200s` —
**both runs finished in ~20 s, no timeout hit.**

| Build | Source | Size | Functions | code / data / undef | Artifacts |
|---|---|---|---|---|---|
| **OLD** (Dec 16 2020, currently flashed) | `tmp/uboot/old-uboot-payload.bin` | 689,728 B | 1686 | 63.18% / 25.02% / 11.80% | `uboot-old-decompiled.c`, `uboot-old-disassembly.asm`, `uboot-old-coverage.txt` |
| **NEW** (Jul 09 2026, never flashed, from `sources/UNVR-5.1.25.bin`) | `tmp/uboot/new-uboot-payload.bin` (= `tmp/sections/uboot-proper-payload.bin`) | 691,152 B | 1683 | 62.81% / 25.33% / 11.86% | `uboot-new-decompiled.c`, `uboot-new-disassembly.asm`, `uboot-new-coverage.txt` |

Banners confirmed by string search on the raw blobs (not just the container
TOC): OLD `U-Boot 2015.07-alpine_db-2.21-HAL (Dec 16 2020 - 05:54:51 +0800)`,
NEW `... (Jul 09 2026 - 19:13:29 +0800)`.

### Correction to the record

The prior spot-check pass's decompile at `tmp/ghidra-out/uboot-stage3-vendor/decompiled.c`
was cited in the issue as covering "the currently-flashed OLD 2020-12-16
build." **It does not — it is the NEW build.** Confirmed two ways: (1) its
`CoverageReport` total is exactly 691,152 B, the NEW size, not OLD's
689,728 B; (2) its embedded banner string reads `Jul 09 2026`, not
`Dec 16 2020`. Its `ghidra-analyse.log` entry shows it was run against
`tmp/sections/uboot-proper-payload.bin`, which `docs/nor-boot-chain.md` and
the task brief both already establish as the NEW carve. The underlying
finding (CCU write mechanism, byte-identical between builds) is unaffected —
OLD's own copy of the same function is now separately confirmed below — but
the directory-to-build mapping in the issue thread was wrong. This doc's
copies are named unambiguously: `uboot-old-*` / `uboot-new-*`.

## 2. The CCU coherency mechanism — confirmed addresses, both builds

`ft_board_setup()` (a U-Boot `CONFIG_OF_BOARD_SETUP` FDT-fixup hook) is live
in the boot path, called from **2 sites** in both builds — both inside the
`bootm` FDT-preparation flow (call sites carry `"ERROR: arch-specific fdt
fixup failed"` / `"ERROR: %s: chosen node create failed"` neighbour strings,
i.e. the `image_setup_libfdt`-equivalent path). It:

1. Fixes up clock-frequency properties (`ft_board_setup_clock`, 3 calls).
2. Disables absent/EP-mode PCIe DT nodes (`ft_board_setup_feature_disable`).
3. Handles an `iocc_force`/`iocc_force_val` env-var override that force-writes
   `/soc/ccu`'s (or `/soc/al-fabric`'s) `io_coherency` property
   (`ft_board_setup_prop_u32_set`).
4. Looks up `/soc/ccu` (real node, `compatible = "annapurna-labs,al-ccu"`,
   `reg = <0xf0090000 0x10000>`, present in both our DTS and the live
   hardware-reference DTS, both carrying `io_coherency = <1>` —
   `docs/hardware.md`, `docs/uboot-port-plan.md:190-191`), reads its
   `io_coherency` property, translates the DT `reg` to an address, and calls
   the CCU-programming helper below with `(ccu_address, io_coherency)`.
5. Fixes up PCIe `ranges` properties (`ft_board_setup_pcie_set`).

| function | OLD VA | NEW VA | how identified |
|---|---|---|---|
| `ft_board_setup` | `0x1104670` | `0x11045d4` | structural (calls 1-5 in GPL order) + 2 call sites in bootm FDT-prep, both builds |
| `ft_board_setup_feature_disable` | `0x1101e20` | `0x1101ccc` | **`__func__` string**, both builds |
| `ft_board_setup_clock` | `0x1101ea0` | `0x1101d4c` | **`__func__` string**, both builds |
| `ft_board_setup_pcie_set` | `0x1101f00` | `0x1101efc` | **`__func__` string**, both builds |
| `ft_board_setup_prop_u32_set` | `0x1102020` | `0x110201c` | **`__func__` string**, both builds |
| CCU register-write helper (`al_ccu_init_inlined`) | `0x112629c` | `0x1125b38` | register-offset/value correlation (below) |

The `__func__` string matches are new evidence, not present in the prior
spot-check — they prove the *helper* functions surrounding the CCU call are
literally named `ft_board_setup_*` inside the binary itself, in both builds,
matching the GPL 1.3.35 `board/annapurna-labs/{alpine_ubnt,alpine_db}/board.c`
static functions of the same names verbatim. `alpine_ubnt/board.c` is a
**UBNT-specific board file** in the public drop, not a generic Annapurna one.

### The CCU-write helper — decompiled body (identical both builds)

```c
void FUN_0112629c(long param_1, int param_2)   /* OLD: 0x112629c, NEW: 0x1125b38 */
{
  if (param_2 != 0) {
    *(undefined4 *)(param_1 + 0x4000) = 1;
    *(undefined4 *)(param_1 + 0x5000) = 1;
  }
  *(undefined4 *)(param_1 + 4) = 7;
  return;
}
```
Called from exactly **one** site in both builds (`ft_board_setup`, immediately
after the `/soc/ccu` lookup): `FUN_..(ccu_address, io_coherency)`. There is
no second call to this specific function — the "two sites" in the original
issue comment refers to `ft_board_setup` itself (§2 above), not this helper.

### Register semantics — resolved via `delroth-alpine_hal` (real source, not just structural)

`delroth-alpine_hal/include/sys_fabric/al_hal_ccu_regs.h` (`struct
al_ccu_regs`) + `services/sys_fabric/al_init_sys_fabric.c`
(`al_ccu_cluster_init`, `al_ccu_init`):

```c
struct al_ccu_regs {
    uint32_t control_override_register;         /* [0x0] */
    uint32_t speculation_ctrl_register_v1_v2;    /* [0x4] */
    uint32_t secure_access_register;             /* [0x8] */
    ...
    struct al_ccu_slave_regs slaves[7];          /* [0x1000], each 0x1000 B,
                                                     .snoop_control_register @+0x0 */
    ...
};
#define AL_CCU_SLAVE_IDX_CLUSTER(idx)  (3 + (idx))   /* -> slaves[3]@+0x4000, slaves[4]@+0x5000 */

void al_ccu_cluster_init(void __iomem *ccu_address, al_bool iocc, unsigned int cluster)
{
    if (iocc) {
        unsigned int slave_num = AL_CCU_SLAVE_IDX_CLUSTER(cluster);
        al_reg_write32(&regs->slaves[slave_num].snoop_control_register,
            CCU_SNOOP_CONTROL_REGISTER_SX_ENABLE_SNOOP_REQUESTS |          /* 1<<0 */
            ((AL_NB_ANPA_NUM_CLUSTERS > 1) ?
             CCU_SNOOP_CONTROL_REGISTER_SX_ENABLE_DVMS : 0));              /* 1<<1 */
        ...
    }
#if (AL_DEV_ID == AL_DEV_ID_ALPINE_V1) || (AL_DEV_ID == AL_DEV_ID_ALPINE_V2)
    al_reg_write32(&regs->speculation_ctrl_register_v1_v2, 7);
#endif
}
void al_ccu_init(void __iomem *ccu_address, al_bool iocc)
{
    for (i = 0; i < AL_NB_ANPA_NUM_CLUSTERS; i++)
        al_ccu_cluster_init(ccu_address, iocc, i);
}
```

The vendor binary's single merged function is the compiler's inlined,
2-iteration-unrolled `al_ccu_init()` for `AL_NB_ANPA_NUM_CLUSTERS == 2`
(AL-324 = 2×2 A57 clusters): both `slaves[3]`/`slaves[4]` writes land in one
function body under one `io_coherency != 0` guard, and the two (idempotent)
`speculation_ctrl_register_v1_v2 = 7` writes from each unrolled
`al_ccu_cluster_init()` call were merged by the optimizer into the single
unconditional write at `+0x4`. `AL_DEV_ID_ALPINE_V2` gating on
`speculation_ctrl_register_v1_v2` matches the AL-324 (Alpine V2) target
exactly.

**Confirmed register meanings** (from the HAL, not guessed):
- `CCU_base + 0x4` = `speculation_ctrl_register_v1_v2` — "disable speculative
  fetches from masters" (HAL comment), Alpine V1/V2-only field, written `7`
  unconditionally on every `bootm`.
- `CCU_base + 0x4000` = `slaves[3].snoop_control_register` =
  `AL_CCU_SLAVE_IDX_CLUSTER(0)` — cluster-0 snoop/DVM enable.
- `CCU_base + 0x5000` = `slaves[4].snoop_control_register` =
  `AL_CCU_SLAVE_IDX_CLUSTER(1)` — cluster-1 snoop/DVM enable.
- Both gated on the DTB's `/soc/ccu` `io_coherency` property (`1` on this
  unit, both DTS sources).

### Open discrepancy — flagged, not resolved (per `docs/ghidra.md` §4 merge rule)

HAL source writes `CCU_SNOOP_CONTROL_REGISTER_SX_ENABLE_SNOOP_REQUESTS (1) |
CCU_SNOOP_CONTROL_REGISTER_SX_ENABLE_DVMS (2) = 3` to each slave register
when `AL_NB_ANPA_NUM_CLUSTERS > 1` (true here — two slave offsets are
touched, confirming a 2-cluster build). **The actual vendor binary writes
plain `1`, not `3`, to both `+0x4000` and `+0x5000`** — i.e. without the
DVMS bit. Two explanations, neither confirmed:
- this contractor's HAL revision (stage3 build string cites `HAL 61afa9c`,
  `docs/nor-boot-chain.md` §2) computed/omitted the DVMS term differently for
  AL-324 than the `delroth-alpine_hal` snapshot used here, **or**
- the local `delroth-alpine_hal` checkout is a different HAL commit than
  `61afa9c`, not independently verified against it.

Logged rather than silently resolved. Does not change the coherency-gate
conclusion (bit 0, snoop-request enable, is set either way) but the DVM
(distributed-virtual-memory / TLB-broadcast) message path may or may not be
enabled — worth resolving before treating "port to `uboot-port`" as a
complete fix rather than a partial one.

### Relocation is not a constant offset

The prior spot-check estimated NEW is relocated "~0x2ff-0x6b0 bytes" versus
OLD. With both builds now fully decompiled, the delta at 6 confirmed anchor
points in the same function cluster is **not constant**:

| function | OLD VA | NEW VA | OLD−NEW |
|---|---|---|---|
| `ft_board_setup_feature_disable` | `0x1101e20` | `0x1101ccc` | `0x154` (340) |
| `ft_board_setup_clock` | `0x1101ea0` | `0x1101d4c` | `0x154` (340) |
| `ft_board_setup_pcie_set` | `0x1101f00` | `0x1101efc` | `0x4` (4) |
| `ft_board_setup_prop_u32_set` | `0x1102020` | `0x110201c` | `0x4` (4) |
| `ft_board_setup` | `0x1104670` | `0x11045d4` | `0x9c` (156) |
| `al_ccu_init_inlined` | `0x112629c` | `0x1125b38` | `0x764` (1892) |

The gap shrinks from 340 B to 4 B between `_clock` and `_pcie_set` (~2
functions' worth of code got smaller in NEW right there), then grows again
to 156 B and then 1892 B further into the image — i.e. real, scattered
size differences accumulate between these points, consistent with (but not
individually attributed to) the already-documented NEW additions: `dt_hd`
sysid support, `bootsign`/FIT env additions (`docs/nor-boot-chain.md` §4,
§8). The whole-image net delta is NEW − OLD = 691,152 − 689,728 = **+1,424
B**, even though every anchor point sampled here has NEW's VA *lower* than
OLD's — meaning the size growth is concentrated after these anchor points,
not before. Not chased further (out of scope for #97; nothing here reads as
AHCI/PCIe/coherency-relevant, see §3).

## 3. Systematic OLD-vs-NEW scan beyond the single function

Keyword sweep across both **full** decompiles (not just the one already-known
function) — `ahci`/`sata`/`pcie`/`coheren`/`snoop`/`iatu`/`inbound`/`outbound`,
case-insensitive:

| keyword | OLD hits | NEW hits | verdict |
|---|---|---|---|
| `ahci` | 6 | 6 | **identical** call sequence: `ahci_init_one`, `ahci_port_start`, SCSI READ CAPACITY10/16 error strings — only address-derived symbol suffixes differ |
| `sata` | 0 | 0 | (driver doesn't use the literal string) |
| `pcie` | 100 | 101 | 1 extra hit in NEW — not AHCI/coherency-related (see below) |
| `coheren` | 5 | 5 | **identical**: the one `ft_board_setup_prop_u32_set(io_coherency)` write path (`iocc_force` override), the one real `ccu`-lookup read, the two `dma-coherent` PCIe DT fixups (`annapurna-labs,alpine-internal-pcie`/`alpine-external-pcie`) |
| `snoop` | 0 | 0 | (HAL-level concept, no string in U-Boot proper) |
| `iatu`/`inbound`/`outbound` | 1 | 1 | one benign log string, `"PCIe %d: outstanding outbound tr[ansactions]"` — no iATU/BAR-window register programming found near it |

**Conclusion: no new AHCI/SATA/coherency-relevant difference between OLD and
NEW beyond the already-known CCU mechanism** (§2), which is itself
byte-identical logic in both. The AHCI driver code is untouched by the 2026
rebuild. The `pcie` +1 and function-count delta (1686 OLD vs 1683 NEW) are
not chased further — consistent with the already-documented, non-AHCI
`dt_hd`/`ea30`/FIT-bootsign additions (`docs/nor-boot-chain.md` §4/§8), and
nothing in the keyword sweep points at anything AHCI/PCIe/coherency-adjacent
being their cause.

The issue thread's most recent hypothesis — a U-Boot-programmed PCIe iATU
(address-translation) window that Linux's `ahci`/PCI core doesn't reset —
is **not confirmed or refuted** by this pass. No iATU register-block writes
were found in U-Boot's PCIe init path via this keyword sweep, but iATU
offsets are raw MMIO numbers with no distinguishing string, so a negative
keyword result here is weak evidence at best. Resolving it needs a targeted
read of the PCIe/AHCI init call graph (`al_init_pcie`, referenced at
`s_al_init_pcie_...` in both decompiles) against the AL HAL's iATU
programming API — not attempted here; flagged as the next real step if the
CCU hypothesis (§2) doesn't pan out on hardware.

## 4. GPL cross-reference — did it help?

**Yes, substantially, for the `ft_board_setup*` family** (§2): the public
`NeccoNeko/UBNT-source-code` `UNVR-1.3.35-GPL/u-boot` tree
(`/mnt/2tb/unvr-port-refs/UBNT-source-code/UNVR-1.3.35-GPL/u-boot/`) —
already known (`docs/nor-boot-chain.md` §7) to be a **different, older
generation** than OLD/NEW (1.3.35 vs the 2020-12/2026-07 builds analysed
here) — contains `board/annapurna-labs/alpine_ubnt/board.c`, a genuinely
UBNT-specific board file whose `ft_board_setup()` and its 4 static helpers
match the decompiled binaries **by literal embedded function name**, not
just structural inference. That's real symbol recovery, not a guess.

**Yes, for the CCU register semantics**: `delroth-alpine_hal` (also already
known, `docs/ghidra.md` §4 "gold" HAL source) fully defines `struct
al_ccu_regs`, `al_ccu_init()`, `al_ccu_cluster_init()` — real C source for
exactly the mechanism the decompile shows, including the `AL_DEV_ID ==
AL_DEV_ID_ALPINE_V2` conditional compile that matches this SoC. This
resolved `CCU_base+0x4` to a named register (`speculation_ctrl_register_v1_v2`)
and `+0x4000`/`+0x5000` to named struct fields (`slaves[3..4].snoop_control_register`)
that the prior spot-check pass could only describe by raw offset.

**No, `al_ccu_init` itself is not in the GPL u-boot tree** — as
`docs/licence-audit.md`/`docs/nor-boot-chain.md` already predicted, it's
proprietary Annapurna HAL, absent from the GPL drop. `delroth-alpine_hal`
supplied it instead (dual GPLv2/commercial licensed per its own file
header), which is why it was usable here.

**Net: the "closest available GPL ancestor" framing from
`docs/nor-boot-chain.md` §7 holds, but it undersold how useful the ancestor
actually is** — despite being 5+ years and one full generation off, the
`alpine_ubnt` board file is source-identical enough at the function-name
level to confirm 5 of 6 named functions by literal string match, not just
correlation.

## 5. Files committed here

- `uboot-old-decompiled.c` / `uboot-old-disassembly.asm` / `uboot-old-coverage.txt` — full Ghidra export, OLD build.
- `uboot-new-decompiled.c` / `uboot-new-disassembly.asm` / `uboot-new-coverage.txt` — full Ghidra export, NEW build.
- `uboot-old-names.sym` / `uboot-new-names.sym` — ApplyAlRegs-format name ledgers (6 confirmed functions each).
- `uboot-vendor-names.md` — shared OLD/NEW naming table with rationale.
- `uboot-ccu-coherency.md` — this file.

## 6. Still open (unchanged scope from #97 — not attempted here, device untouched)

- Port the CCU coherency-fixup mechanism to `uboot-port`: add an MMU mapping
  for `0xf0090000`/`0x10000` (issue confirmed by hardware test that
  `uboot-port` currently data-aborts reading this region at all — no mapping
  exists), then a `ft_board_setup`/`board_late_init` hook reading
  `/soc/ccu`'s `io_coherency` and writing `+4=7`, `+0x4000=1`, `+0x5000=1`
  (or `=3` if the DVM-bit discrepancy in §2 gets resolved in `uboot-port`'s
  favour) if set.
- Test on hardware whether this actually fixes the `ata3.00` IDENTIFY hang —
  static RE only establishes plausibility, not causation.
- The iATU/PCIe-address-translation alternate hypothesis (§3) — not chased
  here, no evidence for or against found in this pass.
