# [Setup] Branch / repo structure for the port
labels: porting, setup, meta
---

Stand up the branch scheme so the 6.12 / 6.18 / 7.1 tracks and reference trees don't
collide. Complements the meta tracker (live #4).
Roadmap: [Branch scheme](../porting-roadmap.md#branch-scheme).

## Branches
- [ ] `reference/vendor-4.1` — Ubiquiti 4.19.152 GPL, read-only.
- [ ] `reference/alpine-6.12` — community 6.12 port + our 6.12.103 ea16 build.
- [ ] `alpine-v2-6.18-bringup` — dirty bring-up (works > clean).
- [ ] `alpine-v2-6.18-clean` — reworked into reviewable patches (**deploy branch**).
- [ ] `alpine-v2-7.1` — forward-port (**upstream-dev branch**).
- [ ] `alpine-v2-upstream` — per-subsystem submission slices.

## Commit hygiene
- [ ] Separate **functional** commits (new capability) from **mechanical** ones
  (rebase, API rename, mass reformat).
- [ ] Reference trees stay read-only (`/mnt/2tb/unvr-port-refs/` mirrors upstream).

## Acceptance criteria
- [ ] All branches exist with the intended base; policy documented in the roadmap.
- [ ] A commit's type (functional vs mechanical) is obvious from its message.

Relates: meta (live #4).
