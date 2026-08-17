# SFP+ cage + optic (module UNCONFIRMED)

Record the **cage** (on-board connector) and the **module** (removable optic) separately.

## SFP+ cage (on-board)

- **Function:** 1× 10G SFP+ cage for the 10G port (enp0s2 / al_eth2).
- **Confirmed:** dmesg — port2 `phy No, addr 0, mdio 2500 kHz, SFP Yes, media 5`; LM mode `AL_ETH_LM_MODE_10G_OPTIC`. ✅ live. **No external PHY** — the MAC talks SerDes straight to the optic.
- **MAC:** on-SoC al_eth port 2 (`1c36:0002`). SerDes @ `0xfd8c0000` — [hardware.md#mmio-and-address-map](../hardware.md#mmio-and-address-map).
- Passive part; no datasheet needed.

## Inserted module (optic) — UNCONFIRMED

- **Known:** a **third-party Intel** SFP+ module is fitted (per project notes). Exact model/vendor **not captured** — `ethtool -m` returned "No data available" on the stock 4.19.152 kernel (no SFF-8472 diagnostics path).
- **Type:** behaves as a 10G optic (`10G_OPTIC` LM mode). Could be SR/LR fibre or a 10GBASE optic; not distinguished yet.

## How to confirm the module

1. **`ethtool -m enp0s2`** on a kernel with SFP/SFF-8472 support (mainline sfp phylink, or a build with the diagnostic read) — dumps vendor, part, serial, wavelength, DOM.
2. **Read the module EEPROM directly** at I²C **0x50** (SFF-8472 base) / **0x51** (DOM) on the cage's I²C — likely behind the [PCA9546A](pca9546a.md) mux; find the segment first.
3. **Physical label** on the module body (vendor + P/N).

## Datasheet

- **None** — module unidentified. Once ethtool/EEPROM names the part, fetch that stock optic datasheet.

## RE / repurpose notes

- Stock kernel gives no DOM — closing this needs either a mainline sfp/phylink kernel or a direct I²C EEPROM read.
- Third-party optic already works in the cage → the port does not enforce a vendor lock at the optic level.
