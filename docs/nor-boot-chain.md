# UNVR SPI-NOR boot chain — full reverse-engineering

Comprehensive map of the 32 MiB SPI-NOR (MX25U25635F) boot chain and every NOR
partition on the Ubiquiti UNVR (Annapurna Labs Alpine V2 / AL-324, quad
Cortex-A57, aarch64, sysid **0xea16**, serial `74acb941a811`). Foundational for a
mainline-Linux port (the boot chain must be understood to replace it) and for
forensics.

Method: `aarch64-linux-gnu-objdump` / `arm-linux-gnu-objdump` (Fedora stock
`objdump` is x86-only), `strings`, `nm`, python structure parsers. binwalk /
radare2 not installed; Ghidra present but decompiler-export hangs (OpenJDK 25) —
not used. ✅ = proven by instruction decode / byte compare. ❓ = inferred.

Builds on (not restated): [bootloader.md](bootloader.md),
[boot-flow.md](boot-flow.md), [uboot-update-path.md](uboot-update-path.md),
[nand-1.3.35.md](nand-1.3.35.md), [firmware-5.1.25.md](firmware-5.1.25.md),
[licence-audit.md](licence-audit.md), [sources.md](sources.md).

Scripts: `scripts/parse-al-toc.py`, `scripts/decode-preboot-dtmap.py`,
`scripts/dump-uboot-board-table.py`, `scripts/dump-ubnt-board-table.py`,
`scripts/diff-nor-blobs.py`, `scripts/decode-identity.py`, `scripts/arm-xref.py`,
`scripts/carve-dtb.py`. Carved artifacts in `docs/nor-reference/`.

---

## 0. Two boot containers, one device state

- The device has **exactly one bootloader, unchanged since manufacture.** All 13
  on-device `mtd00` u-boot dumps across the full 1.3.35→5.1.25 upgrade ladder are
  **byte-identical** ✅: `sha256 8af165bc3754b498764f271b10cb2cf7d3c61daaf6d153dfa2155544f37848d9`.
  Firmware upgrades never write the boot container (confirmed
  [uboot-update-path.md](uboot-update-path.md): ea16 `fwupdate` device fields are
  empty).
- So there are only **two** distinct containers to reverse:
  - **OLD** — on-device, built **2020-12-16**, 1,835,008 B. THE ONE THAT RUNS.
    `images/mtd/UNVR-…-post-5.1.25-final/…mtd00-u-boot-1835008B….img`.
  - **NEW** — packaged in `sources/UNVR-5.1.25.bin` → `tmp/sections/01-uboot.bin`
    (1,395,740 B carve). U-Boot built **2026-07-09**, stage3 rebuilt **Jan 15
    2021**. **Never flashed to this unit.**

## 1. NOR container layout (Annapurna flash TOC)

TOC magic `0x070c070c` at flash **0x80000**. Entry (0x20 B): `id[4] type[4]
name[12] off[4] size[4] rsvd[4]`. Type low nibble = object class; **type high
nibble = multi-DT instance index** ✅. Per-object image header magic
`0x000b9ec7`, payload size at +0x28, load/entry at +0x30/+0x38.

| Object | type | OLD off/size | NEW off/size | Content |
|---|---|---|---|---|
| `preboot` | 0x0d | 0x000000 / 0x080000 | 0x000000 / 0x080000 | S2 SPI loader + al_boot + stage3 (below) |
| `dt` (v5.0, **ea16**) | 0x0000_0002 | 0x081000 / 0x7000 | 0x081000 / 0x7000 | DTB instance 0 |
| `dt_pro` | 0x1000_0002 | 0x088000 / 0x8000 | 0x088000 / 0x8000 | DTB instance 1 |
| `dt_ai` | 0x2000_0002 | 0x090000 / 0x8000 | 0x090000 / 0x8000 | DTB instance 2 |
| `dt_bt` | 0x3000_0002 | 0x098000 / 0x9000 | 0x098000 / 0xa000 | DTB instance 3 |
| `dt_hd` | 0x4000_0002 | — | **0x0a2000 / 0xa000** | DTB instance 4 (**new**) |
| `uboot` | 0x05 | 0x0a1000 / 0x11f000 | 0x0ac000 / 0x114000 | U-Boot proper, load/entry 0x1100000 |
| `uboot-env` | 0x07 | 0x1c0000 / 0x10000 | same | env slot (unused on device — zeroed) |
| `uboot-re` | 0x08 | 0x1d0000 / 0x230000 | same | recovery slot descriptor (overlaps mtd05 recovery kernel) |

Sub-blobs INSIDE the `preboot` object (0..0x80000):

| Sub-blob | Container off | Size | ISA / load | Role |
|---|---|---|---|---|
| **S2 SPI loader** | 0 (hdr `"S2\0\0"`, size 0x61b4 @+0x08, code @0x20) | 25,044 B | ARM A32→Thumb-2 @0xF2200020 | boot-ROM-loaded SPI loader. **Zero strings.** IDENTICAL old==new ✅ |
| **al_boot preboot** | img-hdr @0x20000, payload @0x21000 | 435,892 B (0x6a6b4) | ARM A32 @0x01000000 | Annapurna al-boot v2.10.0 **+** contractor stage3 multi_dt (co-resident) |

## 2. Boot chain, stage by stage

```
SoC mask boot ROM (on-chip)
  └─ reads S2 SPI loader from SPI off 0 → SRAM @0xF2200000, runs it
      └─ S2 loads al_boot preboot payload (off 0x21000) → @0x01000000
          ├─ Annapurna al-boot v2.10.0  (banner "stage2_loader v2.22.3")
          │    CVOS "agent_wakeup v2.10"; DDR/DRAM init; SoC/fabric/PBS/UART bring-up
          └─ contractor Stage 3 v2.22.0  (multi_dt)
               stg3_early_init → stg3_board_init → dt_based_init:
                 • read boardid (sysid) from SPI-NOR EEPROM part @flash 0x1F000C (BE)
                 • map sysid → DTB instance (§3) → al_flash_toc_find_id(DT,inst)
                 • load chosen DTB, serdes/pcie/thermal init, power_down_secondary_cpus
                 • load U-Boot (TOC "uboot", app) → 0x1100000, jump
                     └─ U-Boot 2015.07-alpine_db-2.21-HAL  (§4)
                          run load_fdt; loadbootargs; multiboot → bootnand → kernel
```

- Runtime banner order (boot log): `stage2_loader v2.22.3` → `Stage 3 version:
  2.22.0 / Commit 6088bc3 / CVOS bac1d52 / HAL 61afa9c` → U-Boot banner. The
  `stage2_loader`/`Stage 3` banner strings both live in the **al_boot payload**
  (0x21000); the off-0 `S2` blob is stringless.
- stage3 role functions (⟵ __func__ strings): `stg3_early_init`,
  `stg3_board_init`, `dt_based_init`, `dt_based_init_pcie`,
  `dt_based_init_serdes_group`, `thermal_sensor_trim_init`,
  `power_down_secondary_cpus`, `dram_clear`, `eeprom_preload_parser`,
  `al_flash_toc_find_id`, `al_flash_toc_find_id_with_fallback`.
- al_boot/stage2 role (build paths): `HAL/drivers/pbs/al_hal_bootstrap.c`,
  `al_hal_uart.c`, `sys_services/al_hal_timer.c`,
  `sys_fabric/al_hal_sys_fabric_utils.c`, `aarch64_resume_agent/main.c`,
  `src/aarch64/fiq.c` (`irq_setup_secure`). DDR: `DDR size not supported!`,
  `DRAM frequency violation!`.

## 3. sysid → DTB selection — GAP CLOSED ✅

The known-gap ("new 5.1.25 preboot multi_dt mapping not disassembled") is closed.
Decoder: `scripts/decode-preboot-dtmap.py`; disasm artifact
`docs/nor-reference/preboot-dt-selection.asm`.

**Where sysid comes from:** stage3 `dt_based_init` reads the board id via the
Annapurna flash-read fn-ptr from **SPI-NOR flash offset 0x1F000C** — i.e. the
**EEPROM partition (mtd04) + 0xC**, the 2-byte board-id field — then byte-swaps
it (stored big-endian). It also reads devid at `0x1F0010` (4 B). **Not** eFuse,
**not** a strap. (VA 0x0100167c–0x10016e4: `mov r0,#12; movt r0,#31` = 0x1F000C;
`mov r0,#16; movt r0,#31` = 0x1F0010; `blx r3`.)

**Dispatch** (VA 0x01001708, both preboots):

```
ldrh  r3,[r7,#2]            ; r3 = boardid (byte-swapped)
sub   r3,r3,#0xea00
sub   r3,r3,#0x16           ; r3 = sysid - 0xea16
cmp   r3,#N                 ; N=11 (OLD) / 26 (NEW)
ldrls pc,[pc,r3,lsl#2]      ; jump table @0x01001720
b     default               ; out of range
```

Each jump-table slot targets a `b common`; the `mov r2,#instance` sits at
target+4 (r2/r6 = instance, stored to state; drives `al_flash_toc_find_id(type=2,
instance)`). Instance → TOC object by high-nibble.

| sysid | instance | DTB / board-cfg | board | OLD | NEW |
|---|---|---|---|---|---|
| **0xea16** | **0** | `dt` = `one nas v5.0` | **UNVR (this unit)** | ✅ | ✅ |
| 0xea20 | 1 | `dt_pro` = `one nas pro v2.0` | UNVR-PRO | ✅ | ✅ |
| 0xea21 | 2 | `dt_ai` = `one nas ai v3.0` | UNVR-AI | ✅ | ✅ |
| 0xea1a | 3 | `dt_bt` = `one nas bt v1.0` | UNVR (eMMC variant) | ✅ | ✅ |
| 0xea30 | 4 | `dt_hd` = `one nas hd` | new HD model | — | ✅ |
| any other 0xea17–0xea2f | 0xFF | — | INVALID → error path | | |

Cross-validated **three ways**: (1) the `mov r2,#N` at each slot+4; (2) exact
agreement with the OLD U-Boot's own sysid switch (ea16→0, ea20→1, ea21→2,
ea1a→3, [bootloader.md](bootloader.md), [uboot-update-path.md](uboot-update-path.md));
(3) the DTB `board-cfg` strings per instance (carved `tmp/uboot-dtbs/`). OLD table
has 12 slots (ea16..ea21); NEW adds ea30→dt_hd (27 slots). The mapping is thus in
**both** preboot (primary, pre-U-Boot) and U-Boot's own switch (redundant).

- The `preboot` payload differs byte-for-byte OLD↔NEW (recompiled; 262,007 /
  435,892 bytes changed; stage3 build date Sep 8 2020 → Jan 15 2021, same commit
  6088bc3) but the dispatch is at the **same VA 0x01001718** in both.

## 4. U-Boot proper — Ubiquiti additions over stock 2015.07

Base = denx **U-Boot v2015.07** (byte-identical blob SHAs proven,
[bootloader.md](bootloader.md)), fork = Annapurna alpine_db-2.21-HAL + Ubiquiti.
Load/entry **0x1100000** (`CONFIG_SYS_TEXT_BASE`). Banner OLD `… (Dec 16 2020 -
05:54:51 +0800)`, NEW `… (Jul 09 2026 - 19:13:29 +0800)`.

Ubiquiti/Annapurna additions absent from published stock source
([bootloader.md](bootloader.md)): `ALPINE_UBNT_NAS_ALL>` prompt, `Unabled to
match any board, sysid: %0x%0x`, `No boardinfo matched, skip sysid setup`,
`ubnthal.sysid`, `multiboot`/`multi_dt`, custom cmds (`bootupd`, `delenv`,
`lcd_print`, `setenvmem`, `incenv`, `eth_1g_params_set`,
`eth_link_training_enable`).

**U-Boot board table** — parser `scripts/dump-uboot-board-table.py` (NEW: file
0x139340, VA 0x118d2f8, stride 0x70, 13 entries). sysid → `{name, bootfrom,
model, ethprime}`; `$bootfrom` is set from this table by board code (NOT in the
default env). It covers the **whole AL-324 product line** — which is why one boot
container ships to every product (dead weight on a UNVR, [uboot-update-path.md](uboot-update-path.md)):

| sysid | name | bootfrom | sysid | name | bootfrom |
|---|---|---|---|---|---|
| **0xea16** | **UNVR** | **bootnand** ← this unit | 0xea32 | UDM-PRO-MAX | bootemmc |
| 0xea1a | UNVR | bootemmc | 0xea39 | UDM-PRO-XG | bootemmc |
| 0xea20 | UNVR-PRO | bootemmc | 0xea2a | UDW | bootemmc |
| 0xea51 | UNAS-PRO | bootemmc | 0xea15 | UDM-PRO | bootemmc |
| 0xea63 | UNAS-PRO-8 | bootnand | 0xea11 | UDM-BASE | bootemmc |
| 0xea67 | UNAS-PRO-4 | bootemmc | 0xea19 | UXG-PRO | bootemmc |
| 0xea2c | UDM-PRO-SE | bootemmc | | | |

ea16 → **bootnand** is why `multiboot` runs `bootnand` on our unit. The table's
sysid set differs from the preboot DTB switch (§3): the DTB switch is UNVR-family
(ea16/1a/20/21/30); the board table is product-line-wide. (ea21/UNVR-AI and
ea30/dt_hd are not in this table snapshot.)

**Redundant sysid→DTB switch** — OLD U-Boot also carries its own sysid switch
(file 0xa318c, board struct 0x149648: ea16→0, ea20→1, ea21→2, ea1a→3, ea2c→5),
identical mapping to the preboot (§3). In the new design the **preboot** is the
primary selector; U-Boot's copy is redundant for the DT that preboot already
loaded.

Default env chain (the OS-boot path):

```
bootcmd    = run load_fdt; run loadbootargs; run multiboot   (NEW; OLD inlines load_fdt)
load_fdt   = cp.b $fdtaddr $loadaddr_dt 7ffc; fdt addr $loadaddr_dt
multiboot  = run $bootfrom; [run ${bootfrom}_bkp; run bootemmcblk;] run bootspi
bootnand   = … nand read … ; run dobootm         (NEW: run loadimg_nand; run prebootm; run dobootm)
dobootm    = run bootunsign                        ← DEFAULT
bootunsign = bootm $loadaddr_payload - $fdtaddr    ← plain, UNSIGNED
bootsign   = bootm $loadaddr_payload#$model@$fit_index   ← FIT, signed (NEW only; NOT default)
```

Old↔new env deltas: [uboot-update-path.md](uboot-update-path.md) §Env. `bootdelay
= 2`; env slots (mtd01/02) are **zeroed** on the device → U-Boot runs on
compiled-in defaults ([nand-1.3.35.md](nand-1.3.35.md)).

## 5. Trust model — what is and isn't verified, stage by stage

| Stage | Verifies | Over what | Gates OS boot? | Key location |
|---|---|---|---|---|
| Boot ROM → S2 | Annapurna secure-boot (capability, if fused) | S2/preboot | only if eFuse secure-boot burned ❓ | eFuse OTP |
| stage3 (al_boot) | **RSA capability signature** | an EEPROM "capabilities" blob | **No** — gates SoC capabilities | eFuse modulus **hash** vs EEPROM modulus |
| U-Boot | **legacy CRC checksum only** (OLD); FIT/RSA present-but-unused (NEW) | uImage | **No** — `dobootm=run bootunsign` | (public key compiled in, unused) |
| userspace `ubnt-tools fwupdate` | RSA-2048 signature + container `ENDS` | firmware `.bin` | n/a (upgrade, not boot) | public key in ubnt-tools / `unas.pub`/`fw.pub` |

- **stage3 RSA capability check** (Annapurna al-boot, [bootloader.md](bootloader.md)):
  strings (preboot payload offsets) `EEPROM Revision ID = %02x` (0x30700),
  `failed to authenticate EEPROM's content` (0x30790), `RSA decryption failed!`
  (0x30f28), `signature decryption does not match the hashed capabilities`
  (0x30f40), `eFuse hashed modulues does not match the EEPROM hashed modulus`
  (0x30f80), `no opening/closing EEPROM encapsulation`, `eeprom_preload_parser`.
  This verifies a **signed SoC-capabilities blob** against an **eFuse-burned
  modulus hash** — real signature checking, **over SoC caps, not the kernel**.
- **U-Boot boots the OS UNSIGNED.** OLD U-Boot has legacy-image handling only
  (`Legacy image found`, `Verifying Checksum …`, `Bad Data CRC`, `## Booting
  kernel from Legacy Image`) — **no** `Verifying Hash Integrity`, **no** FIT,
  **no** RSA/sha256 over the kernel. NEW adds `bootsign` (FIT) but keeps
  `dobootm=run bootunsign`. `bootsecure` / `## Error: Secure boot command not
  specified` is U-Boot's CLI-lockdown var, inert at `bootdelay=2`.
- **The EEPROM RSA-2048 private key (§6) is NOT a boot key.** It is a device
  secret (identity/attestation ❓); the boot chain never reads it. Firmware images
  are verified with *public* keys held elsewhere.

**Verdict for own-Linux:** the OS kernel is unverified at every boot stage. An
arbitrary unsigned `uImage` boots (TFTP into RAM, or written to NAND mtd9) with
no signing, no flash change to the bootloader. Secure-boot capability exists in
the SoC/stage3 but is **not enforced over the OS** — the only enforced signature
in the whole path is the userspace firmware-updater, which the kernel-replacement
route bypasses ([boot-flow.md](boot-flow.md)).

## 6. Identity partitions (this unit's data — format only; no secrets reproduced)

Decoder `scripts/decode-identity.py`; field map
`docs/nor-reference/eeprom-fieldmap.json`.

### Factory (mtd03, 65536 B) — BLANK
- 100% `0xFF`, never written. Unchanged from 1.3.35. **All identity lives in the
  EEPROM.**

### EEPROM (mtd04, 65536 B) — written (content out to 0xe330)
Primary header @0x0000:

| Off | Sz | Field | Value |
|---|---|---|---|
| 0x0000 | 6 | base MAC | `74:ac:b9:41:a8:11` |
| 0x0006 | 6 | second MAC | `76:ac:b9:41:a8:11` (base +2 on octet0) — new vs 1.3.35 docs |
| 0x000C | 2 | **board id / sysid** | `0xea16` — read by stage3 (§3) |
| 0x000E | 2 | hw rev | `0x0777` |
| 0x0010 | 4 | device id | `0x000b101d` |

Structured UBNT record @0x8000 (redundant board data): `0x8000` magic `UBNT`;
`0x8004` cksum `0x2a2866d3`; `0x8008` len `0x64`; `0x800C`/`0x800E` version words
`2`/`1`; `0x8010` hwrev `0x0777`; `0x8012` boardid `0xea16`; `0x8014` devid
`0x000b101d`; `0x8018` base MAC; `0x801E` MAC-count `2`.

UBNT-TLV block @0xd000 (ONIE `TlvInfo` framing, new vs 1.3.35): `0xd00b` type
0x01 = mfg date `20200524`; `0xd022` type 0x04 = BOM `113-02832-29`; `0xd030`
type 0xFE = CRC `0xd29bebb4`.

Unidentified blob @0x9000–0xc000 (12,288 B): entropy 7.983 bits/byte
(near-random), no redundant copy. Purpose unknown ❓ (candidate: device-unique
secret / entropy pool).

**RSA-2048 PRIVATE key @0xe004** (location/properties only — key NOT reproduced):
- Region 0xe004–0xe330 = 812 B. Format **SSH-wire** (length-prefixed BE mpints),
  **not DER**. Prefix `39 31 4e 54 02 03 25`, `ssh-rsa` tag @0xe00b.
- Fields: `e` @0xe016 (65537), `n` @0xe01d (2048-bit, 257 B incl leading 0x00),
  `d` @0xe122 (256 B), `p` @0xe226 (129 B), `q` @0xe2ab (129 B). **p·q == n
  verified.** Full private key (d,p,q all present).
- Purpose ❓ — device identity / attestation key. It is a **device secret**, not
  a firmware-verification key (firmware images are verified by *public* keys in
  ubnt-tools / unas.pub, [firmware-5.1.25.md](firmware-5.1.25.md)). Not used by the
  boot chain (§5). See "still unknown".

### config (mtd06, 14,675,968 B) — ext4
- Superblock @0x400 magic `0xEF53`; block size 1024; UUID
  `e98263ea-0a3d-4765-8377-f6de9b33da4e` (**same UUID as 1.3.35** — never
  reformatted across generations); last-mounted `/tmp/.config`.
- Root files (read via debugfs, not mounted): `version`
  (`UNVR4.al324.v4.1.22.…`), `version.pre` (`…v3.1.16.…`), `.upgrade-bootup`
  (0-byte flag). No `.factory-reset`/`.network-upgrade` at dump time. Config flag
  semantics: [boot-flow.md](boot-flow.md).

### cksum (mtd07, 4096 B) — ZEROED
- 100% `0x00` (actively written nulls, not erased-0xFF). Not a live checksum
  store on this generation. Unchanged from 1.3.35.

## 7. Provenance / licence per component

| Component | Version / build id | Build path (host) | Toolchain | Licence |
|---|---|---|---|---|
| S2 SPI loader | (no banner) | — | Annapurna | Annapurna SDK — **proprietary, unpublished** |
| al-boot preboot | `stage2_loader v2.22.3`, `agent_wakeup v2.10`, al-boot **v2.10.0** | `/project/users/barak/tasks/al-boot-release/…/alpine-v2-al-boot-v2.10.0.tmp/…` | Annapurna | Annapurna al-boot — **proprietary, unpublished** |
| contractor stage3 | `Stage 3 v2.22.0`, commit `6088bc3`, CVOS `bac1d52`, HAL `61afa9c`; build Sep 8 2020 (OLD) / Jan 15 2021 (NEW) | `/home/winder/projects/data/customers/ubiquiti/multi_dt/preboot_v2/stage3/` | Annapurna | **Ubiquiti-commissioned contractor — proprietary, unpublished** |
| DTBs (5) | `model="Annapurna Labs Alpine V2 UBNT"` | (multi_dt) | dtc | DT source (GPL/BSD by convention) — **unpublished for 5.1.25** |
| U-Boot | `2015.07-alpine_db-2.21-HAL` (Dec 16 2020 OLD / Jul 09 2026 NEW) | OLD `/home/dio/…/ubnt_unvr_all-161-2020-12-16-…`; NEW `…all1-59-2026-07-09-…` | **gcc 6.3.0 (Debian 6.3.0-18) + binutils 2.28** (both builds) | base denx **U-Boot v2015.07 = GPLv2**; fork = Annapurna HAL + Ubiquiti (proprietary) |

- **U-Boot toolchain is frozen** at gcc 6.3.0 (2017): even the 2026-built NEW
  U-Boot uses it — distinct from the gcc 10.2.1 used for kernel/userspace.
- **GPL obligation:** U-Boot 2015.07-alpine_db is the one hard GPL component of
  the boot chain. **No matching public source drop for 5.1.25.**
  `NeccoNeko/UBNT-source-code` (`UNVR-1.3.35-GPL/`) matches only the **1.3.35**
  generation, not this 2020-12-16/2026 U-Boot. HAL for a rebuild recoverable from
  `fabianishere/udm-kernel` + `delroth/alpine_hal` ([sources.md](sources.md),
  [licence-audit.md](licence-audit.md)).
- S2 / al-boot / stage3 are proprietary Annapurna code (no GPL strings) — unpublished
  is expected, not a violation.

## 8. OLD vs NEW boot-code diff (`scripts/diff-nor-blobs.py`)

| Sub-blob | OLD | NEW | Result |
|---|---|---|---|
| S2 SPI loader | 25,044 B | 25,044 B | **byte-identical** (never rebuilt) |
| al-boot preboot | 435,892 B | 435,892 B | changed (recompile, 262,007 B differ); stage3 date Sep 2020→Jan 2021, same commit |
| dt / dt_pro / dt_ai / dt_bt | 4 DTBs | 4 DTBs, all changed | minor size deltas |
| dt_hd | — | present | **new 5th DTB** (`one nas hd`) |
| U-Boot | 689,728 B | 691,152 B | changed (rebuilt 2026); TOC moved 0xa1000→0xac000 to fit dt_hd |
| sysid→DTB table | ea16/1a/20/21 | + ea30 | ea30→dt_hd added (§3) |

Carved: `docs/nor-reference/s2-loader-stage2_v2.22.3-25044B.bin`,
`docs/nor-reference/dt_hd-NEW-5.1.25.dtb`.

## 9. Still unknown / next steps

- **stage3 RSA capability check — enforcement.** Confirmed present and what it
  verifies (SoC caps vs eFuse modulus hash), but not traced whether a failure
  **halts** boot or only disables capabilities, nor whether secure-boot is fused
  on this SoC. Needs disasm of the check's callers + an eFuse read (device-side,
  out of scope here).
- **EEPROM RSA-2048 private key purpose.** Location/format proven (mtd04 @0xe004,
  SSH-wire, p·q==n). Whether it is used for device attestation, cloud enrolment,
  or nothing on-device — not established. Search the rootfs (`ubntnas`,
  `ubnt-tools`) for a consumer of `/dev` EEPROM + `ssh-rsa`.
- **EEPROM blob @0x9000–0xc000 (12 KB, entropy 7.98).** Unidentified. Candidate:
  the signed capabilities blob stage3 authenticates, or a device-unique secret.
- **EEPROM/TLV checksums** (`0x8004`=0x2a2866d3, TlvInfo 0xFE=0xd29bebb4) — not
  matched to standard CRC-32/sha256; likely eCos `cyg_crc32`/`cyg_ether_crc32`
  over an unpinned range (ubnthal exports both).
- **Recovery kernel (mtd05, "uboot-re" TOC obj / SPI 0x200000 +0x1000000)** not
  dumped in the state preset — its 2019 recovery uImage is not analysed here.
- **`config /version` (4.1.22) lags the "post-5.1.25" dump label** with
  `.upgrade-bootup` set — dump likely taken mid-ladder, or `/config/version` is
  written at a different upgrade stage. Worth an issue.
- **ea30 / `dt_hd` ("one nas hd")** — which shipping product uses this new sysid
  is not identified (not in the ubnt-tools board table snapshot decoded so far).
