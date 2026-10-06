---
name: no-untracked-files-left-behind
description: Untracked/uncommitted files are never acceptable to just note and leave — always commit or remove them
metadata: 
  node_type: memory
  type: feedback
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-08-31T04:02:18.417Z
---

Never treat an untracked or uncommitted file as background noise to mention
in passing and move on from. Every session should end with a clean `git
status` — nothing untracked, nothing uncommitted, nothing unpushed (unless
the user explicitly says to leave something staged/local).

**Why:** Corrected directly — I described a 351-line, complete, substantive
design doc (`docs/multi-connection-manager-plan.md`, never committed in 4
days) as "pre-existing untracked... not touched" and left it alone. User's
reaction: "why untracked - this is just dumb - what do you accept untracked
as normal?" The file turned out to be real, finished work that should have
been committed the moment it was found, not narrated as an aside.

**How to apply:**
- On finding any untracked/uncommitted file (even one that predates the
  session, even one you didn't create): read it, judge whether it's real
  work or disposable scratch, and act — commit it (with a proper message) or
  delete it. Don't just report its existence and continue.
- This applies whether or not the file relates to the current task.
- Related: [[never-delete-move-to-debris]] for what "disposable" actually
  means on this project (very little — most things go to debris/, not rm).
- Same standard applies across repos this session touches (this was caught
  in awto-unvr but the principle isn't project-specific).
