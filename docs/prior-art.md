# Prior art: repo trace and project status

Traced 2026-08-15 with `scripts/research-prior-art.py` (metadata, forks ahead of
parent, issues, PRs). Raw data: `tmp/logs/prior-art.json`, readable dump
`tmp/logs/prior-art.md`. Re-run to refresh.

Two families of project, often confused:

- **Kernel/OS port** — run your own Linux on UNVR hardware. What we want.
- **Protect-in-Docker** — run Ubiquiti's Protect userland on *other* arm64 hardware.
  Nothing to do with booting the UNVR.

## Kernel / OS port

Repo-by-repo table (last commit, stars, notes): [sources.md](sources.md) §1-2.
Headline: **linux-alpine-v2 is most advanced** (6.12 LTS, UNVR DTS, tested on real
hardware); every project in the family is **dormant** — newest code anywhere is
linux-alpine-v2 (Apr 2026) and mornepousse's al_eth fork (Mar 2026). Expect to be
your own maintainer.

(`gh search`'s `updated_at` is bumped by stars/metadata; `pushed_at` — used in
sources.md's table — is the real code date. linux-alpine-v2 is 2026-04-15, not
"July": four months idle, not three weeks.)

### Open issues worth knowing before you start

- **linux-alpine-v2 #1 (open, 0 comments):** *AHCI port 2 fails to link up on warm
  reboot (both controllers).* Only open issue on the port, and it hits a 4-bay NAS
  directly. Unresolved.
- **UNVR-NAS #31 (closed):** *Old USB UNVR* — our variant specifically.
- **UNVR-NAS #22 (closed, 9 comments):** *Bootcmdrecovery won't load.*
- **UNVR-NAS #24 (closed, 8 comments):** rescue mode sees no drive, rootfs errors,
  missing quota.
- **UNVR-NAS #33 (closed, 2026-08-02):** *"I am surprised you stopped"* — confirms
  the project is abandoned, not paused.
- **UNVR-diy-os #2 (open, 14 comments):** *New hardware version* — later UNVRs differ
  (eMMC instead of the USB stick). Check which variant before following any guide.
- **urnvr #7 (open, 5 comments):** *Project Status Update.* #6: modules missing on
  first boot.

## Protect-in-Docker (different problem)

Runs Protect userland on generic arm64. Does **not** emulate Alpine V2 hardware and
is not a path to booting the UNVR. Repo table + the storage-gap root cause
(absent enclosure i2c, not a missing disk): [sources.md](sources.md) §7.

- **markdegrootnl #17 (closed, 7 comments): *"using qemu to run this on x86?"*** and
  **PR #18 *"first working version on x86"*** — relevant if x86 emulation of the
  userland is ever wanted. Again, userland only.
- Gotcha with real cost: the setup wizard hides the username; it is `root`, and a
  wrong username returns 403 (reads like a lockout).

## GPL source: legally required, practically withheld

Ubiquiti is obliged under GPL-2.0 and does not comply — sources, repo table, and
the request-goes-unanswered evidence: [sources.md](sources.md) §3. Escalation
paths beyond FSF: kernel.org (for kernels), Software Freedom Conservancy.

**Practical consequence:** do not plan around receiving a 5.1.25-era GPL kernel.
The realistic source is [UrNVR/unvr-kernel](https://github.com/UrNVR/unvr-kernel) —
4.19.152, mirrored 2022, matching the *kernel version* the current firmware still
runs even though the firmware itself has moved to 5.1.25. Filing a request costs
nothing and strengthens the compliance record, but it is not a dependency.

## Tooling note

`mcp__playwrong__search` returned *"no results parsed — DuckDuckGo may have changed
its markup"* on one query during this research. Second playwrong gap found today,
after the missing binary-download tool. Both belong in
`awtoau/awto-playwrong`; drafts held pending approval (public repo).
