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
`patches/ahci-alpine-per-port-msix.patch` (`drivers/ata/ahci_alpine.c` +
Kconfig/Makefile/`ahci.c`/`unvr_defconfig`) is required - both kernel build
scripts FATAL if `CONFIG_AHCI_ALPINE=y` is missing (#92). The
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
