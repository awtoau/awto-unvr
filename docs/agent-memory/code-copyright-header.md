---
name: code-copyright-header
description: "awto-unvr code headers: new/original files = Awto/Daniel Tyrrell copyright + GPL-2.0-or-later; copied vendor/HAL files keep their original headers"
metadata: 
  node_type: memory
  type: project
  originSessionId: 0edb0769-b7b5-4d90-bc3a-84dd99588559
  modified: 2026-08-19T01:43:59.651Z
---

Copyright/license convention for source we add to **awto-unvr** (esp. the U-Boot port, kernel drivers):

- **New / original files we author** (shims, glue, drivers, commands) →
  ```
  /* SPDX-License-Identifier: GPL-2.0-or-later */
  /* <desc>
   * Copyright (C) <year> Awto / Daniel Tyrrell <dan@awto.au>
   * Co-authored with Claude (Anthropic).
   * Derived from <upstream> (Copyright (C) <orig>, <orig-license>); reimplemented on U-Boot primitives.
   */
  ```
  Credit Claude as a **co-author** (real engineering), NOT a "vibe-coded"/"AI-generated" tag. Copyright stays with the humans/entity — an AI is a contributor, not a copyright holder.
  GPL-2.0-or-later is the default (matches U-Boot, which is GPL-2.0+). A dual `... OR BSD-3-Clause` only if we want to re-share outside U-Boot.
- **Copied vendor/HAL files, unchanged** (e.g. from delroth-alpine_hal, Annapurna) → **keep their original headers untouched.** Do NOT stamp our copyright on files we merely copied.
- **Modified vendor/HAL files** → keep the original header + add one line `Modified (C) <year> Awto / Daniel Tyrrell`.

**Why:** we reimplement the platform shim / DM_ETH glue / DDR command ourselves (original work → our copyright), but the HAL bulk is Annapurna's tri-licensed source (their copyright stands). Keep provenance honest.
