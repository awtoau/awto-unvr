# [Patch 7] Ethernet DMA core / modern al_eth
labels: porting, ethernet, patch
---

Implement the Ethernet driver per the Phase 3 decision.
Roadmap: [patch order](../porting-roadmap.md#patch-order),
[Phase 3](../porting-roadmap.md#phase-3--ethernet-ancestry-test-before-choosing-a-driver).

## Scope (depends on #04 outcome A/B/C)
- [ ] MAC + DMA datapath (extend existing / extract common ring lib / clean driver).
- [ ] PCI-ID bind (`1c36:0001` 1G, `1c36:0002` 10G) + `of_device_id` match data for
  board params (replaces hardcoded board data).
- [ ] Shared MDIO bus (one bus, both MACs).
- [ ] NAPI + page_pool; modern ethtool (netlink).
- [ ] 1G RGMII path (AR8031); hand 10G link mgmt to phylink (#16).

## Live evidence
- `al_eth v3.5.3`; 1G RGMII → AR8031 addr 4; 10G SFP+ optic, no ext PHY.
  ([dmesg.txt](../hw-reference/20260816-104601/dmesg.txt),
  [lspci.txt](../hw-reference/20260816-104601/lspci.txt))
- `at803x` `depends on REGULATOR` → ensure `CONFIG_REGULATOR=y` (#21).

## Acceptance criteria
- [ ] 1G iface passes traffic under 6.18.
- [ ] No duplicated datapath if outcome A; clean phylink/phylib boundary.

Depends: #01, #04, #14. Blocks: #16.
