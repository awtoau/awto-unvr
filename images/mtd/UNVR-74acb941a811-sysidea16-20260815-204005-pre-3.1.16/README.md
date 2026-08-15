# UNVR MTD dumps — 20260815-204005

Taken 2026-08-15T20:40:05+10:00 over the serial console + netcat (no SSH; the device has no open ports).

## Unit

| Field | Value |
|---|---|
| `board.shortname` | `UNVR` |
| `board.name` | `UniFi Network Video Recorder` |
| `board.sysid` | `0xea16` |
| `board.serialno` | `74acb941a811` |
| `board.hwrev` | `0xb101d` |
| `board.bom` | `113-02832-29` |
| `board.uuid` | `810d6686-46ac-5d49-9aa5-428a04c10400` |
| `board.qrid` | `icvpQD` |
| kernel | `4.19.152-ui-alpine` |

## Partitions

Partitions unique to this unit (Factory, EEPROM, u-boot env, config,
cksum) are **committed** - they exist nowhere else. Partitions that ship
in the firmware `.bin` and can be re-extracted (al_boot, device_tree,
linux_kernel, chike, u-boot, recovery kernel, rootfs) are gitignored;
rootfs alone is 1004 MB, 10x GitHub's per-file limit.

| Device | Name | Bytes | Erase | Tracked | sha256 | File |
|---|---|---|---|---|---|---|
| `mtd0` | u-boot | 1835008 | 4096 | no | `8af165bc3754b498…` | `UNVR-74acb941a811-ea16-mtd00-u-boot-1835008B-20260815-204005.img` |
| `mtd1` | u-boot env | 65536 | 4096 | **yes** | `de2f256064a0af79…` | `UNVR-74acb941a811-ea16-mtd01-u-boot_env-65536B-20260815-204005.img` |
| `mtd2` | u-boot env redundant | 65536 | 4096 | **yes** | `764ee2ec520fdda6…` | `UNVR-74acb941a811-ea16-mtd02-u-boot_env_redundant-65536B-20260815-204005.img` |
| `mtd3` | Factory | 65536 | 4096 | **yes** | `71189f7fb6aed638…` | `UNVR-74acb941a811-ea16-mtd03-Factory-65536B-20260815-204005.img` |
| `mtd4` | EEPROM | 65536 | 4096 | **yes** | `5394094021e18872…` | `UNVR-74acb941a811-ea16-mtd04-EEPROM-65536B-20260815-204005.img` |
| `mtd6` | config | 14675968 | 4096 | **yes** | `97a917524bac4e45…` | `UNVR-74acb941a811-ea16-mtd06-config-14675968B-20260815-204005.img` |
| `mtd7` | cksum | 4096 | 4096 | **yes** | `ad7facb2586fc6e9…` | `UNVR-74acb941a811-ea16-mtd07-cksum-4096B-20260815-204005.img` |
| `mtd9` | linux_kernel | 16777216 | 262144 | no | `e82f8e989903dc5c…` | `UNVR-74acb941a811-ea16-mtd09-linux_kernel-16777216B-20260815-204005.img` |

## Restoring

`flashcp` erases first; `dd` to a char MTD does not and leaves a corrupt
partition.

```
flashcp -v <file>.img /dev/mtdN
```

