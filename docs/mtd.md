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
| mtd3 | Factory | NOR | 0x1e0000 | 0x10000 | 64 KB | raw | **yes** | `0xFF`, unused → **awto-U-Boot env** (#81) |
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
