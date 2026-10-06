---
name: kernel-module-mismatch-recurring
description: "awto-unvr's kernel/module deploy mismatch (Oops in resolve_symbol/ref_module) is a recurring, costly failure class -- check deploy provenance FIRST on any unexplained oops"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-09-04T23:22:54.287Z
---

Kernel-binary-vs-module-tree version mismatches on woomera (awto-unvr) are a
recurring failure class, not a one-off: #105 (2026-08-20, root cause), #131
(module-mismatch during the same incident's follow-up), #161 (2026-08-27,
kernel from Aug 17/18, module tree from Aug 26 -- 9 days apart, no deploy-log
trail in either `fedora-deploy.log` or `deploy-modules-woomera.log`, source
never identified). Same crash signature every time: `Oops` in
`resolve_symbol`/`ref_module`/`idempotent_init_module` during a module load,
usually non-fatal per-boot but per #105's postmortem can escalate to RCU
stall -> full soft lockup if the same oops-tainted rootfs is rebooted
repeatedly "to check if it clears up."

**Why:** per-build-OUT-directory separation (the #131 fix, `AWTO_KASAN_BUILD`
picking a distinct `build-out-71-fedora-kasan/`) only stops the *official
build scripts* from colliding with each other. It does NOT stop a manual/
ad-hoc `rsync`/`scp` run directly against the box's `/lib/modules/`, and
**nothing anywhere checks that the running kernel and its module tree
actually match** -- a mismatch can sit silently for 9+ days producing a
recurring, easy-to-misdiagnose Oops on every boot with zero indication
anything is wrong. User has spent considerable money/tokens across multiple
sessions on this exact class of bug recurring.

**How to apply:**
- **`./dev.py verify-versions` FIRST** (#258, `753879b`, 2026-09-05). Every
  stage now embeds `git describe --always --dirty` — kernel `LOCALVERSION`,
  U-Boot `CONFIG_IDENT_STRING`, UEFI version PCD — and this scans the console
  log for each banner and compares. STALE = the answer; UNSTAMPED = binary
  predates stamping, rebuild it. `deploy-ssd --reboot` runs it automatically.
  This is the structural fix #161 asked for; it exists now, use it.
- On ANY unexplained Oops involving `resolve_symbol`/`ref_module`/module
  loading: provenance first, before any other hypothesis.
- **Module file mtimes LIE.** rsync preserves them, so `/lib/modules/*.ko`
  can look 10 h older than the fix that is in them (seen 2026-09-05, #245).
  Compare `modinfo srcversion` or `md5sum` against the build-out, never dates.
- Never reboot a box that just produced this oops "to check if it clears
  up" — #105's postmortem: repeated reboots of an oops-tainted rootfs
  escalated to full soft lockup needing 3 physical power-cycles.
- #242 was the same class in the tooling: `build-on-box.py`'s vermagic
  guard checked `al_eth.ko`, which stopped being built after the 1g/10g
  split, so the guard was silently dead for weeks (fixed `5f56952`). A check
  that names a specific artifact rots when the artifact is renamed.
