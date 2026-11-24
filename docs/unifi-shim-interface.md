# UniFi OS shim interface: ubnthal, ubnt_common, ui-hdd-pwrctl (#383)

Surface the 5.1.25 vendor modules present, and what `modules/ubnt_shim/` provides.
Shims forward to our drivers and do no hardware work.

- Source of facts: `lib/modules/4.19.152-alpine-unvr/extra/{ubnthal,ubnt_common,ui-hdd-pwrctl}.ko` unsquashed from `tmp/sections/03-rootfs.bin` (`unsquashfs -d <dir> 03-rootfs.bin lib/modules`), `nm` / `strings` / `aarch64-linux-gnu-objdump -dr`.
- `tmp/logs/nm-ubnthal.txt` and `objdump-ubnthal.txt` do not exist in the tree; regenerate with the command above.
- Userland strings: `libubnt.so.1.0.0`, `ubnt-systool`, `sfpd`, `uscsi_disk_ready`, `ustate`, `99-ustd.rules`, `ustd-poweroff-bottom`.
- GPL source for the older v1 hdd driver only: `tmp/refs/urnvr-kernel-4.19.152/drivers/power/ubnt-hdd-pwrctl.c`. The shipped `ui-hdd-pwrctl.ko` is v2 (GPLv2, source not in the tree).
- Tags: **userland** = a userland binary reads/writes it; **load** = needed so a vendor module loads; **ignore**; **unknown**.
- `TODO` = guessed or unverified; the shim carries a value only where the code comment or this doc says so.

## Load order and boot wiring (rootfs 5.1.25)

- `lib/modules-load.d/ubnt.conf`, `ubntcommon.conf`, `ubnthal.conf` load `ubnt-common` then `ubnthal` (plus `ramoops`, `ubnt-framework`, others). [userland]
- `lib/modprobe.d/ubnthal.conf`: `install ubnthal /sbin/modprobe-failsafe ubnthal`. [unknown: `modprobe-failsafe` not inspected]
- `ubnthal.ko` has empty `depends=` yet calls `ubnt_common` symbols: order is by modules-load, not modpost. Shim records the dependency properly.
- `ui-hdd-pwrctl` is bound by DT compatible `ui,hdd-pwrctl` (alias `of:N*T*Cui,hdd-pwrctl`); kernel config has `CONFIG_UI_HDD_PWRCTL=y` and the `.ko` also ships.
- `ubnt-framework`: not in `extra/`; not investigated. [unknown]
- `ui-ese`: **absent on UNVR**. No `UI_ESE` in `tmp/sections/kernel.config`, no `ui-ese.ko`. Only referenced by `99-ustd.rules` (DRIVER match, expansion hotplug), `libuled` (`ui-ese-N-ulogo_ctrl` LEDs) and `ustate` (`/sys/devices/platform/ui-ese-%d`, fan/psu). All guarded by existence. [ignore]

## ubnthal

Sources inside the `.ko`: `proc.c`, `system.c`, `system-alpine.c`, `proc-alpine.c`, `board.c`/`boards-unifi`, `proc-flash-wp.c`, `ubnt-flash-wp-sw.c`, `ui_hash.c`. License Proprietary, no DWARF.

### procfs

All under `/proc/ubnthal/`. File list is exact (strings + `proc_*_fops`); line formats from `seq_printf` format strings, order not verified.

| path | rw | readers | tag | shim |
|---|---|---|---|---|
| `system.info` | r | `libubnt` (keys `systemid`, `boardrevision`), `ubnt-systool` (`device.anonid`), `ubntnas`, `sfpd`, `libuled` | userland | yes |
| `board` | r | `ubnt_monitor`, `libubnt` (keys `systemid`?, `bomrev`, `cpuid`, `serialno`, `qrid`, `regdmn[0]`, `countrycode`; "Failed to identify the board" when no match) | userland | yes (superset of `system.info` + `boardid`, `bomrev`, `hwaddrbbase`) |
| `factory` | r | none found | unknown | yes, constant `false` (TODO format) |
| `status/IsDefault` | rw | `libubnt` (`/proc/ubnthal/status/IsDefault`) | userland | yes, RAM only |
| `status/IsLocated`, `IsIsolated`, `MeshStatus`, `ControllerHost`, `ControllerPort`, `AnonNetAppId` | rw | none found | unknown | yes, RAM only |
| `status/IsLte` | r | none found | unknown | yes, constant `false` |

- `system.info` keys (format strings): `cpu=%s`, `cpuid=%08x`, `flashSize=%lu`/`flashsize=%lu`, `ramsize=%lu`, `vendorid=%04x`, `systemid=%04x`, `lcm_count=%d`, `shortname=%s`, `name=%s`, `boardrevision=%d`, `serialno=%02x*6`, `manufid=%04x`, `mfgweek=%04d%02d`, `qrid=%s`, `ethN.macaddr=%02x:..`, `btN.macaddr`, `wifiN.mode=11`, `regdmn[]=`, `device.hashid=%02x*8`, `device.anonid=<uuid, 4xxx-8xxx shape>`.
- `board` keys: `format=%04x`, `version=%04x`, `boardid=%04x`, `vendorid=%04x`, `bomrev=%08x`, `hwaddrbbase=<mac>`, `EthMACAddrCount`, `WiFiMACAddrCount`, `BtMACAddrCount`, `regdmn[%d]=%04x`, `ethN.macaddr`, `cpu_rev_id=%08x`.
- Which key lives in which file is TODO (the shim emits `system.info` keys in both). Shim omits `cpu`, `lcm_count`, `manufid`, `mfgweek`, `qrid`, `btN`, `wifiN`, `regdmn`, `format`, `version`, `*MACAddrCount` (EEPROM offsets unknown).
- Booleans print `true`/`false`; `MeshStatus` values `init wds eth down adopt` (+ `unknown`). Write formats TODO.
- Flash write protect (`proc_flash_control`, DT node `ubnthal,write-protect`, props `default-write-policy`, `label`, `write-policy`, `explicit-write-policy`, buses `bus`/`mtd`/`disk`): proc entry name unknown (only `.uf` string near it). Needs vendor kernel hooks (below). [ignore: no flash writes through it]

### Identity sources (shim)

- Vendor reads the NOR MTD partition labelled `EEPROM` via `ubnt_mtd_partition_read("EEPROM", buf, 0x10000, &pos)`, then 8 bytes at `pos=0xe040`, and indexes `base+0x1150`. Our DT already has that partition (`dts/alpine-v2-ubnt-unvr-ea16.dts`, `partition@1f0000`, label `EEPROM`).
- Offsets used by the shim (docs/nand-1.3.35.md map, 2026 hardware read): MAC `0x00` (6), board id `0x0c` (BE16, `0xea16`), `0x0e` (BE16, `0x0777`, treated as vendorid), `0x10` (BE32, `0x000b101d`, treated as bomrev). TODO: endianness, that `0x0e` is vendorid, and every other field (serial/qrid/regdmn/mfg) unverified.
- `serialno` = base MAC (vendor prints 6 hex bytes; assumed the MAC). TODO.
- `ethN.macaddr`: shim = base MAC + N for N<2 (`enp0s1`/`enp0s2` are `...a8:11`/`...a8:12`, docs/gpio-map.md era notes). TODO vendor derivation.
- `flashsize`: vendor returns hardcoded `0x2000000`; shim module param default 32 MiB. `ramsize`: vendor rounds `si_meminfo` down to 1 GiB; shim rounds **up** (our totalram excludes reserved) TODO.
- `cpuid`: vendor reads SoC registers (`alpine_get_cpuid`); shim = module param, default 0. TODO source (U-Boot reads `chip_id`@0xfd8a815c = 0x00010000, docs/gpio-map.md).
- `boardrevision`: vendor derivation unknown; shim module param default 1 (value `unifi-unvr-emu` uses to satisfy Protect). TODO.
- `device.hashid`/`device.anonid`: vendor = own sha256 (`ui_sha256_hash`, salt string `5edfacbf` nearby, inputs unknown). Shim derives its own `sha256(mac || tag)` in the same shapes. TODO whether anything validates them against Ubiquiti cloud.
- Board table in the `.ko` (names from strings, boardids not decoded): `UniFi-NVR-4`/`UNVR4` (x2, ifaces `enp0s1`,`enp0s2`), `Unifi-NVR-PRO`/`UNVRPRO`. Shim maps `ea16`,`ea1a` to UNVR4 and `ea20` to UNVRPRO from `unifi-unvr-emu` comments. TODO decode `ubnthal_boards` relocations.
- sysid/boardid for this box: `ea16` (cmdline `sysid=ea16 ubnthal.sysid=ea16` in our DT bootargs; the `ubnthal.sysid` module parameter is not seen in the vendor `.ko` strings, TODO).

### Exported symbols (EXPORT_SYMBOL, all; no in-kernel user besides the vendor modules)

Prototypes unknown (no DWARF); all tagged **load** only if another vendor module is loaded (none are). Shim exports the six marked `S` with guessed scalar prototypes.

`ubnthal_get_`: `systemid` S, `vendorid` S, `boardrevision` S, `bomrev` S, `flashsize` S, `ramsize` S, `cputype`, `serialno`, `eth_macaddr`, `eth_mac_info`, `lcm_count`, `led_count`, `led_prefix`, `fan_count`, `switch_cpu_port`, `num_poe_ports`, `reserved_power`, `max_poe_power`, `guid`, `dev_anonid`, `dev_hashid`, `isDefault`, `isLocated`, `isIsolated`, `isLte`, `mesh_status`, `anon_net_app_id`, `controller_host`, `controller_port`.
`ubnthal_set_`: `isDefault`, `isLocated`, `isIsolated`, `mesh_status`.
`ubnthal_log_to_phy_port_id`, `ubnthal_phy_to_log_port_id`.
[ignore] for UNVR (switch/PoE/LCM functions are for other boards).

### Vendor kernel patches ubnthal needs (undefined in a stock kernel)

`ubnt_mtd_partition_read` (ubnt_common), `ubnt_set_disk_ro`, `ubnt_set_part_ro`, `gendisk_callback_for_each`, `mtd_callback_for_each`, `disk_block_events`/`disk_unblock_events`. Flash-WP plumbing. [ignore: shim has no WP]

## ubnt_common

Sources: `ubnt-file.c`, `ui-loglevel.c`, `ubnt-mtd.c`, `string_lib.c`, `ubnt_procfs.c`. License Proprietary.

| item | tag | shim |
|---|---|---|
| `ubnt_mtd_partition_read(const char *label, void *buf, size_t len, loff_t *pos)` -> 0 / -errno, `*pos` advances (derived from the call site in `alpine_scan_eeprom`) | load (ubnthal) | yes, via `get_mtd_device_nm` + `mtd_read` |
| `int ui_loglevel` | load | yes, exported, unused |
| `/proc/<dir>/loglevel` (`ui-loglevel.c`; dir name not found, built from `proc_mkdir`; read/write int) | unknown | no. TODO dir name |
| `ubnt_file_read`, `ubnt_file_write`, `ubnt_freadline`, `ubnt_mtd_get_dev`, `ubnt_mtd_get_devid`, `string_{printf,vprintf,aprintf,vaprintf,bind_a,bind_w,reset,truncate,terminate}`, `ubnt_procfs_{init,deinit,parse_boolean,show_boolean,write_boolean}` (last five not exported) | load (other vendor modules only) | no |

- Needs `/proc/mtd` format `mtd%d: %*x %*x "%n%*s"` and `/dev/mtdblock%d` internally. [ignore]
- `CONFIG_MTD=m` in the perf kernel: the shim depends on `mtd.ko`.

## ui-hdd-pwrctl

v2 driver `ubnt-hdd-pwrctl-v2.c` + `sysfs-slot.c`, `sysfs-platform.c`, `hdd-detect.c`, `platform-dev-link.c`. License GPL v2 (authors Matt Hsu, Ken Lu). Module name `ui_hdd_pwrctl`, driver name `ui-hdd-pwrctl`.

### DT and module interface

- Compatible `ui,hdd-pwrctl`; child nodes with `slot-no` (required, unique), gpios `pwren`, `present`, `fault-led`, `pwrfault`; props `pwren-delay`, `pwren-delay-every`, `force-power-on-wa` (gpio). Our DT (`hdd-pwrctl` node) already uses the same shape. [ignore: ours is separate]
- Module param `pwr_default_on` (uint, "Turn on the disk power by default"). [ignore; shim keeps the name]
- Needs built-in symbol `ui_hdd_pwrctl_blink_set` (vendor LED code). [ignore]
- Exports `hdd_poweroff_notifier`. [ignore]
- Registers a reboot notifier that force-powers-off slots on shutdown ("Skip powering off the disks on shutdown" log). [ignore: we keep power on]

### sysfs

- Per slot device `slot-%u` (parent/location TODO; the module creates a sysfs link, `platform_dev_create_link`) with files `present`, `fault`, `force_power`, `led_state`. `present` is also writable in the vendor driver ("set present to %d"). [userland: `uscsi_disk_ready` waits for `force_power=1` on every slot with `present=1`; `ustd` slot discovery inside compressed Cython strings, unread]
- Platform device files `power_cnt`, `fault_cnt`, `present_cnt`. Value format unknown (v1 printed a slot-index list; shim prints counts). [unknown]
- uevent: `KOBJ_CHANGE` on presence/insert/remove ("inserted"/"removed"). `99-ustd.rules`: `SUBSYSTEM=="platform", ACTION=="change", DRIVER=="ui-hdd-pwrctl"` runs `uhardware block prob`. [userland: DRIVER must equal `ui-hdd-pwrctl`]
- Block hotplug rules `sd*[!0-9]|mmcblk1|nvme*` (`uhardware block prob`) need nothing from the modules. [ignore]

### Shim design

- Real driver `modules/ubnt_hdd_pwrctl` stays the only hardware owner (presence poll, fault LED class). It now exports `ubnt_hdd_pwrctl_{nbays,bay_info,set_fault,register_notifier,unregister_notifier}` (`ubnt_hdd_pwrctl.h`).
- `ui_hdd_pwrctl.ko` registers a platform driver and device both named `ui-hdd-pwrctl` (defers until `ubnt_hdd_pwrctl` is bound), creates `slot-N` children, forwards `present`/`fault`/`led_state` to those exports, re-emits the change uevent from its own device.
- `force_power`: stored in RAM only. Bay rails are `regulator-always-on` (docs/sata-bay-power-ordering.md), so nothing is gated. Default 1 (`pwr_default_on`).
- `led_state`: `on`/`off` only (vendor values unknown). `fault`/`led_state` both drive the amber fault LED. TODO what `led_state` does on the vendor side (blink?).
- `present` read-only (vendor allows override). TODO.

## sfpd (ubnt-sfp-handler role)

- Unit `sfpd.service`, binary `/usr/sbin/sfpd`, CLI `sfpd-cli`. [userland]
- Reads `/proc/ubnthal/system.info`, `/sys/devices/platform/sfp-slot-{3,4}/{present,tx_fault,rx_los}` and raw `/sys/class/gpio/gpio{480..482,488..490,508,510,511}/value` (via export). Slot numbers 3/4 suggest a bigger product's cage numbering; UNVR has one cage. [unknown: whether `sfpd` runs usefully or idles on UNVR]
- Our SFP is DT `sfp0` (`sff,sfp`, `CONFIG_SFP` is off in the vendor kernel, `al_eth` drives it). No shim yet (phase 2: `sfp-slot-N` platform devices over our sfp gpios, and sysfs gpio, `CONFIG_GPIO_SYSFS`, is deprecated in 7.4).
- Hook `usr/lib/ubnt/hooks/system/bootup-top/90-quirk-sfp-fix.sh`. [unknown, unread]

## Other userland touch points found (not modules)

- Fan: `ustd-poweroff-bottom` writes `uhardware config fan-standby` to `/sys/class/hwmon/hwmon0/pwm[0-9]` (or `hwmon0/device/pwm*`). Ours: ADT7475 hwmon (`unvr-fand`). [userland; hwmon index on 7.4 must be 0 or the lookup misses: TODO check]
- LEDs: `libuled` uses `/sys/class/leds/ulogo_ctrl`, `etherlight/`, `mcu0/`, `ui-ese-N-ulogo_ctrl`; `uled-bootup-check.service` scans `/sys/class/leds/`. Ours: `ulogo_blue`/`ulogo_white` LED-class names, no `ulogo_ctrl`. [userland: TODO, shim needed]
- Temperature: `ubnt-systool` `do_cputemp` reads `/sys/class/thermal/thermal_zone*/temp`. Ours: `al_thermal` + `cpu-thermal` zone. [userland, covered]
- `ustorage`/`ubntstorage`/`ustd` are Cython with compressed strings: sysfs paths not extracted. `uscsi-disk-ready.service` is the one confirmed `force_power` reader.
- Upgrade/boot (`KERNELDEV=/dev/mtd9`, `/dev/mtd10`, USERDEV overlay): step 3 of #383, not this doc. See [firmware-5.1.25.md](firmware-5.1.25.md).

## Blocking unknowns for booting the 5.1.25 rootfs (#383 step 4)

- `ustd`/`uhardware` slot discovery paths and how it pairs `slot-N` with block devices (Cython strings compressed).
- `libuled` LED names vs our LED-class names.
- `modprobe-failsafe`, `ubnt-framework`, `ramoops`, other modules-load.d entries.
- EEPROM field map beyond MAC/boardid/vendorid/bomrev; `board` vs `system.info` key split.
- Kernel ABI 4.19 to 7.4 for the userland (cgroup v1/v2 etc.), not looked at.
- No shim has been loaded on the box (build only).
