# [Patch 10] UNVR Ethernet & SFP+ device tree
labels: porting, devicetree, ethernet, sfp, patch
---

Board-level DT describing the UNVR's two network ports and the SFP cage.
Roadmap: [patch order](../porting-roadmap.md#patch-order).

## Tasks
- [ ] 1G port: RGMII, PHY handle to AR8031 @ addr 4 on the shared MDIO bus.
- [ ] 10G port: `sfp` cage node, phylink managed, serdes group3 lane 0, 156.25MHz.
- [ ] SFP cage GPIOs (mod-def0 / los / tx-fault / tx-disable) — confirm mapping on
  hardware (pca9575 expanders / SGPO).
- [ ] i2c bus for SFP module EEPROM.

## Live evidence
- port1 rgmii ext_phy addr 4; port2 auto-detect, `serdes-grp 3`, DAC len 3,
  `force-1000base-x`, `sfp_1g` LED via pca9575 line
  ([live.dts](../hw-reference/20260816-104601/live.dts)).
- Vendor DT uses non-mainline `board-cfg/ethernet` — **do not copy**; express via
  standard phylink/SFP DT bindings.

## Acceptance criteria
- [ ] Both ports come up from DT alone (no hardcoded board data) under #15/#16.

Depends: #13, #16.
