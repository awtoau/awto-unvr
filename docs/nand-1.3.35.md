# NAND / SPI-NOR layout — UNVR sysid 0xea16, firmware 1.3.35

All ✅ read off the live unit on 2026-08-15 and md5-verified against the device.
Dumps in `images/mtd/UNVR-<serial>-sysidea16-<stamp>/`, captured by
`scripts/dump-unvr-mtd.py`, decoded by `scripts/analyse-mtd.py`.

**This is the 1.3.35 generation, not 5.1.25.** The numbering and contents differ
between firmware generations — do not apply this map to a 5.x unit. See
[boot-flow.md](boot-flow.md) for the 5.1.25 initramfs, which expects
`KERNELDEV=/dev/mtd9` and `BOOTDEV=/dev/mtd10`; on this unit `mtd9` is the
**EEPROM** and `mtd10` the **recovery kernel**.

## Unit

| Field | Value |
|---|---|
| `board.sysid` | `0xea16` — "UNVR without eMMC" |
| `board.name` | Protect Network Video Recorder |
| `board.serialno` | `74acb941a811` |
| `board.bom` | `113-02832-29` |
| `board.hwrev` | `0xb101d` |
| `board.uuid` | `810d6686-46ac-5d49-9aa5-428a04c10400` |
| Firmware | `UNVR4.arm64.v1.3.35.0d589c7.201216.1801` (from `mtd11:/version`) |
| Kernel | `4.1.37-ubnt #2 SMP Wed Dec 16 18:18:20 CST 2020` |

**The only published UNVR GPL drop matches this exact firmware** —
`NeccoNeko/UBNT-source-code` → `UNVR-1.3.35-GPL/`, containing
`linux-arm64-unvr-4.1.37-ubnt/` and the full Alpine V2 U-Boot tree. Kernel *and*
bootloader source for precisely what is running. See [sources.md](sources.md).

Platform string changed between generations: `UNVR4.**arm64**` at 1.3.35 vs
`UNVR4.**al324**` at 5.1.25.

## Partition map

Two chips, told apart by erase size: **NAND = 256 KB erase**, **SPI-NOR = 4 KB**.

| # | Name | Bytes | Erase | Chip | Content (verified) |
|---|---|---|---|---|---|
| 0 | `al_boot` | 2097152 | 256K | NAND | **100% 0xFF — ERASED, unused** |
| 1 | `device_tree` | 1048576 | 256K | NAND | **100% 0xFF — ERASED, unused** |
| 2 | `linux_kernel` | 16777216 | 256K | NAND | main kernel |
| 3 | `rootfs` | 1052770304 | 256K | NAND | root filesystem, 1004 MB |
| 4 | `chike` | 1048576 | 256K | NAND | **JPEG**, 2095×1692, Exif |
| 5 | `u-boot` | 1835008 | 4K | SPI-NOR | bootloader |
| 6 | `u-boot env` | 65536 | 4K | SPI-NOR | **100% zeroed** |
| 7 | `u-boot env redundant` | 65536 | 4K | SPI-NOR | **100% zeroed**, md5 identical to mtd6 |
| 8 | `Factory` | 65536 | 4K | SPI-NOR | **100% 0xFF — never written** |
| 9 | `EEPROM` | 65536 | 4K | SPI-NOR | identity block, decoded below |
| 10 | `recovery kernel` | 16777216 | 4K | SPI-NOR | unbrick path |
| 11 | `config` | 14675968 | 4K | SPI-NOR | **ext4**, UUID `e98263ea-…` |
| 12 | `cksum` | 4096 | 4K | SPI-NOR | **100% zeroed** |

> **Our current use (Fedora) — the table above is the vendor 1.3.35 baseline.**
> We repurposed the **`rootfs` region (mtd3)**: `scripts/flash-nand.py` writes our
> **Fedora kernel @`0x1300000` + DTB @`0x2800000`** into it. The vendor `linux_kernel`
> (mtd2 @`0x300000`) is left **intact as recovery**, and Fedora's real rootfs is on the
> **SSD** (root=PARTUUID). See [fedora-on-ssd.md](fedora-on-ssd.md); MTD redesign #30, #54.

## EEPROM (mtd9) — layout confirmed on real hardware

The layout published in linux-alpine-v2's reverse-engineering notes is **correct
on this unit**:

| Offset | Size | Field | Value |
|---|---|---|---|
| `0x0000` | 6 | base MAC | `74:ac:b9:41:a8:11` |
| `0x000C` | 2 | board ID | `0xea16` |
| `0x000E` | 2 | hardware revision | `0x0777` |
| `0x0010` | 4 | device ID | `0x000b101d` |
| `0x8000` | 4 | magic | `UBNT` — redundant copy present |

Note `ubnt-tools id` reports `board.hwrev=0xb101d`, which is the **device ID**
field at `0x0010`, not the `0x000E` field. The `0x000E` value (`0x0777`) is not
surfaced by `ubnt-tools`.

Beyond the header the partition is 23.7% `0xFF` / 56.4% `0x00` with content out
to offset 58160 — more than the identity block alone. Not yet identified.

## `Factory` (mtd8) is blank

100% `0xFF` — erased, never written. All identity lives in the EEPROM. Do not
assume this partition carries anything on this generation.

## U-Boot environment is ZEROED — running on compiled-in defaults

`mtd6` and `mtd7` are **100% `0x00`**, and both have the same md5. That is not
erased flash: erased NOR reads `0xFF`, so these were **actively written with
nulls**. U-Boot therefore ignores them (CRC fails) and falls back entirely to
its built-in defaults.

Zeroing both env copies is precisely what UNVR-NAS's uninstall procedure does
(`dd if=/dev/zero of=/dev/mtd1`, `of=/dev/mtd2`). Whether that was run on this
unit or it is a factory state is **not established**.

**Consequence:** any `setenv`/`saveenv` guidance written for a unit with a live
env does not apply as-is here — there is nothing saved to override.

## Default environment, extracted from mtd5

This is the **1.3.35 / on-device (OLD)** env. Full OLD-vs-NEW-5.1.25 reconciliation:
[nor-boot-chain.md](nor-boot-chain.md) §4; env deltas in
[uboot-update-path.md](uboot-update-path.md) §Env.

```
bootdelay        = 2
loadaddr         = 0x08000000
loadaddr_payload = 0x08000004
loadaddr_dt      = 0x04078000
loadaddr_rootfs_chk = 0x07000000
spi_pt_addr_kernel  = 0x00200000

bootcmd          = cp.b $fdtaddr $loadaddr_dt 7ffc; fdt addr $loadaddr_dt;
                   run loadbootargs; run multiboot
bootcmdrecovery  = setenv bootargsextra $bootargsextra boot=recovery; run bootcmdspi
loadbootargs     = setenv bootargs pci=pcie_bus_perf console=ttyS0,115200
                   sysid=$sysid ubnthal.sysid=$sysid $bootargsextra; printenv bootargs
multiboot        = run $bootfrom; echo "Failed to boot from emmc";
                   run bootspi;   echo "Failed to boot from spi"
bootspi          = lcd_print "Loading OS from spi..."; sf probe;
                   sf read $loadaddr $spi_pt_addr_kernel 4; setenvmem filesize $loadaddr;
                   incenv filesize 4; sf read $loadaddr $spi_pt_addr_kernel $filesize;
                   bootm $loadaddr_payload - $fdtaddr; ...
bootemmc         = lcd_print "Loading OS from emmc ..."; usb start;
                   ext4load usb 0 $loadaddr_payload uImage;
                   bootm $loadaddr_payload - $fdtaddr; lcd_print Failed!
```

### There is NO signature verification on this generation ✅

Both boot paths end in:

```
bootm $loadaddr_payload - $fdtaddr
```

That is the **plain unsigned** form — the same command 5.1.25 names `bootunsign`.
This bootloader contains **no `bootsign`, no `dobootm`, no FIT
`#$model@$fit_index`, and no verification step at all**. It boots arbitrary
unsigned uImages unconditionally. (Generation delta: 5.1.25's U-Boot *adds*
`bootsign`/`dobootm` but still defaults to unsigned. Full per-stage trust model:
[nor-boot-chain.md](nor-boot-chain.md) §5.)

And a firmware upgrade **does not flash U-Boot** — `upgrade_firmware()` calls
only `upgrade_kernel` and `upgrade_rootfs` — so upgrading cannot introduce
signing on this unit. The bootloader is only replaced by writing `mtd5` directly.

### CORRECTION: this unit boots `bootnand`, NOT `bootemmc`

An earlier revision of this document claimed you could drop a `uImage` on the
USB stick and it would boot. **That is wrong for sysid 0xea16.** Two independent
verification passes found the board table inside `mtd05`:

```
sysid ea16 -> UNVR         bootfrom='bootnand'   <-- THIS UNIT
sysid ea1a -> UNVR         bootfrom='bootemmc'
sysid ea20 -> UNVR-PRO     bootfrom='bootemmc'
sysid ea21 -> UNVR-AI      bootfrom='bootemmc'
sysid ea2c -> UDM-PRO-SE   bootfrom='bootdream'
sysid ffff -> UnknownBoard bootfrom='bootspi'
```

`$bootfrom` is **not** in the default environment — it comes from board code,
selected by sysid. On this unit `multiboot` runs **`bootnand`**, so `bootemmc`
(the `ext4load usb 0 … uImage` path) is never reached. Corroborated by `mtd02`
holding a live uImage whose banner matches the running kernel exactly, and
`mtd03` a squashfs built 12 s earlier.

The USB route would require `bootfrom=bootemmc` in a saved environment — which
this unit does not have, since `mtd6`/`mtd7` are zeroed.

`multiboot` tries `$bootfrom`, then falls back to `bootspi`. Recovery is
`bootcmdrecovery` → `bootcmdspi` → the **2019** recovery kernel at `mtd10`, on a
different chip from the NAND kernel, so it survives a bad NAND write. Note
`bootspi` is the routine fallback, not only an emergency path.

### TFTP is available, just not wired up

`tftpboot` is present in the binary, so netboot works interactively even though
this generation has no `bootcmdtftp` variable (5.1.25 adds one). Use
`scripts/tftpd.py` as the server.

## Transport notes — how these dumps were taken

No SSH exists on the device and no TCP port is open; commands go over the serial
console (tio socket) and bulk data over **TFTP**.

Do not use netcat. On this unit BusyBox `nc`:

- transfers **0 bytes** when a pipe reads `/dev/mtdN` — the MTD char device will
  not feed a pipe, though `dd if=/dev/mtdN | wc -c` reads it correctly
- **truncates a staged regular file at exactly 57351 of 65536 bytes**, the same
  figure on every retry, so not a race

TFTP is ACKed per block and cannot silently truncate. Every partition above was
verified by comparing `md5sum /dev/mtdN` on the device with the received file.

Also: `/dev/mtdN` must be staged to a regular file first (`dd ... of=/tmp/x`)
because tftp wants a plain file and the char device will not pipe.

## Restoring

**`flashcp`, never `dd`.** MTD needs an erase cycle first; `dd` to a char MTD
leaves a corrupt partition.

```
flashcp -v <file>.img /dev/mtdN
```
