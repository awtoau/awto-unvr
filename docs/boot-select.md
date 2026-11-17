# Boot selection without a console (#338)

awto-uboot picks a **boot profile** from its saved env; Linux sets it over SSH. No serial console needed.

## Use

```
./dev.py boot-next status                 # bootonce / bootdefault, running profile, bootcmd in sync?
./dev.py boot-next truenas --reboot       # one boot of TrueNAS, then back to the default
./dev.py boot-next truenas --default      # TrueNAS every boot
./dev.py boot-next fedora [--default]     # clear bootonce (or bootdefault)
```

- Works from Fedora and TrueNAS: `scripts/uboot_env.py` is piped to the box's `python3` (no `fw_setenv` needed).
- Hung profile -> `./dev.py power-cycle`: `bootonce` was cleared before that boot, so the next one is the default.
- Running profile: `awto.profile=<name>` on `/proc/cmdline` (absent = Fedora's `/boot/uImage`).

## How

- `bootcmd` (= defconfig `CONFIG_BOOTCOMMAND`):
  - `bootsel = bootonce` if set (then `setenv bootonce; saveenv` **before** booting), else `bootdefault`.
  - `bootsel` set and not `fedora` -> from p2 `/boot/awto/<bootsel>/`: `uEnv.txt` (imports only `bootargs_alt`), `uImage`, `unvr.dtb`, `initrd` (raw); `bootm` with `bootargs_alt`.
  - Any load or `bootm` failure -> bootargs restored -> the unchanged Fedora path (`/boot/uImage`, `/boot/unvr.dtb`).
- Profile `usb` (#360): `usb start`, then the same four files from `usb <d>:2` `/boot/awto/usb/` (p2 of the USB device). No USB device / load failure -> Fedora path. `boot-next usb` skips the on-box presence check.
  - Stick (SanDisk Cruzer Switch, serial in `lsblk -o SERIAL`): MBR, p1 200 MiB FAT32 `USBBOOT-ESP` (reserved), p2 ext4 `awto-usb` made with `-b 4096 -O ^metadata_csum,^64bit,^metadata_csum_seed,^orphan_file` (U-Boot reads only).
  - Stage: mount p2 on the box (`mount /dev/disk/by-id/usb-*-part2 /run/usbp2; mkdir -p /run/usbp2/boot/awto`), then `./dev.py boot-next stage usb --uimage F --dtb F --initrd F --bootargs '<Fedora cmdline>' --base /run/usbp2/boot/awto`; umount. Kernel/dtb = SSD `/boot/uImage` + `/boot/unvr.dtb`; initrd = the 512 B empty cpio of `fedora-alt` (loader requires one; root stays on the SATA SSD by PARTUUID).
  - Result 2026-11-17: `boot-next usb --reboot` -> `awto.profile=usb`, ssh back in 44 s (default Fedora path 44 s). Profile with initrd removed -> `profile=fedora` in 52 s, `bootonce` cleared. Console log not captured (no tio running).
- Load map: uImage `0x02000000`, uEnv/dtb `0x0f000000`, initrd `0x20000000`. Limits enforced by `scripts/_bootsel.py` at stage time.
- U-Boot only **reads** filesystems (#347). Its only write is `saveenv` to NOR.
- Profiles live on p2 (ext4), not the ESP: ESP has ~120 MiB free and holds UEFI experiments; U-Boot's ext4 read of p2 is the proven #216 path.
- Env: NOR mtd "Factory" (`0x1e0000`, 64 KiB, not redundant). Linux DT no longer marks it `read-only`.
  - Every write: CRC check of the old env, raw backup to `tmp/bootsel/env-<stamp>.bin`, erase, write, read-back compare.
  - A torn write = bad CRC = U-Boot's compiled defaults = Fedora.

## Changing bootcmd

- Edit `CONFIG_BOOTCOMMAND` in `uboot-port/configs/alpine_v2_unvr_defconfig`.
- `./dev.py uboot-sandbox` - runs the exact string in U-Boot sandbox (old hush): bootonce/bootdefault/fedora/missing profile/canary. A bootcmd that fails on the box parks it at `awto-nas#` with no console.
- `./dev.py boot-next sync-bootcmd` - the saved env overrides the compiled one (#229); this writes the defconfig string.
- U-Boot binary not reflashed for this: reflashing needs the serial console (tftp via stock), and the saved env wins anyway.

## Stage a profile

```
./dev.py boot-next stage NAME --uimage F --dtb F --initrd F --bootargs '...'
```

- Copies into `/boot/awto/NAME.new`, sha256-verifies, swaps in. `awto.profile=NAME` appended to bootargs.
- TrueNAS profiles are staged by `./dev.py truenas-install` (docs/truenas.md).
