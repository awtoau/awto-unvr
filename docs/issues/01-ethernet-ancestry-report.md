# [FIRST TASK] Ethernet ancestry report — al_eth vs Altera / NXP / DesignWare
labels: porting, ethernet, phase-3, first-task
---

**This is the immediate next step and it is NOT coding.** It decides the whole
Ethernet driver strategy (extend vs extract vs clean front-end). Without it,
"reuse NXP and Altera" is a hypothesis, not a safe decision.

Roadmap: [Phase 3](../porting-roadmap.md#phase-3--ethernet-ancestry-test-before-choosing-a-driver)
and [FIRST TASK](../porting-roadmap.md#first-task--ethernet-ancestry-report).

## Deliverable
An automated register & descriptor comparison report (script in `scripts/`, output
in `tmp/logs/`) between:
- Annapurna `al_eth` + `al_hal_eth` (UrNVR/unvr-kernel 4.19.152; UDM 1.12.22;
  delroth/al_eth-standalone),
- mainline **Altera TSE / mSGDMA**,
- mainline **NXP DPAA / DPAA2 / FMan**,
- mainline **Synopsys DesignWare Ethernet** (stmmac),
- Linux 6.18 implementations of the above.

## Compare mechanically
- [ ] Register offsets & bit meanings
- [ ] Descriptor formats
- [ ] Ring producer/consumer semantics
- [ ] Interrupt registers
- [ ] Reset sequence
- [ ] MDIO registers
- [ ] MAC stats layout
- [ ] Checksum / TSO / RSS metadata
- [ ] PCS/SerDes interface
- [ ] 1G RGMII vs 10G SFP+ datapaths

## Live constraints the driver must satisfy
- Binds by **PCI ID** (`1c36:0001` 1G, `1c36:0002` 10G), not DT — vendor driver
  logs "Unable to find compatible OF node".
- **Shared MDIO** — one bus for both MACs.
- 1G = RGMII → Atheros AR8031 (addr 4); 10G = SFP+ optic, LM mode, no ext PHY.
- Evidence: [dmesg.txt](../hw-reference/20260816-104601/dmesg.txt),
  [lspci.txt](../hw-reference/20260816-104601/lspci.txt).

## Acceptance criteria
- [ ] Report names one outcome with evidence: **A** register-compatible (extend via
  `of_device_id` match data + per-variant layouts + hw-op structs, no duplicated
  datapath); **B** shared DMA / different MAC-PCS (extract common DMA-ring lib,
  separate MAC front-ends); **C** only concepts similar (clean modern al_eth on
  phylink/phylib/SFP/page_pool/NAPI/ethtool-netlink).
- [ ] Explicitly rules out copy-a-whole-Altera/NXP-driver-and-change-offsets unless
  the relationship is demonstrated.
- [ ] Filed as a finding doc (not just chat).

Blocks: #04, #15.
