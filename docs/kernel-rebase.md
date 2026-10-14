# Rebasing the kernel onto newer upstream

The port deliberately tracks `torvalds/linux` HEAD, not a stable base. Pulling
forward and fixing the out-of-tree drivers is the normal workflow.

Check where everything sits: `./scripts/refs-check.py` (add `--fetch` for a
network check). It reports `ours` / `theirs` per tree and names what is behind.

## The trees

| tree | role |
|---|---|
| `tmp/refs/linux-v7.3-fresh` | what we build (`AWTO_KERNEL_SRC`). Store worktree on branch `latest` (our port series), so `.git` is a file. |
| `tmp/refs/truenas-linux`, `linux-qnap-tsx32x`, `urnvr-kernel-4.19.152` | comparison kernels, store worktrees detached at a pinned commit (`scripts/kernel-store.py` `TREES`). |
| `linux-6.12`, `linux-v6.18*`, `linux-v7.1.8` | old bases kept for bisect. Pinned - being behind is their job. |
| `modules/al_*` in this repo | the OOT drivers. Not in the kernel tree. |

## Linux store (#350)

- Every kernel tree is a `git worktree` of the machine-wide Linux store: one object store, every upstream a remote (`torvalds`, `next`, `stable`, `truenas`, `qnap`, `urnvr`, `awto`).
- Never `git clone` a kernel here, never `--depth`. Need another base: add it to `TREES`, `./dev.py kernel-store checkout <tree>`.
- Never `git gc` / `git repack` in the store or its worktrees: shared, maintained by a machine timer.
- Reached through gitignored symlinks, so no host path is tracked:
  - `tmp/linux-store` -> the store (env `UNVR_LINUX_STORE` overrides)
  - `tmp/refs` -> the reference-tree dir (env `UNVR_REFS` overrides)
  - Set up once per checkout: `./dev.py kernel-store setup --store PATH --refs DIR`. `./dev.py clean` removes them; re-run setup.
- `./dev.py kernel-store` shows each tree's HEAD against its pin.
- Remotes other than `torvalds` are `--no-tags`: fetch a tag explicitly (`git -C tmp/linux-store fetch --no-tags <remote> tag <tag>`).
- Our series branches: `awto/unvr/<topic>-<base>`; push to the `awto` remote only with the owner's OK.

## Our carried commits

8 on top of upstream. 6 are board support:

- `3c1292359df6` arm64: dts: amazon: UNVR/UDM-Pro board DTS
- `4e1b0b2db2b8` arm64: configs: unvr_defconfig
- `3ca6472522b9` PCI: al-internal: register defs to a standalone header
- `1cc8ac441dd6` i2c: designware: opt out of `IC_ENABLE.ABORT` + forced SDA RX hold (#86)
- `402f60d8aa1e` ata: ahci_alpine: per-port MSI-X (#92)
- `ec376ab2a93d` arm64: alpine: select `NET_DEVLINK`, enable `AHCI_ALPINE`

`patches/` holds the same changes as files for upstream submission (#224).

## Procedure

1. `./scripts/refs-check.py --fetch` - confirm the delta and the target tag.
2. In the kernel tree: `git rebase <tag>` onto the new base. Conflicts land in
   the 6 commits above; each is small and self-contained.
3. `./dev.py build-fedora` - the OOT drivers are built against the new headers
   here, and this is where a kernel API change surfaces. `CONFIG_WERROR=y`, so
   a new warning in `modules/al_*` fails the build.
4. `./dev.py gate` - `hal-drift` confirms every shared HAL file is the one in
   `modules/al_hal/` and every remaining fork is registered (#264), `dt-drift`
   the Linux/U-Boot board facts (#221).
5. `./dev.py publish-fedora` then `./dev.py deploy-ssd --reboot` - **two
   separate commands**, never chained: an scp once raced the file being written
   and produced a Bad Data CRC.
6. `./dev.py verify-versions` - every stage embeds its git SHA (#258). The
   kernel row must read `OK`, not `STALE`.
7. Retest on the box. Minimum: both al_eth ports at line rate, all disks
   present, zero new dmesg WARN/ERR. `./dev.py soak-test` for load.

## What usually breaks

- **OOT driver API drift.** The `al_*` modules use internal kernel API; a
  rename or signature change is the common failure. `WERROR` catches it at
  step 3.
- **Driver already upstream.** Check whether mainline gained a driver for
  something we carry OOT before fixing ours (#213 decides in-tree vs OOT).
- **defconfig churn.** `unvr_defconfig` can silently lose a symbol when
  upstream renames it. A config symbol is not a working driver - check it
  *bound*, on the box.

## U-Boot and EDK2

Separate forks, not rebased with the kernel:
`/mnt/2tb/git/github.com/awto-au/awto-uboot` (v2026.10-rc3) and `awto-uefi`
(edk2-stable202608), both with `upstream` remotes kept. `refs-check.py`
reports newer tags for the reference trees under `tmp/refs/`.
