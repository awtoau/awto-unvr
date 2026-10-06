---
name: granular-commits-per-fix
description: "one small commit per bug fix, showing the actual fix — no bundling, no shortcuts"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 0edb0769-b7b5-4d90-bc3a-84dd99588559
  modified: 2026-08-17T23:04:44.717Z
---

For the kernel/driver bug fixes (the filed al_* issues — #50 al_ssm crypto,
#52 al_eth UBSAN, #55/#58 of_irq_parse_pci, #56 amba timer, #57 ramoops, etc.),
the owner wants **each fix as its own small individual commit**, not a bundled
"fix all kernel warnings" commit.

**Why:** so they can review *how* each was fixed and that it's a real fix, not a
shortcut/suppression (e.g. don't silence UBSAN with a pragma — fix the array
declaration; don't disable the al_ssm self-test — fix the wrong-AES-output bug).

**How to apply:** one commit per issue, message referencing the issue number and
describing the root cause + the actual change. Prefer proper fixes over
work-arounds; when a proper fix isn't possible, say so explicitly rather than
shipping a shortcut. Compare against the Ubiquiti GPL source (on woomera) where it
helps (esp. al_ssm). Relates to [[storage-repurpose-plan]] work on woomera.
