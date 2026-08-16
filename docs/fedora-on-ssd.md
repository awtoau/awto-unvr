# Fedora aarch64 on the UNVR SSD (host "woomera")

Goal: **stock Fedora 44 aarch64** as the persistent OS on the Samsung SSD, booted
by our existing chain — no netboot, no UEFI, no GRUB, no dracut. Replaces the
throwaway RAM initramfs with a real distro. Device hostname: **woomera**.

- Boot chain: `ROM → S2 → al_boot → U-Boot → our 7.1.8 uImage+DTB → root=/dev/sda2 (Fedora)`.
- Fully reversible: vendor NAND untouched; only the U-Boot env changes.
- Tracking: #40. Related: persistent-boot, storage-repurpose.

## The "hack" — why stock Fedora boots without its bootloader

Fedora arm64 normally boots `UEFI → shim → GRUB2 → dracut initramfs → systemd`.
We have none of those. Instead:

- **U-Boot loads our 7.1.8 kernel + DTB directly** (`bootm`), same as netboot.
- **No initramfs**: our kernel has **AHCI + ext4 built-in**, so it mounts the SSD
  root directly, no dracut. `rootwait` covers SATA probe latency.
- **Fedora's userland doesn't care** what booted it or that the kernel (7.1.8) is
  newer than Fedora ships — systemd/glibc are kernel-version agnostic.

### Two kernels — installer vs production (do NOT reuse the bring-up uImage)

Our bring-up uImage **embeds an initramfs** (`CONFIG_INITRAMFS_SOURCE`) whose
`/init` drops to a shell — it would hijack Fedora's boot. So:

- **Installer kernel** = bring-up uImage with the *enhanced* initramfs
  (`e2fsprogs`+`rsync`+`util-linux`, already added to `build-initramfs-ea16.py`).
  Used once to format + rsync the rootfs onto the SSD.
- **Production kernel** (`scripts/build-linux-71-fedora.py`) = **Fedora 44's own
  aarch64 kernel config** + our Alpine patches, `CONFIG_INITRAMFS_SOURCE=""` (no
  embedded initramfs → runs `/sbin/init`/systemd from the real root). Using
  Fedora's config (not our lean one) means every systemd/fs/net symbol is correct
  by construction — it resolves nearly all of the audit's #43 kernel-config
  findings automatically (PSI, CGROUP_BPF, BPF_JIT, XFS, BTRFS-ACL, zram, BBR,
  YAMA … all `=y/m`). Only ARM64-CE SHA accel (`CRYPTO_SHA2/SHA512_ARM64_CE`)
  is still off even in Fedora's config → add next rebuild. Build disables
  DEBUG_INFO_BTF / MODULE_SIG / debuginfo (build-complexity we don't need).

### Root device — PARTUUID, not /dev/sdaN (audit 7.5a)

A no-initramfs kernel resolves `root=PARTUUID=` natively but **not** `LABEL=`/`UUID=`
(those need udev). `root=/dev/sda2` works but is fragile to enumeration. Use:
`root=PARTUUID=<sda2 GPT partuuid> rootfstype=ext4 rootwait rw`. Get it on the
device: `blkid /dev/sda2` → `PARTUUID=`.

### al_* modules must autoload without dracut (audit 7.5b)

No initramfs → udev autoloads by `modules.alias`. al_eth/al_dma/al_ssm bind by PCI
ID (1c36:0001/0002/0022). After `depmod` in the deployed rootfs, confirm those
aliases are present; if not, ship `/etc/modules-load.d/al.conf` listing all al_*.
Without this, **eth never comes up** (no NAS).

## Deviations from a stock Fedora install (the ONLY ones)

Everything else is default Fedora (`@core` group, weak deps on, NetworkManager,
systemd). Deviations, all deliberate:

| Deviation | Why |
|---|---|
| **SELinux off** (`selinux=0` bootarg + config disabled) | No dracut → no autorelabel path; labels lost on tar/rsync extract. |
| **firewalld removed** | Single-tenant NAS on the user's trusted LAN. |
| **abrt removed** | No crash-report daemon (it thrashed the *host* earlier with GB of cores). |
| **serial-getty@ttyS0 enabled** | Headless; console is ttyS0 @115200. |
| **sshd enabled + PermitRootLogin yes** | Headless remote access. Default Fedora ships neither. |
| **root password set** (`unvr` — CHANGE) | No install-time user-creation step. |

## Build — `scripts/build-fedora-rootfs.py` (on the host)

Needs: `podman` + `qemu-aarch64` binfmt (both present on the Fedora host).

- `podman create --platform linux/arm64 fedora:44` → `dnf group install core` +
  extras (openssh-server, e2fsprogs, dosfstools, rsync, smartmontools, mdadm,
  hdparm, gpiod, lm_sensors, python3) → apply CONFIG_SH → `podman export`.
- Output: `tmp/fedora-rootfs-ea16.tar` (+ .sha256).
- Slow: `dnf` runs under qemu emulation (~minutes), background it.

## Deploy — as executed (2026-08-16)

Prereq done: **SanDisk USB unplugged** → SSD enumerates as `/dev/sda` (sda1 =
100 MB ESP, sda2 = 931 GB root). USB fully backed up (`images/unvr-usb-*.img`).

1. Netbooted the **installer kernel** (bring-up uImage w/ enhanced initramfs:
   mkfs.ext4 + rsync + util-linux) via `scripts/netboot.py --tag 7.1`.
2. `mkfs.ext4 -L unvr-root /dev/sda2` → PARTUUID `dcdc291e-9956-48cd-9d7c-48219877881a`.
3. Streamed the rootfs onto it: host `python3 -m http.server` + device
   `wget -O - http://host:8080/fedora-rootfs-ea16.tar | tar -x -C /mnt/root`.
4. Built the **production kernel** = Fedora config + our patches (`build-linux-71-fedora.py`),
   `INITRAMFS_SOURCE=""`. 56 MB Image → **gzip uImage 18.5 MB** (`mkuimage.py --gzip`).
5. Deployed modules: `rsync` the full `modroot/lib/modules/7.1.8-dirty` (429 MB,
   already `depmod`'d, al_* PCI aliases present) into the rootfs `/lib/modules/`.
6. **Netbooted** the gzip Fedora uImage + DTB with `root=PARTUUID=dcdc291e-…
   rootfstype=ext4 rw rootwait selinux=0 panic=15` → **Fedora 44 booted** (below).

## BOOTED — Fedora 44 verified on hardware (2026-08-16)

Fedora 44 aarch64 booted on the UNVR via netboot of the patched kernel,
root on the SSD. Verified logged in:

- Kernel `7.1.8-dirty aarch64` (Fedora config + Alpine patches, gzip uImage).
- Root `/dev/sda2 ext4` — no initramfs, no UEFI/GRUB/dracut.
- al_eth/al_dma/al_ssm/al_sgpo autoloaded by PCI ID; enp0s1 up, DHCP.
- systemd reached graphical.target; serial + ssh login work.
- al_ssm prints a one-time WARN at udma init but crypto engine comes up (AES-XTS).
- Kernel is 56 MB uncompressed → **gzip uImage** (18.5 MB): fast tftp + U-Boot
  decompresses to 0x08080000, so the standard DTB addr (0x04078000) stays clear.
  `scripts/mkuimage.py Image out --gzip`.

Build/deploy facts:
- SSD = `/dev/sda` (USB removed). `sda2` ext4 label `unvr-root`,
  **PARTUUID=`dcdc291e-9956-48cd-9d7c-48219877881a`**.
- Production kernel: `scripts/build-linux-71-fedora.py` → `build-out-71-fedora/`
  (Image, uImage, dtb, `modroot/` full module tree, all al_* OOT built).
- Bootargs: `console=ttyS0,115200 root=PARTUUID=dcdc291e-… rootfstype=ext4 rw
  rootwait selinux=0 panic=15`.

### Still TODO — persistence (kernel still netbooted)

The *kernel* is tftp'd; a plain reboot drops to the vendor rescue shell. To make
it boot with no host: verify U-Boot can read SATA (`scsi init`) → put gzip uImage
+ DTB on the SSD ESP (sda1, FAT) → U-Boot bootcmd load+bootm → `saveenv`. If
U-Boot can't read SATA, flash the uImage to the NAND kernel partition instead.
Follow-ups: hostname → woomera (`hostnamectl`), change root pw, al_ssm WARN.

## After this — dev work self-hosts on woomera

Once Fedora is up with the toolchain, kernel builds and git move onto woomera
(quad A57, 4 GiB, SSD + 2×8 TB). The host stops being required.
