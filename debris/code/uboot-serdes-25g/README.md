# U-Boot copy of the 25G SerDes HAL (retired, #325)

- `uboot-port/drivers/phy/al_serdes/al_hal_serdes_25g.{c,h}`, `_regs.h`, `_internal_regs.h` (8,522 lines).
- Dead in the shipped image: the linked `u-boot` ELF carries no `al_serdes_25g*` symbol (section-GC'd; only the object's single export, `al_serdes_25g_handle_init`, was never referenced). `al_serdes_10g.c` only `#include`d the header, and drives HSSP group D (`al_serdes_hssp_handle_init`).
- Group E (the 25G complex) is board-cfg SKIP / powered down on this board and external-aborts on first access (see `al_serdes_10g.c`).
- Makefile line and the include dropped; `al_serdes_10g.c` and `al_hal_serdes_hssp.c` syntax-checked against the U-Boot v2026.07 tree with the original build flags (full U-Boot build not run: it resets the shared tree).
