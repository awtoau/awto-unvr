# [Meta] Mainline Linux port for UNVR (Annapurna Labs Alpine V2, sysid ea16)
labels: porting, meta, tracking
---

Tracking issue for the whole mainline port. Goal: **Alpine V2 solid on Linux 6.18
LTS**, minimal Annapurna-specific code, then forward-port to 7.1; upstream generic
wins. Full plan: [docs/porting-roadmap.md](../porting-roadmap.md).

## Why feasible now
- Boot is UNSIGNED (`dobootm = run bootunsign`) — can boot our own kernel. See
  [docs/bootloader.md](../bootloader.md).
- Module signing / lockdown OFF in stock kernel config. See
  [docs/porting-reference.md](../porting-reference.md).
- U-Boot writeable but frozen on ea16 — build on top of it. See
  [docs/uboot-update-path.md](../uboot-update-path.md).
- Live hardware evidence: [docs/hw-reference/20260816-104601/](../hw-reference/20260816-104601/).

## Sub-issues
- [ ] #01 Ethernet ancestry report — **FIRST TASK**
- [ ] #02 Phase 1: establish Linux 6.18 on UNVR
- [ ] #03 Phase 2: patch-set inventory & classification
- [ ] #04 Phase 3: ethernet driver decision (extend/extract/clean)
- [ ] #05 Phase 4: PCIe DesignWare + Alpine glue
- [ ] #06 Phase 5: 10GbE / PCS / SerDes via phylink
- [ ] #07 Phase 6: RAID acceleration via dmaengine
- [ ] #08 Phase 7: device tree cleanup & hierarchy
- [ ] #09 Phase 8: rebase 6.18 → 7.1
- [ ] #10 Confirm DW I2C / DWC PCIe / AHCI register compatibility from live DTB
- [ ] #11–#21 Patch-order items (see roadmap patch table)

## Definition of done
- [ ] Mainline 6.18 boots the box off the USB stick, network + storage usable.
- [ ] Series rebased and booting on 7.1.
- [ ] Generic improvements submitted upstream where practical.

## Ground-truth facts (live 2026-08-16)
- SoC: AL-324 Alpine V2, 4× Cortex-A57, GIC-v3, PSCI, 4 GiB RAM, aarch64.
- Custom (no mainline driver): `al_eth`, `al_dma`, `al_ssm`, `al_nand`,
  `al_thermal`, `al-sgpo`, internal-PCIe glue.
- Already mainline/works: AHCI, xHCI, DW I2C, DW SPI + m25p80, PL061, sp805,
  s35390a, adt7475, pca953x/pca954x, at803x (1G PHY).
