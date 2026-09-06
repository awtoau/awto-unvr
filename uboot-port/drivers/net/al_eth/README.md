# alu_eth — Annapurna Labs Ethernet for modern U-Boot

DM_ETH drivers for the UNVR's two al_eth ports, written against the **shared
Linux HAL** — `modules/al_eth/`, staged into the build tree by
`scripts/stage_hal.py`. There is no HAL copy in this directory (#256).

| port | PCI id | media | front end |
|---|---|---|---|
| eth1 | `1c36:0001` | 1G RJ45, RGMII | AR8033 @ MDIO addr 4, via phylib |
| eth2 | `1c36:0002` | 10G SFP+ | HSSP SerDes lane + 10GBASE-R PCS, no PHY |

## Why one HAL

U-Boot and UEFI kept re-hitting bugs Linux had already fixed. Every fix in
`modules/al_eth/` — `__must_check` on the HAL entry points, the MDIO BUSY race,
the eye-size MSB, the `sch_mode` field, the V3-only 40G path on rev-2 silicon,
clear-on-read counters, board-param-preserving FLR — is now a fix here too,
with nothing to re-apply.

## Layout

- `alu_eth.h` — the glue's one header: ports, MAC addresses, board params.
- `alu_eth_core.[ch]` — rings, adapter init, FLR, send, recv, cache
  maintenance, unit-adapter setup. Both ports share all of it.
- `alu_eth_1g.c` / `alu_eth_10g.c` — the front ends, ~200 lines each.
- `alu_eth_port.c` — port index → PCI function → the three BARs.
- `alu_eth_boardparams.c` — DT `board-cfg` → the MAC scratchpad Linux reads.
- `alu_eth_hwaddr.c` — SPI-NOR base MAC → per-port address → EC filter.
- `alu_eth_rxfwd.c` — EC RX forwarding to UDMA0/Q0.
- `alu_eth_stats.c` / `alu_eth_diag.c` — the `eth` command.

`al_*` is the shared HAL; `alu_*` is this glue. A symbol's prefix says which
tree owns it.

## Binding + register windows (PCI, not DT)

Both ports are PCI endpoints on the internal PCIe, bound by `U_BOOT_PCI_DEVICE`
— the bare `eth0..3` platform nodes at `0xfc000000+` in the stock DT are unused
(`docs/hardware.md`). Three **non-contiguous** BARs, so there is no single base
plus offsets, and the order is not ascending:

| BAR | window | vendor macro |
|-----|--------|--------------|
| 0 | UDMA regs | `AL_ETH_UDMA_BAR` |
| 2 | MAC regs | `AL_ETH_MAC_BAR` |
| 4 | EC regs | `AL_ETH_EC_BAR` |

## Board facts that cost real debugging

- **`PHY_INTERFACE_MODE_RGMII_ID`, not `RGMII`.** Without the AR8033's internal
  RX/TX clock delays the MAC samples RX on the wrong edge and drops every frame
  — 213 in, 213 `ifInErrors`, 0 FCS errors (`30e7c65`). Linux uses `RGMII_ID`
  here too.
- **FLR must preserve board params + the EC MAC.** Raw `al_eth_flr_rmn()` wipes
  both; `al_eth_flr_rmn_restore_params()` is the wrapper that does not (#253).
- **MAC derivation: base+0 = 1G, base+1 = 10G.** Stock's printed `[2]` is a
  COUNT of allocated addresses, not an offset (#222/#223).
- **EC counters are clear-on-read** (`5be0d9b`); `eth stats` accumulates them.
  The Clause-49 PCS counters have exactly one reader, `al_eth_link_status_get()`
  (#196).
- **al_udma-visible memory must be low DRAM.** The master decodes only the low
  window, and U-Boot's heap relocates to the top of the 3GB bank (#90).

## Diagnostics

- `eth diag [<port>]` — PCI BDF, the three BARs, MAC + its source, board params
  decoded, and link state.
- `eth stats [<port>]` — every MAC/EC/UDMA counter, drops first.
- `CONFIG_AL_ETH_DEBUG` — `-DDEBUG` across al_eth and al_serdes: the full HAL
  register trace, thousands of lines per boot.
