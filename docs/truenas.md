# TrueNAS 27 on woomera (#356, #338)

Our self-built arm64 image (`./dev.py truenas-build update`, scale-build `tmp/release/*.update`) with the image's own 6.18 kernel deb, on SSD p3 (`truenas-boot`). Fedora on p2 stays the default boot. The maint boot kernel + modules are the image's own (extracted rootfs), so zfs kmod and userland versions match (#356).

## Use

```
./dev.py boot-next truenas --reboot --wait 900   # one TrueNAS boot; power-cycle if hung -> Fedora
./dev.py truenas-install verify                  # on TrueNAS: kernel, pool, middleware, web UI, links
./dev.py boot-next fedora --reboot               # (from TrueNAS) back to Fedora
```

- Login: `./dev.py ssh` (root key from Fedora's `authorized_keys`); web UI `truenas_admin`, password in `~/.config/awto/secrets.yaml` section `truenas-woomera`.
- Boot mechanism: [boot-select.md](boot-select.md). Profile `/boot/awto/truenas/` on p2: our uImage, `unvr-truenas.dtb`, the installed initrd, `root=ZFS=<bootfs>`.

## Install (re-run from scratch)

```
./dev.py build-dtb --tree truenas
./dev.py truenas-install build          # maint initramfs (image userland + maint modules + /init)
./dev.py truenas-install rehearse       # host, qemu-user: userland smoke, installer chroot steps, initrd contents (extracts the update rootfs once, ~8 GB under tmp/)
./dev.py truenas-install stage [--extra-modules TAR]  # p2 /srv/truenas-install + profile truenas-maint
./dev.py truenas-install run --go       # one maint boot (bootonce) -> back on Fedora, result fetched
./dev.py truenas-install commit-p3 --go # image partition -> p3, read-back verified
./dev.py truenas-install stage-boot     # profile truenas
```

- Maint boot = the image's own kernel (vmlinuz + modules from the extracted rootfs) + `scripts/initramfs/truenas-maint-init`: mounts p2, runs `scripts/truenas-install-job.py` in the image's own rootfs as live env (`.update` -> `rootfs.squashfs`, nested loop mounts, + tmpfs overlay). Log: p2 `/srv/truenas-install/logs/`.
- Image = newest `tmp/release/*.update` under `/mnt/2tb/truenas-build/scale-build`; `TRUENAS_UPDATE=<path>` overrides. No ISO needed.
- Job = the ISO installer's `zpool create` (26.04 copy of `truenas_installer/install.py`) + the update image's own `truenas_install.main()`, in-process, with:
  - `update-grub` / `grub-install` skipped (no GRUB, no ESP; U-Boot boots us),
  - before its `truenas-initrd.py`: the image already carries our kernel deb; the job adds `--extra-modules` tar (al_* for that kver, `<kver>/...` under `usr/lib/modules`) if staged, depmod, initramfs module list, authorized_keys,
  - `out/` gets initrd + vmlinuz of the image kernel; `stage-boot` wraps the vmlinuz in a uImage,
  - SQL: ssh service on, serial console ttyS0 115200.
- Pool built on a p3-sized partition of an image file on p2 (`/srv/truenas-install/p3.img`, loop `-P`), then copied to p3 from Fedora: nothing writes p3 until the install has succeeded and been reviewed.
- Watchdog: SP805 fed while SSD I/O or the job's CPU time moves; 300 s of neither = reset -> Fedora.

## Constraints

- p2: writes only under `/srv/truenas-install` and `/boot/awto`. 8 TB disks: never touched.
- `truenas-initrd.py` (middleware, on config changes) rebuilds the initrd inside the root; U-Boot loads the copy in `/boot/awto/truenas/`. After such a change: `./dev.py truenas-install stage-boot` (reads `out/` - TODO if it diverges).
- Unofficial arm64 userland: middleware/app failures are userland findings for #338, not kernel bugs by default.

## tank/home encryption (#370)

- `tank/home` is ZFS-native encrypted: aes-256-gcm, `keyformat=hex`, `keylocation=prompt`; ICP uses the aarch64 CE impls (#368).
- Key: `~/.config/awto/secrets.yaml` section `woomera-tank-home-key` (32 bytes hex, `password` field). Losing it loses the data; no other copy.
- `./dev.py truenas-config apply` after a reinstall (pool imported, locked):
  - `step_dataset`: absent -> creates it encrypted with the key from secrets; present -> leaves it.
  - `step_unlock`: locked -> `pool.dataset.unlock` with the key from secrets (never from TrueNAS's DB: reinstall rotates `/data/pwenc_secret`, #366), then stores the key in TrueNAS's DB so plain reboots auto-unlock.
  - an unencrypted `tank/home` only logs a WARNING; `verify` fails (`tank/home encrypted`).
- `./dev.py truenas-config key-init` creates the secrets section if absent (do this before encrypting anything).
- `verify` checks: encrypted (aes-256-gcm), key loaded, secrets key section present.
- `pool.dataset.lock` only works for passphrase datasets; to simulate a reinstall: stop cifs/nfs/webshare, `zfs unmount` + `zfs unload-key tank/home`, delete the `storage.encrypteddataset` row (`datastore.delete`), run `apply`.
- Migration of an existing plain dataset (done 2026-11-16): `zfs snapshot -r`, `./dev.py home-manifest dump` (sha256/owner/mode/ACL/xattr), `./dev.py home-key-put put`, `zfs send -R | zfs recv -u -o encryption=aes-256-gcm -o keyformat=hex -o keylocation=file:///root/home.key tank/home-enc`, manifest diff, rename swap, `keylocation=prompt`, `home-key-put shred`.
- The pre-migration dataset is kept as `tank/home-plain-<ts>` (readonly, `canmount=noauto`, not shared) until Dan decides to delete it.

## Boot-time fixes carried as patches (#363, #367, #369)

- `patches/sources/truenas_pydiscovery`: drop our own mDNS/NBNS packets incl. a sibling NIC's (two NICs on one subnet read each other as a name conflict; 20 renames blocked the discovery reload cifs waits on, smbd left stopped) (#367).
- `patches/sources/truenas/0008`: `dhparam_setup` writes fixed RFC 7919 ffdhe2048 (`openssl dhparam 2048` = ~6 min on the A57) (#369).
- `patches/sources/truenas/0009`: `/etc/dhcpcd.exit-hook` sets strict ARP + per-NIC table (100+ifindex) and `ip rule from <addr>` on each lease event (#363).
- Package names for `truenas-build packages --only`: `middlewared`, `truenas_files` (the `truenas` name builds only the meta package).
- `truenas-config apply` destroys a `tank/.system/truesearchd` whose key is unavailable (stale pwenc after reinstall) and recreates it (#366); `verify` waits for the boot-time `directoryservices.setup` restart to settle.
