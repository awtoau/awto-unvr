---
name: al-dma-udma-state-survives-reload
description: "al_dma's UDMA hardware queue state does not reset on rmmod/modprobe — a wedged queue from one dmatest run silently corrupts the next, even after a full module reload; only a reboot clears it"
metadata: 
  node_type: memory
  type: project
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-08-29T04:47:21.925Z
---

Discovered 2026-08-29 while testing an al_dma fix (#23): `al_dma_free_chan_resources()`
never calls anything to disable the HAL's UDMA queue on free (just `dma_free_coherent()`
+ `tasklet_kill()` + `kfree()`). A second `al_ssm_dma_q_init()` after a
free then fails with `udma: queue (0) already enabled!` (-5), and — more
importantly — even a full `rmmod al_dma; modprobe al_dma` does NOT reset the
underlying UDMA queue's hardware state machine, because that's a PCI-device-level
state, not a driver-software-level one.

**Concretely:** ran dmatest, got a real result (op #1 succeeds, later ops
time out). Reverted the code change back to a previously-known-good commit,
did a full module reload, reran dmatest — the *reverted, previously-good*
code now ALSO failed (even op #1). The hardware queue was left wedged by
the earlier test and survived the reload. Only a genuine reboot (SP805
watchdog reset) restored clean state.

**Why:** invalidates any A/B test on al_dma done via module-reload-only
between runs — both "before" and "after" can be silently testing the same
corrupted hardware state, producing a false negative (or false regression)
that has nothing to do with the code change under test.

**How to apply:**
- Never trust a second (or later) al_dma/dmatest result in the same boot
  without a reboot in between. `rmmod`/`modprobe` is not a reset for this
  driver.
- Fix `al_dma_free_chan_resources()` to actually tell the HAL to disable/
  reset the queue before it's safe to skip the reboot-between-runs
  discipline — not done as of this writing.
- [[bench-all-benchmark-suite]] deliberately excludes al_dma/dmatest for
  exactly this reason — every other subsystem it covers (eth/crypto/disk)
  is safely re-runnable without a reboot; al_dma is not.
- Relates to [[eth-udma-tx-hang]] (same UDMA IP family, different failure
  mode) and [[test-every-change-never-assert]].
