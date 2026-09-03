# UNVR flash / MTD map (ea16)

Two flash chips, 12 MTD partitions. Stock 5.1.x layout, captured live
(`docs/hw-reference/20260816-104601/mtd.txt`, `dmesg.txt`, `live.dts`).

**MTD is raw flash, not a filesystem** — each partition is just an offset+length
byte-range on a chip (read / write / **erase-whole-sector**). Some hold a
filesystem (config = ext4, rootfs), most hold raw blobs or images.

## Who defines the layout (there is no on-media table)

The flash chips carry **no partition table** — no GPT/MBR, no ONFI table. The layout
is a pure software convention in the **Device Tree** (`fixed-partitions` nodes,
`partition@N { label=…; reg=<off size>; }`):
- **Ubiquiti's DT source is the authority.** 5.1.25 "deleted" chike/cksum by shipping
  a DTB with fewer partition nodes — nothing was erased, the nodes were just dropped.
- Boot chain: compiled DTB (multi-DT container) → **U-Boot selects by sysid**
  (`0xea16 → index 0`) → **kernel reads `fixed-partitions`** → `/dev/mtdN` (numbers =
  registration order; NOR probes before NAND).
- **How we know this layout:** live `/proc/mtd` + live DTB (`docs/hw-reference/.../
  live.dts`) + reversed U-Boot DTB-selection (`docs/uboot-update-path.md`).
- **We ship our own DTB/U-Boot**, so the layout — and our env offset — is ours to
  define. Nothing in hardware fixes it.

## Two devices — tell them apart by erase size

| | SPI-NOR | NAND |
|---|---|---|
| Part | Macronix **MX25U25635F** | Micron **MT29F8G08ABBCAH4** |
| Size | 32 MB (32768 KB) | 1024 MiB, SLC |
| Erase | **4 KB** (`0x1000`) | **256 KB** (`0x40000`) |
| Page / OOB | (NOR) | 4096 / 224 |
| Driver / node | `m25p80 spi0.0` | `al-nand` |
| Holds | boot + env + identity + config + recovery kernel | boot loader + kernel + rootfs |
| Partitions | mtd0–7 (8) | mtd8–11 (4) |

In `/proc/mtd` the **erasesize column is the giveaway**: `00001000` = NOR,
`00040000` = NAND.

## Partition table — all 12, both chips

NAND offsets **restart at 0x0** because it is a separate chip (the `chip` column,
not the offset, disambiguates). NOR fills 32 MB with no gaps; NAND has a 1 MB hole
before the kernel.

| mtd | name | chip | offset | size | human | content | in-repo | notes |
|---|---|---|---|---|---|---|---|---|
| mtd0 | u-boot | NOR | 0x000000 | 0x1c0000 | 1.75 MB | image | no | stock U-Boot 2015.07 |
| mtd1 | u-boot env | NOR | 0x1c0000 | 0x10000 | 64 KB | env blob | **yes** | key=val; **holds `eth1addr`** |
| mtd2 | u-boot env redundant | NOR | 0x1d0000 | 0x10000 | 64 KB | env blob | **yes** | backup env |
| mtd3 | Factory | NOR | 0x1e0000 | 0x10000 | 64 KB | raw | **yes** | **awto-uboot env lives here** (`CONFIG_ENV_OFFSET=0x1E0000`, #158). Linux flags it read-only (`0x800`) but U-Boot's `sf` driver writes it regardless - `strings /dev/mtd3ro` shows a saved `bootargs=`/`bootcmd=`. **A saved env overrides the compiled `CONFIG_BOOTARGS`/`CONFIG_BOOTCOMMAND`**: after changing either, run `env default -a; saveenv` at `awto-nas#` or the old values silently win (#216). |
| mtd4 | **EEPROM** | NOR | 0x1f0000 | 0x10000 | 64 KB | raw blob | **yes** | **identity** — see below; MAC source (#89) |
| mtd5 | recovery kernel | NOR | 0x200000 | 0x1000000 | 16 MB | image | no | fallback kernel (on NOR) |
| mtd6 | **config** | NOR | 0x1200000 | 0xdff000 | ~14 MB | **ext4** | **yes** | mounted `/tmp/.config`; persistent settings |
| mtd7 | cksum | NOR | 0x1fff000 | 0x1000 | 4 KB | raw | **yes** | **all-`0x00`, inert** — not a live checksum |
| mtd8 | al_boot | NAND | 0x000000 | 0x200000 | 2 MB | **erased (0xFF)** | no | empty on this unit — real al_boot/S2 is in the **NOR** preboot region (0x0–0x80000), not here |
| — | *(hole)* | NAND | 0x200000 | 0x100000 | 1 MB | unpart. | — | reserved gap before kernel |
| mtd9 | linux_kernel | NAND | 0x300000 | 0x1000000 | 16 MB | image | no | stock main kernel |
| mtd10 | rootfs | NAND | 0x1300000 | 0x3ec00000 | ~1005 MB | filesystem | no | root FS (1004 MB, 10× GitHub limit) |
| mtd11 | chike | NAND | 0x3ff00000 | 0x100000 | 1 MB | raw | **yes** | UBNT-specific tail region |

NOR sum = `0x2000000` = 32 MB (no gaps). Board-id is read at flash `0x1f000c` =
mtd4 + 0x0C (`docs/nor-boot-chain.md`). `cksum` is 100 % zeros — protects nothing,
which is *why* repurposing mtd3 for our env is safe.

## What stock U-Boot uses at boot (and what's free)

Verified from the GPL source (`UBNT-source-code/.../board/annapurna-labs/alpine_ubnt/
board.c`) + the live boot log. Stock U-Boot 2015.07 touches ONLY these:

| Partition | Stock U-Boot | Why (source) |
|---|---|---|
| u-boot (mtd0) | **runs from** | preboot + TOC + stock U-Boot image itself |
| u-boot env (mtd1) | **reads/writes env** | `al_config_env_offset_get()`; bootcmd, eth1addr |
| u-boot env redundant (mtd2) | **redundant env** | env redundancy pair |
| EEPROM (mtd4) | **reads (identity)** | `eeprom_per_device_init()` (power), MAC, board config ID |
| recovery kernel (mtd5) | **loads on recovery** | `recovery_mem_read` + `AL_RECOVERY_OFFSET` (board.c:213/381) |
| linux_kernel (NAND) | **loads kernel** | normal boot |
| rootfs (NAND) | bootargs target only | `root=` — the kernel mounts it; U-Boot doesn't read it |

**FREE — NOT used by stock U-Boot (safe to repurpose):**
- **Factory (mtd3)** — blank `0xFF` → repurposed as OUR U-Boot env (#81).
- **config (mtd6)** — Linux ext4 (userspace `/tmp/.config`); U-Boot never touches it.
- **cksum (mtd7)** — inert, all-`0x00`.
- **al_boot (NAND)** — empty (the al_boot code is in the NOR).
- **device_tree (NAND)** — empty (the DTB comes from the NOR TOC / multi-DT).
- **chike (NAND)** — dead firmware selfie.

### mtd4 "EEPROM" = the identity blob (raw, no filesystem)
**Naming trap:** mtd4 is *not* a separate EEPROM chip — it is a 64 KB region of the
**NOR** flash (MX25U25635F @ 0x1f0000) that Ubiquiti *labelled* "EEPROM". The one
real EEPROM chip is the **24C64 at i2c `0x57`**, which holds the **DDR-config blob**,
not identity (see `docs/i2c-map.md`). The MAC is here, in NOR, reached over SPI —
the same path as the mtd3 env.

| off | sz | field | value |
|---|---|---|---|
| 0x0000 | 6 | base MAC | `74:ac:b9:41:a8:11` ← RJ45 port (al_eth 1c36:0001 = eth0) |
| 0x0006 | 6 | second MAC | `76:ac:b9:41:a8:11` |
| 0x000C | 2 | board id / sysid | `ea16` |
| 0x000E | 2 | hw rev | `0777` |
| 0x0010 | 4 | device id | `000b101d` |
| 0x8000 | — | `UBNT` structured record | redundant copy of the above |
| 0xd000 | — | ONIE `TlvInfo` | mfg date, BOM `113-02832-29` |
| 0xe004 | ~812 | **RSA-2048 private key** | **secret — never dump/commit** |

Actual bytes (this unit, verified against the field map):
```
# mtd4 header 0x0000
00000000: 74ac b941 a811 76ac b941 a811 ea16 0777   ← MAC | MAC2 | ea16 | 0777
00000010: 000b 101d 0000 0000 ...                    ← devid, then zero-fill
# UBNT redundant record @0x8000
00008000: 5542 4e54 2a28 66d3 0000 0064 0002 0001   ← "UBNT" | cksum | len | ver
00008010: 0777 ea16 000b 101d 74ac b941 a811 0200   ← hwrev|boardid|devid|MAC|cnt=2
```
Real hexdump source: `xxd .../mtdNN-EEPROM-*.img` under
`/mnt/2tb/git_debris/woomera-mtd/`. MAC sourcing for our U-Boot (#89): read 6 B at
mtd4+0x0000 via `read_rom_hwaddr`. Full field map: `docs/identity-partitions.md`,
`docs/nor-reference/eeprom-fieldmap.json`.

Our Fedora-on-NAND layout **differs** from stock (kernel@0x1300000, dtb@0x2800000,
factory recovery@0x300000) — see the `nand-boot-layout-recovery` memory / flash flow.

## Our running Linux (mainline 7.1.8) does NOT see the NAND — confirmed 2026-08-20

`/proc/mtd` on the live box (Fedora on SSD, `uname -r 7.1.8-dirty`) lists only
**8 partitions, all SPI-NOR** (`spi0.0`, erasesize `0x10000`/`0x1000`-family) —
mtd0–7 above. **None of the 4 NAND partitions appear.** This is expected, not a
fault, and does not contradict the NAND chip being real:

- The DT node is present and enabled: `/proc/device-tree/soc/nand@fa100000/status`
  = `okay`, `compatible` = `annapurna-labs,al-nand`. The platform device
  registers (`/sys/bus/platform/devices/fa100000.nand/` exists, `uevent` shows
  `OF_COMPATIBLE_0=annapurna-labs,al-nand`) — but **no driver binds** (no
  `driver` symlink in that sysfs dir).
- **No driver exists for it.** `grep -rn "annapurna-labs,al-nand"` across
  `linux-v7.1.8/drivers/` (the mainline kernel.org tree this project builds)
  returns zero hits — the Annapurna AL-NAND MTD controller driver was **never
  upstreamed to mainline Linux**. It only exists in Ubiquiti's own out-of-tree
  GPL BSP kernel (4.1.37-ubnt, the one that ships stock 1.3.35 firmware).
- The deployed kernel config (`unvr-ea16-7.1.config`) has
  **`CONFIG_MTD_RAW_NAND is not set`** — the raw-NAND subsystem itself is
  disabled, so even a driver wouldn't attach without a rebuild.
- Consequence: zero probe attempts logged anywhere — not dmesg, not the full
  boot `journalctl -b` (59k lines) — because nothing ever tries to bind. Only
  `spi-nor spi0.0` (the generic mainline `m25p80`/`spi-nor` driver, which the
  chip is generic enough to match) attaches, producing exactly the 8
  SPI-NOR-only partitions seen.
- SPI-NOR still binding while NAND doesn't is exactly the giveaway that these
  are two separate physical chips — the same distinction `docs/mtd.md` already
  makes by erase size (§ "Two devices").

**The NAND chip's physical existence is independently proven**, not assumed:
`docs/hardware.md` "CPU / NAND / identity extras" records a **live JEDEC ID
read under stock 1.3.35** (`Manufacturer ID 0x2c, Chip ID 0xa3` → Micron
MT29F8G08ABBCAH4, 1024 MiB SLC, erase 256 KiB, page 4096, OOB 224) plus **active
ECC correction on real kernel-partition reads** — that's the vendor's own
NAND driver, in its own kernel, actually reading the chip. Two disjoint
software stacks (stock 4.1.37-ubnt kernel: sees+uses NAND; our mainline
7.1.8: NAND node present but undriven) is consistent, not contradictory —
mainline simply never gained this vendor driver.

### `scripts/flash-nand.py` operates below Linux entirely — verified safe

`flash-nand.py` never goes through Linux's MTD layer. It drives **stock
U-Boot's** own `nand read`/`erase`/`write` console commands at the
`ALPINE_UBNT_NAS_ALL>` prompt — the same NAND driver stock U-Boot uses every
normal boot (`bootfrom=bootnand`, `docs/nor-boot-chain.md` §4) to load the
vendor kernel from NAND. That driver is proven live and correct: it's what
boots the box today when doing a stock boot. (Our own custom U-Boot,
`uboot-port/`, has **no NAND driver at all** — grep for `nand`/`al-nand`
across it is empty; it targets SSD/SATA boot only and is not what
`flash-nand.py` talks to.)

**Address sanity vs the documented NAND map (table above, chip-relative
offsets):**

| | value | check |
|---|---|---|
| kernel offset | `0x1300000` | = start of `rootfs` (mtd10, dead on our layout) |
| kernel span | `0x1200000` (18 MiB) | `0x1200000 / 0x40000` = 72 — exact multiple of NAND's 256 KiB erase block |
| kernel end | `0x2500000` | inside `rootfs` (ends `0x40f00000`), well clear |
| DTB offset | `0x2800000` | `/0x40000` = 160 — erase-block aligned; **3 MiB past kernel end**, no overlap |
| DTB erase span | `0x40000` (256 KiB) | matches NAND's own erase granularity, not SPI-NOR's 4 KiB |

Both addresses are 256 KiB-erase-block-aligned and land inside the ~1005 MiB
`rootfs` NAND partition (chip-relative `0x1300000`–`0x40f00000`) — nowhere
near the 32 MB SPI-NOR's address space. (An address like `0x2800000` = 40 MiB
only looks impossible if wrongly compared against the 32 MB SPI-NOR; it is
trivially inside the 1024 MiB NAND.) The geometry is self-consistent with a
**deliberate NAND-aware choice**, not a copy-paste of the wrong chip's numbers.

**Prior empirical proof, not just address arithmetic:** `docs/fedora-on-ssd.md`
"PERSISTENT" section records `flash-nand.py` verified **end-to-end on real
hardware, 2026-08-17** — NAND kernel → decompress → mount SSD root →
`systemd multi-user.target`, cold boot via stock U-Boot autoboot. Issue **#40**
(closed, the SSD-boot milestone) and issue **#54** (open, hardening follow-ups:
FIT/hash verification, DTB placement cleanup, idempotency — not correctness)
both corroborate this. This session's verification (device-tree/sysfs/kernel
config /proc/mtd/journal checks above) is independent corroboration of the
*mechanism* (why Linux's own `/proc/mtd` doesn't show NAND, and why that's
fine); the addresses' *correctness* rests on the 2026-08-17 successful boot,
which is the stronger evidence.

**Terminology check:** searched `docs/`, `scripts/`, `dts/` for "NAND" —
no conflation found. Every use refers to the genuine 1 GiB MT29F8G08 chip;
SPI-NOR is consistently written "NOR"/"SPI-NOR". `docs/mtd.md`'s own
"Two devices" table above is the disambiguation reference.

## Dumping / restoring

- **Capture** (read is safe): in Linux `dd if=/dev/mtdN of=<file> bs=4096`, or over
  serial+netcat when no SSH (how the in-repo snapshots were taken).
- **Restore**: `flashcp -v <file>.img /dev/mtdN` — it **erases first**. `dd` to a
  char MTD does **not** erase → corrupt partition. Never `dd` a restore.
- **Read-only inspect a filesystem partition** without mounting: parse the ext4
  superblock (`scripts/decode-identity.py` does mtd6), or `mtdblock` + loopback.

## In-repo snapshots
`images/mtd/UNVR-<serial>-sysid<id>-<ts>/` — one dir per capture. Identity/config
partitions (Factory, EEPROM, env, config, cksum) are **committed** (exist nowhere
else); regenerable images (u-boot, kernels, device_tree, rootfs, al_boot, chike)
are gitignored. Manifest per snapshot has sizes + sha256.

## See also
- `docs/nor-boot-chain.md` — NOR boot chain + identity partition detail (§6)
- `docs/identity-partitions.md` — decoded per-unit identity fields
- `docs/nand-1.3.35.md` — NAND layout history
- `docs/hardware.md` — chips (MX25U25635F, MT29F8G08 datasheets in `sources/chips/`)
