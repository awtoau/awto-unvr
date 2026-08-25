# SFP+ cage + optic (module confirmed: Finisar FTLX8571D3BCL)

Record the **cage** (on-board connector) and the **module** (removable optic) separately.

## SFP+ cage (on-board)

- **Function:** 1× 10G SFP+ cage for the 10G port (enp0s2 / al_eth2).
- **Confirmed:** dmesg — port2 `phy No, addr 0, mdio 2500 kHz, SFP Yes, media 5`; LM mode `AL_ETH_LM_MODE_10G_OPTIC`. ✅ live. **No external PHY** — the MAC talks SerDes straight to the optic.
- **MAC:** on-SoC al_eth port 2 (`1c36:0002`). SerDes @ `0xfd8c0000` — [hardware.md#mmio-and-address-map](../hardware.md#mmio-and-address-map).
- Passive part; no datasheet needed.

## Inserted module (optic) — CONFIRMED

- **Part: Finisar FTLX8571D3BCL** — 10G-SR SFP+ optic. Read live via direct I²C EEPROM
  read (not `ethtool -m` — stock 4.19.152 gave "No data available", no SFF-8472
  diagnostics path in that kernel).
- **I²C:** 0x50 = SFF-8079 base EEPROM (ID/vendor/part/serial), 0x51 = SFF-8472 DDM
  (temp/Vcc/TX/RX power) — both behind [PCA9546A](pca9546a.md) mux **channel 1**
  (canonical map: [../i2c-map.md](../i2c-map.md)).
- **Type:** 10G optic (`10G_OPTIC` LM mode in al_eth). Third-party optic works fine in
  the cage → the port does not enforce a vendor lock at the optic level.

## Datasheet

- **None saved yet.** Fetch the Finisar FTLX8571D3BCL datasheet if DDM register
  offsets/scaling are needed beyond the standard SFF-8472 layout.

## RE / repurpose notes

- Stock kernel's `ethtool -m` path gives no DOM — a direct I²C EEPROM read (mux ch1,
  0x50/0x51) is the working path; a mainline sfp/phylink kernel would also expose it
  via `ethtool -m`.
