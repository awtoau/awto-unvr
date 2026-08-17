# Upgrade path — 1.3.35 to current

Device is on **v1.3.35+0d589c7** (`UNVR4.arm64.v1.3.35.0d589c7.201216.1801`,
kernel 4.1.37-ubnt, Debian 9 stretch). Target is current. Full release list
pulled from the firmware API 2026-08-15.

Decision taken: **upgrade only, no reset.** The 6.9 TB video array on
`UniFi-NVR:3` and the Protect database in `/volume1/.srv` stay untouched.

## The API

Latest only:
```
https://fw-update.ubnt.com/api/firmware-latest?filter=eq~~platform~~UNVR
```
**Full history** (the useful one — 48 releases):
```
https://fw-update.ubnt.com/api/firmware?filter=eq~~platform~~UNVR&filter=eq~~channel~~release&sort=created&limit=200
```
Each row carries `sha256_checksum`, `md5`, `file_size`, `tags.ubnt_version` and a
direct `_links.data.href` on `fw-download.ubnt.com`. `sort=-created` for newest
first. Drop the channel filter to include `beta-public`/`alpha`.

## The platform boundary at v1.4.9 ✅

The `ubnt_version` string changes generation mid-history:

| Releases | Platform tag |
|---|---|
| v1.3.0 … **v1.3.39** | `UNVR4.**arm64**.…` |
| **v1.4.9** … v5.1.25 | `UNVR4.**al324**.…` |

**Our device is on the `arm64` side of that line.** The first upgrade that
matters crosses it. This is also where the kernel generation changes — 1.3.x runs
4.1.37-ubnt, later builds run 4.19.152-alpine-unvr — which is why the MTD
numbering differs between generations (see [nand-1.3.35.md](nand-1.3.35.md)).

Image size roughly triples across the history: 289 MB → 786 MB.

## Release ladder

Sizes rounded. Full sha256 in the API response.

| Version | Date | Size | Platform | Note |
|---|---|---|---|---|
| v1.3.35 | 2020-12-16 | 337 MB | arm64 | **installed now** |
| v1.3.39 | 2021-02-05 | 339 MB | arm64 | last of the arm64 line |
| **v1.4.9** | 2021-05-19 | 358 MB | **al324** | **first al324 build — the boundary** |
| v2.2.7 | 2021-09-06 | 357 MB | al324 | first 2.x |
| **v2.3.14** | 2022-02-03 | 407 MB | al324 | **UrNVR pins this exact version** |
| v2.5.11 | 2022-08-09 | 440 MB | al324 | last 2.x |
| v3.0.13 | 2022-11-23 | 393 MB | al324 | first 3.x |
| v3.1.16 | 2023-09-07 | 425 MB | al324 | |
| v3.2.12 | 2024-02-19 | 513 MB | al324 | last 3.x |
| v4.0.6 | 2024-07-01 | 528 MB | al324 | first 4.x |
| v4.1.22 | 2025-04-14 | 614 MB | al324 | |
| v4.4.3 | 2025-10-23 | 701 MB | al324 | last 4.x |
| v5.0.9 | 2026-01-05 | 711 MB | al324 | first 5.x |
| v5.1.25 | 2026-07-27 | 786 MB | al324 | **latest**, already downloaded + verified |

## Recommended ladder

Ubiquiti publishes no supported-upgrade matrix, so this is reasoning from the
boundaries in the data, not from Ubiquiti guidance:

```
1.3.35  ->  1.4.9   ->  2.3.14  ->  3.1.16  ->  4.1.22  ->  5.1.25
            ^           ^
            platform    community-validated
            boundary    (UrNVR pins it)
```

Rationale per hop:

- **1.4.9** — crosses the `arm64` → `al324` boundary on its own, with nothing
  else changing. If a migration is going to break, this is the likeliest hop and
  the cheapest one to isolate.
- **2.3.14** — the only version any community project pins exactly. UrNVR
  requires it, so it is known to be a coherent, reachable state.
- **3.1.16, 4.1.22** — late-in-line builds of each major, i.e. the most-patched
  point before the next major transition.
- **5.1.25** — current; already downloaded and sha256-verified in `sources/`.

Skipping straight to 5.1.25 is *probably* fine — Ubiquiti generally supports
direct upgrades — but it crosses Debian 9 → 11, kernel 4.1 → 4.19, and five
years of Protect schema migrations in one step, with a database holding 6.9 TB
of indexed footage.

## Fetching an intermediate

`scripts/fetch-unvr-firmware.py` currently pins 5.1.25. For another version take
`_links.data.href` and `sha256_checksum` from the API row — verification is
mandatory, the script deletes anything that fails rather than leaving a bad copy.

## Applying it — the two routes

**Neither has been exercised yet.** Both come from reading the firmware, not
from doing it.

### 1. `ubnt-tools fwupdate` (CLI, from the serial root shell)

`ubnt-tools` is a multi-call binary providing `fwupdate` / `fwsplit` / `fwinfo`.
It verifies the image with OpenSSL `d2i_PUBKEY` + `EVP_Verify*` before writing
(`ERROR: Bad FW Image Signature` on failure), so an unmodified stock `.bin`
passes and a tampered one does not.

Get the image onto the box first — TFTP is the reliable transport
(`scripts/tftpd.py`); **netcat is not**, see [nand-1.3.35.md](nand-1.3.35.md).

### 2. Drop-in on the USERDEV stick

`mount_premount` checks `${MNT_RWFS}/upgrade/fw-image.bin` before UniFi OS
starts, flashes it, deletes it and reboots. So placing the image at
`upgrade/fw-image.bin` on the stick and rebooting is sufficient — no CLI needed.

Both paths call `upgrade_firmware()`, which runs `upgrade_kernel` and
`upgrade_rootfs` **only**. U-Boot (`mtd5`), its environment (`mtd6`/`mtd7`) and
the device tree are **not** touched — so the unsigned-boot behaviour documented
in [bootloader.md](bootloader.md) survives any upgrade.

## Before the first hop

- Confirm `/volume1` still mounts and `.srv/unifi-protect` is intact after each
  step, before taking the next.
- MTD dumps are already captured — `images/mtd/UNVR-…-164103/` and `-164222/`.
  They are the 1.3.35 state and cannot be re-made once it is overwritten.
- Recovery if a hop bricks the boot: hold reset ~10 s at power-on for the SPI
  recovery kernel at `mtd10`, which lives on a different chip from the NAND
  kernel an upgrade rewrites.
