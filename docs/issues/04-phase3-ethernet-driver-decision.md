# [Phase 3] Ethernet driver decision (extend / extract / clean front-end)
labels: porting, ethernet, phase-3
---

Turn the ancestry report (#01) into a chosen driver architecture.
Roadmap: [Phase 3](../porting-roadmap.md#phase-3--ethernet-ancestry-test-before-choosing-a-driver).

## Decision
- [ ] **A** register-compatible → add Alpine support to the existing mainline driver
  via `of_device_id` match data, per-variant register layouts/caps, small hw-op
  structs, **no duplicated datapath**.
- [ ] **B** shared DMA, different MAC/PCS → extract/extend a common DMA-ring library,
  separate MAC front-ends.
- [ ] **C** only concepts similar → write a modern al_eth around
  phylink/phylib/SFP/page_pool/NAPI/ethtool-netlink/XDP-later/DT topology.

## Constraints
- Remove old al_eth's hardcoded board data, internal SerDes impl, obsolete ethtool.
- Copying a whole Altera/NXP driver and changing offsets is **worse** than a clean
  driver unless the relationship is demonstrated (#01).
- Must handle: PCI-ID bind, shared MDIO, 1G RGMII (AR8031) + 10G SFP+ (LM/optic).

## Acceptance criteria
- [ ] One outcome (A/B/C) chosen, with the ancestry evidence cited.
- [ ] A concrete module/file layout agreed before #15 starts.

Depends: #01. Blocks: #15, #16.
