# UNVR boot flow — U-Boot and the stock initramfs

## U-Boot has working Ethernet and a built-in TFTP boot path ✅

Extracted from `tmp/sections/01-uboot.bin` (1.33 MB, section 1 of the container).
**Canonical U-Boot env (full default chain, OLD vs NEW deltas): [nor-boot-chain.md](nor-boot-chain.md) §4.**
The env excerpt below is what the netboot recipe needs.

The bootloader contains the **Annapurna `al_eth` driver**, the `Annapurna`
string, `eth0`–`eth3`, the `Net:` init banner, PHY handling and
`Waiting for PHY realtime link` ✅. Networking is not an initramfs-only feature.

Default environment (verified strings):

```
loadaddr         = 0x08000000
loadaddr_payload = 0x08000004
loadaddr_dt      = 0x04078000
kern_img         = uImage
autoload         = n

bootcmd          = run load_fdt; run loadbootargs; run multiboot
bootcmdspi       = run load_fdt; run loadbootargs; run bootspi
bootcmdrecovery  = setenv bootargsextra $bootargsextra boot=recovery; run load_fdt; run loadbootargs; run bootcmdspi
bootcmdtftp      = run load_fdt; run loadbootargs; run loadimg_tftp; run prebootm; run dobootm; run fail; exit

loadimg_tftp     = run preloadimg; lcd_print "Loading OS..."; tftpboot $loadaddr_payload ${tftpdir}${kern_img};
                   if test $? -ne 0; then run fail; exit; fi;
load_fdt         = cp.b $fdtaddr $loadaddr_dt 7ffc; fdt addr $loadaddr_dt
loadbootargs     = setenv bootargs pci=pcie_bus_perf console=ttyS0,115200
                   sysid=$sysid boot_carrier=$boot_carrier root=$rootfs $bootargsextra; printenv bootargs
multiboot        = run $bootfrom; ... run ${bootfrom}_bkp; ... run bootemmcblk; ... run bootspi
bootspi          = run loadimg_spi; run dobootm; lcd_print bootspi Failed!
loadimg_spi      = run preloadimg; sf probe; sf read $loadaddr $spi_pt_addr_kernel 4; ...
fail             = echo Failed!; lcd_print "Failed!"
```

### Signature verification is skipped by default ✅

`dobootm = run bootunsign` ships as the default (`bootunsign = bootm
$loadaddr_payload - $fdtaddr`, plain/unsigned; the FIT-signed `bootsign` exists but is
not run). **Consequence: U-Boot boots an arbitrary unsigned `uImage` over TFTP into
RAM with zero flash writes** — the safest bring-up (no USB, no MTD write, no env change
beyond volatile `setenv`; power-cycle returns to stock).

Full per-stage trust model (OLD vs NEW, FIT/RSA present-but-unused, stage3 RSA capability
check): **[nor-boot-chain.md](nor-boot-chain.md) §5** (canonical).

Serial console is `ttyS0,115200` per `loadbootargs` ✅.

### Netboot recipe

At the U-Boot prompt (Esc Esc within ~2 s of power-on):

```
setenv ipaddr   <unvr-ip>
setenv serverip <your-tftp-server>
setenv tftpdir  ''            # prefix, joined to $kern_img
setenv kern_img uImage
run bootcmdtftp
```

`bootcmdtftp` loads the FDT, builds bootargs, TFTPs `${tftpdir}${kern_img}` to
`0x08000004`, then `bootm`s it unsigned. `dhcp` and `bootp` are also compiled in
(`autoload=n`, so DHCP will not auto-fetch).

First sanity test: TFTP the **stock** kernel back — `tmp/sections/02-kernel.bin`
is already a valid uImage. If that boots, the path is proven before risking a
custom build.

`bootargsextra` is the hook UNVR-NAS uses (`boot=local rw`), and `rootfs` is the
`root=` value — both are plain env vars, settable volatilely for a test boot.

---

# From the stock initramfs

All ✅ verified: extracted from the kernel of `sources/UNVR-5.1.25.bin` with
`scripts/analyse-unvr-firmware.py --extract`. The kernel is built with
`CONFIG_INITRAMFS_SOURCE`, so a 17.5 MB gzip'd newc cpio is embedded in the
Image. Extracted tree: `tmp/sections/initramfs-0/`.

Stock build path (from `CONFIG_INITRAMFS_SOURCE`):
`/home/dio/jenkins_slave/workspace/irmware.debbox_unifi-nvr_v5.1.25/unvr4.alpine-2/build/target-unvr4.alpine/image/unvr-image/initramfs.cpio`
— note `debbox`, matching `thezim/debbox-kernel` naming.

It is Debian `initramfs-tools` with Ubiquiti overrides in `/scripts/`:
`ubnt`, `ui-boot-proto`, `ui-boot-emmc`, `ui-boot-nand`, `ui-boot-firmware`,
`ui-boot-network`, `ui-helper-functions`, `product-override`,
`persistent-whitelist`.

## Two hardware variants — ours is the NAND/USB one

`scripts/product-override` dispatches on board sysid:

```
case "x${SYSID}" in
xea16) # UNVR without eMMC
    . /scripts/ui-boot-nand ;;
*)  wait_device /dev/boot 5 "true"
    [ ! -b /dev/boot ] && . /scripts/ui-boot-nand ;;
esac
```

**`ea16` = "UNVR without eMMC"** ✅ — the USB-boot variant. This reconciles the
board-ID confusion: `ea16` (no eMMC, ours) vs `ea1a` (the unit linux-alpine-v2
tested). `unifi-unvr-emu` shims `0xea16`. Different UNVRs, not a contradiction.

| | eMMC variant (`product-override`) | **NAND/USB variant (`ui-boot-nand`) — ours** |
|---|---|---|
| Kernel | `/dev/boot1` | **`/dev/mtd9`** |
| Rootfs | `/dev/boot2` | **`/dev/mtd10`**, decompressed to `/dev/ram0` |
| Data | `/dev/boot3` | — |
| Log | `/dev/boot4` | — |
| **User/overlay** | `/dev/boot5` | **`/dev/sdq`** — *the internal USB stick* |
| Config | `/dev/mtdblock6` | `/dev/mtdblock6` (SPI-NOR) |

eMMC GPT layout (`fcd_init_layout`): kernel 2048–264191S (128 MiB), root
264192–4458495S (2 GiB), data →12847103S (4 GiB), log →21235711S (4 GiB),
overlay →30777310S. Total ~14.7 GB — **larger than our 7.93 GB stick**, further
confirming ours is not that variant.

### What this means for our failing USB

**The USB stick holds only USERDEV — the overlay upper layer.** The OS itself
(kernel + rootfs) lives in NAND and is unaffected by the stick failing.

That matches the image exactly: bare ext4 across the whole device with no
partition table, containing `data/` and `.workdir/` — precisely what the overlay
mount expects. See [recovery.md](recovery.md).

Consequence: **write the rescued image to a fresh USB and the device comes back
with its config, users and state intact.** A blank stick would boot too, but
lose all of it.

## Overlay construction

```
mkdir_mount_p -t overlay \
  -o lowerdir=${MNT_ROFS},upperdir=${MNT_RWFS}/data,workdir=${MNT_RWFS}/.workdir \
  overlayfs-root ${rootmnt}
```

`MNT_ROFS=/mnt/.rofs` (squashfs, read-only), `MNT_RWFS=/mnt/.rwfs` (the USB).
Then `mount_move` of `${MNT_BOOT}` → `/boot/firmware`, and `mount_system_part`
for `data`, `log`, `persistent`.

`purify_userdev_data` **deletes** these from the overlay upper on every boot:
`usr/lib/version`, `etc/ld.so.preload`, `etc/ld.so.conf`, `etc/crontab`,
`etc/profile.d`, `etc/environment`, `etc/security`, `etc/modules-load.d`, plus
any `etc/ld.so.conf.d/*.conf` that also exists in the lower layer. **Persistence
hacks placed in those paths will not survive a reboot.**

## Kernel cmdline options ✅

Parsed in `scripts/ubnt` from `/proc/cmdline`:

| Option | Effect |
|---|---|
| `break=<name>` | **initramfs-tools debug shell** — see below |
| `server=<ip>` | `DOWNLOAD_SERVER`, default **192.168.1.8** |
| `client=<ip>` | `DOWNLOAD_CLIENT`, default **192.168.1.64** |
| `sysid=<hex>` | Override board sysid (else read via `ubnt-tools id`) |
| `root=<dev>` | `ROOTFS_IMAGE` |
| `factory` | Factory install: network + download firmware + reset userdev + reboot |
| `no_reboot` | Suppress the automatic reboot |
| `nc_transfer` | Receive firmware over netcat instead of tftp |
| `wget_transfer=<url>` | Receive firmware over HTTP |

## The serial-console entry point: `break=`

The Ubiquiti scripts are littered with `maybe_break` — the stock initramfs-tools
debug hook. `break=<name>` on the kernel cmdline **drops to a root shell on the
console** at that point, with hardware up and before any UniFi service starts.

Ubiquiti-specific break points ✅: `reset_userdev`, `reset_sys_part`,
`reset_storage`, `post_reset`, `network`, `post_network`, `fwdown`,
`post_fwdown`, `fwupdate`, `post_fwupdate`, `post_fwupdate_failed`.

Standard initramfs-tools break points: `top`, `modules`, `premount`, `mount`,
`mountroot`, `bottom`, `init`.

`console_redirect /dev/console` tees all boot output to `/tmp/boot.log`, which
`mount_bottom` moves to `${rootmnt}/boot/boot.log` — so **the previous boot log
is readable from the running system**.

## Network install paths ✅

`scripts/ui-boot-firmware` `download_firmware()` offers three transports, all
reached by `factory` (or the `/config/.network-upgrade` flag):

```
nc_transfer        nc -l -p 5566 > fw-image.bin        # you push it
wget_transfer=URL  wget ${WGET_URL} -O fw-image.bin
default            tftp -g -l <dest> -r fw-image.bin ${DOWNLOAD_SERVER}
```

`scripts/ui-boot-network` `enable_network_single_if()` walks
`/sys/class/net/{eth*,enp*,otx*}`, brings each up, picks the **first with
carrier**, assigns `${DOWNLOAD_CLIENT}/24`, then pings `${DOWNLOAD_SERVER}` up to
10× before panicking. A bridge variant (`enable_network_bridge`) exists but
`product-override` selects the single-interface one.

## Local firmware install without the WebUI ✅

`mount_premount` checks `${MNT_RWFS}/upgrade/fw-image.bin` — drop a firmware
image at `upgrade/fw-image.bin` on the user partition and it flashes on next
boot, then reboots.

**But it is signature-checked.** `check_firmware()` runs
`/sbin/ubnt-tools fwupdate -dc <file>`, falling back to
`-s /etc/ssl/fw.pub`. That pubkey is **not present in the initramfs** ✅, so the
fallback cannot engage — verification is whatever `ubnt-tools` does internally.
Combined with the container's `ENDS` 256-byte RSA signature, **a modified `.bin`
will not install by this path.** That is exactly why the kernel-replacement route
matters: it bypasses the firmware updater.

## Config flags in `/config` (SPI-NOR `mtdblock6`)

`.factory-reset`, `.network-upgrade`, `.reset-to-defaults` (may contain
`FORMAT_EXT_DEVS=true`), `.upgrade-bootup`, `.reset-bootup`, `.recover-bootup`.

## Note on `ui-boot-nand` and the HDDs

The NAND variant can promote USERDEV to **`/dev/md1`**, an mdadm RAID1 across the
installed HDDs, rather than the USB — `mdadm --assemble --run /dev/md1`, or
create with `mdadm -C /dev/md1 -f -R -e1.2 -l 1 --name ${sn}:1`. So on a populated
unit the overlay may live on the disk array. Worth confirming against our image
before assuming the USB is the only copy of config.
