---
name: agents-code-i-coordinate
description: "On awto-unvr, dispatch implementation work to subagents; the main session coordinates, integrates, builds and tests on hardware"
metadata:
  type: feedback
---

**Dispatch programming work to subagents; coordinate rather than write it myself** — user directive 2026-09-04: "make sure agents do the programming work and you coordinate".

**Why:** the project generates more implementation work than one session can carry, and main-session context is better spent on integration, hardware testing and judgement than on typing code.

**How to apply:**
- Give each agent the analysis already done (APIs, blockers, file:line references) so it does not re-derive; say explicitly what NOT to re-research.
- **Hard constraints in every coding-agent prompt:**
  - No hardware: nothing at 192.168.25.x, no serial console, no `deploy-fedora` / `uboot-test` / `power-cycle` / `flash`. The box is a single shared resource.
  - Only ONE agent may run `./dev.py build-fedora` at a time — the build output dir is shared and concurrent builds corrupt it. Tell them to check for a running build first.
  - Do not commit; leave changes in the working tree and report exactly what changed.
  - Point at CLAUDE.md rules: terse point-form comments (3-line default, 6 hard limit), no measurements in comments, SPDX + Awto/Daniel Tyrrell GPL-2.0-or-later on new files.
- Keep agents on **disjoint files** — two agents editing `modules/` at once will conflict.
- **I** do: build, deploy, test on the box, granular commits per fix, file issues with evidence, and verify their claims rather than relaying them. See [[test-every-change-never-assert]].
