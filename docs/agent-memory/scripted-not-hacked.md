---
name: scripted-not-hacked
description: "Never reach for inline python3 -c/console one-liners for anything repeatable on awto-unvr - extend the existing scripts/*.py instead, even mid-investigation"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-09-02T04:29:40.884Z
---

Even during live, exploratory hardware debugging, don't reach for inline
`python3 -c "..."` snippets (console interaction, power on/off, ad-hoc
network probes) for anything that could recur. Extend or add a proper
`scripts/<name>.py` instead, immediately, not as cleanup afterward.

**Why:** User said "this is supposed to be scripted not hacked" after I
wrote an inline `python3 -c` power on/off snippet using `aioesphomeapi`
directly - despite `scripts/power-cycle.py`'s own docstring already
explaining it exists specifically to kill that exact anti-pattern ("that
bypassed this project's own house rule... three times in one session
before this command existed"). I repeated the same mistake for the one
case (plain on/off, not a full cycle) the existing script didn't cover
yet, instead of just adding `--on`/`--off` to it. See
[[granular-commits-per-fix]] and [[never-delete-move-to-debris]] for the
project's general script-discipline expectations.

**How to apply:** Before writing any inline snippet on this project - even
a "quick check" during live debugging - ask whether a `scripts/*.py`
already does something close (grep first) and extend it, or write a new
committed script. This applies especially to: power control, console
interaction, network reconfiguration, anything touching the physical box.
One-off truly-disposable diagnostics (e.g. a single grep/cat) are fine
inline; anything that manipulates box/network state is not.
