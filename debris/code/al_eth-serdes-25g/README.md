# 25G SerDes HAL (retired from modules/al_eth, #325)

- `al_hal_serdes_25g.c`, `.h`, `_regs.h`, `_internal_regs.h` (8,524 lines): group E (offset 0x2000) PMA driver.
- No caller: the 10G SFP+ port is `serdes-grp = <3>` (HSSP group D, DT `dts/alpine-v2-ubnt-unvr-ea16.dts`); the 1G port has no SerDes.
- U-Boot corroborates: `uboot-port/drivers/phy/al_serdes/al_serdes_10g.c` — group E is "board-cfg SKIP / powered down here and external-aborts on first access".
- Only reference was the `al_serdes.c` group-init table (`al_serdes_25g_handle_init`), which filled function pointers and touched no hardware.
- `uboot-port/drivers/phy/al_serdes/al_hal_serdes_25g*` is a separate copy; untouched here.
- Restore only for a 25G AL-324 board (media types 25G / 25G_10G_AUTO).
