---
name: file-issues-directly-awto-unvr
description: "awto-unvr: file/edit GitHub issues directly, no approval-asking (user authorized)"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 0edb0769-b7b5-4d90-bc3a-84dd99588559
  modified: 2026-09-04T23:26:04.837Z
---

For **awtoau/awto-unvr**, create/comment/edit GitHub issues **directly** — NEVER ask for approval first, EVEN THOUGH IT'S PUBLIC. No exceptions.

**Why:** the user explicitly and repeatedly authorized it ("you do not need approval in this repo for any issue to be filed ever ... even if it is public") — they treat issue-filing on their own repo like note-taking. Asking "OK to file?" wastes their time and annoys them.

**How to apply:** **NO scrubbing either** — user, 2026-09-05: "public - no scrubbing". Private `/mnt`/`/home` paths, MACs, IPs, PARTUUIDs all stay (see [[no-redact-macs-ips]]). Secrets/credentials are still never posted — that is not scrubbing, that is a leak. Create, comment, edit, close and label freely; show what was done afterward. Overrides the global tiered-GitHub-approval rule for awtoau/awto-unvr and the two sibling forks awtoau/awto-uboot and awtoau/awto-uefi ([[awto-uboot-uefi-forks]]) — same owner, created under the same instruction.

Subagents doing issue work need this stated in their brief, or they draft locally and wait for approval that never comes (happened 2026-09-05, #140/#249 comments sat in `tmp/` until told).
