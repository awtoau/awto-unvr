# U-Boot update path — how (and whether) U-Boot is ever rewritten

Answers the "5.1.25 ships a `uboot` section but the initramfs only writes kernel
and rootfs" question, and whether flashing Ubiquiti's newer U-Boot replaces the
`fdt rm /soc/nand-flash/partition@1` workaround.

Context, not restated: [bootloader.md](bootloader.md) (TOC, multi-DT, U-Boot RE),
[nand-1.3.35.md](nand-1.3.35.md) (partition map, env), [boot-flow.md](boot-flow.md),
[upgrade-path.md](upgrade-path.md), [firmware-5.1.25.md](firmware-5.1.25.md) §2
(exact flash sequence), [hardware.md](hardware.md).

Tools written for this: `scripts/carve-dtb.py`, `scripts/parse-al-toc.py`,
`scripts/xref-strings.py`. Logs in `tmp/logs/`, carved blobs in
`tmp/dtb-5.1.25/` and `tmp/dtb-current/`.

## 1. THE ANSWER — 5.1.25's ea16 DTB drops `chike` and `cksum`, not `device_tree`

`device_tree` is **still there**. The mtd renumbering Ubiquiti relied on comes
from deleting one **SPI** partition (`cksum`) and one **NAND** partition
(`chike`).

| | this unit (U-Boot 2020-12-16) | 5.1.25 U-Boot (2026-07-09) |
|---|---|---|
| **SPI (`spiflash@0`)** | **8** partitions | **7** partitions |
| mtd0 | u-boot 0x000000 +0x1c0000 | u-boot 0x000000 +0x1c0000 |
| mtd1 | u-boot env 0x1c0000 +0x10000 | same |
| mtd2 | u-boot env redundant 0x1d0000 +0x10000 | same |
| mtd3 | Factory 0x1e0000 +0x10000 | same |
| mtd4 | EEPROM 0x1f0000 +0x10000 | same |
| mtd5 | recovery kernel 0x200000 +0x1000000 | same |
| mtd6 | config 0x1200000 +**0xdff000** | config 0x1200000 +**0xe00000** |
| mtd7 | **cksum 0x1fff000 +0x1000** | **— removed** |
| **NAND (`nand-flash`)** | **5** partitions | **4** partitions |
| | mtd8 al_boot 0x0 +0x200000 | mtd**7** al_boot 0x0 +0x200000 |
| | mtd9 **device_tree** 0x200000 +0x100000 | mtd**8** **device_tree** 0x200000 +0x100000 |
| | mtd10 linux_kernel 0x300000 +0x1000000 | mtd**9** linux_kernel 0x300000 +0x1000000 |
| | mtd11 rootfs 0x1300000 +0x3ec00000 (1004 MB) | mtd**10** rootfs 0x1300000 +0x3ed00000 (1005 MB) |
| | mtd12 **chike** 0x3ff00000 +0x100000 | **— removed** (absorbed into rootfs) |

- 5.1.25 / 2.3.14 hardcode `KERNELDEV=/dev/mtd9`, `BOOTDEV=/dev/mtd10`
  (`initramfs-0/scripts/ui-boot-nand`) — **exactly** what the new table yields.
- `BOOTDEV` is used by `mount_rootfs()`, not just by the upgrader. The mtd number
  is a **boot** dependency, not only an upgrade one.
- Numbering holds because SPI registers first: `CONFIG_MTD_M25P80=y` +
  `CONFIG_MTD_SPI_NOR=y` vs `CONFIG_MTD_NAND_AL=m` (`tmp/sections/kernel.config`).
- `chike` has **zero** users in 5.1.25 (initramfs, rootfs, updater — all clean).
  `cksum` likewise; the only `cksum*` hits are the `cksum_dd`/`cksum_fwextract`
  sha256 helpers in `ui-helper-functions`.

### Which DTB is ours — settled two ways

sysid 0xea16 → multi-DT index **0** = TOC object `dt` = board-cfg
`alpine_v2_ubnt one nas v5.0`.

- Old U-Boot decodes it in a switch at file 0xa318c (`mtd00` image):
  `ea16→0, ea20→1, ea21→2, ea1a→3, ea2c→5`, stored as byte 0 of the board struct
  at file offset 0x149648, whose sysid halfword sits at +2. (Offsets are into the
  `mtd00` image; the U-Boot payload starts at 0xa1048 and loads at 0x1100000.)
- Independent: in **both** builds `dt_bt` (index 3) carries
  `nand-flash { status = "disabled" }` — it is the eMMC variant. It is the only
  other DTB with a `chike`/1004 MB layout, so it cannot be us. `dt_pro`, `dt_ai`,
  `dt_hd` are all `ubifs` 486 MB.
- 5.1.25's U-Boot board table (file 0x139340, stride 0x70, 13 entries) gives
  `0xea16 → "UNVR", bootfrom = bootnand` (0xea1a → `"UNVR", bootemmc`), which
  again rules out the nand-disabled DTB.

### TOC correction

`parse-al-toc.py` decodes the entry as `id[4] type[4] name[12] off[4] size[4]
rsvd[4]`. That resolves the `preboot` row flagged open in
[bootloader.md](bootloader.md#open): it is **off 0x000000 size 0x080000**, not
"off 0x80000 size 0" — so it does contain the payload at 0x21000.

| object | this unit | 5.1.25 |
|---|---|---|
| preboot | 0x000000 +0x080000 | 0x000000 +0x080000 |
| dt (v5.0) | 0x081000 +0x7000 | 0x081000 +0x7000 |
| dt_pro | 0x088000 +0x8000 | 0x088000 +0x8000 |
| dt_ai | 0x090000 +0x8000 | 0x090000 +0x8000 |
| dt_bt | 0x098000 +0x9000 | 0x098000 +0xa000 |
| **dt_hd** ("one nas hd") | — | **0x0a2000 +0xa000** (new, index 4) |
| uboot | 0x0a1000 +0x11f000 | 0x0ac000 +0x114000 |
| uboot-env | 0x1c0000 +0x10000 | same |
| uboot-re | 0x1d0000 +0x230000 | same |

`tmp/sections/01-uboot.bin` is 1,395,740 B (0x154D9C) — a **raw SPI image for
offset 0**, ending exactly at the end of the U-Boot payload. Everything it
covers lies inside the `u-boot` mtd0 window (0..0x1c0000); env and everything
above are untouched by writing it at offset 0.

## 2. Nothing in 5.1.25 writes U-Boot on this board

| path | verdict |
|---|---|
| `initramfs-0/scripts/ui-boot-firmware` | `fwupdate -dc` = **check only**; then kernel + rootfs via `nandwrite`. No `uboot`. |
| `initramfs-0/scripts/*` | zero occurrences of `uboot`/`u-boot` anywhere. |
| `ubnt-systool fwupdate <file>` | `fwupdate -c` (check), `pre-fwupdate` hooks, `fwextract -t updater`, updater `-t top`, `fw_move` → `/boot/fwupdate.bin`, updater `-t bottom -i`, reboot. No MTD write. |
| `04-updater.bin` (uof-updater 1.0.36) | string `uboot` count = **0**. Its recovery-kernel / DEVCFG / QCA-preloader paths are eMMC- and Qualcomm-product code. |
| `usr/lib/ubnt/hooks/system/upgrade-{top,bottom}` | LED, LCM, dpkg status, a curl notify. Nothing flash-related. |
| rootfs at large | only two files contain "uboot" — a udev hwdb and an SPDX list. |

### `fwupdate` *does* have a writer — and it is a no-op on ea16

`ubnt-tools`'s `fwupdate` applet (getopt `b:cdhr`; `-c` = check-only counter):

- With `-c` absent it walks each `PART` section (magic `"PART"`, 0x48-byte
  headers) and writes it to a device: `open(dev, O_RDWR|O_SYNC)`, `ioctl
  BLKROSET 0` to clear read-only if it is a block device, block-diff, write,
  `BLKROSET 1`.
- Target device comes from the static board table (file 0x20c48+, stride 552,
  sysid u32 at +108, names XOR-obfuscated with key `81 93 E0 C4`):
  **`+0xC0` = device used only when the section is named `uboot`;
  `+0x100` = device for every other section.**
- UDM `/dev/sda1` `/dev/sda2`, UDM-Pro `/dev/sdb1` `/dev/sdb2`, CloudKey
  `/dev/mmcblk0`. **sysid 0xea16 (`UNVR`): both fields are empty** (verified in
  the raw bytes at 0x22e18 / 0x22e58).
- ⇒ on this board `fwupdate` without `-c` would `open("")` and fail. There is no
  UNVR U-Boot write path in the shipped software at all.

**Conclusion:** the `uboot` section exists so one container can serve every
al324/Qualcomm product. On a UNVR it is dead weight. U-Boot on this unit has
never been rewritten by any firmware upgrade and never will be.

## 3. Flashing 5.1.25's U-Boot — procedure

Ingredients already in-repo: `scripts/tftpd.py`, `scripts/unvr-console.py`,
`tmp/sections/01-uboot.bin`, backup
`images/mtd/…-203030-on-2.3.14-pre-uboot-env/…-mtd00-u-boot-1835008B-….img`.

Preconditions:

- Unit already on **≥ 2.3.14**. See the one-way-door warning in §4.
- mtd0 backup verified byte-for-byte, and stored off the device.
- Know the SPI-NOR package/location on the PCB — it is the only recovery.
- Serial console up, HDDs may stay in.

Do it from the **U-Boot prompt**, not Linux: U-Boot relocates itself to DRAM at
start-up, so its own flash can be erased and rewritten repeatedly from the live
prompt. The only fatal window is a power loss between erase and a verified write.

```
# host
cp tmp/sections/01-uboot.bin tmp/tftp/boot.img
python3 scripts/tftpd.py --root tmp/tftp

# U-Boot
setenv ipaddr <unit>; setenv serverip <host>
tftpboot ${loadaddr} boot.img          # expect filesize = 154d9c
crc32 ${loadaddr} ${filesize}          # compare against host crc32 BEFORE erasing
sf probe
sf erase 0 0x1c0000                    # whole u-boot partition; stops at env
sf write ${loadaddr} 0 ${filesize}
sf read 0x0a000000 0 ${filesize}       # read back to a scratch address
cmp.b ${loadaddr} 0x0a000000 ${filesize}   # MUST report all bytes the same
reset
```

`crc32`, `cmp` and `sf read/erase/write` are all present in both builds
(verified in the binaries); `cmp` reports `Total of %ld byte(s) were the same`.

- `sf erase 0 0x1c0000` rather than `+${filesize}` so no stale tail remains —
  the 5.1.25 `dt_hd` object at 0x0a2000 overlaps where the old `uboot` object
  started (0x0a1000), so a partial write would leave a mixed image.
- If `cmp.b` fails, **do not reset** — re-run `sf erase`/`sf write`/`cmp.b`.
- Ubiquiti's own built-in `run bootupd` does the same thing (tftp `boot.img`,
  `sf erase 0 +${filesize}`, `sf write … 0 …`) but without the read-back
  compare. Prefer the explicit sequence.

### Env

- Both env copies (mtd1, mtd2) are currently **all zeros**. Their stored CRC
  (0x00000000) cannot match `crc32(zeros[65532]) = 0x510b66bf`, so U-Boot is
  already running on its **compiled-in default env**. Nothing to preserve.
- The write stops at 0x1c0000, so mtd1/mtd2 are physically untouched — but the
  *effective* env changes because the built-in defaults change:

| var | this unit | 5.1.25 |
|---|---|---|
| `bootcmd` | `cp.b $fdtaddr $loadaddr_dt 7ffc; fdt addr $loadaddr_dt; run loadbootargs; run multiboot` | `run load_fdt; run loadbootargs; run multiboot` (`load_fdt` is the same `cp.b`+`fdt addr`) |
| `bootargs` | `… sysid=$sysid ubnthal.sysid=$sysid $bootargsextra` | `… sysid=$sysid boot_carrier=$boot_carrier root=$rootfs $bootargsextra` — **`ubnthal.sysid=` dropped** |
| `multiboot` | `run $bootfrom; run bootspi` | `run $bootfrom; run ${bootfrom}_bkp; run bootemmcblk; run bootspi` |
| `bootnand` | inline nand read + `bootm` | `run loadimg_nand; run prebootm; run dobootm` |
| signed boot | absent | `bootsign=bootm $loadaddr_payload#$model@$fit_index` exists, but `dobootm=run bootunsign` ⇒ **unsigned boot is still the default** |

- `bootfrom` for 0xea16 is set from the U-Boot board table to **`bootnand`**.
- `bootdelay=2` unchanged; `delenv` script still available to wipe stored env.

### Recovery if it fails

- The SoC boot ROM loads stage-2 from **SPI offset 0**. There is no second copy.
- Reset-hold / `bootspi` boots the *recovery kernel* at SPI 0x200000 — it needs a
  working U-Boot, so it does **not** protect against a bad U-Boot write.
- ⇒ the only recovery from a bricked mtd0 is an **external SPI-NOR programmer**
  (SOIC clip + `flashrom`) restoring the saved `mtd00-u-boot-1835008B-*.img`.

## 4. Does it replace the `fdt rm` workaround? — yes, with caveats

**Yes.** The arithmetic is exact and comes from Ubiquiti's own table, not from a
guess: 7 SPI partitions (mtd0–6) + `al_boot`(7) + `device_tree`(8) +
`linux_kernel`(**9**) + `rootfs`(**10**). That is precisely what 2.3.14 and
5.1.25 hardcode. It is a permanent fix, unlike the volatile `fdt rm`.

Confidence, stated honestly:

- **High** that the ea16 DTB in 5.1.25's U-Boot yields mtd9/mtd10 — read
  directly out of the binary, both DTB and the SPI/NAND tables.
- **High** that ea16 selects that DTB — proven in the old build's switch, and by
  elimination in the new one (`bootfrom=bootnand` for ea16; the only other
  1004 MB-layout DTB has NAND disabled). The new preboot's `multi_dt` mapping was
  **not** disassembled; that is the one link taken by inference.
- **Untested end-to-end.** Nobody has booted this U-Boot on this unit.

Risks, in order of seriousness:

1. **Brick.** Single-copy bootloader, no software fallback. See §3.
2. **One-way door for old firmware.** 1.4.9's `ui-boot-nand` uses
   `KERNELDEV=/dev/mtd10`, `BOOTDEV=/dev/mtd11`. Under the new table mtd10 is
   *rootfs* and mtd11 does not exist ⇒ 1.4.9 can no longer mount its rootfs.
   **Do not flash while on 1.4.9**, and treat any downgrade below 2.3.14 as
   closed afterwards.
3. **`chike` and `cksum` become invisible to Linux** (absorbed into `rootfs` and
   `config`). No user found in 5.1.25 for either. `chike` content is still
   physically present at NAND 0x3ff00000 but now inside the rootfs partition —
   dump it before flashing if it is wanted.
4. **Board-support drift if old firmware is kept.** The new ea16 DTB also
   changes: GPIO phandles reshuffled, HDD activity LEDs re-gpio'd with a new
   `slot-no` property, `hdd_pwrctl` reworked, reset button moved to a different
   GPIO controller, `ramoops_region` 0x92000000 → 0x12000000, an
   `annapurna-labs,al-eth` compatible added, and a new `ubnthal-wp` node
   (SPI default `rw`, `Factory` and `EEPROM` forced `ro`). A 2.3.14 userland on
   this DTB may mis-drive disk LEDs/power. Untested.
5. **`ubnthal.sysid=` is no longer on the kernel command line.** `ubnthal.ko`
   reads identity from the EEPROM anyway ([firmware-5.1.25.md](firmware-5.1.25.md)),
   so this should be benign — not verified on old firmware.

If the goal is only "run 5.1.25", the `fdt rm` workaround plus the ladder in
[upgrade-path.md](upgrade-path.md) reaches the same place with **no brick risk**.
Flashing buys permanence, not capability.
