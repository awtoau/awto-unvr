---
name: docs-terse-no-history
description: "How Dan wants documentation written - terse, bullets, no legacy/history"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 0edb0769-b7b5-4d90-bc3a-84dd99588559
  modified: 2026-08-26T00:49:55.039Z
---

Documentation and comments:
- **No legacy / no history.** State how it is now, not the journey to it. No "previously X, now Y", no changelog narration inside docs, no re-arguing past decisions.
- **Terse, but not lossy.** Keep every fact, number, path, offset, gotcha. Cut connective tissue, not content.
- **Bullets over block text.** Prefer point form. Avoid prose paragraphs.

**Why:** blocks of prose and history bloat docs and bury the facts; the next reader (agent or human) wants current state fast.

**How to apply:** default to bullet lists. A 20-line paragraph becomes ~5 bullets with the same facts. Rationale earns a place only when non-obvious and load-bearing - one line. Reinforces [[global agent-rules]] terse-docs rule; applies to md, PRs, issues, commit bodies, code comments.

**Repeat offender: GitHub issue/PR comments specifically (2026-08-26).** Wrote several awto-unvr issue comments as narrative prose with bold headers and full sentences explaining rationale/methodology - exactly the pattern this memory already forbids. Flagged again by Dan as still too verbose. When posting findings to a GH issue: short bullets, code blocks for register dumps/evidence, one-line conclusion. No "## Header \n paragraph" structure, no restating the investigation narrative - state the finding and the evidence, nothing else.
