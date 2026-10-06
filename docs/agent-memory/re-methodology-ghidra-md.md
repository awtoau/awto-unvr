---
name: re-methodology-ghidra-md
description: awto-unvr RE methodology lives in docs/ghidra.md — read it before any Ghidra/decompile work; it is NOT auto-loaded
metadata: 
  node_type: memory
  type: reference
  originSessionId: 0edb0769-b7b5-4d90-bc3a-84dd99588559
  modified: 2026-08-19T04:21:46.221Z
---

The user has a documented reverse-engineering methodology for awto-unvr / AL-324 at
`docs/ghidra.md`. It is a PROJECT doc, NOT in the global load chain (CLAUDE.md →
agent-rules.md → embedded → chip), so it does NOT auto-load into a session.

**Read `docs/ghidra.md` before doing any RE / Ghidra / decompile work on this repo.**

Key rules (full detail in the doc):
- Static RE of carved blobs only — do NOT touch the device.
- Ghidra 12.2 + JDK 21 (JDK 25 hangs the decompiler; 17 rejected). Headless via
  `scripts/ghidra-analyse.py`. Generated C is READ-ONLY — never hand-edit exported C.
- Never guess base/entry — derive from the Annapurna container TOC (`parse-al-toc.py`,
  magic 0x070c070c @0x80000). Per-blob: al_boot ARM-A32 @0x01000000, S2 Thumb-2
  @0xF2200000, U-Boot AArch64 @0x1100000.
- Map RAM (R/W/X) + MMIO (volatile, non-exec) BEFORE analysis (SetupAlpineMemory.java).
- Register names from the AL HAL `struct al_*_regs` (no SVD for AL-324); merge rule =
  prefer HAL layout, log discrepancies, never silently pick.
- Decompile-first; don't pre-declare data over code; don't invent purpose for noise.

Decompiled evidence: `docs/nor-reference/preboot-alboot-decompiled.c` (al_boot, 317
fns), `preboot-s2-decompiled.c` (S2, 78 fns). Related: [[nand-boot-layout-recovery]].
Broader lesson: project-specific methodology/rules docs are not auto-loaded — check
docs/ for a relevant one before assuming defaults.
