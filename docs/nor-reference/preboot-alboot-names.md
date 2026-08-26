# al_boot payload — function-name ledger

Machine form: `preboot-alboot-names.sym` (applied via ApplyAlRegs). Method:
[ghidra.md](../ghidra.md) §6 iterative naming loop.

Pre-existing entries (everything above the "reset/entry chain" marker in the
`.sym`, ~110 names) predate this `.md` companion — added across earlier
passes (mostly `__func__`-string-confirmed AL HAL names: `al_ddr_*`,
`al_pll_*`, `al_serdes_*`, `al_flash_*`, etc). Rationale for those isn't
retroactively documented here; only what this pass added.

## This pass — reset/entry chain (Gap 1, 2026-08-26)

Addresses below are the **corrected** al_boot VAs (payload carved
`container[0x21004:0x6b6b4]`, load `0x01000000` — see
[preboot-coverage.md](../preboot-coverage.md) "Carve correction"). Earlier
prose in `preboot-decompile.md` used the **uncorrected** carve (4 bytes
high); every address quoted there for this chain has been re-derived from
the current decompile, not trusted as-is.

Confidence per [ghidra.md](../ghidra.md)'s rule for this pass: `confirmed`
only for an exact `__func__`/string or exact HAL-struct/register match;
everything else is `speculative` (`s_` prefix) or left bare.

| addr | name | confidence | rationale |
|---|---|---|---|
| 0x010005a8 | `s_reset_tail` | speculative | calls `s_stage3_orchestrator` then `halt_baddata` (never returns); no `__func__` of its own |
| 0x01002e8c | `s_stage3_orchestrator` | speculative | body: memcpy of AArch64 resume-agent to phys `0x1000`, ISB/DSB, `set_vectors`, SCTLR compose, nb-service+CCU pokes, calls `stg3_board_init`/`s_soc_fabric_port_init`/`s_boot_app_load_exec`; returns `_DAT_fbff4150==0x31415926` (shared_params DDR-ready magic) |
| 0x010129d8 | `s_memcpy` | speculative | word-aligned copy loop, byte-tail fallback; two call shapes confirm generic `memcpy(dst,src,len)`: `(0x1000,&DAT_01029158,0x5e18)` = AArch64 resume-agent → phys `0x1000` (independently confirmed in [preboot-coverage.md](../preboot-coverage.md):94), `(&SUB_fbff4200,&LAB_0102ef70,0x15fc)` = SRAM-agent blob → PBS SRAM (`preboot-decompile.md` SRAM_AGENT_ADDRESS section) |
| 0x0100036c | `s_sctlr_bits_set` | speculative | composes an SCTLR set/clear mask from 3 booleans (I-cache `0x1000`, align `4`, branch-predict `0x800`) then `coproc_moveto_Control` + DSB; called `(1,1,1)` (enable all) in the orchestrator and `(0,1,0)` (disable I-cache only) in `s_boot_app_load_exec` right before handoff |
| 0x010274e4 | `s_nb_acf_misc_wr_pos_set` | speculative | `*(base+0xd0) = (base+0xd0 & ~bit30) \| (param2?bit30:0)`; base+0xd0 = NB_GLOBAL `acf_misc`, bit30 = `NB_GLOBAL_ACF_MISC_WR_POS_DEV_AFTER_DEV_DIS` (`al_hal_nb_regs_v1_v2.h:1328`) — exact register+field match, but function itself carries no string, kept speculative |
| 0x0102752c | `s_nb_acf_misc_wr_pos_clear` | speculative | unconditional clear of the same bit; paired teardown of `s_nb_acf_misc_wr_pos_set`, called from `s_boot_app_load_exec` right before U-Boot handoff |
| 0x01027504 | `s_ccu_cluster_snoop_enable` | speculative | writes CCU `+4=7` (`speculation_ctrl_register_v1_v2`) always, and if `param2!=0`: `+0x4000=1`, `+0x5000=1` — `al_hal_ccu_regs.h`'s `slaves[4]`/`slaves[5]` (`AL_CCU_SLAVE_IDX_CLUSTER(1)`/`(2)`) `snoop_control_register`, bit0 `CCU_SNOOP_CONTROL_REGISTER_SX_ENABLE_SNOOP_REQUESTS`. **Same three offsets/values as U-Boot proper's later `al_ccu_init_inlined`** ([uboot-ccu-coherency.md](../uboot-ccu-coherency.md)) — al_boot does this CCU coherency setup first, unconditionally (called with `param2=1` fixed) |
| 0x0102753c | `s_ccu_cluster_snoop_disable` | speculative | zeroes the same three CCU offsets (`+4`, `+0x4000`, `+0x5000`) — paired teardown, called from `s_boot_app_load_exec` immediately before loading/executing U-Boot |
| 0x01002800 | `s_soc_fabric_port_init` | speculative | loops 4 ethernet ports (PLL/base table `DAT_010290a0..`, `+0x964` register tweak), then `al_ddr_rev_get` + conditional `dram_clear()`, then relocates the SoC trace buffer per DT `soc_trace`/`reg` property |
| 0x01002a88 | `s_boot_app_load_exec` | speculative | loads app image from SPI (recovery vs `0x40000` normal path), tears down CCU/nb-service coherency knobs (see above), then either falls through to `WaitForInterrupt` (primary-core AArch64 setup via `al_sys_fabric_core_*`) or busy-waits UART TX drain for the APCEA path — this is the proprietary→GPL U-Boot handoff |

### `stg3_early_init` string vs function

The multi_dt switch (sysid @ NOR `0x1f000c`, hw-rev @ `0x1f0010`, 5-way
switch `0xea16/0xea1a/0xea20/0xea21/0xea30`) is inside `stg3_board_init`
(`0x01001108`, `__func__`-confirmed via `s_stg3_board_init_0102913c` at 5
call sites) — not a separate `stg3_early_init` function.

The embedded `__func__` string `stg3_early_init` (`0x0102912c`, 16 bytes
before `stg3_board_init`'s own string) is used for exactly one error path
("cpu resume regs invalid") inside `thermal_sensor_trim_init` (`0x01000b30`)
— the function that copies + invokes the SRAM-agent blob and validates the
resume-magic it writes. That function's dominant embedded name is
`thermal_sensor_trim_init` (used for its OTP/thermal-trim error paths); it
likely inlines what source called `stg3_early_init` as a static helper, so
both strings survive in one Ghidra-recovered function body. Not renamed
(out of Gap-1 scope).
