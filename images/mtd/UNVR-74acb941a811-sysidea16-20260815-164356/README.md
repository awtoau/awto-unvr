# UNVR MTD dumps — 20260815-164356

Taken 2026-08-15T16:43:56+10:00 over the serial console + netcat (no SSH; the device has no open ports).

## Unit

| Field | Value |
|---|---|
| `board.shortname` | `UNVR` |
| `board.name` | `Protect Network Video Recorder` |
| `board.sysid` | `0xea16` |
| `board.serialno` | `74acb941a811` |
| `board.hwrev` | `0xb101d` |
| `board.bom` | `113-02832-29` |
| `board.uuid` | `810d6686-46ac-5d49-9aa5-428a04c10400` |
| `board.qrid` | `icvpQD` |
| kernel | `4.1.37-ubnt` |

## Partitions

Partitions unique to this unit (Factory, EEPROM, u-boot env, config,
cksum) are **committed** - they exist nowhere else. Partitions that ship
in the firmware `.bin` and can be re-extracted (al_boot, device_tree,
linux_kernel, chike, u-boot, recovery kernel, rootfs) are gitignored;
rootfs alone is 1004 MB, 10x GitHub's per-file limit.

| Device | Name | Bytes | Erase | Tracked | sha256 | File |
|---|---|---|---|---|---|---|
| `mtd2` | linux_kernel | 16777216 | 262144 | no | `b1ed33544a73bba5…` | `UNVR-74acb941a811-ea16-mtd02-linux_kernel-16777216B-20260815-164356.img` |
| `mtd3` | rootfs | 1052770304 | 262144 | no | `9e0b1f52a567638e…` | `UNVR-74acb941a811-ea16-mtd03-rootfs-1052770304B-20260815-164356.img` |
| `mtd10` | recovery kernel | 16777216 | 4096 | no | `15f6ce0a8a1f8f07…` | `UNVR-74acb941a811-ea16-mtd10-recovery_kernel-16777216B-20260815-164356.img` |

## Restoring

`flashcp` erases first; `dd` to a char MTD does not and leaves a corrupt
partition.

```
flashcp -v <file>.img /dev/mtdN
```

