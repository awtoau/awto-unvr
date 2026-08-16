# [Milestone] SW MD RAID baseline (CPU parity, no accel)
labels: porting, raid, phase-6, milestone
---

Stage 8 first step and part of the immediate backlog: prove a **working 4-drive
array on plain MD RAID + CPU parity** BEFORE any dmaengine accel work.
Roadmap: [Stage 8 row](../porting-roadmap.md#14-stage-plan-reconciliation),
[first useful completion](../porting-roadmap.md#first-useful-completion-definition-of-done-for-the-deploy-branch).

## Why separate from the accel issue
- Live #11/#23 (RAID via dmaengine) are the **acceleration** path.
- This is the baseline the whole NAS ships on: array MUST be usable with the accel
  driver absent. Stock already loads `raid456` + `async_pq/async_xor/async_raid6_recov`
  ([modules.txt](../hw-reference/20260816-104601/modules.txt)) — CPU fallback proven.

## Tasks (needs drives fitted — all AHCI ports were link-down at capture)
- [ ] 4 SATA bays enumerate on 6.18 (depends on PCIe glue, live #9/#18).
- [ ] `mdadm` create RAID5/6 across 4 bays; ext4; mount.
- [ ] Degrade / rebuild / scrub cycle, no data loss.
- [ ] Confirm parity runs on CPU (`async_*` modules), accel driver not loaded.

## Acceptance criteria
- [ ] Array survives create → degrade → rebuild → scrub with data intact.
- [ ] Documented as the deploy baseline; accel (live #11/#23) is opt-in on top.

Depends: PCIe/SATA bring-up (live #9, #18). Blocks: accel decision (live #11).
