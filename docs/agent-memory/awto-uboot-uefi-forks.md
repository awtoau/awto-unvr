---
name: awto-uboot-uefi-forks
description: "where the U-Boot and EDK2 forks live (created 2026-09-05), their bases and remotes; the #256 fresh rewrite lands in them, not in awto-unvr's uboot-port/"
metadata: 
  node_type: memory
  type: reference
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-09-04T23:25:44.764Z
---

Two public repos, both real forks with full upstream history so a rebase onto a new release is a normal operation:

| repo | checkout | base | branch |
|---|---|---|---|
| github.com/awtoau/awto-uboot | `/mnt/2tb/git/github.com/awto-au/awto-uboot` | u-boot **v2026.10-rc3** (user chose the rc) | `main`, remote `upstream` = u-boot/u-boot |
| github.com/awtoau/awto-uefi | `/mnt/2tb/git/github.com/awto-au/awto-uefi` | edk2 **edk2-stable202608** = `2970e56` (same pin we build against; no newer tag, edk2 has no rc tags) | `main`, remote `upstream` = tianocore/edk2 |

- Pushed over HTTPS via `gh auth setup-git` — the SSH key is not registered for this account.
- edk2 cloned WITHOUT `--recurse-submodules`; the recursive clone was OOM-killed (exit 137).
- **Nothing is built from them yet.** `scripts/uboot-build.py:39` still defaults `UBOOT_TREE` to `/mnt/2tb/unvr-port-refs/u-boot-v2026.07`, and EDK2 builds from `/mnt/2tb/unvr-port-refs/edk2` (pristine) + `Platform/Ubiquiti/UNVR/` in awto-unvr via PACKAGES_PATH. Repointing is part of #256.
- Version stamping (#258) routes per-stage source via `STAGE_SRC_ENV` in `scripts/_repo.py`: set `AWTO_UBOOT_IDENT_SRC` / `AWTO_UEFI_IDENT_SRC` to these checkouts when the switch happens.
- Two known traps for that switch, on #256: `dev.py`'s U-Boot banner regex requires a `-suffix` and will miss a clean `U-Boot 2026.10 (...)`; `awto-uefi` describes as `edk2-stable201903-10800-g2970e5699b`, not the pinned tag name.
- #256 decisions: Linux's HAL is the single source; U-Boot's HAL is DELETED not merged; U-Boot side rewritten fresh as `alu_*`, old tree is reference only; UEFI's `Platform/Ubiquiti/UNVR/` IS carried across (SATA/USB/eth work). 48-item requirements list is on #256.
See [[nand-boot-layout-recovery]], [[kernel-module-mismatch-recurring]].
