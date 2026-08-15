# UNVR MTD dumps — 20260815-164222

Taken 2026-08-15T16:42:22+10:00 over the serial console + netcat (no SSH; the device has no open ports).

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
| `mtd0` | al_boot | 2097152 | 262144 | no | `4bda3a28f4ffe603…` | `UNVR-74acb941a811-ea16-mtd00-al_boot-2097152B-20260815-164222.img` |
| `mtd1` | device_tree | 1048576 | 262144 | no | `f5fb04aa5b882706…` | `UNVR-74acb941a811-ea16-mtd01-device_tree-1048576B-20260815-164222.img` |
| `mtd4` | chike | 1048576 | 262144 | no | `c811d24072dec7ed…` | `UNVR-74acb941a811-ea16-mtd04-chike-1048576B-20260815-164222.img` |
| `mtd5` | u-boot | 1835008 | 4096 | no | `8af165bc3754b498…` | `UNVR-74acb941a811-ea16-mtd05-u-boot-1835008B-20260815-164222.img` |
| `mtd11` | config | 14675968 | 4096 | **yes** | `0392220808645d2e…` | `UNVR-74acb941a811-ea16-mtd11-config-14675968B-20260815-164222.img` |

## Restoring

`flashcp` erases first; `dd` to a char MTD does not and leaves a corrupt
partition.

```
flashcp -v <file>.img /dev/mtdN
```

