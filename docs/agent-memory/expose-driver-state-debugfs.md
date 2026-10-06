---
name: expose-driver-state-debugfs
description: "Add debugfs/devlink visibility as a matter of course when writing driver code - don't wait until something is undiagnosable"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-09-03T22:06:11.057Z
---

When touching driver code on awto-unvr, **add debugfs (or devlink) visibility as a matter of course** — user directive 2026-09-04: "we should debugfs more often".

**Why:** the project keeps hitting bugs that are undiagnosable because nothing is exposed. `al_ssm` had **no userspace surface at all**, which is why #182 (AF_ALG wedges a process in D-state) was pure guesswork. `al_eth` read the Clause 49 PCS counters and used only bit 15, discarding the errored-block and BER counters that #196 needed. `al_eth_ec_stats_get()` had zero call sites — 38 drop-reason counters unread. `al_udma_stats_get()` is an empty stub in all three UDMA drivers (#212).

**How to apply:**
- **debugfs** = arbitrary driver diagnostics, no stable ABI by design, conventional in dmaengine/crypto (19 in-tree users there). Cheap; do it while writing the code, not after.
- **devlink health reporters** = the durable surface — `diagnose` for state, `devlink_health_report()` to flag faults, auto-dumps on error. Crypto precedent exists (`otx2_cpt_devlink.c`). Needs `CONFIG_NET_DEVLINK`, which is a **select-only symbol** an OOT module can't enable (see [[in-tree-vs-oot-modules]] / #213).
- **`ethtool -S`** for anything netdev — that's the NIC convention.
- Rule of thumb: **debugfs for bring-up, devlink for anything that outlives bring-up.**
- Treat "a HAL counter block with no consumer" as a defect, same class as declared-but-unreachable code (`al_eth_lm_static_parameters_override()` zero call sites, `al_reboot` never built).
