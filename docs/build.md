# Build options — one map for all of them

Every build in this repo goes through `./dev.py <command>`. This doc is the
index; each linked doc covers the version-specific deltas in depth.

## Inventory

Kernel target is `/mnt/2tb/unvr-port-refs/linux-v7.3-fresh`, tracking
`torvalds/linux` mainline directly (this repo's own methodology: pull
forward, port whatever breaks). Board support (DTS, `pcie-al-internal.c`,
the `pcie-al.c` DBI fix, `unvr_defconfig`) lives as real commits in that
tree's own git history - not as patches applied by any script here.
`patches/*.patch` are staging artifacts for that: `git apply` + commit them
in the kernel tree, they are not applied by a build.

### `patches/` - kernel tree (`/mnt/2tb/unvr-port-refs/linux-v7.3-fresh`)

| Patch | Covers | Enforced by |
|---|---|---|
| `ahci-alpine-per-port-msix.patch` | `drivers/ata/ahci_alpine.c` + `drivers/ata/{Kconfig,Makefile}`, `ahci.c` handoff, `unvr_defconfig` | `CONFIG_AHCI_ALPINE=y` FATAL, both kernel scripts (#92) |
| `i2c-designware-no-enable-abort.patch` | `snps,no-enable-abort` + `snps,no-sda-rx-hold` DT opt-outs in `i2c-designware-common.c` | `check_kernel_patches()` source-marker FATAL (#86) |
| `arm64-alpine-select-net-devlink.patch` | `ARCH_ALPINE select NET_DEVLINK if NET` in `Kconfig.platforms` | `CONFIG_NET_DEVLINK=y` FATAL, `build-linux-fedora.py` |

The i2c one has no config symbol of its own - it is a behaviour change
inside `__i2c_dw_disable()`, so an unapplied patch would build clean and
wedge the pld bus on the box. `_repo.py`'s `KERNEL_PATCH_MARKERS` /
`check_kernel_patches()` greps the tree's source instead; add an entry there
for any future patch that is likewise config-invisible.

The board DTS is **not** a patch - `build-linux-fedora.py` stages
`dts/*.dts{,i}` from this repo over the kernel tree at every build, so
`dts/` is the single source of truth (see `stage_dts()`).

### `patches/uboot/` - U-Boot tree (`/mnt/2tb/unvr-port-refs/u-boot-v2026.07`)

Extracted from `uboot-build.py`'s in-place string surgery (#224). All eight
apply cleanly to pristine `v2026.07` and together reproduce the tree exactly.

| Patch | Covers | Was |
|---|---|---|
| `0001-board-alpine-v2-unvr-target.patch` | `TARGET_ALPINE_V2_UNVR` + board Kconfig source in `arch/arm/Kconfig` | `txt.replace()` |
| `0002-net-al_eth-wiring.patch` | `drivers/net/{Kconfig,Makefile}` al_eth hooks | `txt.replace()` |
| `0003-phy-al_serdes-wiring.patch` | `drivers/Makefile` obj + `drivers/phy/Kconfig` source | `txt.replace()` + a hand-written truncation guard (#102) |
| `0004-crypto-al_ssm-wiring.patch` | `drivers/crypto/{Kconfig,Makefile}` al_ssm hooks | `txt.replace()` |
| `0005-i2c-designware-raw-scl-hcnt-lcnt.patch` | raw SCL hcnt/lcnt from DT (#86) | whole-file copy |
| `0006-ahci-block-size-and-spinup-wait.patch` | `MAX_SATA_BLOCKS_READ_WRITE` 0x80->0x800 (#92), spin-up link wait (#94) | whole-file copy |
| `0007-spi-designware-bound-rx-to-fifo-depth.patch` | bound Rx to FIFO depth (#91) | whole-file copy |
| `0008-pci-ecam-reject-aliased-devfn.patch` | `awto,single-devfn` devfn filter (#140) | whole-file copy |

0005-0008 modify existing upstream files but were previously **invisible**:
`uboot-build.py` copies `uboot-port/drivers/**` over them wholesale, so
nothing showed what actually changed. 0008 is the patch found silently
unstaged on 2026-08-27 (the incident in `unstage_stale()`'s docstring).

`uboot-build.py` still performs the staging (the copies and the Kconfig /
Makefile hooks) - it now calls `verify_patch_state()` afterwards, which
re-reads the tree and FATALs naming the missing patch file if any of 11
markers is absent. That is the U-Boot equivalent of the
`CONFIG_AHCI_ALPINE=y` FATAL: a half-applied tree is loud, not a
silently-wrong `u-boot.bin`.

The
6.12/6.18/7.1-specific scripts (`build-linux-612-ea16.py`, `build-linux-618-ea16.py`, the old
`build-linux-71-ea16.py`/`build-linux-71-fedora.py`) are retired to
`debris/scripts/` - not runnable via `dev.py`, kept for history only.

| # | What | dev.py | Script | Output |
|---|------|--------|--------|--------|
| 1 | Netboot installer (bare-disk bootstrap for `deploy-fedora-rootfs`) | `build-ea16` | `build-linux-ea16.py` | `build-out-ea16/` (`AWTO_KERNEL_OUT`) |
| 2 | Fedora rootfs (**daily driver**) | `build-fedora` | `build-linux-fedora.py` | `build-out-fedora/` |
| 2a | ↳ same script, full-debug variant (KASAN, UBSAN, DMA_API_DEBUG, lockdep, panic-on-lockup) | `AWTO_KASAN_BUILD=1 build-fedora` | (same) | `build-out-fedora-kasan/` |
| 3 | On-box native (no cross-compile) | *(none)* | `build-on-box.py` | `/lib/modules/<kver>/updates/` on the box |
| 4 | Fedora rootfs tarball | `build-fedora-rootfs` | `build-fedora-rootfs.py` | `tmp/fedora-rootfs.tar.gz` |
| 5 | awto-uboot (our U-Boot) | `build-uboot` | `uboot-build.py` | staged into `/mnt/2tb/unvr-port-refs/u-boot-v2026.07`, chainload-able |
| 6 | EDK2/UEFI (our platform port, docs/uefi.md) | `build-uefi-p0` | `build-uefi-p0.py` | builds against `/mnt/2tb/unvr-port-refs/edk2` (pristine `edk2-stable202608` clone), `UNVR.fd` chainload-able |

Rows 5 and 6 both keep the actual source of truth (`uboot-port/`,
`Platform/Ubiquiti/UNVR/`) tracked in this repo, with the upstream tree
(`u-boot-v2026.07`, `edk2`) as a pristine, disposable clone outside it -
neither U-Boot nor EDK2's platform port modifies any existing upstream
file, so there's no reason to vendor either as its own fork (contrast
the kernel, row 1-2a, where board support really is patches into
existing upstream files and tracking a real fork - `awto-au/linux` -
earns its keep). Row 6 goes one step further than row 5's copy-based
staging: EDK2's own `PACKAGES_PATH` mechanism (`build-uefi-p0.py`)
references `Platform/Ubiquiti/UNVR/` directly from this repo, no copy
step at all - avoids the class of bug `uboot-build.py`'s `stage()` hit
in #140 (a removed file staying resident in the build tree because
nothing reverted it).

`AWTO_KERNEL_SRC`/`AWTO_KERNEL_OUT`/`AWTO_KERNEL_VER` override the kernel
tree/output dir/version string for any of rows 1-2a - both the producer
scripts and their deploy-side consumers (`_fedora_deploy.py`,
`deploy-modules-woomera.py`, `deploy-fedora-boot.py`, `dev.py`'s
`deploy-fedora-rootfs`) resolve these through the shared
`kernel_build_out()`/`ea16_build_out()`/`kernel_build_ver()` in
`scripts/_repo.py` - never hardcode a build-out path in a new script, call
those instead.

## Which one do I actually want?

- **Testing a change fast, box already has a working rootfs** → `ram-boot-deploy`
  (RAM-boots a kernel+DTB+modules over tftp through stock U-Boot, no NAND
  flash, no SSH needed to reach it - see its own `--help`).
- **Diagnosing a crash with real instrumentation** → row 2a, full-debug build, then
  `ram-boot-deploy` it the same way.
- **Making it the new daily driver** → row 2 (`build-fedora`), then
  `publish-fedora` + `flash` (writes to NAND, is what actually boots on
  power-cycle) or `build-fedora-rootfs` if the userspace itself changed.
- **CPU/thermal load-testing, not meant to boot as the daily driver** → row 3,
  on-box native (also see #116: native gcc measurably underperforms
  cross-compile for this driver - fine for a load-test workload, not for
  production).
- **Iterating on the U-Boot chainload itself** → row 5, then `chainload` to
  stage it onto the box without touching NAND.

## Build isolation: kbuild `O=` + ccache

`build-linux-fedora.py` (rows 2/2a) builds via kbuild's own `O=` output
directory (`OUT/kbuild`), not in-place in the shared source tree - each `OUT`
gets a fully independent `.config` + generated headers + `.o` tree, so the
full-debug and plain variants can never clobber each other's build state.
`CC="ccache aarch64-linux-gnu-gcc"` lets variants share cache hits for
anything neither actually changed, so a small source edit doesn't force a
full rebuild of everything.

Not yet applied to `build-linux-ea16.py` - see #145 for that remaining work.

If a source tree has ever been built in-place (no `O=`) before, kbuild
refuses a fresh `O=` build until `make mrproper` runs in it - safe, only
removes generated files, never touches tracked/uncommitted source.

## Per-version detail docs (deltas, not full rebuilds of this doc)

- [on-box-build.md](on-box-build.md) - native build setup/usage, clock workaround, `--reload` risk.
- [fedora-on-ssd.md](fedora-on-ssd.md) - the persistent Fedora-rootfs boot path row 2 targets.
- [bootloader.md](bootloader.md) / [boot-flow.md](boot-flow.md) - what actually boots what, independent of which kernel you built.

The 6.12/6.18/7.1 per-version delta docs (`linux-6.12-build.md`,
`linux-6.18-build.md`, `linux-71-build.md`) that used to describe those
retired scripts have themselves been removed - the scripts' history is
still in `debris/scripts/`, but there's no separate doc for it. This doc
is the current, accurate index.
