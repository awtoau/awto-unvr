# SATA bay power ordering (#104)

## Problem

AHCI probe had no formal dependency it could wait on for drive-bay power. Bay
power-enable (PCA9575 @0x21, i2c-0) was a plain DTB `gpio-hog` — fires at
gpiochip probe time, no ordering guarantee relative to AHCI's own probe. Also
blocked `i2c-designware` from ever being a loadable module (the hog needs the
gpiochip up before anything else touches those lines).

## Fix: regulator-fixed + PCI *-supply stubs

Bay pwren lines 0-3 are now 4× `regulator-fixed` nodes instead of a hog:

```
reg_hdd_bay1  gpios = <&i2c_gpio1 0 GPIO_ACTIVE_HIGH>
reg_hdd_bay2  gpios = <&i2c_gpio1 1 GPIO_ACTIVE_HIGH>
reg_hdd_bay3  gpios = <&i2c_gpio1 2 GPIO_ACTIVE_HIGH>
reg_hdd_bay4  gpios = <&i2c_gpio1 3 GPIO_ACTIVE_HIGH>
```
All `regulator-boot-on` + `regulator-always-on` (see Limitations below).

Two DT stub nodes, matched to the AHCI PCI endpoints by devfn
(`pci_set_of_node`) - stock `ahci.c` doesn't read anything from them, they
exist purely so `fw_devlink` gates AHCI probe on the regulators:

```
sata@8,0 { reg = <0x4000 ...>; port0-target-supply = <&reg_hdd_bay3>; port2-target-supply = <&reg_hdd_bay4>; }
sata@9,0 { reg = <0x4800 ...>; port0-target-supply = <&reg_hdd_bay2>; port2-target-supply = <&reg_hdd_bay1>; }
```

Port-to-bay mapping is carried over unchanged from the existing `sata-leds`
node (`dts/alpine-v2-ubnt-unvr-ea16.dts`), not re-derived here.

## Status

Landed in `dts/alpine-v2-ubnt-unvr-ea16.dts` on `console-tcl-tooling`
(originally #104, recovered from `git stash` - see the issue for that
history). **Hardware-verified** (2026-08-23, real boot with 2× 7.3T drives
connected):
- `/sys/bus/pci/devices/0000:00:0{8,9}.0/of_node` correctly symlink to
  `sata@8,0`/`sata@9,0` - the PCI-to-DT-stub matching works.
- All 4 regulators registered + `state: enabled`
  (`/sys/class/regulator/regulator.{1,2,3,4}`, names `hdd-bay1-4-pwr`).
- `/sys/kernel/debug/devices_deferred` empty - nothing stuck waiting.
- Both bay drives detected correctly at their real sizes.

AHCI's own probe happens very early (~2.8s) since the regulator dependency
chain (i2c-0 -> pca9575 @0x21 -> gpio -> regulator) resolves in well under a
second - no probe deferral was ever needed in this boot, which is the
correct/expected outcome, not evidence the dependency isn't wired up (the
OF-node + regulator-state checks above confirm it independently of timing).

## Limitations

`regulator-always-on` means bay power still isn't runtime-gateable -
`hdd-manage.py` can only `hdparm -Y` spin-down, same as before this change.
Power-gating an unused bay means dropping `regulator-always-on` on that rail
and giving it a real consumer instead of just the AHCI-ordering stub (risk:
mis-gate a live bay). Tracked as a follow-up, not part of #104's scope.

## Unused AHCI ports masked off

Each of the two AHCI PCI controllers (`0000:00:08.0`/`0000:00:09.0`) implements
4 ports (`PI` register = 0xf) but only 2 per controller are actually wired to a
bay (`port0`/`port2` - matches the `sata@8,0`/`sata@9,0` `*-target-supply`
mapping above). Ports 1/3 on each showed `SATA link down` every boot - real,
persistent hardware, not transient. `CONFIG_SATA_AHCI=y` is built-in (no
modprobe path), so masking needs a kernel boot arg, not a modprobe option or
the DT `ports-implemented` property (that's platform-driver-only, never read
by the PCI `ahci.c` probe path our board's internal-PCIe AHCI uses).

Added `ahci.mask_port_map=0x5` (bits 0,2) to `CONFIG_BOOTARGS`
(`uboot-port/configs/alpine_v2_unvr_defconfig`) and live U-Boot env
(`setenv bootargs ...; saveenv`, both updated to match). Verified on
hardware: both controllers now report `2/4 ports implemented (port mask
0x5)`, the two dead ports show as `ata2`/`ata4`/`ata6`/`ata8: DUMMY`
(instantly skipped, no probe attempt), and all 3 real drives (2 bay HDDs +
boot SSD) still detected correctly via `ata1`/`ata3`/`ata7`.
