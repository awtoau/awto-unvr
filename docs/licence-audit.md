# Licence + fingerprint audit — UNVR 5.1.25

Components in `sources/UNVR-5.1.25.bin`, their versions, licences, and whether
Ubiquiti meet the GPL/LGPL source-offer obligation. Evidence: `readelf`/`strings`
on `tmp/sections/*`, dpkg `var/lib/dpkg/status` (437 pkgs), copyright files.

For the *broader* GPL-withholding pattern (removed download links, unanswered
`opensource-requests@ui.com`, community mirrors, per-console GPL drops) see
[sources.md §3](sources.md#3-gpl-source--the-real-situation) and
[prior-art.md](prior-art.md#gpl-source-legally-required-practically-withheld).
This file adds the **5.1.25-specific version fingerprint** and per-component
compliance verdict.

## Toolchain / build fingerprint
- **GCC (Debian 10.2.1-6) 10.2.1 20210110** — kernel, U-Boot, `ubnthal.ko`, and
  the ubnt userspace all built with it (`.comment` + `linux_banner`).
- Kernel `CONFIG_GCC_VERSION=100201`. Build host `bdd@builder`, `#2 SMP Fri Jul 10 16:11:15 CST 2026`.
- Container built on Jenkins: `.../jenkins_slave/workspace/irmware.debbox_unifi-nvr_v5.1.25/...` (`debbox`, cf. `thezim/debbox-kernel`).
- BuildIDs (sha1): `ubnt-tools` `b6348cdc…`, `ubnthal.ko` `84c9b753…`, `busybox` `53c46711…`.

## OS base
- **Debian 11 bullseye**, `VERSION_ID=11`, `/etc/debian_version` = `11.11`, aarch64.
- `etc/apt/sources.list` → stock `deb.debian.org` / `security.debian.org` / `archive.debian.org` bullseye only. **No Ubiquiti apt repo listed** despite `ubnt-archive-keyring` being installed (their pkgs come from an out-of-band repo).

## Component table

| Component | Version | Licence | Source obligation | Met? |
|---|---|---|---|---|
| Linux kernel | **4.19.152-alpine-unvr** | GPL-2.0 | Ubiquiti-modified (al_eth, alpine, ubnt board) | **NO — gap** |
| U-Boot | **2015.07-alpine_db-2.21-HAL** | GPL-2.0 | Annapurna+Ubiquiti fork | **NO — gap** |
| al_boot / SPL | `al_boot v2.10.0` (Annapurna) | mixed (see bootloader.md) | Annapurna proprietary blob + GPL parts | **NO** |
| `ubnthal.ko` | in-tree, `license=Proprietary` | Proprietary kmod on GPL kernel | derivative-work / `EXPORT_SYMBOL_GPL` question | **NO — gap** |
| `ubnt_common.ko`, `ui-hdd-pwrctl.ko` | in-tree | Proprietary kmod | same as above | **NO** |
| glibc | 2.31-13+deb11u14 | LGPL-2.1 | stock Debian | yes (Debian archive) |
| OpenSSL (`libssl1.1`/`libcrypto.so.1.1`) | **1.1.1w** (2023-09-11), pkg `1.1.1w-0+deb11u8` | Apache-2.0 (1.1.1 dual OpenSSL/SSLeay) | stock Debian | yes |
| zlib (`libz.so.1.2.11`) | 1:1.2.11.dfsg-2+deb11u2 | zlib | stock Debian | yes |
| BusyBox | **1.30.1** (Debian 1:1.30.1-6+deb11u1) | GPL-2.0 | stock Debian, static | yes |
| libevent | 2.1.12-stable-1 | BSD-3 | stock Debian | yes |
| systemd | 247.3-7+deb11u8 | LGPL-2.1+ | stock Debian | yes |
| util-linux | 2.36.1-8+deb11u2 | GPL-2 / LGPL | stock Debian | yes |
| coreutils | 8.32-4 | GPL-3.0 | stock Debian | yes |
| mdadm | 4.1-11 | GPL-2.0 | stock Debian | yes |
| bash | **5.1-2-4+gcef01e29e98d** | GPL-3.0 | **Ubiquiti rebuild** (custom `+g…` version) | offer likely due, source not shipped |
| dpkg | 1.20.14 | GPL-2.0 | stock Debian | yes |
| `ubnt-tools`, `libubnt`, `ubnt-common`, `ubntnas`, `ubnt-sfp-handler`, `ustorage` | Ubiquiti | **Proprietary** (copyright files say so) | none | n/a |
| ~400 other Debian pkgs | Debian bullseye | GPL/LGPL/BSD/MIT/MPL/Apache | stock Debian | yes (Debian archive) |

Licence-string tally across all `copyright` files (rough): MPL≈2.5k, MIT≈3.6k,
GPL-2≈1.6k, BSD≈1.2k, LGPL≈0.9k, GPL-3≈0.33k, Apache≈0.19k, Proprietary/ARR lines
in the ubnt packages.

## GPL-compliance gaps (flag)

1. **Modified Linux kernel 4.19.152-alpine-unvr — source not in image, not offered.**
   Package copyright (`usr/share/doc/linux-image-.../copyright`) points ONLY to
   `kernel.org` mainline — which does **not** contain the al_eth/alpine/ubnt
   patches. No source tree, no written offer anywhere in the image. Nearest public
   copy is the community mirror `UrNVR/unvr-kernel` (4.19.152, provenance ≈2.3.14,
   **not** 5.1.25) — not a substitute for Ubiquiti's own offer.
2. **Modified U-Boot 2015.07-alpine_db — source not in image, not offered.**
   Only the 1.3.35-era U-Boot tree is public (`NeccoNeko/UBNT-source-code`); no
   published tree matches this 2026 build.
3. **Proprietary kernel modules on a GPL kernel.** `ubnthal.ko` (and
   `ubnt_common.ko`, `ui-hdd-pwrctl.ko`) declare `license=Proprietary` yet are
   statically linked against the GPL kernel. Classic `EXPORT_SYMBOL_GPL` /
   derivative-work exposure; module source is unpublished.
4. **No written offer for source accompanies the binary.** GPL-2 §3(b) requires a
   written offer if source is not included. None present (`find` for
   written/source-offer files: nil; apt sources are Debian-only).
5. **Rebuilt bash** carries a custom Ubiquiti version suffix — a GPL-3 rebuild
   whose source is not shipped (minor; trivially reconstructable, but technically
   owed).

Stock unmodified Debian binaries (glibc, openssl, busybox, systemd, coreutils,
mdadm, zlib, …) are **not** a gap: identical to Debian bullseye, source in the
Debian archive that `sources.list` already points at.

## Kernel security posture (for completeness)
`CONFIG_MODULE_SIG` not set, `CONFIG_KEXEC` not set, no lockdown — see
[porting-reference.md](porting-reference.md#stock-kernel-config--settles-the-big-questions).
No secure-boot enforcement in the kernel; the only signing is (a) the container
`ENDS` RSA signature and (b) `ubnt-tools` FW-image RSA/SHA-1 check — both
bypassed by the direct-MTD kernel-replace route, since U-Boot boots unsigned by
default ([bootloader.md](bootloader.md)).
