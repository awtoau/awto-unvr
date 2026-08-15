# Firmware 5.1.25 — unpack, upgrade logic, binaries

Analysis of `sources/UNVR-5.1.25.bin` (`UNVR4.al324.v5.1.25.84c48e7.260710.1602`,
786253430 B, `ENDS`-signed, all CRCs OK). Newest UNVR release, built 2026-07-10.
Unpacked by `scripts/analyse-unvr-firmware.py --extract` → `tmp/sections/`.

Links, not restated here:
- Container format + section table + kernel uImage: [porting-reference.md](porting-reference.md#firmware-container-anatomy--decoded-and-verified)
- Kernel config (module sig, kexec, al_eth built-in): [porting-reference.md](porting-reference.md#stock-kernel-config--settles-the-big-questions)
- U-Boot env, unsigned-boot default, netboot: [boot-flow.md](boot-flow.md), [bootloader.md](bootloader.md)
- initramfs overlay/USERDEV/break= shells: [boot-flow.md](boot-flow.md)
- Licence + toolchain + GPL-compliance table: [licence-audit.md](licence-audit.md)

## 1. Sections — what each contains

Table in [porting-reference.md](porting-reference.md#sections-in-unvr-5125). Contents:

| # | Name | Payload | Content |
|---|---|---|---|
| 1 | `uboot` | 1.33 MB | Annapurna U-Boot `2015.07-alpine_db-2.21-HAL`, built 2026-07-09. SPL `al_boot v2.10.0` + main U-Boot. GCC 10.2.1. Flashed to SPI-NOR `mtd5` NOT by the updater. |
| 2 | `kernel` | 13.20 MB | legacy uImage → gzip arm64 Image `4.19.152-alpine-unvr`, load/entry `0x04080000`. Embeds `IKCFG` config + 17.5 MB gzip newc-cpio **initramfs** (`CONFIG_INITRAMFS_SOURCE`). |
| 3 | `rootfs` | 730.86 MB | **SquashFS 4.0, zstd**, 262144 block, 63607 inodes, built 2026-07-10 18:10:55. Debian 11 bullseye (`11.11`) aarch64. `unsquashfs`-able directly. |
| 4 | `updater` | 4.44 MB | **Go static aarch64 ELF, stripped** — `github.com/ubiquiti/uof-updater` **v1.0.36~43+g15a16dd** (base `0x3d400000`, in-RAM). Orchestrates upgrade: calls `fwextract`, version-compare, USERDEV/RAID migration (`AutoUNVR4USBMigrate`, `/dev/md1`→`/mnt/.rwfs`), stops unifi/mongodb, `fsck.ext4`, then `/bin/sleep 10; /sbin/reboot`. Not persisted to MTD. |
| — | `ENDS` | 256 B | RSA signature over the whole image. Repack ⇒ invalid; cannot re-sign. |

initramfs build path (`CONFIG_INITRAMFS_SOURCE`):
`/home/dio/jenkins_slave/workspace/irmware.debbox_unifi-nvr_v5.1.25/unvr4.alpine-2/...`
Extracted trees: `tmp/sections/initramfs-0/`, `tmp/sections/rootfs/`.

## 2. Upgrade mechanism (5.1.25) — the definitive path  ⭐

Source: `tmp/sections/initramfs-0/scripts/{ubnt,ui-boot-firmware,ui-boot-nand}`.
Supersedes the 1.3.35 `scripts/ubnt.nand` (`fwsplit`/`nandwrite`,
`FIRMWARE_PUB_KEY=/etc/ssl/unas.pub`). What changed 1.3.35 → 5.1.25:

| | 1.3.35 | **5.1.25** |
|---|---|---|
| splitter tool | `fwsplit` | **`fwextract`** (both are `ubnt-tools` symlinks) |
| fallback pubkey | `/etc/ssl/unas.pub` | **`/etc/ssl/fw.pub`** |
| kernel target | (gen-specific) | **`KERNELDEV=/dev/mtd9`** |
| rootfs target | (gen-specific) | **`BOOTDEV=/dev/mtd10`** |

### Staging path (drop-in, no CLI)
- `FIRMWARE_FILE="fw-image.bin"`, `MNT_RWFS=/mnt/.rwfs` (USERDEV mount).
- `FIRMWARE_BIN=${MNT_RWFS}/upgrade/fw-image.bin` → **`upgrade/fw-image.bin` on the USERDEV** (`/dev/sdq` USB stick, or `/dev/md1` HDD RAID1 on ea16).
- `mount_premount()` (scripts/ubnt): if that file exists → `upgrade_firmware`, then delete it, `touch /config/.upgrade-bootup`, reboot.
- Also triggered by `/config` flags `.factory-reset`, `.network-upgrade` (network fetch via tftp/nc/wget → same `upgrade_firmware`).

### Signing key path
- Primary: `ubnt-tools fwupdate -dc <file>` — RSA pubkey **compiled into `ubnt-tools`** (`d2i_PUBKEY` + `EVP_Verify*`, digest `EVP_sha1`). PEM `-----BEGIN PUBLIC KEY-----` frame present in the binary; key body base64 (`EVP_DecodeBlock`).
- Fallback: `-s /etc/ssl/fw.pub`. **`fw.pub` is NOT present** in initramfs or rootfs → fallback never engages; verification is the built-in key only.
- Failure string: `ERROR: Bad FW Image Signature`. A tampered `.bin` is rejected here AND by the container `ENDS` signature.

### Exact flash sequence (NAND variant, our ea16) — `ui-boot-nand`
```
check_firmware:  ubnt-tools fwupdate -dc  fw-image.bin      # signature gate
upgrade_kernel:                                             # -> KERNELDEV=/dev/mtd9
  sz=$(fwextract -Skt kernel fw-image.bin)                  # decompressed size
  hdr=<sz as 8-byte LE>                                     # size prefix
  flash_erase /dev/mtd9 0 0 -q
  (echo -ne $hdr; fwextract -kt kernel fw-image.bin) | nandwrite /dev/mtd9 -p -q
upgrade_rootfs:                                             # -> BOOTDEV=/dev/mtd10
  sz=$(fwextract -Skt rootfs fw-image.bin)
  pad=<sz right-padded to 4096 with \x00>                   # 4096-byte prefix
  flash_erase /dev/mtd10 0 0 -q
  (echo -ne $pad; fwextract -kt rootfs fw-image.bin) | nandwrite /dev/mtd10 -p -q
triple_sync
```
- `upgrade_firmware()` runs **kernel + rootfs only**. U-Boot (`mtd5`), env, device-tree are **untouched** ⇒ unsigned-boot behaviour survives every upgrade (see [bootloader.md](bootloader.md)).
- eMMC variant (`ui-boot-emmc`, other sysids) instead does `cksum_fwextract … | dd` to GPT `/dev/boot1`,`/dev/boot2`, sha256-checked.
- **Caveat (generation mismatch, already logged):** these `/dev/mtd9`,`/dev/mtd10` names are the initramfs's expectation. On the live 1.3.35 unit mtd9=EEPROM, mtd10=recovery kernel — see [nand-1.3.35.md](nand-1.3.35.md). The MTD renumbering is exactly why the ladder upgrade must be walked, not skipped.

## 3. Binary inventory + reverse-engineering

Rootfs: Debian 11 aarch64, 437 dpkg packages. Priority binaries:

| Binary | Type | Role |
|---|---|---|
| `/sbin/ubnt-tools` (166K, stripped, PIE) | multi-call | `fwupdate`/`fwsplit`/`fwinfo`/`fwextract`/`id` symlinks. FW verify + split. |
| `/lib/modules/4.19.152-alpine-unvr/extra/ubnthal.ko` (149K) | kmod, **not stripped** | board table / EEPROM / flash-WP. |
| `/sbin/ubnt-systool` (51K) | ELF + shell | reboot/poweroff/cputemp/reset2defaults/resetbutton. |
| `/usr/bin/ustorage` (342K, `ubntstorage`→) | ELF | disk/RAID/SMART. |
| `/usr/bin/ubntnas` (6.8 MB, `ubntnvr`→) | ELF | Protect NVR app (not RE'd — too large, low firmware value). |
| al_eth | **built into kernel** | no `.ko`; analysed via kernel strings (below). |

Tooling note: Ghidra IS installed (`~/.local/bin/analyzeHeadless`, OpenJDK 25) and
auto-analysis of all four succeeded (`tmp/ghidra_proj/`), but the decompiler-export
post-script hangs on project-reopen on this Ghidra+JDK25 build. Findings below are
from `nm`, `readelf`, `aarch64-linux-gnu-objdump -dr`, and `strings` — which for the
**not-stripped** `ubnthal.ko` give full symbol + disasm coverage. Symbols in
`tmp/logs/nm-ubnthal.txt`, disasm in `tmp/logs/objdump-ubnthal.txt`. (Fedora's
stock `objdump` is x86-only — "architecture UNKNOWN" on aarch64; use the
`aarch64-linux-gnu-` cross binutils.)

### ubnt-tools (firmware verify + flash)
- Links `libcrypto.so.1.1` (OpenSSL 1.1.1w), `libevent-2.1`, `libpam`, `libz`.
- Verify: `d2i_PUBKEY` → `EVP_VerifyInit/Update/Final` with `EVP_sha1`; base64 body via `EVP_DecodeBlock`. Compiled-in RSA pubkey (no external key needed).
- `fwextract -kt <part>` streams a named container section to stdout; `-Skt` prints decompressed size; `-dc` decrypt+check. Targets: `uboot`,`kernel`,`rootfs`,`updater`.
- Emits Protect board JSON: `{"board":{"name","shortName","subType","hwaddr","BOM":"113-%05d-%02d","uuid"},"firmware":{"version"}}`; REST hook `/api/fwupdate`; `UNASPRO` marker.

### ubnthal.ko (board table, EEPROM, flash write-protect)
- `modinfo`: `license=Proprietary`, vermagic `4.19.152-alpine-unvr SMP mod_unload aarch64`, GCC 10.2.1. **Has `.symtab` (not stripped) but NO `.debug_*` DWARF** — correcting the earlier "full DWARF" claim; symbol names only.
- Sources: `proc-alpine.c`, `proc-flash-wp.c`, `board.c`. Exposes procfs `/proc/ubnthal/system.info` (`device.anonid`, `boardid=%04x`, `boardrevision`, `cpuid=%08x`, `flashsize`, `ramsize`) + `board`, `factory`, controller-host/port, mesh/isolation flags.
- Reads identity from **EEPROM**: `alpine_scan_eeprom` → `alpine_raw_read_eepromdata`, decoded by `alpine_dump_boarddata`/`board_dump`. `chars2uint`, `cyg_crc32`/`cyg_ether_crc32`, `sha256_*` (own impl).
- Disasm (objdump): `alpine_scan_eeprom` allocs a **0x10000 (64 KB)** buffer, reads via `ubnt_mtd_partition_read`, and indexes the **redundant block at +0x8000** (`boardid` at base+0xC and +0x800C) — matches the hardware-verified EEPROM map in [nand-1.3.35.md](nand-1.3.35.md).
- `alpine_get_flashsize` returns a **hardcoded 0x2000000 (32 MB)**; `alpine_get_ramsize` reads RAM and rounds down to 1 GB (`& 0xffffffffc0000000`).
- Exports kernel symbols: `ubnthal_get_boardrevision`, `ubnthal_get_flashsize`, `ubnthal_get_ramsize`.
- **Flash write-protect gate:** `flash_wp_sw_init`/`_exit`, `proc_flash_wp_init`, `ubnt_flash_disable_protection`, `ubnt_blk_wp_callback` — SW write-protect on the boot flash, toggled via proc. Relevant to any direct-MTD reflash.
- sysid→board mapping is EEPROM-driven here; the static sysid table lives in U-Boot board code (see [nand-1.3.35.md](nand-1.3.35.md) for the ea16/ea1a/ea20/ea21/ea2c list).

### al_eth / al_mod_eth (SFP / 10G / link manager) — kernel built-in
Strings from `tmp/sections/kernel-Image` and `01-uboot.bin`. Driver = Annapurna
`al_eth` with newer `al_mod_*` symbol set + Link Manager (LM).
- **No third-party SFP vendor lock found.** No whitelist/deny strings; SFP i2c A0h EEPROM is read only to *pick a MAC mode*, not to gate vendor. An Intel optic is not blocked by identity.
- Link-flap cause is **mode forcing**, not a lock. Knobs present:
  - DT prop **`force-1000base-x`** → pins port to 1000BASE-X.
  - `link-training` enable/disable, `auto-neg`, `auto-fec`, `fec_enable` (`al_eth_lm_fec_config`).
  - SFP detect: `gpio-sfp-present` GPIO or i2c polling (`al_eth_sfp_detect`, `al_eth_qsfp_detect`); on i2c-read failure → `Force mode to default`.
  - Kernel `CONFIG_AL_ETH_FORCE_SFP_1G` **not set** (the compile-time 1G pin is off; forcing is via DT instead).
  - Runtime sysfs: `sfp_probe_1g`, `sfp_probe_10g` (`al_mod_eth_sfp_probe_enable_set`).
  - Retimer path: `al_mod_eth_lm_retimer_ds125` (TI DS125 retimer), SerDes 25G.
- U-Boot exposes interactive controls: `eth_1g_params_set <port> <type> <autoneg> [speed] [duplex]`, `eth_link_training_enable <port> <enable/disable>`, `al_eth_sfp_detect`. So SFP link params are tunable from the U-Boot prompt without a rebuild.
- **Actionable for the flapping Intel 10G optic:** the board forces 1000BASE-X with AN/LT/FEC off; a 10G optic in that port mismatches. Toggle via the `sfp_probe_*` sysfs or the DT `force-1000base-x`/`link-training` props; see the UDM 1.12.x `al_eth` with DT board-params + GPIO SFP control in [sources.md](sources.md#the-finding-a-newer-al_eth-exists-for-the-same-silicon).

### ubnt-systool / ustorage
- `ubnt-systool`: reads `/proc/ubnthal/system.info`; `do_reboot`/`do_poweroff`/`do_cputemp`/`do_reset2defaults`/`do_resetbutton`; writes reboot reason, `FORMAT_EXT_DEVS=true` to `/boot/reset2defaults`, dpkg-lock aware.
- `ustorage` (`→ /usr/sbin/ubntstorage`): disk enumeration, mdadm RAID, SMART. (Ghidra dump `tmp/logs/ghidra-ustorage.txt`.)

## 4. Fingerprint summary
Full table + licences in [licence-audit.md](licence-audit.md). Headline:
Debian 11 bullseye, glibc 2.31, GCC 10.2.1, OpenSSL 1.1.1w, zlib 1.2.11,
BusyBox 1.30.1, kernel 4.19.152-alpine-unvr, U-Boot 2015.07-alpine_db.
