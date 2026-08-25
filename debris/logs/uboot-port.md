# U-Boot port — progress notes

Session 2026-08-17. Plan: `docs/uboot-port-plan.md`. Build log: `tmp/logs/uboot-port.log`.

## Done
- Mainline v2026.07 inventory: **zero** Alpine/AL-324 support; all standard-IP drivers present.
- Confirmed chainload framing: TEXT_BASE 0x01100000, vendor `SKIP_LOWLEVEL_INIT` (no DDR in
  vendor U-Boot — done by proprietary preboot), `go` passes no DTB → embed own DTB.
- Confirmed DDR init is NOT in GPL U-Boot; only published copy = delroth `alpine_hal/ddr/`
  (DDR4). BIST/margins/ECC = vendor `common/cmd_ddr.c` + `cmd_dram_margins.c` (HAL-backed,
  run against live controller — no SPL needed).
- SATA analysis: EPs `1c36:0031` behind internal-PCIe host (no mainline U-Boot driver = the
  gap); generic mainline AHCI binds once enumerated; coherency (AXI snoop vs U-Boot cache
  maintenance) is the open on-device test.
- Created scaffold `uboot-port/` + `scripts/uboot-build.py`. **Built OK**: 321 KB aarch64
  `u-boot-dtb.bin`. Cleaned build; reverted shared ref tree.
- Env gotcha: OpenSSL 3.5 removed `engine.h`; disable `TOOLS_LIBCRYPTO`+`TOOLS_KWBIMAGE`.

## Next (not started)
- On-device boot-test Stage-1 (`go 0x1100000`) → expect `unvr#` on ttyS0.
- Stage-1b: add DW-SSI+`sf` + DW-I2C DT nodes (direct mainline drivers).
- Stage-2 (priority): port internal-PCIe host enum from vendor `common/pci.c` / Linux
  `pcie-al.c`; `scsi scan`; test coherency path (a) cache-maintenance-only first, (b) AXI snoop.
- File issues for: SATA coherency test result; upstream openssl-3.5 host-tools note (#46 child).

## Open questions (need on-device)
- Does vendor `scsi scan` actually fail to see the SATA SSD? (task asserts; verify).
- Does mainline AHCI DMA work with cache maintenance alone, or need AXI snoop config?
- Boot-ROM S2-loader contract (SPI off 0, `"S2\0\0"`, SRAM 0xf2200000) — needed for target-2 SPL.
