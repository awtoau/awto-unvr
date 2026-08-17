# Handover — `al_reboot` driver (fixes the reboot hang, #51)

Status: **written, NOT built or tested.** Everything to finish it is below.

## What / why

Linux `reboot` on woomera hangs at "Restarting system" (#51) because mainline 7.1
has **no restart handler** for the AL-324. The vendor's mechanism is the **SP805
watchdog** (`wdt0` @ `0xfd88c000`): arm it for an immediate reset and the SoC resets
to U-Boot. We **validated this works** manually — `scripts/reboot-to-uboot.tcl` armed
`/dev/watchdog` and the box reset cleanly to U-Boot with no power-cycle.

`al_reboot` registers a proper kernel **restart handler** that pokes the SP805, so
`reboot`/`shutdown -r`/`systemctl reboot` Just Work.

## Files

- **`/mnt/2tb/unvr-port-refs/linux-alpine-v2/modules/al_reboot/al_reboot.c`** — the driver.
- **`.../al_reboot/Makefile`** — OOT build.

Ported from the vendor GPL `linux-arm64-unvr-4.1.37-ubnt/drivers/power/reset/alpine-reboot.c`.
**Only port change:** the vendor set `arm_pm_restart = ...` (that hook was removed after
kernel 5.x) → we use `register_restart_handler()` with a `notifier_block`. SP805 register
sequence is identical: unlock `WDTLOCK=0x1ACCE551`, `WDTLOAD=1`, `WDTCONTROL=INT|RESET`.

Binds the DT node **`annapurna-labs,alpine-reboot`** (`wdt-parent = <&wdt0>`), which the
ea16 DTS **already declares** — so no DTS change is needed; the module just binds on load.

## Build (on the host)

```
cd /mnt/2tb/unvr-port-refs/linux-alpine-v2/modules/al_reboot
make            # -> al_reboot.ko (aarch64, matches the 7.1.8-dirty kernel on woomera)
```
(Uses `KDIR=/mnt/2tb/unvr-port-refs/linux-v7.1.8` + `aarch64-linux-gnu-`.)

## Deploy + test (woomera)

woomera runs Fedora with sshd (root/`unvr`) on its LAN IP (DHCP lease — find it on the
box or via the router; it is not the U-Boot `ipaddr`/tftp address). Either scp or push the
`.ko` over the serial console (base64), then:

```
insmod al_reboot.ko            # expect dmesg: "AL-324 SP805 restart handler registered"
reboot                         # should reset via SP805 -> boot back to Fedora (NAND kernel)
```

**Success** = the box resets promptly (no "Restarting system" hang) and comes back on its
own. **If it does NOT reset**, the handler's `mdelay(500)` falls through and reboot behaves
as before (hang) — no worse than today; recover with `reboot-to-uboot.tcl` or a power-cycle.

## Make it permanent

Once proven, add `al_reboot` to the OOT list in
`scripts/build-linux-71-fedora.py` (the `for m in ("al_eth","al_dma","al_ssm","al_sgpo")`
loop → add `"al_reboot"`), so it ships in the module tree and autoloads. Then a NAND
reflash carries it. (Or build it in via `CONFIG_POWER_RESET_ALPINE` if we vendor the
source into the kernel tree.)

## Watch-outs

- The module must **probe** to register the handler — that needs the `annapurna-labs,
  alpine-reboot` node present in the running DTB (it is, in our ea16 DTB).
- `.remove` uses the **void** signature (kernel ≥6.11). If building against an older tree,
  switch to `int` return.
- This is the **clean `reboot`** fix (#51). The separate **early-watchdog / crash
  auto-reboot** idea (#60) is a different feature (a petting daemon + always-on hardware
  watchdog so a *crash*, not a clean reboot, self-heals) — not this driver.

## Related

- #51 (reboot hang — this fixes it), #60 (early watchdog), `scripts/reboot-to-uboot.tcl`
  (the manual SP805 reset we validated), the vendor `alpine-reboot.c` source cited above.
