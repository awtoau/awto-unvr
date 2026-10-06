---
name: track-latest-mainline-port-forward
description: "This project's kernel tree deliberately tracks torvalds/linux HEAD directly; pulling forward and re-verifying OOT drivers is the normal workflow"
metadata: 
  node_type: memory
  type: project
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-08-31T04:02:43.926Z
---

`/mnt/2tb/unvr-port-refs/linux-v7.3-fresh` has `origin` = `torvalds/linux.git`
directly (not a distro/stable fork) and a local branch `latest` that tracks
`origin/master`. This is deliberate: the project's methodology is "take
mainline HEAD, make it work" — done before starting from 7.1, done again
2026-08-31 pulling 473 commits forward to Linux 7.3-rc1, no issues.

**Why:** Corrected directly. When a kernel workqueue crash showed up on a
freshly-enabled driver, I suggested NOT pulling latest mainline (473 commits,
risk of breaking the 5 heavily-patched OOT Alpine modules) in favor of
caution. User's reaction: "that is silly - that is our whole basis - we take
the latest and make it work - we did this from 7.1 with no issues stop
[w]avering." Pulling forward is the default, not an exception needing
special justification.

**How to apply:**
- Don't treat "pull latest mainline and rebuild" as a big risky ask requiring
  extra hesitation — it's this project's normal cadence.
- Board-support patches (DTS, `pcie-al-internal.c`, PCIe Kconfig/Makefile
  hooks, `unvr_defconfig`) must be **real git commits** on top of `latest`,
  not permanent uncommitted working-tree diffs — the old pattern (found
  2026-08-31: patches existed only as dirty-tree state, never committed in
  the tree's history) meant every update needed a manual `git stash` dance.
  Commit them; future updates become `stash → merge --ff-only → pop`
  (or just a clean history if committed before the pull) instead of hoping
  nothing conflicts.
- `awto-au/linux` (public fork, remote name `awto`) is where these board
  patches get pushed, as small topic branches (e.g.
  `unvr/board-support-7.3`), matching the existing branch-per-topic
  convention (`r8169/pcie-max-speed-param`) — never push to `origin`
  (Linus's tree).
- After any pull: full rebuild via `./dev.py build-fedora` (kernel + all 5
  OOT modules) is the validation step, not a separate ask.
- See [[no-untracked-files-left-behind]] for the broader git-hygiene point
  this incident also surfaced.
