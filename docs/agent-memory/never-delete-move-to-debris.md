---
name: never-delete-move-to-debris
description: "awto-unvr: never delete files — move to debris/ (scripts → debris/scripts/); overrides global regenerable-delete rule"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 0edb0769-b7b5-4d90-bc3a-84dd99588559
  modified: 2026-08-18T23:56:44.767Z
---

In **awto-unvr**, retired files are **moved to `debris/`, never deleted**. Scripts specifically go to **`debris/scripts/`**. Use `git mv`, not `git rm`.

**Why:** the user's explicit practice — "we don't delete, move to debris" / "scripts are never deleted - just move to debris/scripts". They want retired work recoverable *in-tree* (debris is tracked + committed), which is more discoverable than digging through git history.

**How to apply:**
- Retiring any script/one-off → `git mv scripts/<f> debris/scripts/<f>`. Other retired files → `debris/<type>/`.
- This **overrides** the global agent rule "Regenerable content: just delete" for this repo. Even a regenerable scratch `.tcl`/`.py` goes to debris here, not `rm`.
- Don't pre-create empty debris subdirs beyond what's needed. `debris/` is tracked — `git add` it.
- Precedent: audit [[file-issues-directly-awto-unvr]] cleanup (#77) moved 35 dead one-offs to `debris/scripts/`.
