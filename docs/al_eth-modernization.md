# al_eth driver: modernization review (2026-08-30)

Agent-assisted review of `modules/al_eth/al_eth_main.c` (~5000 lines,
vendor-authored ~2016, last kernel-tested against ~4.1.37), triggered by
investigating the TX throughput collapse (see `network-throughput-testing.md`).
Every finding cross-checked against the actual kernel tree this driver
builds against (`/mnt/2tb/unvr-port-refs/linux-v7.1.8`), not assumed from
the vendor source or the driver's own comments - a symbol that "looks"
valid but is only reachable via a never-compiled branch turned out to be a
real build-breaking landmine (`skb_tx_hash()`, see below).

## Fixed this session

- **RX path**: converted `netdev_alloc_frag()`+`dma_map_single()` per-packet
  to `page_pool`-based buffer recycling. #168.
- **TX queue selection**: `al_eth_select_queue()` used `smp_processor_id()`
  (per-packet, not per-flow) under `CONFIG_ARCH_ALPINE` - root cause of a
  major (but not the only) TX throughput problem. Fixed with
  `netdev_pick_tx()`. See `network-throughput-testing.md` for the full
  investigation and remaining open TX mystery.
- **`ethtool -L` (`set_channels`)**: added (previously only `get_channels`
  existed). Required two follow-up fixes once actually tested: (1) the RSS
  indirection table wasn't reclamped to the new queue count - shrinking RX
  queues via `ethtool -L` would leave hardware RSS steering traffic into a
  torn-down ring; (2) the down/up cycle needs the real `al_eth_close()`/
  `al_eth_open()` (PHY/MDIO teardown+setup), not the lower-level
  `al_eth_down()`/`al_eth_up()` `al_eth_reset_task()` uses - the legacy-PHY
  1G port needs the former, the phylink-based 10G port tolerates the latter
  but the 1G port doesn't.
- **`kcompat.h`**: header comment falsely claimed "targeting ONLY Linux
  6.12 LTS" - the file is actually shared across every build variant this
  project produces (see `docs/build.md`), and has zero actual
  `LINUX_VERSION_CODE` branching. Comment corrected (since updated again
  when the project moved from pinned kernel versions to tracking mainline
  HEAD directly).
- **`scripts/bench-all.py`**: crypto (AF_ALG) benchmark's `accept()` loop
  had no bound at all (a documented, deliberate trade-off - `settimeout()`
  on that socket breaks `accept()` itself after ~128 calls, a real kernel
  quirk). Left a `python3` process stuck in kernel D-state (uninterruptible,
  unkillable) for 20+ minutes live. Fixed with `signal.alarm()`, which
  doesn't touch the socket option that caused the original problem.

## Filed, not yet implemented

Ranked by what a genuine correctness/stability risk vs. pure modernization:

| # | Severity | Summary |
|---|---|---|
| #171 | High | `rtnl_lock` self-deadlock: `al_eth_reset_task()` vs `al_eth_close()`'s `cancel_work_sync()` - hangs *all* rtnl-based networking on the box, not just this NIC, if they race |
| #172 | High | TX error-unwind double-unmaps a DMA buffer that was never mapped, on TSO/csum setup failure specifically - real memory-safety hazard on IOMMU-backed platforms |
| #175 | Medium | UDMA M2S/S2M error interrupt ("potential user data corruption" per the driver's own comment) is logged but triggers zero automated recovery |
| #173 | Medium | `NETIF_F_NTUPLE` advertised, no `.set_rxnfc` backing at all |
| #174 | Medium | ethtool RSS `get/set_rxfh` doesn't expose or validate hash key/hfunc |
| #179 | Medium | `BUG()` (whole-machine crash) on oversized TX fragment count instead of `WARN_ON_ONCE`+drop; not exploitable under this build's `.config` but no compile-time guard ties it to `CONFIG_MAX_SKB_FRAGS` |
| #176 | Low | ethtool completeness: no `.set_ringparam`/`.self_test`/`.get_regs`/`.set_phys_id` |
| #177 | Low | MSI-X setup uses legacy `pci_enable_msix_exact()` instead of `pci_alloc_irq_vectors()` - works fine today, just dated; also only IRQ affinity *hints*, never enforced |
| #178 | Cosmetic | Two separate `dma_set_mask()`/`dma_set_coherent_mask()` calls instead of the combined `dma_set_mask_and_coherent()` |
| #180 | n/a | Not a task - reference record of what was checked and confirmed already correct (SFP/phylink delegation, IRQ handler style, RX RSS design, `u64_stats_sync` usage, `netif_napi_add` form, NAPI enable/disable sequencing). Filed so a future session doesn't re-investigate the same questions. |

## Separately found, real, dead code (not yet stripped)

A large fraction of this driver is permanently dead on this specific
hardware, because the vendor's one source file supports two very different
deployment models: this board (`CONFIG_ARCH_ALPINE=y`, `board_type` always
`ALPINE_INTEGRATED`) vs. Alpine silicon deployed as a discrete PCIe NIC card
in someone else's (typically x86, virtualization-capable) host. Confirmed
dead on this build:

- Every `#ifndef CONFIG_ARCH_ALPINE` / `#else` of `#ifdef CONFIG_ARCH_ALPINE`
  branch (`CONFIG_ARCH_ALPINE=y` always, confirmed in the build `.config`).
- `CONFIG_AL_ETH_SRIOV`-gated code, in its entirety - not a real Kconfig
  symbol anywhere in this build (SRIOV only makes sense for the NIC-card-in-
  a-hypervisor deployment model we don't ship; a discrete PCIe card
  presenting virtual functions to VMs has no application on a single-OS NAS
  appliance with no hypervisor).
- `IS_NIC(adapter->board_type)`/`board_type != ALPINE_INTEGRATED` runtime
  checks (~12 call sites) - compile fine, never execute; the `pci_device_id`
  table only ever registers `ALPINE_INTEGRATED` entries for the variants
  this project actually builds (`AL_ETH_BUILD_1G_ONLY`/`_10G_ONLY`, real and
  actively used, do NOT confuse with the dead conditions above).

`CONFIG_PHYLIB` and `CONFIG_PHYLINK` are both live (`=y`) - corrected an
initial assumption they might also be dead. `al_eth_phylink.c` already has a
good exemplar pattern for this class of gap: `#if defined(CONFIG_ARCH_ALPINE)
&& !IS_ENABLED(CONFIG_PHYLINK)` triggers a loud `#warning` instead of
silently compiling a stub - worth using as the template.

Confirmed: this dead code is not itself a build-time landmine except in one
case - `skb_tx_hash()` (`al_eth_select_queue()`'s original non-ARCH_ALPINE
branch) is `static`/core-internal in this kernel, not exported, and would
NOT compile if that branch were ever made live. This was discovered turning
the smp_processor_id() fix into reality, not by inspection - a reminder that
"never compiled on this platform" and "definitely still compiles" are not
the same claim, for any of the dead branches above, until actually verified.

Stripping this dead code (not yet done) was agreed as a follow-up, not
bundled into the TX investigation - large, mechanical, low-risk-if-careful,
best done as its own pass rather than mixed into live debugging.
