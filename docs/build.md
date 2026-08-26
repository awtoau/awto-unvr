# Build options — one map for all of them

Every build in this repo goes through `./dev.py <command>`. This doc is the
index; each linked doc covers the version-specific deltas in depth.

## Inventory

| # | What | dev.py | Script | Kernel ver | SRC tree | Output |
|---|------|--------|--------|-----------|----------|--------|
| 1 | Linux 6.12, netboot | `build-612` | `build-linux-612-ea16.py` | 6.12 | `linux-6.12` | `build-out/` |
| 2 | Linux 6.18, netboot | `build-618` | `build-linux-618-ea16.py` | 6.18.44 | `linux-v6.18.44` | `build-out-618/` |
| 3 | Linux 7.1, netboot/initramfs | `build-71-ea16` | `build-linux-71-ea16.py` | 7.1.8 | `linux-v7.1.8` (`AWTO_KERNEL_SRC`) | `build-out-71/` (`AWTO_KERNEL_OUT`) |
| 4 | Linux 7.1, Fedora rootfs (**daily driver**) | `build-fedora` | `build-linux-71-fedora.py` | 7.1.8 | `linux-v7.1.8` (`AWTO_KERNEL_SRC`) | `build-out-71-fedora/` |
| 4a | ↳ same script, KASAN variant | `AWTO_KASAN_BUILD=1 build-fedora` | (same) | 7.1.8-kasan | (same) | `build-out-71-fedora-kasan/` |
| 4b | ↳ same script, latest mainline (**not standardised** - see #156) | `AWTO_KERNEL_SRC=/mnt/2tb/unvr-port-refs/linux-v7.3-fresh AWTO_KERNEL_OUT=/mnt/2tb/unvr-port-refs/build-out-71-3-fresh build-fedora` | (same) | 7.2.0-13187-g66498c75b4f8 (pre-7.3 snapshot) | `linux-v7.3-fresh` (`AWTO_KERNEL_SRC`) | `build-out-71-3-fresh/` |
| 5 | On-box native (no cross-compile) | *(none)* | `build-on-box.py` | matches whatever's flashed | `/root/src/linux-v7.1.8` on the box itself | `/lib/modules/<kver>/updates/` on the box |
| 6 | Fedora rootfs tarball | `build-fedora-rootfs` | `build-fedora-rootfs.py` | n/a (userspace) | dnf `--installroot`, not a kernel tree | `tmp/fedora-rootfs.tar.gz` |
| 7 | awto-uboot (our U-Boot) | `build-uboot` | `uboot-build.py` | n/a | `uboot-port/` (this repo) + staged into `/mnt/2tb/unvr-port-refs/u-boot-v2026.07` | chainload-able U-Boot image |

"4x Linux" = rows 3/4/4a/5 (the live, actively-used 7.1 variants: netboot, Fedora
daily-driver, its KASAN twin, and on-box native). Rows 1-2 are the earlier
6.12→6.18 forward-port work that established the driver deltas 7.1 inherits
unchanged - kept for history/reference, not actively rebuilt.

Row 4b (latest mainline) was a one-off experiment, driven by env-var overrides
on the fedora script rather than its own scripted/tested build - built once
successfully (`tmp/logs/build-v73-fresh.log`), never boot-tested. Its
`al_eth.ko` is the old combined module (built before the #131 split), so
booting it live would still hit the original udev race. See #156.

## Which one do I actually want?

- **Testing a change fast, box already has a working rootfs** → `ram-boot-deploy`
  (RAM-boots a kernel+DTB+modules over tftp through stock U-Boot, no NAND
  flash, no SSH needed to reach it - see its own `--help`).
- **Diagnosing a crash with real instrumentation** → row 4a, KASAN build, then
  `ram-boot-deploy` it the same way.
- **Making it the new daily driver** → row 4 (`build-fedora`), then
  `publish-fedora` + `flash` (writes to NAND, is what actually boots on
  power-cycle) or `build-fedora-rootfs` if the userspace itself changed.
- **CPU/thermal load-testing, not meant to boot as the daily driver** → row 5,
  on-box native (also see #116: native gcc measurably underperforms
  cross-compile for this driver - fine for a load-test workload, not for
  production).
- **Iterating on the U-Boot chainload itself** → row 7, then `chainload` to
  stage it onto the box without touching NAND.

## Build isolation: kbuild `O=` + ccache

`build-linux-71-fedora.py` (rows 4/4a) builds via kbuild's own `O=` output
directory (`OUT/kbuild`), not in-place in the shared source tree - each `OUT`
gets a fully independent `.config` + generated headers + `.o` tree, so the
KASAN and plain variants can never clobber each other's build state (they
used to, silently, before this - see #145). `CC="ccache aarch64-linux-gnu-gcc"`
lets variants share cache hits for anything neither actually changed, so a
small source edit doesn't force a full rebuild of everything.

Not yet applied to `build-linux-71-ea16.py`, `build-linux-612-ea16.py`, or
`build-linux-618-ea16.py` - see #145 for that remaining work.

If a source tree has ever been built in-place (no `O=`) before, kbuild
refuses a fresh `O=` build until `make mrproper` runs in it - safe, only
removes generated files, never touches tracked/uncommitted source.

## Per-version detail docs (deltas, not full rebuilds of this doc)

- [linux-6.12-build.md](linux-6.12-build.md) - the original ea16 port, netboot/initramfs mechanism explained in full.
- [linux-6.18-build.md](linux-6.18-build.md) - 6.12 → 6.18 API deltas.
- [linux-71-build.md](linux-71-build.md) - 6.18 → 7.1 API deltas (all three carried over unchanged).
- [on-box-build.md](on-box-build.md) - native build setup/usage, clock workaround, `--reload` risk.
- [fedora-on-ssd.md](fedora-on-ssd.md) - the persistent Fedora-rootfs boot path row 4 targets.
- [bootloader.md](bootloader.md) / [boot-flow.md](boot-flow.md) - what actually boots what, independent of which kernel you built.

None of these five docs currently mention `build-linux-71-fedora.py`, KASAN
builds, or `ram-boot-deploy` - they predate that work. This doc is the
current, accurate index; treat the per-version docs as historical delta
records for the 6.12→7.1 migration specifically, not a build guide on their
own.
