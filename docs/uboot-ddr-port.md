# DDR bring-up port — Annapurna HAL → U-Boot SPL (AL-324 / Alpine V2)

Goal: from-reset standalone U-Boot SPL initialises DDR4 without the proprietary preboot.
Unblocks "standalone replace vendor U-Boot" (target 2, `uboot-port-plan.md` §5) and #29
DRAM-overclock (BIST/margins).

Marks: ✅ confirmed this session · ⚠ needs on-device test · 📄 from source.

Key insight (CONFIRMED ✅): the DDR init is **open, not to be reversed**. Two byte-identical
GPLv2 copies of the Annapurna DDR HAL exist:
- `/mnt/2tb/unvr-port-refs/delroth-alpine_hal/ddr/` (the RE drop).
- `/mnt/2tb/unvr-port-refs/urnvr-kernel-4.19.152/drivers/soc/alpine/HAL/ddr/` (ships in the
  **vendor GPL kernel**).
- `diff -q` on `al_hal_ddr_init_alpine_v2.c` (5608 lines each) → **IDENTICAL**. The delroth
  copy is the same source the vendor already publishes under GPLv2. Use the **kernel-tree
  copy** as the license-clean origin (§License).

The HAL is the "how" (generic training/BIST). The **"what" — OUR board's DDR config** (speed
bin, CL/tRCD/tRP, addrmap, ranks, impedance) — was the only missing piece and is now
**RESOLVED (#67)**: decoded from the live `0x57` EEPROM through the vendor S2 algorithm →
full `al_ddr_init_cfg` for ea16 (**DDR4-1866 CL13, 4 GiB, ×16, 1 rank**, impedance from the
`0xCC` block). See [ddr-s2-parser-analysis.md](ddr-s2-parser-analysis.md) (byte-exact +
drop-in portable C). Only `al_bootstrap.ddr_pll_freq` (running-point strap, SPD-bounded
≤1866) remains a live read.

---

## 1. HAL entry point + call graph

Top-level: `al_ddr_init(struct al_ddr_init_cfg *cfg)` —
`delroth-alpine_hal/ddr/src/al_hal_ddr_init_alpine_v2.c:5552`.
Prototype + config struct in `ddr/include/al_hal_ddr_init.h:870` / `:844`.

Call graph (`al_hal_ddr_init_alpine_v2.c`):

```
al_ddr_init(cfg)                                    :5552
├─ al_ddr_init_print_params(cfg)                    :719   (dump)
├─ al_ddr_reset(cfg)                                        (assert ctrl/PHY reset)
├─ al_ddr_active_byte_lanes_mark(cfg)               :5562  (fills cfg->calc.active_byte_lanes; ECC lane)
├─ al_ddr_ctrl_cfg(cfg)                             :5566  (uMCTL2 regs: MSTR/timing/addrmap/perf/ECC/MRs)
├─ al_ddr_phy_init(cfg)                             :3407  (PUB PHY regs: PGCR/PTR/DTPR/impedance/VREF)
├─ al_ddr_phy_unused_dq_pd(cfg)                     :5575  (power down unused DQ)
├─ al_ddr_unit_reset_release(cfg)                   :5580
├─ al_ddr_ctrl_dfi_init(cfg)                        :5584  (DFI handshake ctrl↔PHY)
├─ if cfg->misc.training_en:
│    al_ddr_phy_training(cfg)                       :4656  ── the training sequence ──
│      └─ al_ddr_phy_training_step[_ext](...)       :4202/:4187  (per-step PIR trigger + poll)
│           steps: ZQ cal, DDL, write-leveling (WL),
│           read DQS gate (DQSGATE/QSTRN), WLADJ (wla_wa :4282),
│           read-data-eye (RDEYE), write-data-eye (WREYE), VREF train
│      └─ al_ddr_phy_training_failure_print(cfg)    :5388  (on error: which octet/step)
├─ al_ddr_phy_run(cfg)                              :5595  (release PHY to mission mode)
└─ al_ddr_ctrl_run(cfg)                             :5600  (controller to normal op)
```

Returns 0 on success, <0 on any step failure. Single-shot, no board callbacks — everything
comes from `cfg`.

## 2. Config struct — fields to fill (`al_hal_ddr_init.h:844`)

`struct al_ddr_init_cfg` members:

| member | struct@line | source of value | our value |
|---|---|---|---|
| `ddr_cfg` | `al_ddr_cfg` `al_hal_ddr.h:1692` | register bases + rev + channel | §4 |
| `org` | `:165` | SPD + topology | §3 |
| `addrmap` | `al_hal_ddr.h:656` | derived from density/width/ranks | §3 |
| `tmg` | `:453` | SPD speed bin + JEDEC DDR4 table | §3 |
| `impedance_ctrl` | `:234` | **board-specific** (ODT/DIC/RTT/VREF/ROUT) | ⚠ §3 |
| `misc` | `:357` | ECC/training/CRC/parity/DBI enables | §3 |
| `perf` | `:601` (optional) | suggested-value defaults in header | defaults |
| `calc` | `:814` | **filled by HAL**, leave zeroed | — |

`org` (memory organisation, `:165`): `data_width` (32/64), `ranks` (1/2/4), `cids`, `dimms`,
`rdimm`, `udimm_addr_mirroring`, `ecc_is_supported`, and V2+: `ddr_type` (DDR3/DDR4),
`ddr_device` (x4/x8/**x16**), `dq_rank_swap_enabled`, `dq_nibble_map[16]`, `cb_nibble_map[2]`.

`tmg` (timing, `:453`): `ref_clk_freq_mhz`, `ddr_freq` (enum `al_ddr_freq` 800…3200 `:69`),
`t_faw_ps`, `t_ras_min_ps`, `t_rc_ps`, `t_rcd_ps`, `t_rrd_ps`, `t_rp_ps`, `t_rfc_min_ps`,
`t_rfc2_ps`, `t_rfc4_ps`, `t_ccd_ps`, `t_rrd_s_ps`, `cl`, `cwl`, `al`. (DDR3-only fields
`t_wtr/t_rtp/t_wr` unused for DDR4.) HAL computes MR0..MR6 + nCK values into `calc`.

`addrmap` (`al_hal_ddr.h:656`): per-bit sysaddr→{col,bank,bg,row,cs,cid} index tables +
`swap_14_16_with_17_19` + optional XOR masks. Worked DDR4 examples in the header comment
`al_hal_ddr.h:587-655`. For x16 / 8Gb / 2-rank the mapping follows the density.

`impedance_ctrl` (`:234`, **the least-recoverable block, all board-specific**): `dic`, `odt`,
`odt_dyn`, `rtt_park`, `host_initial_vref`, `vrefdq`, `hv_min/hv_max`, `phy_rout_pu/pd[2]`,
`phy_pu_odt[2]`, `wr_odt_map[4]`, `rd_odt_map[4]`, `phy_rout[2]`, `phy_odt[2]`. Not in SPD —
must be read back from the **live trained controller/PHY** (see §Params confidence).

`misc` (`:357`): `ecc_is_enabled`, `ecc_scrub_dis`, `training_en=1`, `phy_dll_en=1`,
`crc_enabled`, `parity_enabled`, `ocpar_enabled`, `read_dbi_enabled`, `write_dbi_enabled`.

## 3. OUR board DDR params (the "what")

DRAM = **Samsung K4A8G165WB-BCRC ×4** (`docs/chips/dram-ddr4.md`), 8 Gb ×16 DDR4, soldered,
**no DIMM** (point-to-point, `rdimm=0`, `dimms=1` logical, `udimm_addr_mirroring=0`).

Decoded from part number (📄, needs SPD/live cross-check ⚠):

| param | value | source | conf |
|---|---|---|---|
| type | DDR4 | marking + live `DRAM: 4 GiB` | ✅ |
| device width | ×16 (`AL_DDR_DEVICE_X16`) | K4A8G**16**5 marking | ✅ |
| density/die | 8 Gb per device | K4A**8G** marking | ✅ |
| total | 4 GiB = 4×8Gb | live U-Boot | ✅ |
| speed bin | **DDR4-2400, CL17 (17-17-17)** | `-BCRC` Samsung suffix → 2400@0.833ns | 📄 needs confirm ⚠ |
| ranks | 2 (2 banks in live.dts: 3 GiB@0 + 1 GiB@0x200000000) | live.dts | ⚠ (may be 2× single-rank on 32-bit sub-buses) |
| data width | 64-bit (4×16) | 4 devices ×16 | 📄 ⚠ |
| ECC | **none** (no 9th/ECC device; ×16 parts) | BOM = 4 devices only | 📄 ⚠ (`ecc_is_supported=0`) |
| run frequency | ≤ 2400; actual set by AL-324 bootstrap ref-clk | read live MR / ctrl regs | ⚠ NEEDS TEST |

Two-bank split (DRAM0 3 GiB @0, DRAM1 1 GiB @0x200000000) is the **DDR remap / hole** for the
1 GiB PCIe/MMIO window at 3–4 GiB, NOT two independent controllers — single `alpine-mc`
@0xf0080000. `al_addr_map_dram_remap_set` (preboot `FUN_0102383c`, `preboot-decompile.md`)
programs it.

Timing PS values (DDR4-2400, JEDEC, tCK=833ps) — to VERIFY against SPD (`al_hal_ddr_jedec_ddr4.h`
has the JEDEC constants the HAL cross-checks):
- CL=17, CWL=16(2400), tRCD=tRP=17nCK≈14.16ns, tRAS≈32ns, tRC≈46.16ns, tRFC1(8Gb)=350ns,
  tFAW≈21ns(x16), tRRD_S/L, tCCD_L per DDR4-2400. **These are placeholders — the SPD read is
  authoritative.**

### Param sources, in confidence order
1. **SPD EEPROM @ I²C 0x57 (PRIMARY, authoritative for `org`/`tmg`/`addrmap`)** ✅ readable
   live — vendor `stage2_loader` reads it (`dram-ddr4.md:6`). DDR4 SPD (SPD5, JEDEC 4.1.2.12)
   gives density, width, ranks, speed bin, CL support map, tRCD/tRP/tRAS/tRC/tRFC/tFAW.
   **First step below writes the reader/decoder.** SPD does NOT contain `impedance_ctrl`.
2. **Live trained controller/PHY readback (PRIMARY for `impedance_ctrl` + actual freq)** ⚠ —
   run `al_ddr_cfg_init` (§5) against the running controller, dump MRs (MR0=CL/BL, MR1=ODT/DIC,
   MR2=CWL/RTT_WR, MR5=RTT_PARK, MR6=VREFDQ) and PHY impedance regs. This recovers the exact
   ODT/DIC/VREF/ROUT the vendor preboot programmed — the numbers SPD can't give. Do this from a
   chainloaded U-Boot before attempting SPL.
3. **Vendor preboot al_boot** — DEAD END for a param table ✅: `preboot-decompile.md` confirms
   DDR training is delegated to the **CVOS agent** (`agent_wakeup v2.10`, mailbox 0xf0070000/
   0xf0090000, poll `0xfbff4150==0x31415926`); al_boot only pokes mailboxes. No timings/cfg in
   the payload. The agent binary (separate) could hold a cfg but is not needed given SPD+readback.
4. **GPL U-Boot `alpine_ubnt/board_cfg.h`** — no DDR params (PLL only) ✅ verified. `cmd_ddr.c`
   uses `al_ddr_cfg_init` (live readback), not `al_ddr_init`. HAL header absent from GPL U-Boot.

## 4. Register bases (from `alpine_v2/include/al_hal_iomap.h`) ✅

`AL_NB_BASE = 0xf0000000`. `al_ddr_cfg_init(nb, ctrl, phy, cfg)` /
`al_ddr_init` use:

| arg | macro | address | hardware.md xref |
|---|---|---|---|
| `nb_regs_base` | `AL_NB_SERVICE_BASE` (`+0x70000`) | **0xf0070000** | nb-service (= preboot agent mailbox) |
| `ddr_ctrl_regs_base` | `AL_NB_DDR_CTL_BASE` (`+0x80000`) | **0xf0080000** | memctl (uMCTL2) ✅ |
| `ddr_phy_regs_base` | `AL_NB_DDR_PHY_BASE` (`+0x88000`) | **0xf0088000** | (PUB PHY, inside memctl 64K window) |
| `ddrc_regs_base` | — | NULL for V2 (V3+ only) | — |
| CCU (coherency) | — | 0xf0090000 | ccu (io_coherency) |
| DDR-ready poll | — | 0xfbff4150 | preboot handshake |

`rev = AL_DDR_REV_ID_ALPINE_V2 (2)`, `ch = 0`. Set `AL_DEV_ID = AL_DEV_ID_ALPINE_V2` at compile.

## 5. BIST / margins / ECC for #29 — works WITHOUT SPL ✅

All against the **already-trained live controller** — safe from a chainloaded U-Boot, no
retrain. Init handle first: `al_ddr_cfg_init(0xf0070000, 0xf0080000, 0xf0088000, &ddr_cfg)`
(`al_hal_ddr.c:662`).

- **Data-eye shmoo / margins (the #29 core):** PHY DATX BIST family in `al_hal_ddr.h:1351`+ —
  `al_ddr_phy_datx_bist_pre[_adv]` (disable VT calc), `al_ddr_phy_datx_bist(cfg, params,
  err_status)`, `al_ddr_phy_datx_bist_post[_adv]`. Params `struct al_ddr_bist_params`
  (`:813`: mode LOOPBACK/DRAM, pattern WALK0/WALK1/LFSR/USER, word count). Sweeping RDQS/WDQS
  delay while running BIST = the shmoo the vendor `dram_margins` produces.
  - NOTE ✅: vendor `dram_margins` runs a **prebuilt SRAM agent blob** (`cmd_dram_margins.c`
    memcpy's `agent_ddr_margins_arr` to SRAM), NOT HAL calls. Our port uses the HAL BIST
    directly — cleaner, no blob.
  - ⚠ RUNTIME: vendor `dram_margins` **crashed the live U-Boot with a Synchronous Abort**
    (read-only capture, [uboot-2015.07-hal-capture.md](uboot-2015.07-hal-capture.md)) —
    the blob is fragile; treat as read-with-care. Extra reason to use the HAL BIST path.
- **A/C BIST:** `al_ddr_phy_ac_bist(cfg, err_status, pat)` (`:1362`).
- **ECC status/inject (needs ECC on — likely OFF on this board, see §3):** `al_ddr_ecc_status_get`,
  `al_ddr_ecc_cfg_get`, `al_ddr_ecc_data_poison_enable/disable`, `al_ddr_ecc_corr/uncorr_*_clear`
  (`al_hal_ddr.h:1381`+; vendor wrappers `cmd_ddr.c:86/145`).
- **Training results:** `al_ddr_phy_training_results_get/print` (`:1439`) — per-octet WL/DQS-gate/
  read/write-eye deltas; `al_ddr_phy_training_results` struct `:1290`.
- **Address translate (for targeted poison/BIST):** `al_ddr_address_translate_sys2dram`
  (vendor `cmd_ddr.c:175`).
- **PMU (bandwidth counters, not margins):** `al_hal_ddr_pmu.h` — `al_ddr_pmu_*` on
  `AL_NB_SERVICE_BASE`. Useful for perf, separate from shmoo.

**#29 quick win = port these as U-Boot commands on the delroth/kernel HAL, run from the
chainloaded U-Boot (Stage-1 already builds).** No SPL, no DDR retrain. Only *changing DDR
frequency* needs the SPL retrain path (§6).

## 6. SPL integration plan (staged)

Prereq: Stage-1 chainload U-Boot builds ✅ (`uboot-port-plan.md` §9). DDR work layers on
`uboot-port/` + `scripts/uboot-build.py`.

- **S0 (now, no SPL):** add `mach-alpine` DDR HAL sources (kernel-tree copy) + a `ddr` command
  group backed by `al_ddr_cfg_init` → BIST/margins/ECC/training-dump (§5). Runs in chainloaded
  U-Boot-proper. Delivers #29 tooling + recovers `impedance_ctrl`/actual-freq params (§3 src 2).
- **S1 params:** freeze `al_ddr_init_cfg` for our board from SPD (§Params src 1) + live readback
  (src 2). Store as `board/annapurna/alpine/alpine_ddr_cfg.c`.
- **S2 SPL skeleton:** `CONFIG_SPL` running from SoC SRAM (`AL_LL_SRAM_BASE 0xf2000000`, S2 link
  base 0xf2200000). SPL does: minimal clock/PLL (vendor `pll_init.c` + `al_pll_init`), then
  `al_ddr_init(&cfg)`, then load U-Boot-proper to 0x1100000.
- **S3 wire HAL into SPL:** compile `al_hal_ddr_init_alpine_v2.c` + `al_hal_ddr_alpine_v2.c` +
  `al_hal_ddr.c` + reg headers with `AL_DEV_ID=AL_DEV_ID_ALPINE_V2`. Shim `al_hal_reg_utils`/
  `al_hal_plat_services` (readl/writel, udelay, printf) to U-Boot equivalents (§7).
- **S4 boot-ROM handshake ⚠:** to boot from reset without vendor S2, the mask ROM must accept
  our SPL in the **S2 SPI-loader format** (`"S2"` header, SPI off 0, link 0xf2200000 —
  `nor-boot-chain.md` §1, `preboot-decompile.md`). Contract NOT yet fully reversed (S2 jumptable
  @0xf22000fc unrecovered). Fallback: keep vendor S2, replace only al_boot's DDR-agent step.
- **S5 remap:** program DRAM remap for the 3–4 GiB PCIe hole (`al_addr_map_dram_remap_set`) to
  reproduce the two-bank layout.

### What compiles as-is vs needs shims
- **As-is:** `al_hal_ddr_init_alpine_v2.c`, `al_hal_ddr_alpine_v2.c`, `al_hal_ddr.c`, all
  `*_alpine_v2.h` / `al_hal_ddr_jedec_ddr4.h` / `al_hal_ddr_cfg.h` — pure register logic.
- **Shims needed:** `al_hal_plat_services.h` (al_reg_read32/write32 → U-Boot readl/writel;
  al_udelay; al_dbg/al_err printf), `al_hal_types.h` (al_bool/al_phys_addr_t), `al_hal_common.h`.
  Kernel tree has Linux-flavoured versions to adapt: `.../HAL/include/`. Small, mechanical.
- **`AL_DEV_ID` gating:** the source is multi-SoC via `#if AL_DEV_ID`. Fix to
  `AL_DEV_ID_ALPINE_V2` so the V2 struct layout / ZQ-segment counts (2) / MR set compile.

## 6a. Overclock recoverability — test in RAM, commit to NOR last

Same rule as the MTD/NAND work: do whatever we like, because every test is non-persistent
and the persistent thing is backed up. Layered, weakest failure to worst:

1. **Test in RAM/SRAM, never NOR.** The overclock SPL is loaded + run via the *existing*
   vendor chain (reboot → vendor U-Boot → load SPL to SRAM → jump). NOR's S2 blob is
   untouched → a hang/failed-train recovers by **power cycle** (boots normally).
2. **DDR config is volatile.** PLL + controller/PHY are registers; a power-cycle/SoC reset
   wipes them to defaults. No "stuck overclocked" state exists.
3. **Watchdog catches a training hang.** Arm SP805 (`0xfd88c000`) before `al_ddr_init`; a
   deadlock at an unstable freq → watchdog reset → BootROM → vendor chain.
4. **Validation gate — never trust un-validated DRAM.** Run memtest + HAL BIST (§5) at the
   new freq BEFORE handing DRAM to anything. Instability = detectable BIST/memtest failure,
   not silent corruption. Fail → fall back to known-good, don't proceed.
5. **NOR touched LAST, reversibly.** Only flash SPL→NOR after many stable RAM-loaded boots.
   Fallbacks: full NOR image + S2 blob dumped (reflash); AL-324 BootROM UART/USB recovery;
   external SPI programmer on the MX25U25635F.

Staged progression, each step BIST-gated: **1866 (SPD-validated) → 2133 → 2400** (reclaim the
K4A8G165WB-BCRC die's marked bin the SPD capped) → push until BIST fails, record the margin.

## 7. Dependencies to port alongside DDR
- **PLL/clock** before DDR: vendor `board/annapurna-labs/common/pll_init.c` + `al_pll_init`
  (`preboot FUN_01023ed4`). Sets DDR ref clock → `tmg.ref_clk_freq_mhz` / `ddr_freq`.
- **SerDes:** NOT needed for DDR (only PCIe/SATA PHY). Skip for SPL DDR.
- **HAL plat shim** (§6). No serdes/PCIe pulled in by the DDR files (checked — DDR src includes
  only ddr + reg-util + plat-services).

## License note
- DDR HAL is **tri-licensed**: Annapurna Commercial **OR GPLv2 OR BSD-3-Clause** (header,
  `al_hal_ddr_init.h:1-34`). Same header on the kernel-tree copy.
- U-Boot is **GPLv2**. Using the HAL under its **GPLv2** option is compatible ✅.
- Prefer the **kernel-tree copy** (`urnvr-kernel-4.19.152/.../HAL/ddr/`) as origin: it ships in
  the vendor's published GPL source, so provenance is a clean GPLv2 distribution (not an RE of a
  binary). It is byte-identical to delroth's (`diff -q` = IDENTICAL ✅).
- Keep the tri-license header intact on every ported file; record derivation in
  `docs/issues/36-licence-derivation-ledger.md`.

## Concrete status + next
- **DONE — DDR cfg (#67):** live `0x57` dumped (`scripts/read-ddr-spd.py`, 16-bit path) +
  decoded through the vendor algorithm ([ddr-s2-parser-analysis.md](ddr-s2-parser-analysis.md))
  → full ea16 `al_ddr_init_cfg`. The §3 "what" is resolved.
- **DONE — chainload U-Boot:** builds (`scripts/uboot-build.py`); `CMD_MEMTEST` widened +
  `CMD_MEMORY` (md/mw) added for live DDR-register inspection.
- **NEXT (S0):** test the chainload U-Boot on woomera (reboot → vendor U-Boot → load
  `u-boot.bin` @0x1100000 → `go` → `mtest` on live DRAM). Port the HAL + a `ddr` command
  backed by `al_ddr_cfg_init` for BIST/margins on the running controller (§5). No SPL yet.
- **NEXT (S2/S3 — overclock):** SPL from SRAM (§6) using the decoded cfg → `al_ddr_init` at a
  parametrised freq → memtest/BIST gate (§6a). 1866 known-good first, then step up.
