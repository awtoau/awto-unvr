---
name: genuine-stock-firmware-wont-boot
description: "the 'factory recovery' NAND partition is NOT genuine UBNT stock anymore, and the genuine pristine stock kernel doesn't even RAM-boot on this unit's current firmware state — see #166"
metadata: 
  node_type: memory
  type: project
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-08-29T05:32:36.779Z
---

Investigated 2026-08-29 (issue **awtoau/awto-unvr#166**) while trying to verify
"the vendor source is a working reference" (the assumption behind #23's
al_dma structural-diff work) by actually booting genuine Ubiquiti stock
firmware, not just reading its GPL source.

- **`run bootnand` no longer boots genuine stock.** NAND `0x300000` was assumed
  untouched (see [[nand-boot-layout-recovery]]) but isn't — it now holds an
  intermediate build from this project's own history (`4.19.152-alpine-unvr`,
  2026-07-10), confirmed by CRC32 mismatch against the pristine day-1 dump.
- **Genuine pristine stock (`4.1.37-ubnt`) does not RAM-boot on this unit
  right now** — uImage header verifies, decompresses, DT reservations set up,
  prints `Starting kernel ...`, then total silence (300s wait, nothing).
  Tried with both an external DTB and the box's own native `$fdtaddr` —
  identical hang either way, ruling out a DTB mismatch. Points at PSCI/SMCC/
  EL2-EL3 firmware-interface drift between this unit's current U-Boot state
  and what a Dec-2020 kernel build expects, not a bad image.

**Why it matters:** any future "fall back to booting real vendor code as
ground truth" plan (e.g. for al_dma/al_eth work) is currently blocked. Static
source analysis remains the only available vendor-reference method until
#166's open next-steps are tried (older recovery_kernel build, PSCI version
comparison).

**Where the good backups live:** `/mnt/2tb/git_debris/woomera-mtd/UNVR-74acb941a811-sysidea16-20260815-164356/` — full pristine MTD dump from before any of
this project's flashing work. `mtd02-linux_kernel` = primary kernel
(`20201216` build, the unit's actual last-shipped kernel). `mtd10-recovery_kernel` = older factory image (`20191213` build). **Raw MTD kernel dumps have
a 4-byte length prefix before the real uImage** (`bootnand`'s own env reads 4
bytes for `filesize` first) — strip it before feeding to `bootm`/`ram-boot-deploy`, or `bootm` fails instantly with "Wrong Image Format".

Relates to [[dont-blame-hardware-assume-our-code]] (the assumption this
investigation was testing) and [[al-dma-udma-state-survives-reload]].
