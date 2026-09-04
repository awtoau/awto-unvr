# al_eth — Annapurna Labs 1G RGMII Ethernet for modern U-Boot (#83)

DM_ETH driver + curated Annapurna HAL subset for the UNVR's **1G RJ45** (eth1,
`ethernet@fc100000`, PCI `1c36:0001`, RGMII, AR8033 PHY). **RGMII 1G only** — no
serdes / KR / PCS / link-management (that is the 10G SFP+ eth2 path, out of scope).

## Provenance
- **HAL:** `delroth-alpine_hal` @ eb6b9f1 — files copied verbatim into `hal/`,
  original Annapurna Labs headers (GPLv2 OR BSD-3-Clause) preserved untouched.
- **Glue reference:** vendor U-Boot 2015 `al_eth.c` (UNVR-1.3.35-GPL) — its
  init/send/recv HAL call sequence, re-expressed as DM `eth_ops` in `al_eth_dm.c`.
- **Register map:** `platform/alpine_v2` `al_hal_iomap.h`.

## Layout
- `al_eth_dm.c` — the DM_ETH driver (UCLASS_ETH). New file (Awto).
- `al_eth_stubs.c` — MAC v3/v4 (10G/25G) handle-init stubs; the rev-id dispatch in
  `al_hal_eth_mac.c` references them but RGMII (rev 2) never calls them, so we stub
  rather than drag in the serdes closure. New file (Awto).
- `shim/al_hal_plat_services.h`, `shim/al_hal_plat_types.h` — the **plat_api shim**:
  maps the HAL's platform services onto U-Boot primitives (regs→read/write*,
  udelay/mdelay, dsb/dmb barriers, printf, string/mem). Header-only (all static
  inline / macros), so no shim `.c`. Reconcile with the #80 DDR agent's shim — this
  layer is generic (no eth deps). New files (Awto).
- `hal/include/` — flattened HAL headers (one `-I`).
- `hal/{eth,udma,iofic}/` — curated HAL `.c` subset (copied verbatim).

## HAL .c subset compiled (RGMII MAC + UDMA + MDIO)
udma: `al_hal_udma_main.c`, `al_hal_udma_config.c`, `al_hal_udma_iofic.c`;
iofic: `al_hal_iofic.c`;
eth: `al_hal_eth_main.c`, `al_hal_eth_mac.c`, `al_hal_eth_mac_internal.c`,
`al_hal_eth_mac_v1_v2.c`, `al_hal_eth_common.c`, `al_hal_eth_epe.c`,
`al_hal_eth_field.c`, `al_hal_eth_rfw.c`.
**Excluded** (10G/25G/serdes): `al_hal_eth_mac_v3.c`, `al_hal_eth_mac_v4.c`,
`al_hal_eth_kr.c`, all `serdes/`, and the `services/eth` LM/KR/retimer layer.
`AL_ETH_EX` is left **undefined** — the extended/ex-internal (serdes-adjacent)
code paths compile out.

## Binding + register windows (PCI, not DT)
al_eth is a **PCI-enumerated endpoint** on the internal PCIe (bus 0). The driver
binds by **PCI ID** (`U_BOOT_PCI_DEVICE`, vendor `0x1c36` dev `0x0001` = eth1 1G),
**not** a DT compatible — the bare `eth0..3` platform nodes at `0xfc000000+` in the
stock DT are unused (docs/hardware.md). It maps **three separate BARs** (the UDMA
and MAC live in non-contiguous windows; there is no single base + offset):

| BAR | reg offset | window | vendor macro |
|-----|-----------|--------|--------------|
| BAR0 | `PCI_BASE_ADDRESS_0` | UDMA regs | `AL_ETH_UDMA_BAR = 0` |
| BAR4 | `PCI_BASE_ADDRESS_4` | EC regs   | `AL_ETH_EC_BAR   = 4` |
| BAR2 | `PCI_BASE_ADDRESS_2` | MAC regs  | `AL_ETH_MAC_BAR  = 2` |

(Indices from the vendor `al_hal_eth.h`, matched by `drivers/net/al_eth_pci.c`.
Note EC=BAR4 / MAC=BAR2, i.e. **not** ascending.) Probe enables `PCI_COMMAND_MEMORY
| PCI_COMMAND_MASTER` for DMA. PHY details (RGMII, at803x @ addr 4, MDIO 1000 kHz,
ref clk 500 MHz) are set **explicitly** — not read from the MAC scratchpad
(`al_eth_board_params_get`) nor DT — robust for chainload and standalone.

## DMA coherency
Relies on the **#74 AXI SMCC snoop fix** (applied in `board_late_init`) that makes
the internal-PCIe units cache-coherent — so no explicit cache flush/invalidate,
same as the vendor driver on this coherent SoC. If HW bring-up shows stale
descriptors/buffers, add flush(TX submit)/invalidate(RX consume).

## Diagnostics
- `eth diag [<port>]` — PCI BDF, the three BARs, MAC + its source, board params
  decoded, and link state (1G: PHY id + AN result; 10G: PCS block-lock, SerDes
  grp/lane, TX equalisation taps in force). Read-only; safe on a live port.
- `eth stats [<port>]` — every MAC/EC/UDMA counter, drops first.
- `CONFIG_AL_ETH_DEBUG` — `-DDEBUG` across al_eth + al_serdes: the full HAL
  register trace. Off by default; thousands of lines per boot.

## Status
Compile-clean, wired to the HAL. Traffic blocked on #90 (UDMA M2S descriptor
read hangs, both ports). Board params confirmed (PHY addr 4, MDIO 1000 kHz, ref
clk 500 MHz, RGMII). The TX completion poll bound is still ~8000x worst case —
it cannot be tightened until #90 makes a completion observable.
