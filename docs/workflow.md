# Workflow — cold-start bring-up narrative (2026-08-15)

Reproducible sequence for the UNVR (`ea16`, USB-boot variant) from a cold start.
Work done 2026-08-15 — the historical record of the initial bring-up.
**Current state is [project-status.md](project-status.md)**: the box now runs Fedora 44
standalone (kernel ports + Fedora + full hardware RE all landed since — see the
milestones at the end). Each step: what / command / result / gotcha.

Marks follow [sources.md](sources.md): ✅ verified here, 📄 reported elsewhere.

Companion docs: [recovery.md](recovery.md) (the failing stick),
[boot-flow.md](boot-flow.md) (U-Boot + initramfs), [porting-reference.md](porting-reference.md)
(hardware/kernel/container), [sources.md](sources.md) and [prior-art.md](prior-art.md)
(external work).

---

## 1. Identify the failing USB stick

**What:** the UNVR's internal boot stick, pulled and plugged into the workstation.

```
lsblk -o NAME,SIZE,TRAN,MODEL,SERIAL,TYPE
sudo dumpe2fs -h /dev/sdd
dmesg
```

**Result** ✅

| | |
|---|---|
| Model | Phison `UF3 8GB` (USB `13fe:5500`) |
| Serial | `07190176402AEE98` |
| by-id | `/dev/disk/by-id/usb-_UF3_8GB_07190176402AEE98-0:0` |
| Size | 7927234560 B = 15482880 × 512 B sectors (7.93 GB / 7.38 GiB) |
| Layout | **no partition table** — whole-disk ext4 |
| ext4 UUID | `ff18e0d4-05d8-47a0-a6b0-59b3b4d51c18` |
| Contents | `data/` + `.workdir/`, hostname `UNVR` |
| Health | Medium Error / unrecovered read error, first at **LBA 229376** (112 MiB) |
| Read rate | 770 kB/s sequential — controller retrying internally over a wide area |

**Gotchas**

- Whole-disk ext4, no partition table, is *correct* — it is USERDEV, the overlay
  upper layer, and `mkfs_p ${USERDEV}` runs against the whole device.
- Journal carries `needs_recovery`. Mount `-o ro,noload` or a plain mount replays
  it and mutates the evidence.
- `/dev/sdX` shifts between plugs. Everything downstream keys off the USB serial.
- 8 GB Phison under constant Protect logging is the known product defect, not a
  one-off — see [recovery.md](recovery.md).

## 2. Image it with ddrescue

**What:** get the only copy of the old `/data/etc` + unifi-core state off a dying
device before touching anything else.

```
sudo ./scripts/rescue-unvr-usb.py rescue     # two passes, resumable
sudo ./scripts/rescue-unvr-usb.py status     # ddrescuelog -t on the mapfile
sudo ./scripts/rescue-unvr-usb.py manifest   # sha256 + images/README.md
```

**Why ddrescue, not dd** ✅ — dd died on the medium error at LBA 229376: no retry,
no skip, not resumable. ddrescue records progress in a mapfile, so a killed run
resumes where it stopped and never re-reads good regions.

**Shape**

- Pass 1 `-n`: fast, skip error zones — get the bulk off before further stress.
- Pass 2 `-r3`: scrape and retry only what pass 1 skipped.
- `--idirect -b 512`, both passes share one mapfile.
- Identity guard: `by-id` serial + exact sector count + not-mounted, all checked
  before a single read. Wrong size → refuse.
- Timeouts: 120 s (pass 1) / 300 s (scrape) with **no** successful sector.
  Derived from the measured 770 kB/s good-region rate — a 64 KiB cluster lands in
  ~85 ms, so nothing at all for 120 s means a wedged controller, not slow media.
  On expiry ddrescue exits non-zero, the mapfile is intact, rerunning resumes.
- Outputs `chown`ed back to the invoking user (the script runs under sudo).

**Outputs:** `images/unvr-usb-07190176402AEE98.img` (gitignored, sparse),
`images/unvr-usb-07190176402AEE98.map` (**tracked** — the record of what was and
was not recoverable), `images/README.md` (manifest), `tmp/logs/rescue-unvr-usb.log`.

**Gotcha:** do not run further full-device reads against the original. Work from
the image.

## 3. Fetch the current firmware

**What:** Ubiquiti publishes no flashable USB image — only a `.bin` container.
Get it and prove it arrived intact.

```
./scripts/fetch-unvr-firmware.py fetch
./scripts/fetch-unvr-firmware.py verify
```

API: `https://fw-update.ui.com/api/firmware-latest?filter=eq~~platform~~unvr`
(platform `UNVR`, product `unifi-nvr`). Re-query to pick up a newer release.

**Result** ✅ — `sources/UNVR-5.1.25.bin`, 786253430 B, sha256
`74f2833356e832bd97f59bb3686eaedaf704335631b5daabedfce4d30bb222fc`, md5
`3f661a852b6fb70946e2a94712df6923`, `ubnt_version`
`UNVR4.al324.v5.1.25.84c48e7.260710.1602`, released 2026-07-27.
Manifest written to [../sources/README.md](../sources/README.md).

**Gotchas**

- Fetched with `curl` **by explicit approval** — playwrong has no binary-download
  tool (only `pdf` writes files). The Ubiquiti sha256 is what makes that safe: the
  silent-wrong-content failure the no-curl rule guards against is detectable here.
- Verification is mandatory and destructive on failure — a bad copy is deleted
  rather than left looking valid. Size **and** sha256 **and** md5 must match.
- curl stall guard: abort under 10 kB/s for 30 s; `--max-time 3600` backstop.
  Partial file removed on any non-zero exit.
- `sources/` also holds the 5.1.19 and 5.1.15 CDN downloads for comparison. They
  are not in the manifest and not verified.

## 4. Decode the firmware container

**What:** find out what is inside the `.bin` and whether it can be repacked.

```
./scripts/analyse-unvr-firmware.py
```

**Format** ✅ (Ubiquiti's own, GPL-published as `mkfwimage` `src/fw.h` in
openwrt/firmware-utils; verified byte-for-byte, **all CRCs valid**):

```
header  268 B (0x10C): magic[4] "UBNT" + version[256] + crc32 u32 BE @0x104 + pad
                       header crc = crc32(0, image, 0x104)
record   56 B (0x38):  magic[4] + name[16] + pad[12] + 6x u32 BE:
                       memaddr, index, baseaddr, entryaddr, data_size, part_size
        payload data_size B at record+0x38
        trailer  8 B: crc32(0, record_start, 0x38+data_size) u32 BE + pad
        stride = 0x38 + data_size + 8
END.    12 B  terminator, crc32 over the whole preceding image
ENDS   264 B  terminator, 256-byte RSA signature, no crc
```

**Walk the length chain, never scan for magics** — `PART`/`FILE` occur inside
compressed payloads, and the tag differs between revisions (UNVR 4.1.9 used
`FILE` for rootfs, 5.1.25 uses `PART`). The tag is cosmetic; the chain is the
structure. Known magics: `UBNT`/`OPEN`/`GEOS`, `PART`, `FILE`, `EXEC`, `GART`,
`END.`, `ENDS`.

**Sections in 5.1.25** ✅ (header crc `0xd9417360` OK, 4 sections, 0 CRC failures):

| # | Off | Magic | Name | data_size | part_size | base |
|---|---|---|---|---|---|---|
| 1 | 0x0000010C | FILE | `uboot` | 1395740 (1.33 MB) | 2097152 | 0x00000000 |
| 2 | 0x00154D68 | FILE | `kernel` | 13846342 (13.20 MB) | 14680064 | 0x00200000 |
| 3 | 0x00E894EE | PART | `rootfs` | 766357504 (730.86 MB) | 1010827264 | 0x01000000 |
| 4 | 0x2E96452E | FILE | `updater` | 4653056 (4.44 MB) | 5242880 | 0x3D400000 |
| — | 0x2EDD456E | ENDS | — | — | — | RSA-signed |

**Gotcha — the image is `ENDS`-terminated, i.e. RSA-signed.** A modified
container cannot be re-signed, so repacking a stock `.bin` is not a route.
Replace the `uImage` on the boot device instead.

Rootfs squashfs at 0x00E89526: SQUASHFS 4:0, **zstd**, block 262144, 63607
inodes, built 2026-07-10 18:10:55. Read in place without extracting 730 MB:
`unsquashfs -o 15242534 sources/UNVR-5.1.25.bin`. Full listing (63612 lines) in
`tmp/logs/rootfs-listing.txt`.

## 5. Extract kernel, IKCONFIG and initramfs

```
./scripts/analyse-unvr-firmware.py --extract      # -> tmp/sections/
```

**Result** ✅

| Artifact | Detail |
|---|---|
| `tmp/sections/02-kernel.bin` | legacy U-Boot uImage, magic `0x27051956`, 64-byte header |
| uImage name | `4.19.152-alpine-unvr @ 20260710-` |
| load / entry | **0x04080000** (both) |
| comp / arch | 1 (gzip) / 22 (arm64) |
| Built | 2026-07-10T08:11:27Z |
| `kernel-Image` | 20744704 B, `ARM\x64` magic at 0x38 confirmed |
| `kernel.config` | 92970 B, from embedded `IKCFG_ST` |
| `initramfs-0.cpio` | **17538560 B** newc cpio, gzip stream at offset **12154300** in the Image, 34255 blocks extracted to `tmp/sections/initramfs-0/` |

**How the initramfs is found:** the kernel is built with
`CONFIG_INITRAMFS_SOURCE`, so the script tries *every* gzip stream in the Image
and keeps the ones inflating to newc cpio (magic `070701`) over 4096 B. Scanning
for a single known offset would not survive a firmware bump.

**Gotcha:** `IKCFG_ST`'s gzip stream ends at `IKCFG_ED`, so `gzip.decompress`
fails on the trailing bytes — `zlib.decompressobj` tolerates them.

Stock build path from `CONFIG_INITRAMFS_SOURCE` ✅:
`.../irmware.debbox_unifi-nvr_v5.1.25/unvr4.alpine-2/build/target-unvr4.alpine/image/unvr-image/initramfs.cpio`
— `debbox`, matching `thezim/debbox-kernel` naming.

The SPI-NOR `recovery kernel` partition is a **separate** kernel, not this one.

## 6. What the kernel config settled

**What:** questions that would otherwise need a running device, answered offline
from `tmp/sections/kernel.config`.

| Setting | Value | Consequence |
|---|---|---|
| `CONFIG_MODULE_SIG` | **not set** | Module signing **not enforced** — `insmod` your own modules |
| `CONFIG_KEXEC` | **not set** | Not built in; would need `kexec-mod`, unported to Alpine V2 → **kexec is out** |
| lockdown | **absent entirely** | No kernel lockdown |
| `CONFIG_IKCONFIG` / `_PROC` | `y` / `y` | Config embedded; `/proc/config.gz` also on a live box |
| `CONFIG_NET_AL_ETH` | `y` | al_eth **built in**, not a module — no `.ko` to lift |
| `CONFIG_AL_DMA`, `CONFIG_AL_HAL` | `y` | Built in |
| `CONFIG_AL_THERMAL_V2`, `_V3` | `y` | Built in |
| `CONFIG_ARCH_ALPINE` | `y` | |
| `CONFIG_AL_ETH_ALLOC_FRAG` | `y` | frag allocator (not PAGE, not SKB) |
| `CONFIG_AL_ETH_FORCE_SFP_1G` | not set | SFP+ not pinned to 1G |

Built in per `modules.builtin`: `al_eth_drv`, `al_dma_drv`, `soc/alpine/hal`,
`soc/alpine/al_hal_export`. Shipped as modules: `al_nand`, `marvell10g`,
`phylink`, `ubnthal`, `ubnt_common`, `ui-hdd-pwrctl`.

## 7. Read the initramfs boot scripts

**What:** learn the boot decision tree before touching the hardware.
Source: `tmp/sections/initramfs-0/scripts/`. Full detail in
[boot-flow.md](boot-flow.md).

Debian `initramfs-tools` plus Ubiquiti overrides: `ubnt`, `ui-boot-proto`,
`ui-boot-emmc`, `ui-boot-nand`, `ui-boot-firmware`, `ui-boot-network`,
`ui-helper-functions`, `product-override`, `persistent-whitelist`.

**Variant dispatch** ✅ — `scripts/product-override`:

```
case "x${SYSID}" in
xea16) . /scripts/ui-boot-nand ;;          # "UNVR without eMMC" - OURS
*)  wait_device /dev/boot 5 "true"
    [ ! -b /dev/boot ] && . /scripts/ui-boot-nand ;;
esac
```

| | eMMC variant | **NAND/USB variant (`ui-boot-nand`) — ours** |
|---|---|---|
| Kernel | `/dev/boot1` | `/dev/mtd9` |
| Rootfs | `/dev/boot2` | `/dev/mtd10`, decompressed to `/dev/ram0` |
| **User/overlay** | `/dev/boot5` | **`/dev/sdq`** — the internal USB stick |
| Config | `/dev/mtdblock6` | `/dev/mtdblock6` |

Overlay: `lowerdir=/mnt/.rofs` (squashfs ro), `upperdir=/mnt/.rwfs/data`,
`workdir=/mnt/.rwfs/.workdir`. **So the USB holds only USERDEV** — kernel and
rootfs live in NAND and are unaffected by the stick failing. That matches the
rescued image exactly.

`purify_userdev_data` **deletes** these from the overlay upper on every boot:
`usr/lib/version`, `etc/ld.so.preload`, `etc/ld.so.conf`, `etc/crontab`,
`etc/profile.d`, `etc/environment`, `etc/security`, `etc/modules-load.d`, plus
`etc/ld.so.conf.d/*.conf` also present in the lower layer. Persistence hacks in
those paths do not survive a reboot.

**Kernel cmdline options** ✅ (parsed in `scripts/ubnt`): `break=<name>`,
`server=<ip>` (default 192.168.1.8), `client=<ip>` (default 192.168.1.64),
`sysid=<hex>`, `root=<dev>`, `factory`, `no_reboot`, `nc_transfer`,
`wget_transfer=<url>`.

**`break=` is the console entry point** — the stock initramfs-tools debug hook,
dropping to a root shell with hardware up and before any UniFi service starts.
Ubiquiti-specific points: `reset_userdev`, `reset_sys_part`, `reset_storage`,
`post_reset`, `network`, `post_network`, `fwdown`, `post_fwdown`, `fwupdate`,
`post_fwupdate`, `post_fwupdate_failed`. Standard: `top`, `modules`, `premount`,
`mount`, `mountroot`, `bottom`, `init`.

**Network install transports** ✅ (`ui-boot-firmware` `download_firmware()`, reached
by `factory` or `/config/.network-upgrade`):

```
nc_transfer        nc -l -p 5566 > fw-image.bin        # you push it
wget_transfer=URL  wget ${WGET_URL} -O fw-image.bin
default            tftp -g -l <dest> -r fw-image.bin ${DOWNLOAD_SERVER}
```

`ui-boot-network` `enable_network_single_if()` walks
`/sys/class/net/{eth*,enp*,otx*}`, brings each up, picks the **first with
carrier**, assigns `${DOWNLOAD_CLIENT}/24`, pings `${DOWNLOAD_SERVER}` up to 10×
then panics.

**Gotcha:** dropping `upgrade/fw-image.bin` on USERDEV *does* trigger an install
— but `check_firmware()` runs `/sbin/ubnt-tools fwupdate -dc <file>`. Combined
with the container's `ENDS` signature, a modified `.bin` will not install this
way. The `/etc/ssl/fw.pub` fallback key is **not in the initramfs** ✅, so
verification is whatever `ubnt-tools` does internally.

## 8. Extract U-Boot, find the netboot path

**What:** establish a bring-up route with **zero writes to any flash**.
Source: `tmp/sections/01-uboot.bin` (1.33 MB, section 1).

**Result** ✅ — U-Boot contains the Annapurna `al_eth` driver, the `Annapurna`
string, `eth0`–`eth3`, the `Net:` banner, PHY handling and
`Waiting for PHY realtime link`. Networking is not initramfs-only. Build path
`/project/users/barak/tasks/al-boot-release/`.

Default environment (verified strings):

```
loadaddr = 0x08000000   loadaddr_payload = 0x08000004   loadaddr_dt = 0x04078000
kern_img = uImage       autoload = n

bootcmd         = run load_fdt; run loadbootargs; run multiboot
bootcmdtftp     = run load_fdt; run loadbootargs; run loadimg_tftp; run prebootm; run dobootm; run fail; exit
bootcmdrecovery = setenv bootargsextra $bootargsextra boot=recovery; run load_fdt; run loadbootargs; run bootcmdspi
loadbootargs    = setenv bootargs pci=pcie_bus_perf console=ttyS0,115200
                  sysid=$sysid boot_carrier=$boot_carrier root=$rootfs $bootargsextra

bootsign   = bootm $loadaddr_payload#$model@$fit_index    <- FIT, signed
bootunsign = bootm $loadaddr_payload - $fdtaddr           <- plain, unsigned
dobootm    = run bootunsign                               <- DEFAULT
```

**`dobootm` runs the unsigned path by shipped default** ✅ — not a bypass.
Independently confirms linux-alpine-v2's note that U-Boot signature checking is
"currently skipped". Consequence: U-Boot will boot an arbitrary unsigned `uImage`
pulled over TFTP into RAM, no flash writes, no env change beyond volatile
`setenv`. Power-cycle returns to stock.

Netboot recipe (U-Boot prompt = **Esc twice within ~2 s** of power-on):

```
setenv ipaddr   <unvr-ip>
setenv serverip <your-tftp-server>
setenv tftpdir  ''
setenv kern_img uImage
run bootcmdtftp
```

First sanity test: TFTP the **stock** kernel back — `tmp/sections/02-kernel.bin`
is already a valid uImage. Prove the path before risking a custom build.

`console=ttyS0,115200` in `loadbootargs` is where the console baud comes from ✅.

## 9. Serial console bring-up

**Wiring** ✅ — 4-pin header on the PCB **behind the SFP+ cage**, middle of board.
Use **three pins only: GND, TXD, RXD**. **Do not connect the 3V3 pin** — the
board supplies its own rail and back-feeding it can damage something.
Adapter TX → UNVR RX, adapter RX → UNVR TX, GND → GND. 3.3 V TTL. **115200 8N1.**
(UNVR Pro's header is near the DC Power Backup port 📄 — different board.)

**Adapter triage, in order:**

```
./scripts/serial_loopback_test.py            # TX-RX jumper, adapter only
./scripts/serial_baud_sweep.py               # 11 bauds on the same loopback
./scripts/unvr-console.py --sweep --port /dev/ttyUSB0   # against the device
```

**Result** ✅

- Loopback on the **Prolific PL2303**: 0 bytes back at 9600 / 115200 / 921600
  across 610 repeat rounds, and `RTS→CTS`, `DTR→DSR`, `DTR→CD` all "does not
  follow". `serial_baud_sweep.py` likewise got nothing at all 11 bauds. A plain
  TX-RX wire is rate-agnostic and must echo at *every* baud — nothing anywhere
  means a dead adapter/cable, not a baud problem.
- Switched to the **Silicon Labs CP2102**. Real console output at 115200.
- `unvr-console.py --sweep` scores the printable-ASCII ratio of the reply per
  baud: wrong rate returns high-entropy bytes, right rate returns mostly ASCII.
  Listen window 0.4 s/baud (~100× a device's few-ms response), whole sweep ~4 s.

**Gotchas**

- Address the adapter by **`/dev/serial/by-id/...`**, not `ttyUSBn` — the number
  shifted between plugs, and one early session opened the wrong one and logged
  pure garbage.
- Prefer the CP2102: Silicon Labs boards are natively 3.3 V logic. PL2303 cables
  vary and some are 5 V.
- The autoboot window is ~2 s and easy to miss by hand. `unvr-console.py --catch`
  streams ESC at 20/s — start it **before** powering on and the device lands at
  the U-Boot prompt on its own.
- **tio pty gotcha** ✅: with a pipe or `/dev/null` on stdin, `tio` takes its
  documented `echo cmd | tio` pipe mode — sends stdin to the device, sees EOF,
  **exits 0 immediately, and never creates the socket**. Evidence in
  `tmp/logs/dev.log`: the 16:10:07 launch reported a pid, and `console-status`
  six seconds later found no tio and an absent socket. Fix is
  `setsid script -qec "<tio cmd>" /dev/null` — `script` allocates the pty,
  `setsid` detaches it so it outlives the caller. The 16:12:02 launch with that
  wrapper stayed up.

## 10. The console model

One `tio` owns the serial port and exposes a unix socket. Humans and agents both
attach to the socket instead of fighting over `/dev/ttyUSB*`. tio does its own
logging (`-L --log-strip -t`), so there is no tmux and no pane lifecycle.

```
./dev.py console                      # start tio (setsid script wrapper)
./dev.py console-status               # pid / socket / port / log size
./dev.py console-attach               # interactive; Ctrl-C detaches, tio survives
./dev.py console-send 'cat /proc/mtd' # send one line, print what comes back
./dev.py console-stop
./dev.py doctor                       # tio, nc, ddrescue, unsquashfs, gh, nmap, serial
./dev.py describe                     # machine-readable JSON for agents
```

- Port preference: CP2102 by-id → `/dev/ttyUSB1` → `/dev/ttyUSB0`.
- Socket `$XDG_RUNTIME_DIR/tio-unvr.sock`; log `tmp/logs/unvr-console.log`.
- `--log-strip` keeps the on-disk copy free of ANSI — ANSI in a log ruins every
  later grep.
- `console-send` read window is **1.5 s**: a shell echoes in milliseconds, so
  ~1000× expected latency, and on no reply the caller sees empty output, not a hang.
- `_console_pid()` finds the real tio by `pgrep -x tio`, not the recorded wrapper
  pid, and clears a stale pidfile.

## 11. Find the device on the LAN

```
./scripts/find-unvr.py                      # discovery + mDNS
./scripts/find-unvr.py --sweep              # plus nmap 443,22 on the local /24
./scripts/find-unvr.py --subnet 192.168.25.0/24
```

Three independent methods, because any one can miss: Ubiquiti discovery (UDP
10001 broadcast, TLV reply with MAC/model/firmware/hostname), mDNS via
`avahi-browse`, and a TCP sweep for the UniFi OS web UI (443) and SSH (22).
Read-only — probes and TCP connects, nothing written to any device. 3 s collect
window (~500× a LAN device's single-digit-ms reply).

**Result** ✅ — **nothing.** No Ubiquiti discovery reply, no UniFi mDNS records,
no open TCP/UDP ports. The device answers **ICMP only**.

**Gotcha — that is not a network fault.** The initramfs brings the interface up
(`enable_network_single_if`) but the boot aborts before userspace, so nothing
ever binds a port. Network discovery cannot find this box; the serial console is
the only way in. Its address came from the console instead (step 12).

## 12. What the live device told us

With the console up, the device was powered on and dropped to the initramfs
BusyBox shell on its own — `Failure: can not detect /dev/sdq`, then
`/dev/sdq is corrupted, format and restore configs`, `Touch restore flag`,
`mount: mounting /dev/sdq on /mnt/.rwfs failed: No such file or directory`,
`ERROR: failed mounting /dev/sdq`. Exactly the `ui-boot-nand` path from step 7,
failing at USERDEV.

```
./dev.py console-send 'cat /proc/mtd'
./dev.py console-send 'ubnt-tools id'
./dev.py console-send 'ip -4 addr show'
./dev.py console-send 'cat /proc/cmdline'
./dev.py console-send 'uname -a'
```

**`/proc/mtd` — 13 partitions** ✅ (`tmp/logs/unvr-console.log`):

| Dev | Size | Erase | Name | Flash |
|---|---|---|---|---|
| mtd0 | 0x00200000 | 0x40000 | `al_boot` | NAND |
| mtd1 | 0x00100000 | 0x40000 | `device_tree` | NAND |
| mtd2 | 0x01000000 | 0x40000 | `linux_kernel` | NAND |
| mtd3 | 0x3ec00000 | 0x40000 | `rootfs` | NAND |
| mtd4 | 0x00100000 | 0x40000 | `chike` | NAND |
| mtd5 | 0x001c0000 | 0x1000 | `u-boot` | SPI-NOR |
| mtd6 | 0x00010000 | 0x1000 | `u-boot env` | SPI-NOR |
| mtd7 | 0x00010000 | 0x1000 | `u-boot env redundant` | SPI-NOR |
| mtd8 | 0x00010000 | 0x1000 | `Factory` | SPI-NOR |
| mtd9 | 0x00010000 | 0x1000 | `EEPROM` | SPI-NOR |
| mtd10 | 0x01000000 | 0x1000 | `recovery kernel` | SPI-NOR |
| mtd11 | 0x00dff000 | 0x1000 | `config` | SPI-NOR |
| mtd12 | 0x00001000 | 0x1000 | `cksum` | SPI-NOR |

NAND = mtd0–4, erase 256 K. SPI-NOR = mtd5–12, erase 4 K.

**`ubnt-tools id`** ✅

```
board.sysid=0xea16          board.name=Protect Network Video Recorder
board.shortname=UNVR        board.reboot=30      board.upgrade=310
board.cpu.id=411fd073-00000000
board.uuid=810d6686-46ac-5d49-9aa5-428a04c10400
board.bom=113-02832-29      board.hwrev=0xb101d
board.serialno=74acb941a811 board.qrid=icvpQD
```

**`/proc/cmdline`** ✅ —
`pci=pcie_bus_perf console=ttyS0,115200 sysid=ea16 ubnthal.sysid=ea16 reboot=cold`

**`uname -a`** ✅ — `Linux (none) 4.1.37-ubnt #2 SMP Wed Dec 16 18:18:20 CST 2020
aarch64 GNU/Linux`

**`ip -4 addr show`** ✅ — `enp0s1` at `192.168.25.140/24`, carrier up. (This is the
stock initramfs / U-Boot `ipaddr` tftp context; the later Fedora DHCP lease may differ —
don't treat `.140` as woomera's current IP.)

**Gotchas**

- The unit runs **4.1.37-ubnt (Dec 2020)** — UniFi OS 1.x era, matching the
  stick's last write in 2022. The firmware we analysed is 4.19.152 / 5.1.25.
  **The two are different kernels; do not assume offsets or numbering carry over.**
- **MTD numbering is version-specific.** The 5.1.25 `ui-boot-nand` expects kernel
  `/dev/mtd9`, rootfs `/dev/mtd10`, config `/dev/mtdblock6`. On this live 4.1.37
  box the kernel is mtd2, rootfs mtd3, config **mtd11** (the console shows
  `EXT4-fs (mtdblock11): mounted filesystem`). riptidewave93's published 7-entry
  map (mtd0 = `u-boot`) is our SPI-NOR set shifted by 5. **Read `/proc/mtd` on the
  actual box before writing anything.**
- Reaching the shell required no `break=` — the USERDEV failure drops there by
  itself. Convenient now, but it is a failure path, not a supported entry point.

## 13. Prepare a replacement USERDEV

**What:** a fresh stick formatted the way `ui-boot-nand` expects, so the device
can boot and self-populate.

```
./scripts/prep-userdev.py --list
./scripts/prep-userdev.py --serial A20043FE1501C484            # dry run
./scripts/prep-userdev.py --serial A20043FE1501C484 --yes      # does it
```

**Shape** — `wipefs -a`, then `mkfs.ext4 -F` across the **whole device, no
partition table**. `recovering_userdev` runs `mkfs_p ${USERDEV}` against the
whole device, and `mountroot` creates `data/` and `.workdir/` itself, so a blank
filesystem is enough. Label defaults to none, matching stock.

**Guards** ✅ — target selected by **USB serial**, never `/dev/sdX`:

- `DENY_SERIALS = {07190176402AEE98}` — the original stick can never be a target.
- transport must be `usb`, type must be `disk`.
- size ceiling 128 GiB (stock part is 8 GB; generous headroom that still excludes
  any plausible SSD/NVMe mis-selection).
- refuses if the device or any partition of it is mounted.
- dry run unless `--yes`.

**Result** ✅ — dry run at 16:08 selected `/dev/sde`, SanDisk Ultra, serial
`A20043FE1501C484`, 63233327104 B. The UNVR stick showed `[DENYLISTED]` in
`--list`. **Not yet executed with `--yes`.**

**Gotcha:** device letters shift between plugs and a `mkfs` on the wrong one
destroys a 1 TB SSD. This is why nothing here accepts a `/dev/sdX` argument.

---

## Alongside: desk research

```
./scripts/research-prior-art.py    # -> tmp/logs/prior-art.json + prior-art.md
```

`gh api` over 15 repos: metadata, forks ahead of parent, issues, PRs. Written up
in [prior-art.md](prior-art.md) and [sources.md](sources.md). Two conclusions
that changed the plan:

- **kexec is out** — confirmed twice, by the absent `CONFIG_KEXEC` (step 6) and by
  `udm-kernel-tools` being UDM-1.x-only with `kexec-mod` never reported working on
  Alpine V2.
- **The port already exists** — `bcyangkmluohmars/linux-alpine-v2`, Linux 6.12 LTS
  with `alpine-v2-ubnt-unvr.dts` and `unvr_defconfig`, tested on real UNVR
  hardware. Start there, not from scratch.

Dates recorded are `pushed_at` (last commit), not `updated_at` — `gh search`
reports the latter and stars bump it.

---

## Current state (2026-08-15 snapshot — SUPERSEDED)

Historical snapshot of the first day. Current state: [project-status.md](project-status.md)
(box runs Fedora 44 standalone; USB removed; MTD dumps taken; kernel ports done).

| Thing | State (as of 2026-08-15) |
|---|---|
| Boot USB | Failing but **fully recovered**. Every allocated ext4 block rescued, **0 unrecovered bytes**. Verified by `scripts/verify-rescue-coverage.py`. (Later fully backed up + **unplugged**.) |
| `images/*.img` | 6.2 GB written, sparse, gitignored. Manifest **not yet generated** (`rescue-unvr-usb.py manifest`) |
| Firmware 5.1.25 | Downloaded, sha256-verified, container fully decoded, all CRCs valid |
| Kernel / config / initramfs | Extracted to `tmp/sections/`, read |
| U-Boot | Extracted; netboot path (`bootcmdtftp` + `bootunsign`) identified. (Later **exercised**; then custom 6.12/6.18/7.1 kernels netbooted.) |
| Serial console | Working. CP2102 by-id, 115200 8N1, tio + socket via `./dev.py console` |
| Device | 2026-08-15: at the initramfs BusyBox shell, ICMP-only. **Now runs Fedora 44 standalone** (NAND kernel + SSD rootfs) — [project-status.md](project-status.md) |
| Replacement stick | Selected (SanDisk Ultra), dry run passed. (Superseded — USB removed; Fedora rootfs is on the SATA SSD.) |
| MTD dumps | 2026-08-15 none taken. **Later taken** + identity preserved — [nand-1.3.35.md](nand-1.3.35.md), [nor-boot-chain.md](nor-boot-chain.md) §6. |
| Git | Repo initialised. (Now under active version control.) |

## 14. Verify the rescue covered the data that matters

**What.** Decide whether the rescue can stop before 100 %, with evidence.

**Why it needs a script.** ddrescue writes a **sparse** image, so an un-rescued
region reads back as **zeros, not an I/O error**. Mounting the partial image and
finding every file readable proves nothing — a file in a gap returns zeros
silently. The only sound test is set arithmetic: ext4's allocation bitmap minus
the rescued ranges.

**Command.**
```
./scripts/verify-rescue-coverage.py            # exit 0 = safe to stop
./scripts/verify-rescue-coverage.py --domain   # also emit a domain mapfile
```

**Result.** Of the 7.4 GiB device only **871.4 MiB is allocated**, in 793 ranges.
Those ranges are **not** confined to the start — gaps were found at `0xc1820000`
(~3.1 GB), `0x10c000000` (~4.5 GB) and `0x188000000` (~6.5 GB), so "the rest is
empty space" is not safe to assume without checking.

Final: **COMPLETE — every allocated block rescued, 0 unrecovered bytes.**

**Gotcha — the sequential pass wastes ~30 minutes.** The last outstanding ranges
were error regions *behind* the read head, which pass 1 (`-n`) skips and only
pass 2 revisits. So finishing meant grinding through 1.4 GB of free space first.
`--domain` writes a ddrescue domain mapfile of just the allocated ranges;
`ddrescue --domain-mapfile=… -r3` then goes straight at them:

```
sudo ddrescue --idirect -b 512 -v -r3 --timeout=300 \
     --domain-mapfile=images/<stem>.domain.map \
     /dev/disk/by-id/usb-_UF3_8GB_<serial>-0:0 images/<stem>.img images/<stem>.map
```

Took **31 s** and recovered the remaining 416 KiB. Those blocks came back on
retry, which corroborates the original `dd` failure being **request-size
related, not dead NAND** — the stick is degraded, not dying.

**Mapfile parser gotcha.** Two 3-field line shapes exist. The status line is
`<current_pos> <status_char> <pass>`; only block lines have hex in *both* the
first and second field. Match on that or the parser dies on `?`.

---

## What's next

1. ~~Finish the rescue~~ — **done**, verified complete. Still owed:
   `sudo ./scripts/rescue-unvr-usb.py manifest` for the sha256 and
   `images/README.md`.
2. **Mount the image read-only** and confirm what USERDEV actually held:
   `losetup --find --show --read-only`, then `mount -o ro,noload`. `noload` is
   mandatory — the journal says `needs_recovery` and a plain mount rewrites it.
   Also check for `/dev/md1`: `ui-boot-nand` can promote USERDEV to an mdadm RAID1
   across the HDDs, so the USB may not be the only copy of config.
3. **Preserve identity before any write.** `Factory` (mtd8) and `EEPROM` (mtd9)
   hold MAC, board ID, HW rev and device ID — downloadable from nowhere.
   `dump-unvr-mtd.py` needs SSH; until userspace boots, the alternative is
   `cat /dev/mtd8` over the console from the initramfs shell. **Restore with
   `flashcp`, never `dd`** — MTD needs an erase cycle first.
4. **Format the replacement stick** (`prep-userdev.py --yes`), fit it, power on,
   allow up to 30 min for self-population. HDDs out as a precaution. That should
   restore a working stock box.
5. **Then, and only then, bring-up:** Esc-Esc into U-Boot, TFTP the **stock**
   `tmp/sections/02-kernel.bin` back to prove `bootcmdtftp` end to end with zero
   writes, before any custom kernel.
6. **Custom kernel** from `linux-alpine-v2` (6.12 LTS, `unvr_defconfig`,
   `alpine-v2-ubnt-unvr.dts`) — details and the al_eth source decision in
   [porting-reference.md](porting-reference.md).

**Open, unresolved:**

- Whether the recovery kernel in SPI-NOR (mtd10) still works as the safety net —
  reset held ~10 s at power-on, telnet `ubnt:ubnt` 📄. Untested here.
- The unit runs 4.1.37 while every published guide targets 2.3.14+ or 5.x.
  Numbering, paths and `/dev/boot*` assumptions all need re-checking on the box.
- linux-alpine-v2 issue #1: AHCI port 2 fails to link up on warm reboot, both
  controllers ✅ — unresolved upstream, and it hits a 4-bay NAS directly.

---

## Since then — milestones (2026-08-16 → 17)

The cold-start narrative above (2026-08-15) is history; the box has moved well past it.
Live state: [project-status.md](project-status.md). What landed:

- **Netboot proven, then kernel port** — `bootcmdtftp` + `bootunsign` exercised; the
  stock uImage booted, then **6.12.103 → 6.18.44 LTS → 7.1.8** all netboot-verified
  full-platform. [linux-71-build.md](linux-71-build.md), [porting-roadmap.md](porting-roadmap.md).
- **Fedora 44 boots standalone on woomera** — NAND kernel (@`0x1300000`) + SSD rootfs,
  no host / netboot / UEFI / GRUB / dracut (U-Boot can't read SATA).
  [fedora-on-ssd.md](fedora-on-ssd.md). #40 closed.
- **Boot chain fully reversed** — canonical [nor-boot-chain.md](nor-boot-chain.md);
  [bootloader.md](bootloader.md), [preboot-decompile.md](preboot-decompile.md).
- **MTD dumps taken + identity preserved** — [nand-1.3.35.md](nand-1.3.35.md),
  [nor-boot-chain.md](nor-boot-chain.md) §6 (over serial/TFTP, no SSH needed).
- **Hardware fully catalogued** — [components.md](components.md) (130-photo master BOM),
  [rps-subsystem.md](rps-subsystem.md) (RPS populated; ttyS2=RPS UART, not BT),
  [gpio-switches-leds.md](gpio-switches-leds.md); I2C/SPI scan; JTAG-candidate header lead.
- **`al_reboot` SP805 restart driver** written (untested, #51) —
  [reboot-driver-handover.md](reboot-driver-handover.md).

The **What's next** list above is largely done or superseded — track live work in
[project-status.md](project-status.md).
