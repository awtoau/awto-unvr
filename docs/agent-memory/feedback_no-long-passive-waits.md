---
name: no-long-passive-waits
description: "never schedule a long blind wakeup/sleep to wait out a slow service timeout - poll short and often, or find another way to unblock, instead"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-08-24T07:05:36.300Z
---

Never schedule a single long wakeup (e.g. 10 minutes) to passively wait out a slow boot/service timeout (e.g. NetworkManager stalling on the #131 al_eth crash). User called this out directly: "you are not waiting 10 minutes - that is fucked reduce the timerss on all of those stiing thisiss just dumb."

**Why:** a long blind sleep wastes the whole interval even if the real state changes in seconds — there is no way to know in advance, and the user has no visibility into what's happening during the wait. It reads as passive/lazy rather than actively working the problem.

**How to apply:**
- If genuinely blocked on something slow (a service timeout, a long build), poll with SHORT intervals (tens of seconds, not minutes) and check real state each time — don't guess a duration and sleep through it.
- Prefer actively unblocking over waiting: check if the thing being waited on is even a real dependency (e.g. console/serial access on this box does NOT depend on NetworkManager finishing — check that first before assuming a wait is required at all).
- If a wait is truly unavoidable, keep checks frequent and visible, and use the time productively (parallel work) rather than a single opaque gap.
- Applies broadly: this project's own CLAUDE.md rule (aggressive 1.25x timeouts, no orders-of-magnitude margin) is really the same principle — don't pad wait times "to be safe," derive/check the real state instead.
