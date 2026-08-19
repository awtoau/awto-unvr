# Annapurna DDR HAL (Alpine V2 subset) + U-Boot `ddr` command

Curated minimal subset of the Annapurna Labs DDR HAL, pinned to Alpine V2
(AL-324), plus a U-Boot `ddr` command driving it against the **live, already-
trained** controller (no SPL, no retrain). See `docs/uboot-ddr-port.md` §5.

## Provenance
- **HAL source:** delroth-alpine_hal @ `eb6b9f1` (2022-02-02), byte-identical to
  the copy shipped in the Ubiquiti GPL kernel (`urnvr-kernel-4.19.152/drivers/
  soc/alpine/HAL/`). Copied verbatim — Annapurna headers untouched.
- **Command:** adapted from the stock Annapurna U-Boot (2015-era)
  `board/annapurna-labs/common/cmd_ddr.c`. The stock `cmd_dram_margins.c` (a
  prebuilt SRAM-agent blob) is intentionally NOT ported; BIST/margins here use
  the HAL PHY DATX BIST family directly.

## License
- HAL files are tri-licensed **Annapurna Commercial OR GPLv2 OR BSD-3-Clause**
  (see each file's header). Used here under **GPLv2**, compatible with U-Boot.
- Files we authored (`al_hal_plat_services.h`, `al_hal_plat_types.h`,
  `ddr_cmd.c`, `Makefile`) are **GPL-2.0-or-later**, © 2026 Awto / Daniel
  Tyrrell, derived from the HAL, co-authored with Claude (Anthropic).

## Files
Copied verbatim (Annapurna, unmodified):
- `al_hal_ddr.c` — rev detect + `al_ddr_cfg_init` (V2: dispatch compiled out).
- `al_hal_ddr_alpine_v2.c` — the Alpine V2 implementation (`_al_ddr_*`).
- `al_hal_sys_fabric_utils.c` — nb/fabric int helpers (referenced by the
  compiled-in, unused parity-int-unmask path).
- Headers: `al_hal_ddr*.h`, `al_hal_ddrc_regs.h`, `al_hal_ddr_ctrl_regs_*.h`,
  `al_hal_ddr_phy_regs_alpine_v{1,2}.h`, `al_hal_ddr_jedec_ddr4.h`,
  `al_hal_ddr_utils.h`, `al_hal_common.h`, `al_hal_reg_utils.h`,
  `al_hal_types.h`, `al_hal_nb_regs*.h`, `al_hal_sys_fabric_utils.h`.

Authored here:
- `al_hal_plat_services.h` / `al_hal_plat_types.h` — the U-Boot platform shim
  replacing the HAL's `plat_api/sample/` versions (readl/writel, printf,
  udelay, barriers, memset). Shared with the future eth HAL port.
- `ddr_cmd.c` — the `ddr` command (`U_BOOT_CMD`).
- `Makefile` — obj-y + `AL_DEV_ID=AL_DEV_ID_ALPINE_V2` flag set + include path.

## Build pin
`AL_DEV_ID=AL_DEV_ID_ALPINE_V2` (V1=0 V2=1 V3=2 V4=3, per the kernel's
`alpine_hal.mk`). This makes the header alias `al_ddr_*` → `_al_ddr_*` and
compiles only the V2 struct layout / ZQ counts / MR set. Nothing from
alpine_v1 / v3 `.c` is pulled in.

## `ddr` command
- `ddr info` — controller/PHY config dump + mode registers MR0..MR6.
- `ddr training` — per-octet PHY training results.
- `ddr ecc [clear]` — ECC status counters (this board has no ECC device;
  expect "disabled").
- `ddr bist [dram]` — PHY DATX BIST. Default LOOPBACK (PHY-internal, non-
  destructive); `dram` mode drives the array (test range) and may fault a live
  U-Boot — opt in explicitly.
