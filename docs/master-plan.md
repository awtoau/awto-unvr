# Master plan — UNVR as a TrueNAS box

End state: **TrueNAS SCALE on the UNVR**, booting through a stable
stock-U-Boot -> awto-uboot -> awto-uefi chain, on bleeding-edge mainline.

Strategy owner: Dan. This doc is the spine; `porting-roadmap.md` is the
kernel-port detail under phase 2.

## Phases

| # | Phase | State |
|---|---|---|
| 1 | Hardware understood, drivers working | mostly done |
| 2 | Bleeding-edge mainline Linux + Fedora userland, stable on the box | **current** |
| 3 | Boot chain stable: stock U-Boot -> awto-uboot -> awto-uefi | partial |
| 4 | Port the TrueNAS SCALE arm64 tree onto the UNVR | not started |

Phases 2 and 3 are prerequisites, not the goal. The goal is phase 4.

## Phase 2 — mainline + Fedora (current)

- Kernel tracks torvalds HEAD directly, not a stable base. Currently
  **7.3.0-rc6**, 8 carried commits replayed on each pull-forward.
- Userland: Fedora 44 on the SSD. Deploy = `publish-fedora` + `deploy-ssd`
  (scp, not a flash).
- 9 OOT `al_*` modules. Shared HAL in `modules/al_hal/` since #264.
- Done: 10G at line rate, NAND read+write verified, SATA, USB, thermal.
- Open: #182 (al_ssm D-state), #184 (UDP TX hang), #253 (U-Boot 10G TX).

## Phase 3 — boot chain

```
stock U-Boot -> NAND 0x1300000 (awto-uboot, raw, `go`) -> awto-uefi -> Linux
```

- awto-uboot: working, boots the SSD every boot since #216.
- awto-uefi: EDK2 builds and chainloads; **not stable**. #330 asks whether to
  keep, park or drop the track.
- Stock U-Boot is the only stage with working networking, so it is the
  recovery path. `run bootnand` is NOT (#166).
- Forks: `/mnt/2tb/git/github.com/awto-au/{awto-uboot,awto-uefi}`.

## Phase 4 — TrueNAS SCALE port (the goal)

- Target: **TrueNAS 27** (released; 27.0.0-RC.1 preceded it). Ships **kernel
  6.18** and **OpenZFS 2.4**, with frequent updates through 27.x.
- Target tree: `/mnt/2tb/unvr-port-refs/truenas-linux`, branch
  `truenas/linux-6.18`, full arm64 support present.
- **Refetch before use.** Our snapshot is 6.18.46 from 2026-08-25; upstream is
  at **6.18.55**, **4824 commits ahead**. `master-nightly` moves, so pin a
  commit when phase 4 starts.
- **Direction DECIDED (Dan, 2026-10-13): carry our drivers BACK to 6.18.**
  Not TrueNAS's deltas forward. Development stays on bleeding-edge 7.3+; 6.18
  is a build target, not where we work.
- **It must be a generated patch series, not a hand-maintained branch.** The
  7.3 tree keeps moving, so every change has to backport mechanically, with
  hand-coding the exception. Precedent: `patches/*.patch` with
  `check_kernel_patches()` in `scripts/_repo.py` already does this for
  mainline. Tracked as #338.
- No near-miss mainline tree is kept as a diff baseline: the exact source is a
  clone away, so `linux-v6.18.44` was retired with the rest.
- Needs ZFS on arm64 with our drivers, and the storage stack #32 is baselining.
- Hardware prerequisites met: 20.1 TB of disk freed, al_dma XOR/PQ verified
  at 0 failures in 40k ops (#286), so RAID parity offload is available.

## Why this governs disk retention

`/mnt/2tb/unvr-port-refs/` is 62 GB. The plan decides what earns its space:

| keep | why |
|---|---|
| `truenas-linux` 5.4G | **phase 4 target.** Not stray material |
| `UBNT-source-code` 2.6G | GPL drop + working 4.1.37 kernel. Irreplaceable |
| `linux-v7.3-fresh` 1.8G | the mainline reference every audit diffs against |
| `urnvr-kernel-4.19.152` 1.2G | vendor's later NAND driver (#268 model) |
| `linux-qnap-tsx32x` 1.9G | Alpine V1 reference |
| `edk2` 1.1G, `u-boot-v2026.07` 738M | phase 3 references |
| `build-out-fedora` 5.4G | the live build output |

Retired 2026-10-13 to `/mnt/2tb/wastebasket/` (derived artifacts have no
legacy; GB-scale so wastebasket, not `rm`): 35.0 GB of stale `build-out-*`
from Aug-Sep, and 8.2 GB of superseded mainline trees (6.12, v6.18,
v6.18.44, v7.1.8). Refs 62 GB -> 20 GB. `scripts/reclaim-stale-buildouts.py`,
dry-run by default.

## Upstream trees: one mirror, many branches

**Never clone a second copy of a repo we already mirror.** Use
`/mnt/2tb/git_mirror/<owner>/<repo>`, maintained by
`awto-dan/scripts/get_to_mirror.py`. Every branch is `origin/<branch>`, all
tags fetched, one branch checked out; another version is a `git checkout` or
`git worktree add` away.

| mirror | size | covers |
|---|---|---|
| `torvalds/linux` | 8.4 GB | **951 tags** - v6.12, v6.18, v7.3-rc6 all present |
| `truenas/linux` | 5.0 GB | **203 branches**, incl. `truenas/linux-6.18` (phase 4) |
| `u-boot/u-boot` | - | mainline U-Boot |

Why: five single-version clones of linux cost **15.4 GB** and gave one
version each. The two mirrors above cost 13.4 GB and give every version.

Stable point releases (e.g. `v7.1.8`) are **not** in `torvalds/linux` - mirror
`gregkh/linux-stable` if one is needed.

`unvr-port-refs/` keeps only trees that are NOT plain upstream clones:
extracted vendor drops, the UBNT GPL source, and the live `build-out-fedora`.
