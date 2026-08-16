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
  root (`root=/dev/sda2`) with no dracut. `rootwait` covers SATA probe latency.
- **Fedora's userland doesn't care** what booted it or that the kernel (7.1.8) is
  newer than Fedora ships — systemd/glibc are kernel-version agnostic.

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

## Deploy — put it on the SSD (steps, not yet scripted)

Prereq: **unplug the SanDisk USB stick (sda)** so the SSD enumerates as `/dev/sda`
(sda1 = existing 100 MB ESP, sda2 = 931 GB root). USB fully backed up
(`images/unvr-usb-*.img`).

1. Boot an **enhanced initramfs** (adds `mkfs.ext4` + `rsync` to the bring-up one).
2. `mkfs.ext4 -L unvr-root /dev/sda2` (fstab mounts root by that label).
3. Mount, extract the rootfs tar, add our 7.1.8 modules
   (`build-out-71/modules/lib/modules/7.1.8-dirty` → `/lib/modules/`).
4. Kernel+DTB location — depends on whether **vendor U-Boot can read SATA** (TBD,
   check at prompt: `scsi init` / `ls scsi 0`):
   - **yes** → put uImage+DTB on the SSD (ESP FAT or ext4 /boot), fully self-contained.
   - **no** → flash our uImage to the NAND kernel partition; rootfs still on SSD.
5. Set U-Boot env → `bootargs 'console=ttyS0,115200 root=/dev/sda2 rootfstype=ext4
   rw rootwait selinux=0 panic=15 ...'` + a bootcmd that loads kernel+DTB; `saveenv`.
6. Reboot → persistent Fedora on the SSD.

## After this — dev work self-hosts on woomera

Once Fedora is up with the toolchain, kernel builds and git move onto woomera
(quad A57, 4 GiB, SSD + 2×8 TB). The host stops being required.
