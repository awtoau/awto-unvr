# Vendor U-Boot proper — confirmed function names, OLD vs NEW

Companion to `uboot-old-names.sym` / `uboot-new-names.sym` (ApplyAlRegs-format
ledgers) and `uboot-ccu-coherency.md` (findings writeup, issue #97). One shared
table (not two separate per-blob `.md` files) because the whole point is the
OLD-vs-NEW comparison — see `docs/ghidra.md` §"Iterative naming loop" for the
normal per-blob convention this deviates from.

| name | OLD VA | NEW VA | confidence | rationale |
|---|---|---|---|---|
| `ft_board_setup` | `0x1104670` | `0x11045d4` | confirmed | structural: calls all 4 helpers below in GPL board.c's `ft_board_setup` order, plus the `/soc/ccu` lookup block; called from 2 sites in both builds' `bootm` FDT-prep path (matches `image_setup_libfdt`-style flow: "chosen node create failed" / "arch-specific fdt fixup failed" error strings at the call sites) |
| `ft_board_setup_feature_disable` | `0x1101e20` | `0x1101ccc` | confirmed | `__func__` string `s_ft_board_setup_feature_disable_*` embedded in its own error path, both builds; matches GPL `board/annapurna-labs/{alpine_db,alpine_ubnt}/board.c` static fn of the same name |
| `ft_board_setup_clock` | `0x1101ea0` | `0x1101d4c` | confirmed | `__func__` string, same as above |
| `ft_board_setup_pcie_set` | `0x1101f00` | `0x1101efc` | confirmed | `__func__` string, same as above |
| `ft_board_setup_prop_u32_set` | `0x1102020` | `0x110201c` | confirmed | `__func__` string, same as above — this is the `iocc_force`/`iocc_force_val` env-var override writer |
| `al_ccu_init_inlined` | `0x112629c` | `0x1125b38` | confirmed | register-write correlation against `delroth-alpine_hal`'s `al_ccu_cluster_init()`/`al_ccu_init()` (see uboot-ccu-coherency.md) — no `__func__` string exists for this path in the HAL source, so the name is inferred, not literal |

All 6 are byte-identical in logic between OLD and NEW (only the VA differs,
by a non-constant offset — see uboot-ccu-coherency.md §"Relocation is not a
constant offset"). GPL source for the first 5: `board/annapurna-labs/alpine_ubnt/board.c`
(and the near-duplicate `alpine_db/board.c`) in
`/mnt/2tb/unvr-port-refs/UBNT-source-code/UNVR-1.3.35-GPL/u-boot/` — note
`alpine_ubnt` is a **UBNT-specific** board file, not generic Annapurna. GPL
source for `al_ccu_init_inlined`'s semantics: `delroth-alpine_hal`
`services/sys_fabric/al_init_sys_fabric.c` (`al_ccu_cluster_init`,
`al_ccu_init`) + `include/sys_fabric/al_hal_ccu_regs.h` (`struct al_ccu_regs`).
