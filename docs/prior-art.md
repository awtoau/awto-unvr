# Prior art: repo trace and project status

Traced 2026-08-15 with `scripts/research-prior-art.py` (metadata, forks ahead of
parent, issues, PRs). Raw data: `tmp/logs/prior-art.json`, readable dump
`tmp/logs/prior-art.md`. Re-run to refresh.

Two families of project, often confused:

- **Kernel/OS port** — run your own Linux on UNVR hardware. What we want.
- **Protect-in-Docker** — run Ubiquiti's Protect userland on *other* arm64 hardware.
  Nothing to do with booting the UNVR.

## Kernel / OS port

| Repo | Last commit | ★ | Status |
|---|---|---|---|
| [bcyangkmluohmars/linux-alpine-v2](https://github.com/bcyangkmluohmars/linux-alpine-v2) | 2026-04-15 | 3 | **Most advanced. 6.12 LTS, UNVR DTS, tested on hardware.** |
| [delroth/linux-qnap-tsx32x](https://github.com/delroth/linux-qnap-tsx32x) | 2025-05-14 | 8 | Same SoC via QNAP. 2.1 GB tree. |
| [delroth/al_eth-standalone](https://github.com/delroth/al_eth-standalone) | 2025-03-05 | 6 | Upstream of the al_eth port |
| [mornepousse/al_eth-standalone](https://github.com/mornepousse/al_eth-standalone) | **2026-03-20** | 0 | **Fork AHEAD of delroth** — MDIO C22 callbacks for kernel ≥ 6.3 |
| [mornepousse/al_thermal-standalone](https://github.com/mornepousse/al_thermal-standalone) | 2026-03-20 | 0 | Alpine thermal sensor for modern kernels |
| [delroth/alpine_hal](https://github.com/delroth/alpine_hal) | 2022-02-02 | 3 | Annapurna HAL, 6 MB |
| [UrNVR/unvr-kernel](https://github.com/UrNVR/unvr-kernel) | 2022-02-24 | 2 | **Ubiquiti GPL kernel 4.19.152, 187 MB.** Dormant but complete |
| [UrNVR/urnvr](https://github.com/UrNVR/urnvr) | 2022-02-26 | 17 | Buildroot OS, SSH install. **Dormant, 7 open issues** |
| [riptidewave93/UNVR-NAS](https://github.com/riptidewave93/UNVR-NAS) | 2024-12-08 | 113 | Debian+OMV. **Dormant ~20 months**, 11 forks none newer |
| [NeccoNeko/UNVR-diy-os](https://github.com/NeccoNeko/UNVR-diy-os) | 2020-12-23 | 28 | Exploration notes, hardware specs. Dormant |
| [NeccoNeko/UBNT-source-code](https://github.com/NeccoNeko/UBNT-source-code) | 2022-02-24 | 12 | GPL source archive, 158 MB |

**Correction to an earlier note:** linux-alpine-v2's last *commit* is 2026-04-15,
not July. `gh search` reports `updated_at` (bumped by stars/metadata); `pushed_at`
is the real code date. Four months idle, not three weeks.

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

**Nobody is actively maintaining a UNVR OS port.** Every project is dormant; the
newest code anywhere in this family is linux-alpine-v2 (Apr 2026) and mornepousse's
al_eth fork (Mar 2026). Expect to be your own maintainer.

## Protect-in-Docker (different problem)

Runs Protect userland on generic arm64. Does **not** emulate Alpine V2 hardware and
is not a path to booting the UNVR.

| Repo | Last commit | ★ | Status |
|---|---|---|---|
| [dciancu/unifi-protect-unvr-docker-arm64](https://github.com/dciancu/unifi-protect-unvr-docker-arm64) | 2026-08-04 | 167 | **The live one.** 71 issues, actively maintained |
| [rjmotion/unifi-unvr-emu](https://github.com/rjmotion/unifi-unvr-emu) | 2026-07-26 | 1 | Fork of snowsnoot. Has a `BREAK-LOG.md` of 5 build bugs |
| [snowsnoot/unifi-unvr-arm64](https://github.com/snowsnoot/unifi-unvr-arm64) | 2025-02-24 | 45 | Fork of markdegrootnl |
| [markdegrootnl/unifi-protect-arm64](https://github.com/markdegrootnl/unifi-protect-arm64) | 2023-06-14 | 145 | **ARCHIVED.** 3 forks ahead |

- Despite the name, `unifi-unvr-emu` is **not** hardware emulation — it's a Docker
  container. Verified on a Pi CM4, UNVR firmware v5.1.19 → unifi-protect 7.1.77.
  Camera adoption and PTZ work; storage reports unconfigured.
- Storage is the recurring failure across all of them (gRPC `ustate` needs enclosure
  hardware). dciancu's mock is the proven fix.
- **markdegrootnl #17 (closed, 7 comments): *"using qemu to run this on x86?"*** and
  **PR #18 *"first working version on x86"*** — relevant if x86 emulation of the
  userland is ever wanted. Again, userland only.
- Gotcha with real cost: the setup wizard hides the username; it is `root`, and a
  wrong username returns 403 (reads like a lockout).

## GPL source: legally required, practically withheld

You are right that Ubiquiti must publish kernel sources — GPL-2.0 obliges it. In
practice they do not, and this is documented:

- GPL source links **used to** sit beside the firmware downloads. **All were removed**
  when the download area was redesigned. There is now no reference on ui.com to open
  source being used at all.
- The stated route is a request to `opensource-requests@ui.com` (or `support@ui.com`).
- Community reports: requests unanswered for 6+ months to years; when files did
  arrive they were incomplete. Threads are **locked** with no UI response.
  <https://community.ui.com/questions/GPL-Source-Code/3b658a92-3a51-4a92-a40e-8de31d40775b>
- Raised on the curl mailing list 2026-02-18 as a pattern of GPL violation, citing
  four separate community threads.
  <https://curl.se/mail/archive-2026-02/0003.html>
- Escalation paths named by the community: FSF <https://www.fsf.org/licensing/compliance>
  for distributions, kernel.org for kernels, and the Software Freedom Conservancy.

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
