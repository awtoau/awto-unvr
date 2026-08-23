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
history). **Not hardware-tested** - no boot-with-drives run has confirmed
AHCI actually defers on the supply the way `fw_devlink` is expected to.

## Limitations

`regulator-always-on` means bay power still isn't runtime-gateable -
`hdd-manage.py` can only `hdparm -Y` spin-down, same as before this change.
Power-gating an unused bay means dropping `regulator-always-on` on that rail
and giving it a real consumer instead of just the AHCI-ordering stub (risk:
mis-gate a live bay). Tracked as a follow-up, not part of #104's scope.
