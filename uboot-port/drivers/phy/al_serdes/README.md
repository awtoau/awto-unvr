# al_serdes — AL 25G SerDes + 10GBASE-R PCS (UNVR SFP+, fixed 10G)

Brings up the Annapurna Labs (AL-324 / Alpine V2) 25G SerDes lane for the UNVR's
10G SFP+ port (`eth2`) at a **fixed 10.3125 Gbps** (10GBASE-R), **KR auto-neg +
link-training DISABLED** (mode `10G_OPTIC` / passive-DAC). SerDes/PCS half only —
the MAC/UDMA come from the `al_eth` driver (separate effort). Part of #83 "later: 10G".

The group-D retarget is hardware-verified (eac280a). The EQ tap values are not —
see **Open** below.

## Files

Authored (Awto, GPLv2-or-later; on U-Boot primitives):
- `al_serdes_10g.c` / `.h` — DT base lookup, fixed-10G init,
  `serdes` U-Boot command.
- `al_hal_plat_services.h`, `al_hal_plat_types.h` — U-Boot shims for the HAL.
  Byte-identical to the DDR port's shims — **dedupe at merge** (move to a shared
  `board/annapurna/alpine/al_hal/` or similar).

Vendor HAL, verbatim (Annapurna Labs, GPLv2 OR BSD-3-Clause — headers untouched):
- `al_hal_serdes_hssp.c` — HSSP group HAL (#111): the REAL driver for group D
  (the UNVR SFP+ 10G lane). Ported byte-identical from `modules/al_eth/` (the
  Linux driver tree) — no adaptation was needed to compile under U-Boot.
- `al_hal_serdes_hssp.h`, `al_hal_serdes_hssp_regs.h`, `al_hal_serdes_hssp_internal_regs.h`
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
- Fixed 10G = the HAL's "KR" electrical mode (10.3125G NRZ, `gen.ctrl` speed
  field 9). Clause-73 AN / Clause-72 LT live in the MAC-KR FSM and are **never
  invoked** — the lane comes up at a fixed rate by construction.

## Command

    serdes init     # configure the lane for fixed 10GBASE-R (KR/AN/LT off)
    serdes status   # group-D gen.version + PMA signal-detect
    serdes tx       # show the optic TX equaliser params (set one to re-init)
    serdes          # init then status

PCS link state and the taps **actually in force** are in `eth diag 2`
(`drivers/net/al_eth/alu_eth_diag.c`) — the MAC driver owns the PCS.

## On-box verification (the human test)

After `serdes init`, `serdes status` prints (HSSP group D, lane 0):
- **gen.version** — group register read, proves the PMA window is mapped.
- **signal_detect** — PMA rxrandet status (RX signal present).

PCS link state is `eth diag 2`: link up/down, local/remote fault,
`errored_blocks` / `ber_count` (clear-on-read), taps in force. There is no
separate block-lock readout.

## Open

- **Optic TX/RX EQ VALUES** — applied for real (#111, via the HSSP vtable), but
  the values are the vendor `10G_OPTIC` defaults with the #121 `c_plus_1`
  change. Whether they suit this board's optic is open (#207); the HAL's
  `rx_equalization` sweep is the retune path. `eth diag 2` reads back what
  landed, so an apply that silently failed is visible.
- A fresh (non-chainloaded) bring-up may need clock/PBS init first — today
  preboot has already done it, so `al_serdes_10g_init()` only reads.

Settled: the lane is group D (`serdes-grp 3`) lane 0 per the DT, confirmed by
the retarget that stopped the SError (eac280a). The PCS belongs to the MAC
driver, not this one.
