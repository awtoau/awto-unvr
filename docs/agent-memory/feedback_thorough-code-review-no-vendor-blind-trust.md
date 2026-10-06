---
name: thorough-code-review-no-vendor-blind-trust
description: "When porting/diffing driver code (e.g. Linux vs U-Boot al_eth/HAL work), check every line carefully - don't skim, don't trust vendor code blindly, verify git tracking"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-08-23T23:29:53.128Z
---

When comparing/porting code between two implementations (e.g. Linux al_eth driver vs U-Boot al_eth driver, both derived from the same Annapurna vendor HAL):

- **Check every single line** - don't sample-check or assume similarity based on file/function names matching. A superficial diff misses the actual bug.
- **Don't blindly trust old vendor code.** The vendor HAL (Annapurna Labs delroth-alpine_hal lineage) was written for a much older kernel/environment - assumptions baked into it (struct layouts, timing, register sequencing) may not hold. Verify correctness of vendor-derived code, don't just port it forward as-is.
- **Verify git tracking explicitly.** Confirm any file that's supposed to be part of the repo actually IS tracked (`git status`, `git ls-files`) - not sitting only in an external reference tree. This project has been bitten by this before: `pcie-al-internal.c` existed only in `/mnt/2tb/unvr-port-refs/linux-v7.1.8/` and was never tracked in `awto-unvr` at all (filed as #129).

**Why:** Said explicitly during the 2026-08-24 overnight #90/#132 U-Boot UDMA hang investigation, after a stale-build incident that same session (agent tested a "reverted" kernel that was never actually rebuilt) already burned significant time from insufficiently careful verification. The standing worry: an agent (or me) skims a diff, assumes vendor code is correct because "it's vendor code," or misses that a file isn't actually committed.

**How to apply:** Any task involving comparing Linux vs U-Boot driver code, or porting/adapting vendor HAL code in this repo - go line-by-line, cite exact file:line on both sides, and explicitly confirm git tracking status of every file touched or referenced as authoritative.
