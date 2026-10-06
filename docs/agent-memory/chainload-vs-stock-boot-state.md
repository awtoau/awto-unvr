---
name: chainload-vs-stock-boot-state
description: "woomera's default boot since #216 is stock -> awto-uboot (NAND) -> SSD -> Fedora, so a plain reset passes THROUGH awto-nas# every time; how to land there for dev work"
metadata: 
  node_type: memory
  type: project
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-09-04T23:23:41.968Z
---

A plain reset/power-cycle boots to Fedora login with no interaction — but via awto-uboot at NAND `0x1300000`, which then ext4loads the kernel off the SSD (#216, 2026-09-04). See [[nand-boot-layout-recovery]].

- **To land at `awto-nas#` for dev work: `./dev.py uboot-test --cold`.** Reliable. It power-cycles, catches stock, tftp-chainloads the FRESH `u-boot.bin` from the tree (so the stamped banner of the current build appears, #258) and stops at the prompt. It does not touch NAND.
- **The `awto-nas#` prompt is NOT sticky.** Its autoboot countdown keeps running; a slow follow-up command lands in Linux instead. Scripts race it once deliberately (`range(30)` × 1 s catch loops) — do not remove that.
- `catch-uboot.py` stops at **stock's** prompt (`ALPINE_UBNT_NAS_ALL>`), not ours. Different tool, different job.
- Stock's `bootcmd` is not overridden and must not be — stock env is the one thing this project is strict about never touching. The chainload is what NAND `0x1300000` holds, not a stock env change.

**Why:** this memory said the default was stock→NAND-Fedora and that a reset never reached awto-nas#; both flipped with #216 and it sat stale two weeks.
**How to apply:** check `./dev.py console-peek` before assuming which stage the box is at. If a script is typing into the wrong stage, the first echoed keystroke proves it — abort, do not wait out a timeout (`cdc58d1`).
