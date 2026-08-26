# SRAM-agent blob (`agent_wakeup`) — function-name ledger

Machine form: `preboot-alboot-sram-agent-names.sym` (applied via
ApplyAlRegs). Method: [ghidra.md](../ghidra.md) §6 iterative naming loop.
Target: `docs/nor-reference/preboot-alboot-sram-agent-decompiled.c` (38
functions, disassembled standalone at load/entry `0xfbff4200` — see
`docs/preboot-decompile.md` `SRAM_AGENT_ADDRESS` section for how this blob
was found/carved). 29 named (8 confirmed, 21 speculative), 9 left bare.

Confidence rule for this pass: `confirmed` only for an exact
`__func__`/string or exact HAL-struct/register match; everything else is
`speculative` (`s_` prefix) or left bare — the ledger doesn't require 100%
naming.

**Banner-string trace, checked and negative**: grepped this sub-blob's own
decompile/disassembly for the `agent_wakeup`/`v2.10` banner text or any
`s_`-labelled string reference — none found. The banner lives in al_boot's
own `.rodata` (printed elsewhere in the parent image, not from this copied
SRAM sub-blob), so it can't be used to pin down which of the 38 functions
is the "true" top-level entry via a string reference. Entry identity below
is argued from call-site/load-address evidence instead (still not a
`__func__` match, so kept `speculative`).

| addr | name | confidence | rationale |
|---|---|---|---|
| 0xfbff4200 | `s_agent_entry` | speculative | address equals both the blob's own load base and al_boot's direct call target — `(*(code *)&SUB_fbff4200)()` at `preboot-alboot-decompiled.c:750`, right after the 0x15fc-byte memcpy into PBS SRAM. No `__func__`/banner string traced to it (see above), so not "confirmed" by this pass's rule despite the strong address match |
| 0xfbff4278 | `cpu_affinity_id` | confirmed | `coproc_movefrom_Identification_registers(5,...) & 0xf` = `MRC p15,0,Rt,c0,c0,5` = MPIDR read, masked to affinity-0 field — exact register match; mirrors S2's own `cpu_affinity_id` (`preboot-s2-names.sym`) |
| 0xfbff4284 | `cpacr_vfp_enable` | confirmed | `CPACR \|= 0xff00000` (enable CP10/CP11 = VFP/NEON) + DSB/ISB — exact register match; identical shape to al_boot main body's `FUN_01000618` |
| 0xfbff42f4 | `l2ctlr_smp_park_sync` | confirmed | spins on a peer-set flag (`*DAT_fbff4350 != 2`), writes an implementation-defined "peripheral system" CP15 register, then bit `0x82`/`0x200082` (by `param_1`) into **L2CTLR** (`opc1=1,CRn=c9,CRm=c0,opc2=2` — Cortex-A15/A17 L2 Control Register, exact register match) — textbook SMP-coherency-before-caches secondary-CPU bring-up; near-identical shape to al_boot main body's `FUN_01027558` |
| 0xfbff43b8 | `s_fnptr_trampoline` | speculative | `(*param_2)()` — trivial indirect-call wrapper, first param unused; no call site resolved in this decompile to confirm purpose |
| 0xfbff437c | *(bare)* | — | `coprocessor_moveto(0xf,0,0,param_1,...)` — target register (CRn=c0 with opc1/opc2 from Ghidra's intrinsic args) not confidently identified; c0 is normally read-only ID-register space, so the exact target is unclear — left unnamed rather than guessed |
| 0xfbff4588 | `s_agent_wakeup_sequence` | speculative | the entry's callee: runs `s_freq_setup_early`, `nb_cpu_resume_setup_all`, `s_nb_gic_dist_init_all(0xf0000000)`, then a no-op — i.e. the actual "do the wakeup" body invoked from `s_agent_entry` |
| 0xfbff45b0 | `s_freq_setup_early` | speculative | reads a PLL/config struct, calls the frequency-decode/lookup cluster below, writes a derived value (`>>4`) via a coprocessor move and to a MMIO word — low confidence on *which* clock; not chased further |
| 0xfbff4628 | `s_itoa` | speculative | classic division-based digit-extraction with sign/base/pad handling — textbook itoa, used by the printf backend below |
| 0xfbff46d2 | `s_fmt_sink_write_str` | speculative | buffered-string-or-UART output sink (falls back to `s_uart_puts` when no buffer given) — helper of `s_vprintf_format` |
| 0xfbff4712 | `s_fmt_sink_write_char` | speculative | single-char version of the same sink pattern (falls back to `s_uart_putc_crlf`) |
| 0xfbff4740 | `s_vprintf_format` | speculative | format-string parser: `%d/%x/%X/%p/%u/%c/%s`, width/zero-pad — the debug-print backend's core |
| 0xfbff4880 | `s_debug_printf` | speculative | fixed-arity wrapper around `s_vprintf_format`; called throughout this blob for error/trace logging (e.g. `FUN_fbff4c18`'s bad-param path) |
| 0xfbff48a0 | `nb_cpu_resume_setup_all` | confirmed | loops `nb_cpu_resume_addr_set` over CPUs 0..3, then writes `SRAM_CPU_RESUME_ADDRESS` (`0xfbff4120`, HAL-confirmed, `al_hal_iomap.h:357`) validity magic `0xf0e1d2c4` and zeroes the rest of the resume struct — exact HAL address + value match, already established in `docs/preboot-decompile.md`'s SRAM_AGENT_ADDRESS section |
| 0xfbff48fc | `s_cpu_wake_dispatch_loop` | speculative | per-CPU wait loop: `nb_cpu_power_ctrl_set(...,3)` (park) while polling a wake-target table, `nb_cpu_power_ctrl_set(...,0)` (run) once a resume address + flag are set, then GIC/EL3 setup and `(*UNRECOVERED_JUMPTABLE)()` to the resume target — matches the already-documented park=3/run=0 `nb_cpu_power_ctrl_set` call pattern |
| 0xfbff49bc | `s_freq_table_init_once` | speculative | classic lazy-init guard (`if (*flag==0) { build table; *flag=1; }`) around the frequency-lookup cluster below |
| 0xfbff49f8 | *(bare)* | — | large switch decoding a config word into ~6 output fields against const tables (`DAT_fbff4b5c..4bcc`) — a clock/divider decode table, but which PLL/domain not confidently identified |
| 0xfbff4bd4 | *(bare)* | — | frequency-ish arithmetic (`(x*mul+mul)/(y*n+n)`) over the table `s_freq_table_init_once` built — helper, purpose not pinned down |
| 0xfbff4c06 | *(bare)* | — | one-line predicate on a bitfield range `[8:19]` vs `6`/`7` — condition for the two functions above |
| 0xfbff4c18 | *(bare)* | — | 3-way dispatch (`param_3` 0/1/2) selecting one of 3 const table pointers/sizes; logs+returns error via `s_debug_printf` on an unrecognised selector |
| 0xfbff4c78 | *(bare)* | — | linear search of a table (stride 0x18) for a matching `iVar1` computed via `FUN_fbff4bd4` |
| 0xfbff4cb0 | *(bare)* | — | table lookup + divide, gated on `FUN_fbff4c06`'s predicate; error-logs via `s_debug_printf` (`DAT_fbff4d0c`/`4d10`) on one path |
| 0xfbff4d14 | *(bare)* | — | 2-word struct init from a lookup at `&DAT_00004400 + param_2` — table-entry decode, callee of `s_freq_table_init_once` |
| 0xfbff4d24 | *(bare)* | — | `*p=a; p[1]=b;` — generic pair-store, called 4× building a small array in `s_freq_table_init_once`; no purpose beyond that visible |
| 0xfbff4d2c | `nb_cpu_power_ctrl_set` | confirmed | writes `al_nb_cpun_config_status.power_ctrl` (offset `0x20`) at `NB_SERVICE_BASE + 0x2000 + cpu*0x100` — exact HAL struct match (`al_hal_nb_regs_v1_v2.h:368-411`) |
| 0xfbff4d54 | `nb_cpu_resume_addr_set` | confirmed | writes `al_nb_cpun_config_status.resume_addr_l`/`.resume_addr_h` (offsets `0x28`/`0x2c`), same base — exact HAL struct match, called in a loop over 4 CPUs from `nb_cpu_resume_setup_all` |
| 0xfbff4d88 | `nb_gic_dist_init` | confirmed | base offset `0x200000` (id=0) vs `0x9000` (id=1) off `AL_NB_BASE` — exactly `AL_NB_GIC_DIST_BASE(id)`'s ternary (`al_hal_iomap.h:188`); writes priority bytes `0x80808080` and enable/clear mask `0xffffffff` at the GICv2 Distributor offsets (`ICDIPR`/`ICDISER`/`ICDICER`) — exact HAL+register match |
| 0xfbff4e0c | `s_nb_gic_dist_init_all` | speculative | calls `nb_gic_dist_init(0)` then `(1)` — wrapper; kept speculative since the wrapper itself touches no register directly (its callee is the confirmed one) |
| 0xfbff4e26 | `s_noop_stub` | speculative | `{ return; }` — literally empty; likely a compiled-out hook/callback slot |
| 0xfbff4e28 | `s_gic_cpuif_init` | speculative | writes the same `0x80808080` priority-byte pattern as `nb_gic_dist_init` into two other register arrays plus an `0xff`/enable-mask write — GIC CPU-interface (banked, not Distributor) init by shape, not independently HAL-address-confirmed |
| 0xfbff4eb0 | `s_gic_cpuif_ack_wait` | speculative | reads `cpu_affinity_id`, writes 0 to a per-core-banked word (`base + id*0x20000`), polls a status bit, then sets an `0xff` signal byte — plausible GIC ack/handshake; low confidence, not chased further |
| 0xfbff4ed8 | `s_uart_putc_crlf` | speculative | `s_uart_tx_raw(c)`, and `s_uart_tx_raw('\r')` when `c=='\n'` — LF→CRLF translation on top of the raw UART writer |
| 0xfbff4ef4 | `s_uart_puts` | speculative | loops a NUL-terminated string through `s_uart_putc_crlf` |
| 0xfbff4f08 | `s_uart_tx_raw` | speculative | busy-waits a status bit then stores to a UART data register — raw single-character TX |
| 0xfbff4f8c | `hvc_call` | confirmed | `software_hvc(0)` — literal HVC instruction, exact match |
| 0xfbff4fa0 | `s_scr_ns_set_and_jump` | speculative | `SCR \|= 1` (NS bit — exact register/field, but combined with an indirect jump through `DAT_fbff50a4` whose target isn't resolved, so kept speculative as a whole function) |
| 0xfbff4ff4 | `s_smc_el3_transition` | speculative | `software_smc(0)` plus MPIDR read, `SCR`/`VBAR`(`c12,c0`)-shaped coprocessor writes, and an indirect call through `DAT_fbff50b8` — multi-register EL transition, not a single exact match |
| 0xfbff5058 | `s_actlr_nsacr_smp_setup` | speculative | `ACTLR \|= 0x80000000` (SMP-alike bit), `NSACR \|= 0x60c00`, plus a 64-bit coprocessor pair move (`MRRC/MCRR`, plausibly `VTTBR` — not confirmed) — matches `docs/preboot-decompile.md`'s already-noted "EL3→NS/Hyp handoff" description |

## Cross-reference — CCU coherency (relevant to #97)

Not part of this blob. The al_boot **main-body** reset/entry chain
separately does CCU coherency setup (`s_ccu_cluster_snoop_enable` /
`_disable`, `preboot-alboot-names.md`) — see that ledger; this SRAM-agent
blob only handles secondary-CPU wake/resume + GIC, no CCU register
touches found in its decompile.
