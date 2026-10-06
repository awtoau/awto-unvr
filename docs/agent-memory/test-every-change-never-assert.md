---
name: test-every-change-never-assert
description: "every woomera change must be booted+verified, never claimed \"fixed\" from source/build alone"
metadata: 
  node_type: memory
  type: feedback
  originSessionId: 0edb0769-b7b5-4d90-bc3a-84dd99588559
  modified: 2026-08-19T01:49:26.170Z
---

Never call a woomera change "fixed" until it's been **booted and verified on the running box**. Source edits, a clean build, or a deployed file are NOT proof.

**Why:** this session I repeatedly asserted fixes (timer0, @0x29, misalign, crypto-disable) that were unverified — and a boot-source problem meant deploys to `/boot` never even loaded, so "fixed" was hollow. Wasted the user's time and trust.

**How to apply:**
- **A clean build is NOT a status/milestone — do not report it as "done ✅".** "Compiles, exit 0, zero warnings" is just the gate you pass before the real work; it proves nothing. Report a result ONLY after it runs on woomera and the specific behaviour is verified (e.g. `ddr bist` prints margins, `ping` replies, `serdes` shows lane lock). Until then say "compiles — UNTESTED on hardware", never "done". This applies to the U-Boot port agents (DDR #80 / eth #83 / serdes): they build compile-only; the real deliverable is a box test cycle I run after merge.
- **Any Linux/kernel/driver work is validated on the actual box** — a clean cross-compile is a de-risk step, NOT validation. The al_eth 7.2 modernization (#79) must be loaded + link brought up on woomera, not just built. (7.1.8 = the established Fedora-build kernel; build against exactly that.)
- Module changes (al_eth/al_ssm iofic, crypto): verify by reload OR boot — check dmesg for the specific symptom = 0.
- DTB/kernel changes: **first resolve where U-Boot actually loads kernel+DTB from** (read `printenv bootcmd` at the stock U-Boot; the box booted OLD kernel+DTB despite new `/boot` files — likely NAND or an ESP autofs shadowing the ext4 `/boot`), deploy THERE, then boot and grep dmesg to confirm the symptom is gone.
- Prefer "real fix" over "disable" (e.g. timer0: add `clock-names="apb_pclk"` so the SP804 primecell probes, don't `status=disabled`).
- Reboots use the SP805 watchdog, never plain `reboot` (#51). Test over SSH / 2nd tio (see [[box-testing-over-ssh]]).
