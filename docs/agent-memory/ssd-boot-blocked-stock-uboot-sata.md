---
name: ssd-boot-blocked-stock-uboot-sata
description: "SSD boot WORKS via awto-uboot since #216 (2026-09-04); the old \"handoff hangs at rootwait\" (#97) is fixed. Stock 2015.07 still has no SATA at all."
metadata: 
  node_type: memory
  type: project
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-09-04T23:23:20.641Z
---

**Current state:** awto-uboot boots Linux from the SSD every boot — this IS the shipped chain since #216 (`stock → NAND 0x1300000 awto-uboot → ext4load /boot/uImage → Linux`, 45 ms vs stock's 1580 ms). See [[nand-boot-layout-recovery]].

- The "ata3 never completes IDENTIFY at rootwait" hang (#97) was CCU coherency: vendor U-Boot enabled it from `ft_board_setup()` on every bootm, ours never did. Fixed by `al_ccu_early_coherency_enable()` in `uboot-port/board/annapurna/alpine/alpine.c`. #97 closed.
- awto-uboot also powers the SATA bays itself via PCA9575 @0x21 gpio-hogs (`awto-alpine-v2-unvr-uboot.dts:437-440`) — on a warm reset the expander drops its outputs and every link times out without this. UEFI entered from awto-uboot inherits powered bays (#249).

**Still true:** stock U-Boot 2015.07 has no SATA (`scsi init` → 4 links time out → 0 devices). That is why stock is the netboot/recovery stage and cannot boot from SSD.

**Why:** this memory said SSD boot was blocked for two weeks after it became the default path; two contradictory index entries pointed at it.
**How to apply:** do not treat "our U-Boot cannot hand off to Linux" as a constraint — it is the normal boot. If SATA fails at awto-nas#, check bay power (the hogs) before AHCI.
