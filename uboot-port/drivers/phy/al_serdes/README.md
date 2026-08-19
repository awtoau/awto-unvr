# al_serdes — AL 25G SerDes + 10GBASE-R PCS (UNVR SFP+, fixed 10G)

Brings up the Annapurna Labs (AL-324 / Alpine V2) 25G SerDes lane for the UNVR's
10G SFP+ port (`eth2`) at a **fixed 10.3125 Gbps** (10GBASE-R), **KR auto-neg +
link-training DISABLED** (mode `10G_OPTIC` / passive-DAC). SerDes/PCS half only —
the MAC/UDMA come from the `al_eth` driver (separate effort). Part of #83 "later: 10G".

**Compile-verified only.** A human iterates the real lane bring-up on the box.

## Files

Authored (Awto, GPLv2-or-later; on U-Boot primitives):
- `al_serdes_10g.c` / `.h` — DT base lookup, fixed-10G init, 10GBASE-R PCS,
  `serdes` U-Boot command.
- `al_hal_plat_services.h`, `al_hal_plat_types.h` — U-Boot shims for the HAL.
  Byte-identical to the DDR port's shims — **dedupe at merge** (move to a shared
  `board/annapurna/alpine/al_hal/` or similar).

Vendor HAL, verbatim (Annapurna Labs, GPLv2 OR BSD-3-Clause — headers untouched):
- `al_hal_serdes_25g.c` — 25G/10G SerDes group config + lane calibration + status.
- `al_hal_serdes_25g.h`, `al_hal_serdes_25g_regs.h`, `al_hal_serdes_25g_internal_regs.h`
- `al_hal_serdes_interface.h` — the `al_serdes_grp_obj` vtable + enums/params.
- `al_hal_common.h`, `al_hal_types.h`, `al_hal_reg_utils.h` — HAL common layer.

Provenance: `delroth-al_eth-standalone/src/` (byte-identical to the
`mornepousse-al_eth-standalone` and Ubiquiti GPL-kernel `al/` copies; delroth
alpine_hal @ eb6b9f1). Only `al_hal_plat_{services,types}.h` were replaced with
U-Boot shims; every other vendor file is unmodified.

## Design

- No hardcoded MMIO bases — all from the device tree (`docs/hardware.md` is the
  hardware-of-record). Node `serdes@fd8c0000`, compatible
  `annapurna-labs,al-serdes-25g`, read by reg-name via `ofnode`:
  - `serdes` — 25G SerDes PMA (`0xfd8c0000`, size `0x2400`). Required.
  - `pcs` — eth2 MAC-adapter window holding the 10GBASE-R PCS sub-block.
    Optional; **shared with the eth/MAC agent's node**.
- Fixed 10G = the HAL's "KR" electrical mode (10.3125G NRZ, `gen.ctrl` speed
  field 9). Clause-73 AN / Clause-72 LT live in the MAC-KR FSM and are **never
  invoked** — the lane comes up at a fixed rate by construction.
- PCS offsets are transcribed from `al_hal_eth_mac_regs.h` (KR-PCS indirect
  `kr.pcs_addr`/`pcs_data` @ `0xa00/0xa04`; `gen_v3.pcs_10g_ll_cfg`/`_status` @
  `0xe38/0xe3c`) so the MAC HAL is **not** pulled in.

## Command

    serdes init     # configure the lane for fixed 10GBASE-R (KR/AN/LT off)
    serdes status   # read PLL lock / signal-detect / CDR lock / rx-valid / PCS block-lock
    serdes          # init then status

## On-box verification (the human test)

After `serdes init`, `serdes status` reads (via the HAL vtable + PCS regs):
- **PLL lock** — `al_serdes_25g_pll_lock_get` (SerDes CMU PLL).
- **signal-detect** — `al_serdes_25g_signal_is_detected(lane)` (RX signal present).
- **CDR lock** — `al_serdes_25g_cdr_is_locked(lane)`.
- **rx-valid** — `al_serdes_25g_rx_valid(lane)`.
- **PCS block-lock** — KR-PCS BASE-R Status 2 (reg `0x21`) bit 15, via the
  indirect `kr.pcs_addr/data`. This is the 10GBASE-R link-up gate.

Good link with an optic/DAC inserted: PLL LOCKED, signal yes, CDR LOCKED,
rx-valid yes, **pcs_block_lock LOCKED**.

## Hardware-iteration points (tagged `HW:` in the source)

- **Lane index** (`AL_SFP_LANE`, default `LN0`) — confirm which physical 25G
  lane the SFP+ TX/RX is wired to.
- **Optic TX/RX EQ params** — vendor optic defaults; retune per the real SFP/DAC
  (the HAL's `rx_equalization` sweep).
- **PCS base + reset ordering** — the `pcs` window base (`docs/hardware.md` lists
  eth2 @ `0xfe120000`; the task brief said `0xfc200000` — resolve on the box) and
  the PCS-vs-MAC reset sequencing are owned by the MAC agent; reconcile at merge.
- The `mode_set_kr` group-config polls PLL/lane-OK with the HAL's own timeouts;
  a fresh (non-chainloaded) bring-up may need extra clock/PBS init first.
