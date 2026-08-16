# Issue drafts — mainline Linux port

Local GitHub-issue drafts. **Not filed.** A human files them (scrub + approval)
after review. File in NN order; cross-references use the NN numbers.

Plan: [../porting-roadmap.md](../porting-roadmap.md). Evidence:
[../hw-reference/20260816-104601/](../hw-reference/20260816-104601/).

| NN | Title | Labels | Hook |
|---|---|---|---|
| 00 | [Meta] Mainline Linux port for UNVR | porting, meta, tracking | Umbrella tracking issue for the whole port |
| 01 | [FIRST TASK] Ethernet ancestry report | porting, ethernet, phase-3, first-task | **Do this first** — decides the driver strategy (not coding) |
| 02 | [Phase 1] Establish Linux 6.18 on UNVR | porting, phase-1, bringup | Boot 6.18 on frozen U-Boot; ordered bring-up milestones |
| 03 | [Phase 2] Patch-set inventory & classification | porting, phase-2, analysis | Bucket every vendor file: mainline vs custom vs DT |
| 04 | [Phase 3] Ethernet driver decision | porting, ethernet, phase-3 | Turn #01 into extend/extract/clean choice |
| 05 | [Phase 4] PCIe DesignWare + Alpine glue | porting, pcie, phase-4 | Smallest custom area; gates USB/SATA/eth/DMA |
| 06 | [Phase 5] 10GbE / PCS / SerDes via phylink | porting, ethernet, phase-5, sfp | Move link mgmt out of al_eth into phylink/SFP |
| 07 | [Phase 6] RAID acceleration via dmaengine | porting, raid, dmaengine, phase-6 | Expose RAID/XOR via dmaengine; CPU fallback mandatory |
| 08 | [Phase 7] Device tree cleanup & hierarchy | porting, devicetree, phase-7 | Clean dtsi/board split; correct partition map |
| 09 | [Phase 8] Rebase 6.18 → 7.1 | porting, phase-8, rebase | Forward-port series; 6.18 deploy / 7.1 upstream-dev |
| 10 | Confirm DW I2C / DWC PCIe / AHCI register compat | porting, verification | Prove which "looks-mainline" blocks really are |
| 11 | [Patch 1] ARM64 DT binding for Alpine V2 | porting, devicetree, patch, upstream | Binding docs + MAINTAINERS |
| 12 | [Patch 2] Alpine V2 SoC .dtsi (+patch 6 clocks) | porting, devicetree, patch | Extend mainline dtsi with SoC peripherals |
| 13 | [Patch 3] UNVR board DT | porting, devicetree, patch | `alpine-v2-ubnt-unvr.dts` |
| 14 | [Patch 4-5] DWC PCIe quirks + internal controller | porting, pcie, patch | AXI snoop, APP_CONTROL, DBI offset |
| 15 | [Patch 7] Ethernet DMA core / modern al_eth | porting, ethernet, patch | MAC+DMA datapath per #04 outcome |
| 16 | [Patch 8-9] MDIO/phylink + Alpine PCS/SerDes | porting, ethernet, sfp, patch | Link layer + PCS provider |
| 17 | [Patch 10] UNVR Ethernet & SFP+ DT | porting, devicetree, ethernet, sfp, patch | Board net/SFP nodes via standard bindings |
| 18 | [Patch 11] SGPO LED controller + bay LEDs | porting, leds, gpio, patch | Serial-GPIO driver, 8 SATA bay LEDs |
| 19 | [Patch 12] RAID dmaengine driver (al_dma) | porting, raid, dmaengine, patch | Code for Phase 6 |
| 20 | [Patch 13] Crypto-engine driver (al_ssm) | porting, crypto, patch | Last functional driver |
| 21 | [Patch 14] defconfig & docs | porting, defconfig, docs, patch | Booting defconfig + build docs |

## Dependency highlights
- **#01 blocks #04 blocks #15 blocks #16** (ethernet chain).
- **#14 blocks #02, #15, #19, #20** (PCIe gates the integrated endpoints).
- **#11 → #12 → #13 → #17** (DT chain).
- **#09** depends on all bring-up issues.

## Filing notes
- Public-repo scrub before filing: no host filesystem paths, keep it generic
  (Alpine V2 / mainline). Device MAC/serial are owner-cleared for publish.
- The roadmap patch table maps patches 1–14 to these issues.
