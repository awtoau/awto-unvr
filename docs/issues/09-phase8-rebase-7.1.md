# [Phase 8] Rebase 6.18 → 7.1
labels: porting, phase-8, rebase
---

Forward-port the series onto Linux 7.1.
Roadmap: [Phase 8](../porting-roadmap.md#phase-8--618--71).

## Method
- [ ] Rebase series onto 7.1 (a local mainline mirror is already on 7.1).
- [ ] Resolve API changes **one subsystem at a time**.
- [ ] Boot after each subsystem group.
- [ ] Compare boot logs / PCI enum / DT state against the 6.18 baseline.
- [ ] Run identical storage + network tests on both.

## Branch policy
- [ ] Keep **6.18 = conservative deployment branch**.
- [ ] **7.1 = upstream-dev branch**.

## Acceptance criteria
- [ ] 7.1 boots the UNVR with the same peripherals working as 6.18.
- [ ] Diff of boot log / PCI enum / DT vs 6.18 reviewed, deltas explained.

Depends: all bring-up issues.
