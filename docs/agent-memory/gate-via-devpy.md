---
name: gate-via-devpy
description: "awto-unvr: run ./dev.py gate before calling any task done"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-08-19T23:17:02.312Z
---

For **awtoau/awto-unvr**, gate every task through `./dev.py gate` (fmt-check + lint + test) — run it before reporting a task complete, not just when explicitly asked to test.

**Why:** user directive (2026-08-20): "from now on i want all task to be gate via dev.py." `dev.py` is the repo's canonical entry point (`docs/agents/agent-rules.md` convention) — `describe` gives machine-readable help.

**How to apply:**
- Any code/config change in this repo → run `./dev.py gate` before declaring done. If it fails, fix root cause, don't skip.
- `fmt-check`/`lint` currently WARN-skip because `ruff` isn't on PATH in this env — known gap, not a gate failure to chase yet.
- Fixed 2026-08-20: bare `pytest -q` (no `pytest.ini`) was collecting the vendored U-Boot source tree under `tmp/uboot-build/source/` and always failing gate — added root `pytest.ini` (`testpaths = tests`). See #96 (closed).
- For hardware-touching work, box verification (per [[test-every-change-never-assert]]) is still separate/additional — dev.py gate covers the repo-local static/unit checks, not on-box behavior.
