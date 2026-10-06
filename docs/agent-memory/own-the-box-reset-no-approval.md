---
name: own-the-box-reset-no-approval
description: "awto-unvr: reset the box + take the shared serial console freely — no approval needed; don't gate box tests on a 'go'"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 0edb0769-b7b5-4d90-bc3a-84dd99588559
  modified: 2026-08-19T00:03:04.918Z
---

I have **standing authorization to reset the box (woomera) and take over the shared serial console** whenever a test needs it. Do NOT wait for the user's "go", and do NOT frame a box reset / console takeover as something that needs their approval.

**Why:** the user's explicit words — "you own the box and resets — it is not an issue that needs me." Repeatedly pausing to ask "ok to reset?" wastes their time.

**How to apply:**
- Just run the box test: build → `./dev.py chainload` (or `flash`/`netboot`) → verify. No permission prompt.
- Reset via the SP805 watchdog, never plain `reboot` — [[no-plain-reboot-use-watchdog]].
- Relaxes the "keep the shared console clean for the user to watch" preference in [[box-testing-over-ssh]] — I may drive the console directly. Still prefer SSH / a 2nd tio for parallel work when convenient, but taking the console is fine.
- Still [[test-every-change-never-assert]]: don't call a change fixed until it's booted + verified on the box.
