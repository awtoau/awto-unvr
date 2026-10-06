---
name: stale-issues-drop-unreproducible
description: "awto-unvr issue triage rule (user, 2026-09-05) — cannot reproduce + project has moved on → close it; hardware checks are quick tests, not investigations"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-09-04T23:25:29.169Z
---

User directives during the 2026-09-05 full-issue review (109 → 59 open):
- "If we cannot replicate an issue given we have moved so far, drop it."
- "Don't go deep — quick test and move on."
- "Stop the soak test and try to close easy issues" — closing beats soaking when both compete for the box.

**Why:** stale issues are noise that misleads the next session — #90's title sent three sessions chasing TX/DMA when the bug was RX. Old issue bodies here routinely describe symptoms fixed hours or weeks later.

**How to apply:**
- Verify status against the tree (`git log --grep=<n>`, read the code), never trust the issue text.
- Close if unreproducible AND overtaken. Keep open only if the defect is still visible in the code — the test is "can I still find this problem", not "has anyone hit it lately".
- Hardware confirmations are timeboxed: a few minutes each, record what you saw, move on. Skip anything needing real code work.
- Post a terse point-form status on every issue touched; close with what changed since (cite commits).
- Route measurement-only findings to the parked perf bucket (#241); do not relitigate there.
- Label bootloader issues `uboot`/`uefi`; those overtaken by the #256 rewrite get `rewrite` and become REQUIREMENTS, not bugs.
See [[file-issues-directly-awto-unvr]], [[agents-code-i-coordinate]].
