# TrueNAS 26.04 on woomera (#338 phase 3)

Community arm64 userland (joel0 build, `/vendors/truenas/README.md`) + OUR 6.18 kernel (`./dev.py build-truenas`, [build.md](build.md#truenas-618-338)), on SSD p3 (`truenas-boot`). Fedora on p2 stays the default boot.

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
./dev.py truenas-install build          # maint initramfs (live-env userland + 17 modules + /init)
./dev.py truenas-install rehearse       # host, qemu-user: userland smoke, installer chroot steps, initrd contents
./dev.py truenas-install stage          # p2 /srv/truenas-install + profile truenas-maint
./dev.py truenas-install run --go       # one maint boot (bootonce) -> back on Fedora, result fetched
./dev.py truenas-install commit-p3 --go # image partition -> p3, read-back verified
./dev.py truenas-install stage-boot     # profile truenas
```

- Maint boot = TrueNAS kernel + `scripts/initramfs/truenas-maint-init`: mounts p2, runs `scripts/truenas-install-job.py` in the ISO's live env (`filesystem.squashfs` + tmpfs overlay). Log: p2 `/srv/truenas-install/logs/`.
- Job = the ISO installer's `zpool create` (`truenas_installer/install.py`) + the update image's own `truenas_install.main()`, in-process, with:
  - `update-grub` / `grub-install` skipped (no GRUB, no ESP; U-Boot boots us),
  - our kernel added to the new root before its `truenas-initrd.py` (modules, `/boot/vmlinuz-<kver>`, `config-<kver>`, depmod) -> the installer builds our initrd like any of its own kernels,
  - SQL: ssh service on, serial console ttyS0 115200.
- Pool built on a p3-sized partition of an image file on p2 (`/srv/truenas-install/p3.img`, loop `-P`), then copied to p3 from Fedora: nothing writes p3 until the install has succeeded and been reviewed.
- Watchdog: SP805 fed while SSD I/O or the job's CPU time moves; 300 s of neither = reset -> Fedora.

## Constraints

- p2: writes only under `/srv/truenas-install` and `/boot/awto`. 8 TB disks: never touched.
- `truenas-initrd.py` (middleware, on config changes) rebuilds the initrd inside the root; U-Boot loads the copy in `/boot/awto/truenas/`. After such a change: `./dev.py truenas-install stage-boot` (reads `out/` - TODO if it diverges).
- Unofficial arm64 userland: middleware/app failures are userland findings for #338, not kernel bugs by default.
