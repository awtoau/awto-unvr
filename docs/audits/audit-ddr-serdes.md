# Adversarial source-of-truth audit — DDR + SerDes (our U-Boot port)

Date: 2026-08-19. Scope: `uboot-port/board/annapurna/alpine/al_ddr/` (DDR HAL subset + `ddr_cmd.c`),
`uboot-port/drivers/phy/al_serdes/` (`al_serdes_10g.c`, `al_hal_serdes_25g.c`).
Trigger: fabricated UDMA ring size (5 vs HAL min 32) already reached HW in al_eth — hunt the same class in DDR/SerDes.

## Headline

- **Every hardware register offset, bitmask and mode constant traces to a source and MATCHES.** No
  constant contradicts the HAL (unlike the al_eth ring bug). The PCS offsets, the 0x50 config word,
  the block-lock reg/bit, the optic EQ params, the KR read protocol, the 10G speed word — all verified
  identical to the Annapurna HAL / al_eth driver.
- **2 fabricated-no-source values**, both in the DDR BIST params, both *valid* (in range) but *not
  derived* from geometry the way the HAL derives them: `wc=16`, `col_max=0x38`.
- **The real risks are behavioral, not magic-number:** (1) a DDR BIST timeout resets the live PHY;
  (2) the standalone PCS bring-up is intentionally incomplete so it can never block-lock on its own;
  (3) our `al_hal_serdes_25g.c` copy silently dropped 10 register-read error checks.
- Our two HAL `.c` copies were diffed against source: `al_hal_ddr.c` + `al_hal_ddr_alpine_v2.c` are
  **byte-identical**; `al_hal_serdes_25g.c` differs **only** by removed error-handling (values/offsets/order unchanged).
- DDR + SerDes register bases are read from the DT via ofnode reg-names (satisfies #84); none hardcoded.
  DT node bases match `docs/hardware.md`. `AL_DEV_ID` pinned to `ALPINE_V2` (correct for AL-324).

Legend: **match** / **mismatch** (value contradicts source) / **fabricated** (no traceable source).

## DDR

| our symbol/value | our file:line | source-of-truth value | source file:line | VERDICT | fix |
|---|---|---|---|---|---|
| nb/ctrl/phy bases from DT `reg-names` via ofnode | ddr_cmd.c:32-43,56-58 | (requirement: not hardcoded) | #84 | **match** — DT-driven, no #defines | — |
| DT node bases nb=0xf0070000 ctrl=0xf0080000 phy=0xf0088000 | awto-alpine-v2-unvr-uboot.dts:141-144 | nb 0xf0070000 / uMCTL2 0xf0080000 / PUB 0xf0088000 | hardware.md:239-ish, uboot-ddr-port.md §4 | **match** | — |
| compat `annapurna-labs,al-ddr`, reg-names nb/ctrl/phy | ddr_cmd.c:29,34 | same in our dts | dts:140,144 | **match** (self-consistent w/ our HW-of-record dts) | — |
| `AL_DEV_ID=AL_DEV_ID_ALPINE_V2` (=1) | Makefile:22 | AL-324 is Alpine V2 | ddr_cmd.c:3-5; kernel alpine_hal.mk | **match** | — |
| `al_ddr_cfg_init(nb,ctrl,phy)`→`cfg_init_v3(...,ddrc=NULL,ch=0,...)` | al_hal_ddr.c:662-670 (identical copy) | same wrapper | delroth ddr/src/al_hal_ddr.c (byte-identical) | **match** | — |
| BIST `p.mode` default `LOOPBACK` (DRAM opt-in) | ddr_cmd.c:212 | HAL margins default `MODE_DRAM` | al_dram_margins.c:699 | **match** (ours is *safer* — loopback is PHY-internal) | — |
| BIST `p.pat = LFSR` | ddr_cmd.c:213 | enum `AL_DDR_BIST_PATTERN_LFSR` | al_hal_ddr.h:806 | **match** | — |
| BIST `p.inc = 8` | ddr_cmd.c:216 | `inc=8` "8 is the smallest increment" | al_dram_margins.c:700 | **match** | — |
| BIST `p.wc = 16` | ddr_cmd.c:214 | HAL: `wc = 2*(col_max/inc)`, cap `MAX_WC=0xFFFC` | al_dram_margins.c:708-710,34 | **fabricated** (no derivation) — but *valid*: multiple of 4, ≤ max | derive from geometry or doc loopback-only fixed value |
| BIST `p.col_max = 0x38` | ddr_cmd.c:217 | HAL: `col_max = num_columns - inc` | al_dram_margins.c:702,696 | **fabricated** (no source) — valid range; immaterial in loopback, weak window in DRAM mode | derive `num_columns-inc` (via `al_ddr_active_columns_get`) or doc as loopback token |
| BIST col_min/row/bank/rank = 0 | ddr_cmd.c:215-223 | HAL sets these from the addr under test; 0 = base | al_dram_margins.c:701-706 | **match** (defaults) | — |
| BIST `all_lanes_active=TRUE`, `active_byte_lanes` from `al_ddr_active_byte_lanes_get` | ddr_cmd.c:224-225 | same pattern | al_dram_margins.c:796-797 | **match** | — |
| BIST `inf=FALSE`, `sonf=FALSE` | ddr_cmd.c:227-228 | `inf=TRUE` path is the `while(1)` poll | al_hal_ddr_alpine_v2.c:316-341 | **match** — ours avoids the unbounded-poll branch | — |
| BIST call order: `_pre` → `_datx_bist` → `_post` (post always) | ddr_cmd.c:232-234 | `_pre` disables refresh-during-training (RFSHDT=0x0), `_post` restores (0x9) | al_hal_ddr_alpine_v2.c:82-161 | **match** (pairing correct; post unconditional) | — |
| `BIST_TIMEOUT` 1000 (1ms); on expiry HAL asserts `PIR_INIT|PHYRST` + re-inits PHY | al_hal_ddr_alpine_v2.c:56,561-579 (identical copy) | same | delroth al_hal_ddr_alpine_v2.c:56,561-579 | **match** to source — **but see MUST-FIX #1** (PHY reset on a live controller) | gate/doc; loopback timeout still resets PHY |
| `BIST_LFSR_RESET_VAL 0x1234ABCD`, `DEFAULT_TIMEOUT 5000` | al_hal_ddr_alpine_v2.c:58; al_hal_ddr_utils.h:59 | identical | delroth same | **match** | — |
| MR0..MR6 dump loop; ECC get/clear calls | ddr_cmd.c:122-124,155-176 | HAL `al_ddr_mode_register_get`, `al_ddr_ecc_*` (identical copy) | al_hal_ddr_alpine_v2.c | **match** | — |

## SerDes

| our symbol/value | our file:line | source-of-truth value | source file:line | VERDICT | fix |
|---|---|---|---|---|---|
| serdes PMA base + pcs base from DT reg-names (ofnode) | al_serdes_10g.c:93-112 | (requirement: not hardcoded) | #84 | **match** — DT-driven | — |
| DT `serdes`=0xfd8c0000/0x2400, `pcs`=0xfe120000/0x10000 | dts:161-163 | PMA @ 0xfd8c0000 size 0x2400; eth2 MAC @ 0xfe120000/0x10000 | hardware.md:239,287,344 | **match** | — |
| compat `annapurna-labs,al-serdes-25g`, reg-names serdes/pcs | al_serdes_10g.c:38; dts:160,163 | same in our dts | dts:160,163 | **match** (self-consistent) | — |
| `AL_SFP_LANE = LANE_0` | al_serdes_10g.c:44 | board wiring (LN0 vs LN1) unverified | — | **unverified** (disclosed "HW:" in code) | confirm SFP+ TX/RX lane on box |
| `mode_set_kr` → `group_cfg_10g_mode` → `group_mode=KR` | al_hal_serdes_25g.c:3297-3304,3379 | KR (10G) path, not 25G | delroth serdes/al_hal_serdes_25g.c (same) | **match** | — |
| 10G speed word `gen.ctrl = 0x901100` (speed field 9) | al_hal_serdes_25g.c:3035,3308 | `0x901100` non-25G; `0x201100` 25G; SPEED_10G=9 | delroth same:3035,3308 | **match** | — |
| `optic_tx_params` (amp 0x1, tdu 0x13, c+1 0x2, rest 0) | al_serdes_10g.c:68-76 | identical struct | al_init_eth_lm.c:484-492 | **match** (byte-for-byte) | HW retune per SFP/DAC (disclosed) |
| `optic_rx_params` (dfe_3db 0x7, 2nd-tap 0x8, 4th-tap 0x8, lfagc 0x7, hfboost 0x4, …) | al_serdes_10g.c:78-90 | identical struct | al_init_eth_lm.c:494-506 | **match** (byte-for-byte) | HW retune (disclosed) |
| `AL_PCS_KR_ADDR 0xa00`, `AL_PCS_KR_DATA 0xa04` | al_serdes_10g.c:53-54 | `kr` @ [0xa00]; `pcs_addr`[0x0], `pcs_data`[0x4] | al_hal_eth_mac_regs.h:657,444-448 | **match** | — |
| `AL_PCS_10G_LL_CFG 0xe38`, `AL_PCS_10G_LL_STATUS 0xe3c` | al_serdes_10g.c:55-56 | `gen_v3`@[0xe00]; `pcs_10g_ll_cfg`[0x38], `pcs_10g_ll_status`[0x3c] | al_hal_eth_mac_regs.h:661,576-578 | **match** | — |
| `AL_PCS_10G_LL_CFG_10GR = 0x00000050` | al_serdes_10g.c:61 | `pcs_10g_ll_cfg = 0x00000050` in `MODE_10GbE_Serial` | al_hal_eth_mac_v3.c:386,408 | **match** — but only 1 of ~13 writes in that mode (MUST-FIX #3) | — |
| `AL_PCS_10G_LL_STATUS_FEC_LOCKED = (1<<0)` | al_serdes_10g.c:57 | `..._FEC_LOCKED (1 << 0)` | al_hal_eth_mac_regs.h:1713 | **match** | — |
| `AL_KR_PCS_BASE_R_STATUS2 = 0x21` | al_serdes_10g.c:58 | `ETH_MAC_KR_PCS_BASE_R_STATUS2 0x00000021` | al_hal_eth_mac_regs.h:2077 | **match** | — |
| `AL_KR_PCS_BLOCK_LOCK = (1<<15)` | al_serdes_10g.c:59 | 10G/25G link read tests `reg & AL_BIT(15)` of BASE_R_STATUS2 | al_hal_eth_main.c (standalone):4304-4306 | **match** | — |
| `al_serdes_kr_pcs_read`: write reg→pcs_addr, read pcs_data, cast u16 | al_serdes_10g.c:144-148 | `al_eth_kr_pcs_reg_read`: same 2-step indirect, u16 cast | al_hal_eth_main.c (standalone):734-748 | **match** (no busy-poll/devad needed) | — |
| PCS reset settle `udelay(1)` | al_serdes_10g.c:139-140 | `AL_ETH_KR_PCS_RESET_DELAY 1 /*usec*/`, used post-reset | al_eth-standalone al_hal_eth_main.c:425,1630 | **match** (cited symbol exists in al_eth standalone, not the alpine_hal tree) | — |
| `pcs` base 0xfe120000 assumed == MAC `mac_regs_base` (kr/gen_v3 offsets are relative) | al_serdes_10g.c:34-36; dts:162 | offsets valid only if 0xfe120000 is the MAC reg file base, not an adapter super-block | al_hal_eth_mac_regs.h struct base | **unverified** (disclosed "HW:") | confirm PCS/MAC base on box, reconcile w/ MAC agent |
| SerDes internal timeouts: INIT 10000us(10ms), RESET_DELAY 10us, GCFSM2/LEQ 2000000us(2s) | al_hal_serdes_25g.c:2187-2188,1299,1425 (identical copy) | identical | delroth serdes/al_hal_serdes_25g.c | **match** (HAL source; 2s polls are large round numbers but source-faithful) | — |
| 10 dropped register-read error checks + `*data=0` init + `al_assert` in our 25g copy | al_hal_serdes_25g.c (diff vs source) | source returns on read error / zero-inits `*data` | delroth serdes/al_hal_serdes_25g.c:117,1592-1602,1745-1751,2271-2291,2367-2388 | **regression** (values unchanged; error paths removed) — MUST-FIX #4 | restore the return-on-error / `*data=0` |

## MUST-FIX shortlist (ranked: wedge/hang/corrupt first)

1. **DDR `ddr bist` timeout resets the LIVE PHY.** On a BIST-done timeout the HAL path
   `al_ddr_phy_wait_for_bist_done` asserts `PIR_INIT | PHYRST` and re-runs PHY init
   (al_hal_ddr_alpine_v2.c:567-579). This command runs against the *already-trained controller that
   is serving U-Boot's own DRAM*. A PHY reset mid-run can corrupt/wedge DRAM. Loopback makes a timeout
   unlikely but not impossible (a glitch/wedge still trips it). **Action:** gate `ddr bist` behind an
   explicit confirm, and document that a timeout resets the PHY; ideally verify the reset branch is
   unreachable in loopback before shipping. (This is HAL behavior we *invoke*, not a fabricated value —
   but it is the top "could corrupt DDR" risk in scope.)

2. **DDR BIST params `wc=16` / `col_max=0x38` are fabricated (not derived).** Harmless in loopback
   (PHY-internal, addresses immaterial). In `ddr bist dram` they drive the real array over a tiny fixed
   column window with only 16 words — weak coverage, and the fixed values aren't checked against the
   live geometry the way the HAL derives them (`col_max=num_columns-inc`, `wc=2*(col_max/inc)`).
   **Action:** derive both from `al_ddr_active_columns_get` per al_dram_margins.c:696-710, or document
   them as loopback-only tokens and reject/adjust for DRAM mode.

3. **Standalone 10GBASE-R PCS bring-up is incomplete → can never block-lock alone.** `al_serdes_pcs_10gr_config`
   writes only `pcs_10g_ll_cfg=0x50`. The HAL `MODE_10GbE_Serial` path also programs 5×TX + 3×RX ASYNC-FIFO
   configs, `mac_sel`, `mac_10g_ll_cfg/ctrl`, ASYNC-FIFO enable, and a gearbox reset
   (al_hal_eth_mac_v3.c:386-409). Without them `serdes status` will report `pcs_block_lock: no`
   regardless of the lane — misleading, though read-mostly and not a wedge. **Action:** keep as-is only
   if the MAC agent owns PCS; otherwise complete the sequence. Already disclosed in-code — surface it so
   `serdes status` output isn't misread as a lane fault.

4. **Our `al_hal_serdes_25g.c` copy silently dropped 10 register-read error checks** (plus a `*data=0`
   init and `al_assert`s). A failed TBUS/internal-register read now returns stale/garbage instead of an
   error, so `serdes status` can report false PLL/CDR/signal state. Read-mostly (no wedge). **Action:**
   restore the return-on-error paths and the `*data=0` init to match source.

5. **`pcs` base 0xfe120000 assumed == MAC register-file base.** The kr(0xa00)/gen_v3(0xe38/0xe3c)
   offsets are correct *relative to `mac_regs_base`*; they're only absolutely correct if 0xfe120000 is
   that base and not an enclosing adapter super-block. Disclosed "HW:" in code. **Action:** confirm on
   the box and reconcile with the eth/MAC agent's node before trusting PCS reads.

## Counts
- **Mismatches (value contradicts source): 0** — no register offset/bitmask/mode constant is wrong.
- **Fabricated-no-source: 2** — DDR BIST `wc=16` and `col_max=0x38` (both valid-but-underived; loopback-safe, weak in DRAM mode).
- **Behavioral/quality risks: 3** — live-PHY reset on BIST timeout (#1), incomplete standalone PCS (#3), dropped SerDes error handling (#4). Plus 2 disclosed HW-iteration unknowns (SFP lane, PCS/MAC base).
- No analogue of the al_eth ring-size bug (a constant *below* a HAL minimum) was found in DDR/SerDes.
