# [Phase 5] 10GbE, PCS, SerDes — move link management to phylink
labels: porting, ethernet, phase-5, sfp
---

Do not leave link management inside al_eth. Move to standard frameworks.
Roadmap: [Phase 5](../porting-roadmap.md#phase-5--10gbe-pcs-serdes).

## Target
- [ ] phylink for link state
- [ ] mainline SFP cage support (module EEPROM/hotplug)
- [ ] `phylink_pcs` for the MAC/PCS boundary
- [ ] small Alpine SerDes/PCS provider only for genuinely proprietary ops
- [ ] DT properties for lane/mode/polarity

## Live evidence
- DT `serdes` @ `0xfd8c0000`; board-cfg serdes **group3 = 10gbe, lane 0,
  156.25 MHz**; port2 `serdes-grp 3 serdes-lane 0`, `10g-serial force-1000base-x`.
- LM mode observed `AL_ETH_LM_MODE_10G_OPTIC` (SFP optic, no ext PHY, mdio 2500kHz).
- Retimer `br410` present but **disabled** in DT; UNVR al_eth tree carries
  `al_mod_eth_lm_retimer_ds125.c` — a straight swap to the UDM al_eth revision is
  **unsafe** ([docs/porting-reference.md](../porting-reference.md)).
- **SFP module EEPROM NOT captured** (`ethtool` "No data available") — decode on
  hardware before finalising. Evidence:
  [dmesg.txt](../hw-reference/20260816-104601/dmesg.txt),
  [live.dts](../hw-reference/20260816-104601/live.dts).

## Acceptance criteria
- [ ] 10G SFP+ link comes up via phylink + SFP + phylink_pcs.
- [ ] Vendor SerDes/PCS reuse only after matching PCS/SerDes registers.

Depends: #04, #16.
