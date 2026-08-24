# al_eth 1G TX: M2S engine ABORTs on the descriptor-read from DRAM (UNRESOLVED)

Port: our U-Boot v2026.07, chainloaded from stock, Alpine V2 / AL-324, eth1 (1G RGMII,
PCI 1c36:0001, rev_id 2). `ping`/`tftp` TX never completes. Tracked as #90.

## Symptom (on-box, reproducible)
- Every send: `TX completion timeout: 1 descs left after 100000 us`.
- Diagnostic dump (`al_eth_dm_dump_tx`, BAR0=0xfe000000):
  - `post-init`: `m2s.state=0x00000000` — engine CLEAN after init.
  - `doorbell`/`timeout`: `m2s.state=0x00002222` — all 4 M2S sub-engines (comp_ctrl/
    stream_if/data_rd/desc_pref) = 2 = ABORT.
  - `drtp` increments (doorbell lands) but `drhp`/`dcp`/`crhp` = 0, `q_tx_pkt` = 0.
- `dcp` never leaves 0 -> the engine NEVER fetches the descriptor. The ABORT is on the
  **descriptor-prefetch read** to `drbp = 0x00000000bfb59b80` (high DRAM, top of the
  U-Boot heap: relocaddr=0xbff54000, sp=0xbfb41650).
- M2S abort log `0xfe000214..0x220` = **all zeros** -> no response captured = a **pure
  AXI timeout** (~10 ms, gen_axi.cfg_1=5000000), NOT a DECERR/SLVERR. The read gets no
  responder.

## Ruled OUT on hardware (each = a build + cold-cycle + ping)
- **Snoop attribute** — eth SMCC (config 0x110, read back live at ECAM 0xfbc08110):
  `0x0` (default), `0x1` (SNOOP_OVR, non-coherent), `0x3` (SNOOP_OVR|EN, coherent) all
  abort identically.
- **FLR** — `al_eth_flr_rmn` before init present AND removed: both abort.
- **UDMA submaster config** — `desc_rd_cfg1=0x01030000`, `desc_rd_cfg2=0x32400000`,
  `ostand=0x10101010` = benign reset defaults, identical to stock (which works).
  `al_udma_m2s_axi_set` is never called for rev_id 2 in ours OR stock.
- **tgtid / tgtaddr** — not programmed for eth in ours or stock; disabled-by-default =
  raw address pass-through, cannot misroute 0xbfb59b80.
- **Queue reset / inherited-dirty state** — engine is clean at init (state=0), so there
  is nothing dirty to reset; `al_udma_state_set(DISABLE)` does NOT clear the ABORT
  (verified live: writing 0x2 to 0xfe000204 leaves state=0x2222).
- **Low-address diagnostic** (2026-08-19): TX ring pinned to low DRAM
  (`desc_block=0x02000000`, confirmed live) — still hangs identically. Not an
  addressing/window problem; the master cannot reach DRAM **at any address**.
- **ROB enable** (`GEN_CTL_19` read-ROB/write-ROB) — `al_eth_dm_unit_adapter_setup()`
  resets+enables it (`uboot-port/drivers/net/al_eth/al_eth_dm.c:255-262`); still hangs.
  **2026-08-24**: confirmed independently on the al_ssm function (PCI 00:04.0,
  `pci display.l 0.4.0 0x240 1` -> `0x00010001`, i.e. read+write ROB already enabled by
  **hardware reset default**, with zero driver config) — and it hangs too. ROB state was
  never the gate.
- **AXI error-tracking, freshly re-verified 2026-08-24**: enabled live on al_ssm
  (`pci write.l 0.4.0 0x114 0x0`, clearing `SMCC_CONF_2` bit 8) then re-ran `iodma` —
  `RD_ERR_ATTR` (cfg 0x1b8) and the latched fail-address (0x1c8/0x1cc) stayed all-zero
  through the timeout. Confirms (independently of eth) the read is never even answered
  with an error; it is dropped/never serviced by the fabric, not merely unmonitored.

## 2026-08-24 — eth-specific hypothesis DISPROVEN: it is a generic al_udma M2S hang

**On-box test (`iodma` command, `uboot-port/drivers/crypto/al_ssm/al_ssm_dma.c`, never
previously run — see #90 comment 2026-08-23): reproduces the IDENTICAL failure signature
on a completely separate al_udma instance (al_ssm crypto/RAID engine, PCI 1c36:0022,
00:04.0), not al_eth's.**

```
iodma: SSM 1c36:0022 rev 2  udma=00000000fe080000 app=00000000fe0a0000
al_ssm_dma_init_aux: non optimal adapter configuration
iodma[post-init]: m2s.state=00000000 tx0 drhp=40000000 drtp=40000000 dcp=40000000 crhp=40000000 qpkt=00000000
iodma[doorbell]: m2s.state=00002222 tx0 drhp=40000000 drtp=40000001 dcp=40000000 crhp=40000000 qpkt=00000000
iodma: completion timeout after 100000 us
```

Same tell as al_eth: `drtp` (CPU-side tail, submitted by us) advances, `drhp` (engine-
side head, the descriptor-prefetch read completing) never leaves its post-init value,
`m2s.state` goes to `0x2222` (all-ABORT) on the first doorbell. This **directly answers
the open question in #90/#132** ("if its drhp advances where al_eth's stays frozen, #90
is eth-function-specific") — it does not advance. **#90 is NOT eth-specific.** It is a
fabric/bring-up-level M2S descriptor-prefetch-read hang affecting (at least) two
independent al_udma-consuming PCI functions under our chainloaded U-Boot.

This resolves next-investigator lead (a) from the prior #90 comment (merge + actually
run `iodma`) and reframes the rest of the investigation: stop looking at al_eth-specific
code (MAC config, RGMII, PHY, FLR) and look at whatever is common to *any* al_udma
bring-up in this environment.

## 2026-08-24 — two more hypotheses DISPROVEN by full source comparison

Per-line diff (not sampled) of every file in the M2S bring-up path, both sides fetched
fresh and diffed with `diff -u`:

| file | Linux (working ref) | uboot-port | result |
|---|---|---|---|
| `al_hal_udma_main.c` | `modules/al_ssm/al_hal_udma_main.c` (1240 lines) | `uboot-port/drivers/net/al_eth/hal/udma/al_hal_udma_main.c` (1241 lines) | **byte-identical** body — diff is only `"x.h"` vs `<x.h>` include-quoting + one `#ifdef AL_ETH_EX` line that's never defined either side |
| `al_hal_udma_config.c` | `modules/al_ssm/al_hal_udma_config.c` (1516) | `uboot-port/.../hal/udma/al_hal_udma_config.c` (1516) | **byte-identical** body, same include-quoting-only diff |
| `al_hal_udma_iofic.c` | `modules/al_ssm/al_hal_udma_iofic.c` (384) | `uboot-port/.../hal/udma/al_hal_udma_iofic.c` (384) | **0-line diff, fully identical** |
| `al_hal_m2m_udma.c` | `modules/al_ssm/al_hal_m2m_udma.c` (197) | `uboot-port/drivers/crypto/al_ssm/al_hal_m2m_udma.c` (197) | **0-line diff, fully identical** |
| `al_hal_ssm.c` | `modules/al_ssm/al_hal_ssm.c` (387) | `uboot-port/drivers/crypto/al_ssm/al_hal_ssm.c` (393) | identical on the RAID/UDMA path; only the crc/crypto-only helpers (`al_ssm_unit_regs_info_get` etc — pure pointer arithmetic, not called by `iodma`, not hardware-touching) are `#if 0`'d out |

**Important vintage caveat**: `modules/al_eth/al_hal_udma_main.c` (the Linux al_eth
module's own copy) is a much OLDER HAL snapshot — 604 lines, no `rev_id >= 4` branches,
no `ostand_cfg`/addr_hi-selector logic, 1M-cycle AXI timeout not 5M. It is NOT the
right diff target for uboot-port's UDMA core; `modules/al_ssm`'s copy is (near-identical
line count, and both trees' `al_eth_adapter_params`/`al_hal_eth.h` structs differ in the
same way — uboot-port carries `dev_id`/`common_mode`/`unit_adapter`/`mac_common_regs`
fields `modules/al_eth/al_hal_eth.h:609-623` does not have at all). The two Linux copies
of the "same" HAL have already drifted (matches #132 point 2 — one shared HAL source is
still unfinished). Conclusion: **the M2S/UDMA bring-up HAL code itself is proven correct
against a working reference — this is not a HAL bug.**

**`al_unit_adapter_init` is also disproven as "the missing step".** Previously flagged
here as "the leading candidate" because our port stubs it
(`uboot-port/drivers/net/al_eth/al_eth_stubs.c:36-43`, reached only if
`unit_adapter != NULL`; both `al_eth_dm.c:334` and `al_ssm_dma.c:253` pass
`unit_adapter = NULL`). Checked whether Linux's WORKING reference actually calls it:
- `modules/al_eth/al_hal_eth.h:609-623` (`struct al_eth_adapter_params`) has **no
  `unit_adapter` field at all** — the concept doesn't exist in al_eth's current Linux
  HAL vintage.
- `modules/al_ssm/al_ssm_main.c:790` does `memset(&dma_params, 0, sizeof(dma_params))`
  and never sets `.unit_adapter`, so it stays NULL.
- `modules/al_ssm/al_hal_ssm.c:91-93`: `if (params->unit_adapter && !params->func_num &&
  !params->skip_adapter_init) al_unit_adapter_init(...); else if (!params->unit_adapter)
  /* non-optimal warning, skip */`.

So Linux's own working al_ssm driver takes the **exact same skip-with-warning path** our
port does. `al_unit_adapter_init` cannot be the missing piece — Linux proves DMA
completes without it.

**Queue-init vs state-set ordering, checked and found benign.** Linux
(`al_ssm_main.c:799` then `:802`→`:692`) sets `al_ssm_dma_state_set(UDMA_NORMAL)`
*before* `al_ssm_dma_q_init`; uboot-port's `al_ssm_dma.c:273,277` does q_init *then*
state_set (reverse order). Traced both register writes in the (verified-identical)
HAL: `al_udma_q_init` (`al_hal_udma_main.c:581-666`) writes only per-queue ring
registers and unconditionally calls `al_udma_q_enable(udma_q, 1)` at the end — it does
not read or depend on the engine-level state. `al_udma_state_set`
(`al_hal_udma_main.c:798-834`) is a single write to the engine-level `change_state`
register, independent of any queue register. The two operations don't interact at the
register level, so this ordering difference is very unlikely to matter — flagged, not
fixed (not tested live by swapping the order; hardware time ran out this session).

**Kernel PCIe host-bridge quirk (`kernel-patches/0001-...patch`, git-tracked, confirmed
wired into the build via `scripts/build-linux-71-ea16.py` — verified, unlike the
untracked-file gap in #129) does nothing beyond SMCC snoop + APP_CONTROL.** Read in
full (298 lines): `al_pcie_internal_notifier()` does exactly 3 register writes (SMCC
sub-master 0, SMCC sub-masters 1-3 for slot<=5, APP_CONTROL) reverse-engineered from
the stock 4.19 kernel's own `al_pci_internal_device_notifier()`. No ATU, no
master-ID/stream-ID table, no window/IOMMU setup — `board_late_init()` in
`uboot-port/board/annapurna/alpine/alpine.c:199-238` already replicates this exactly
(minus the eth carve-out, applied separately post-adapter-init in `al_eth_dm.c`). This
was the complete "does the host-bridge do anything extra" hypothesis; it doesn't.

## The strongest lead now: **vendor stage-3 U-Boot's OWN al_eth TX works, in this same
chainload session, moments before we take over**

On 2026-08-24, catching stock at the `ALPINE_UBNT_NAS_ALL>` prompt and running its
`tftpboot 0x1100000 u-boot-chainload.bin` (809176 bytes) to fetch our freshly-built
image **succeeded** — confirmed indirectly (no other path could have landed us at our
own prompt running the exact freshly-built `2026.07-dirty` image) and directly via
stock's own boot-time banner moments earlier showing `al_eth1 [PRIME], al_eth2` probed
and mapped. Vendor U-Boot 2015.07 has no non-DMA fallback path on this SoC (the Alpine
eth MAC is descriptor-DMA-only) — so **vendor stage-3 U-Boot's al_eth successfully
drives the same physical al_udma M2S engine, in the same boot session, on the same
hardware state our code inherits seconds later via `go 0x1100000`.**

This is a *stronger* reference than Linux: it doesn't require a full kernel boot
(ruling out "needs a full OS-level reset/init"), and it runs in the literal
predecessor state to ours. It also means the earlier "AHCI works, therefore the fabric
routes DMA to DRAM generally" framing (previous revision of this doc) was weaker than
stated — `uboot-port/drivers/ata/ahci.c` is the **generic U-Boot AHCI driver**, with
zero al_udma/al_hal references (confirmed via `grep`); it drives its own AHCI PRDT/
command-list DMA mechanism, a structurally different master from al_udma. AHCI working
does not actually prove anything about al_udma-class master routing — it was never
good evidence either way. The vendor-U-Boot-tftpboot fact is real, better evidence.

**Concrete next step**: vendor U-Boot 2015.07 (`ALPINE_UBNT_NAS_ALL>`) is GPL
(`UNVR-1.3.35-GPL`, already referenced in `docs/reference-sources.md` /
`docs/nand-1.3.35.md` / `docs/uboot-port-plan.md` for other purposes) — pull its
al_eth driver source (or, if unavailable, Ghidra-decompile the stage-3 U-Boot binary
the same way `docs/preboot-decompile.md` already did for the preboot/S2 blobs — that
work explicitly scoped stage-3 U-Boot as "GPL, out of scope" only because source should
exist, not because it's undecompilable) and diff its eth-UDMA bring-up register
sequence against ours. This is the one remaining piece of the chain never directly
inspected.

## 2026-08-24 — vendor stage-3 lead followed through: GPL source has no HAL, actual binary confirms the HAL is not the bug

**GPL U-Boot source does NOT contain the UDMA/eth HAL.**
`UNVR-1.3.35-GPL/u-boot/board/annapurna-labs/alpine_ubnt/Makefile:1` does
`-include $(HAL_TOP)/file_list_base.mk`, and `:50-67` lists `$(HAL_TOP)/drivers/...`
sources — `HAL_TOP` is never defined anywhere in the tarball (`file_list_base.mk`
absent, zero `HAL_TOP :?=` assignments anywhere in the tree). The GPL drop is the
"as-is from Ubiquiti" README says: kernel + the thin U-Boot **glue** layer
(`drivers/net/al_eth.c`, `al_eth_pci.c`, `board/annapurna-labs/common/*.c`), not the
proprietary `al_hal_udma_*`/`al_hal_eth_*` HAL that does the register-level bring-up.
**The originally-planned "diff vendor source's UDMA bring-up against ours" is not
possible from this GPL drop — there is no vendor HAL source in it.**

**Read the full vendor glue layer anyway — nothing eth/UDMA-specific found:**
- `drivers/net/al_eth.c` (`al_eth_register`, `al_eth_dev_init`, `al_eth_init`) and
  `drivers/net/al_eth_pci.c` (`al_eth_pci_probe`): plain BAR map +
  `PCI_COMMAND_MEMORY|PCI_COMMAND_MASTER` enable, then straight into
  `al_eth_adapter_init`/`al_eth_queue_config`/`al_eth_queue_enable` (all HAL, missing
  source). No SMCC, no fabric, no unit-adapter call visible at this layer.
- `board/annapurna-labs/common/pci.c` (full file read): internal-PCI hose init
  (`pci_init_board`) and BAR config (`al_pci_hose_config_device`) are generic —
  no SMCC/snoop/coherency register writes anywhere in the file.
- `arch/arm/include/asm/arch-alpine/iocc.h:21-22` declares
  `pci_internal_snoop_enable(pci_dev_t)` ("Enable internal PCI adapter snooping") —
  **zero implementations, zero call sites anywhere in the tree.** Dead declaration,
  not wired to anything.
- `board/annapurna-labs/alpine_ubnt/board.c` `board_init` (:1460) and
  `board_late_init` (:1626): PLL/MPP/thermal/SerDes-handle/env/DT setup only, no
  SMCC/fabric/PCI-coherency writes.

**Vendor's own `iodma` command disproves the queue-init/state-set ordering
hypothesis.** `board/annapurna-labs/common/iodma.c:152-163` (`iodma_init`) — vendor's
own board-level RAID/UDMA test command, the direct ancestor of uboot-port's
`al_ssm_dma.c` `iodma`:
```c
al_ssm_dma_init(&raid, &raid_udma0_params);              // :152
al_ssm_dma_q_init(&raid, DMA_Q_ID, &raid_tx_params, &raid_rx_params, AL_RAID_Q); // :156
al_ssm_dma_state_set(&raid, UDMA_NORMAL);                 // :160
```
q_init **then** state_set — same order uboot-port uses (`al_ssm_dma.c:273,277`), the
opposite of Linux's `al_ssm_main.c:799`→`:802`. Vendor's own working reference uses
uboot-port's order, not Linux's. The ordering hypothesis (flagged, not tested, in the
previous section) is now **disproven directly**, no live test needed.

**Extracted and Ghidra-decompiled the actual vendor stage-3 U-Boot binary** (not
source — the real shipped code). It was already sitting locally, unnoticed:
`tmp/sections/01-uboot.bin` (a prior session's carve of NOR mtd5, the TOC container,
gitignored/regenerable). `scripts/parse-al-toc.py` on it shows object `[6] uboot`
at container offset `0xac000`, payload size `0xa8bd0`, load/entry `0x1100000`.
Payload starts at image-header+`0x48` (`ghidra.md`'s documented convention) =
container `0xac048`; carved `tmp/sections/uboot-proper-payload.bin` (691,152 B,
gitignored, reproducible from the above offsets — do not commit). Ran
`scripts/ghidra-analyse.py … --arch aarch64 --base 0x1100000` per `docs/ghidra.md`:
clean import, 21 s, **1683 functions**, 0 decompile failures.

**Manually identified and verified the core UDMA HAL functions in the real vendor
binary**, via embedded `AL_ASSERT`-style strings (file/line/`__func__` literals
survive even in a stripped release build because they're passed to non-debug
`printf`/`al_err` calls) in `tmp/ghidra-out/uboot-stage3-vendor/decompiled.c`:

| HAL function | vendor binary (VA / decompiled.c line) | uboot-port reference |
|---|---|---|
| `al_udma_init` | `FUN_0110d908` / :9250 | `al_hal_udma_main.c:524` |
| `al_udma_q_init` | `FUN_0110da2c` / :9316 | `al_hal_udma_main.c:582` |
| `al_udma_q_reset` | `FUN_0110dd44` / :9458 | — |
| `al_udma_q_handle_get` | `FUN_0110de60` / :9514 | — |
| `al_udma_state_set` | `FUN_0110df20` / :9544 | `al_hal_udma_main.c:799` |
| `al_udma_state_get` | `FUN_0110dfbc` / :9576 | — |
| `al_udma_iofic_m2s_error_ints_unmask` | `FUN_0110e528` / :9829 | — |
| `al_udma_iofic_s2m_error_ints_unmask` | `FUN_0110e5c4` / :9853 | — |

Every one of these matches uboot-port **at the register-write level**: `al_udma_state_set`
writes only the engine `change_state` field (decompiled: `*(u32*)(udma_regs+0x204) = ...`,
NORMAL/DISABLE/ABORT enum mapping identical to `al_hal_udma_main.c:813-826`);
`al_udma_q_init` OSCEs the same fields at the same struct offsets and unconditionally
ORs `0x30000` into the queue `cfg` register at the end (matches
`al_hal_udma_main.c:675` `rings.cfg` read-modify-write pattern); the iofic unmask
functions use the identical masks (`0xfffffff`/`0x7fffef47` M2S/S2M, `0x100`/`0x200`
group-3). **No `unit_adapter`/`non_optimal` strings exist anywhere in the vendor
binary at all** (grep for both across the full decompile: zero hits) — vendor's
shipped HAL predates that concept entirely, consistent with (not contradicting) the
already-established finding that Linux's *working* al_ssm skips it too.

**Conclusion: the UDMA-core HAL logic is now confirmed correct against the actual
running vendor binary, not just Linux source.** This closes off "different/older HAL
vintage" as an explanation for good. Combined with the byte-identical Linux-source
diff from the previous section, every register-level UDMA bring-up path we can
inspect (Linux source, our port, and now the real vendor machine code) agrees. The
divergence, if it is register-level at all, is in something **not covered by any of
the three UDMA-HAL-internal comparisons above** — i.e. genuinely outside al_eth/
al_udma/al_ssm code: SoC/fabric-level state that predates all three call sites
(candidates not yet eliminated: ATU/SMMU/master-ID table setup, DRAM controller
region attributes for the heap address actually used, or something the closed
preboot's `FUN_01002804` — preboot-decompile.md's "SoC fabric bring-up, table-driven"
— sets up in a way that isn't visible from any U-Boot-level source, GPL or vendor).

**Reproduce this decompile** (regenerate, don't rely on gitignored `tmp/`):
1. `python3 scripts/parse-al-toc.py tmp/sections/01-uboot.bin` → confirms `uboot`
   object offset/size/load.
2. Carve container `[0xac048 : 0xac048+0xa8bd0]` to a new file (payload = image-header
   `+0x48`, per `docs/ghidra.md` §2).
3. `python3 scripts/ghidra-analyse.py <carved>.bin --name uboot-stage3-vendor --arch
   aarch64 --base 0x1100000 --out tmp/ghidra-out/uboot-stage3-vendor --proj
   tmp/ghidra-proj`.
4. Function names are NOT in the symbol table (stripped) — resolve via the
   `s_al_udma_*`/`s_al_eth_*` assert-string literals next to each `FUN_011408c0(...)`
   call (the AL assert-log helper), same technique as
   `scripts/name-preboot-funcs.py` used for the preboot blob.

## Notes / gotchas
- **Never `md`/`pci display` certain config offsets on the AHCI function (00:08.0)** —
  reading `0x110`/`0x240` there SError'd the box into a full cold reset (2026-08-24,
  this session). `md 0xfe001038` (drtp_inc, write-only) is the same class of gotcha on
  eth. Diagnostic register probing on this hardware is not universally safe to guess at.
- Test loop is gated by a deadlock: a failed ping leaves the eth in ABORT, which
  survives warm reset (change_state can't clear it), so stock's auto-chainload tftp then
  fails -> every build load needs a physical cold-cycle. Stock does not map the eth
  (BAR0 base=0, mem-space disabled), so it can't be un-wedged from the stock prompt.
- Files: `uboot-port/drivers/net/al_eth/al_eth_dm.c` (al_eth_dm_dma_init, the dump
  helper, al_eth_dm_unit_adapter_setup), `uboot-port/drivers/crypto/al_ssm/al_ssm_dma.c`
  (the `iodma` cross-validation test), `hal/udma/al_hal_udma_main.c` (al_udma_set_defaults,
  al_udma_m2s_axi_set), `hal/udma/al_hal_udma_config.c`.
</content>
