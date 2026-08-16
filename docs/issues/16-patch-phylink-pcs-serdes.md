# [Patch 8-9] MDIO/phylink integration + Alpine PCS/SerDes
labels: porting, ethernet, sfp, patch
---

The two link-layer patches (order #8 MDIO/phylink, #9 Alpine PCS/SerDes). Implements
Phase 5 in code.
Roadmap: [Phase 5](../porting-roadmap.md#phase-5--10gbe-pcs-serdes),
[patch order](../porting-roadmap.md#patch-order).

## Patch 8 — MDIO / phylink
- [ ] Register the shared MDIO bus; AR8031 (addr 4) via `at803x`.
- [ ] phylink for link state on both ports.
- [ ] mainline SFP cage support (module EEPROM + hotplug) for the 10G port.

## Patch 9 — Alpine PCS/SerDes provider
- [ ] `phylink_pcs` for the MAC/PCS boundary.
- [ ] Small Alpine SerDes/PCS provider for genuinely proprietary ops only.
- [ ] DT props: lane/mode/polarity (serdes group3 = 10gbe lane0 156.25MHz).

## Live evidence / caution
- LM mode `AL_ETH_LM_MODE_10G_OPTIC`; retimer `br410` disabled in DT but UNVR tree
  ships `al_mod_eth_lm_retimer_ds125.c` — do not blind-swap to UDM al_eth
  ([docs/porting-reference.md](../porting-reference.md)).
- SFP module EEPROM NOT captured — decode on hardware first.
- Evidence: [live.dts](../hw-reference/20260816-104601/live.dts),
  [dmesg.txt](../hw-reference/20260816-104601/dmesg.txt).

## Acceptance criteria
- [ ] 10G SFP+ link up via phylink + SFP + phylink_pcs; hotplug works.
- [ ] Vendor SerDes code reused only after register match.

Depends: #15. Relates: #06.
