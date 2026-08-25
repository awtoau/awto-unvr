# uboot-ddr-port — session notes

Task: map delroth Alpine V2 DDR HAL, extract our board params, plan SPL integration, note #29
BIST. Deliverable: docs/uboot-ddr-port.md. Direct work, no sub-agents.

## Confirmed this session
- delroth `ddr/src/al_hal_ddr_init_alpine_v2.c` (5608 lines) == vendor kernel
  `urnvr-kernel-4.19.152/.../HAL/ddr/src/al_hal_ddr_init_alpine_v2.c` — `diff -q` IDENTICAL.
  => the "RE" HAL is the same code the vendor ships under GPLv2 in its kernel drop. License-clean.
- Entry: `al_ddr_init()` @ init_alpine_v2.c:5552. Call graph in doc §1.
- Config struct `al_ddr_init_cfg` @ include/al_hal_ddr_init.h:844; addrmap @ al_hal_ddr.h:656.
- No populated `al_ddr_init_cfg` anywhere (delroth / kernel HAL / GPL U-Boot) — grep-confirmed.
  Board "what" must come from SPD + live-controller readback.
- Register bases from alpine_v2/include/al_hal_iomap.h: NB=0xf0000000, service=0xf0070000,
  DDR ctrl=0xf0080000, DDR PHY=0xf0088000. Matches hardware.md memctl/nb-service.
- Preboot al_boot does NOT hold DDR params — delegates to CVOS agent via mailbox
  (preboot-decompile.md). Dead end for a param table.
- Vendor `alpine_ubnt/board_cfg.h` = PLL config only, no DDR. `cmd_ddr.c` uses `al_ddr_cfg_init`
  (live readback), not `al_ddr_init`. `cmd_dram_margins.c` runs a prebuilt SRAM agent blob.
- DRAM = Samsung K4A8G165WB-BCRC ×4 = 8Gb x16 DDR4, -BCRC suffix => DDR4-2400 CL17 (needs SPD
  confirm). ECC almost certainly OFF (no ECC device).
- HAL tri-license: Annapurna Commercial OR GPLv2 OR BSD-3. U-Boot GPLv2 => use GPLv2 option.

## Deliverables produced
- docs/uboot-ddr-port.md — full map + params + staged SPL plan + license + first step.
- scripts/read-ddr-spd.py — reads+decodes DDR4 SPD @ I2C 0x57 over console; offline `--file`
  decode verified end-to-end (part#/mfr decode correct on synthetic input).

## Open / needs on-device
- Run read-ddr-spd.py on woomera to get authoritative org/tmg/addrmap.
- Recover impedance_ctrl (ODT/DIC/VREF/ROUT) + actual DDR freq from live MRs via al_ddr_cfg_init
  (not in SPD).
- Boot-ROM S2 loader format contract for from-reset SPL (nor-boot-chain.md §1; S2 jumptable
  @0xf22000fc unrecovered).

## Disk
- No build artifacts created this pass. 39 GB free at start; unchanged (only two text files +
  one script written).
