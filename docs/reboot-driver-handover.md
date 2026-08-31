# Handover — `al_reboot` driver (fixes the reboot hang, #51)

Status: **written, NOT built or tested.** Everything to finish it is below.

## What / why

Linux `reboot` on woomera hangs at "Restarting system" (#51) because mainline 7.1
has **no restart handler** for the AL-324. The stock mechanism is the **SP805
watchdog** (`wdt0` @ `0xfd88c000`): arm it for an immediate reset and the SoC resets
to U-Boot. We **validated this works** manually — `scripts/reboot-to-uboot.tcl` armed
`/dev/watchdog` and the box reset cleanly to U-Boot with no power-cycle.

`al_reboot` registers a proper kernel **restart handler** that pokes the SP805, so
`reboot`/`shutdown -r`/`systemctl reboot` Just Work.

## Reset mechanism — RE-confirmed (2026-08-18)

Deep source/disassembly dive (2 agents). **SP805 watchdog is THE canonical AL-324
whole-SoC reset** — same registers stock U-Boot, stock Linux, and we all use.

- **Canonical reset = SP805 `wdt0` @ `0xfd88c000`:** unlock `WDTLOCK(0xC00)=0x1ACCE551`,
  `WDTLOAD(0x000)=1`, `WDTCONTROL(0x008)=INT_ENABLE|RESET_ENABLE`, spin. Used by:
  - stock U-Boot `reset_cpu()` — GPL `board/annapurna-labs/common/al_board.c:190`.
  - stock Linux `alpine_wdt_restart()` — GPL `drivers/power/reset/alpine-reboot.c:40`
    (author Hani Ayoub @ Annapurna Labs), hooked via `arm_pm_restart` (:70).
  - our `modules/al_reboot/al_reboot.c` (this driver) and our U-Boot `reset_cpu()`
    (`uboot-port/board/annapurna/alpine/alpine.c`).
- **PSCI `SYSTEM_RESET` (0x84000009) is NON-functional — do NOT use `SYSRESET_PSCI`.**
  Proof is vendor code: Annapurna's own kernel **overrides** the auto-registered PSCI
  restart with the watchdog poke (they wouldn't if PSCI reset worked). PSCI `VERSION`
  + `CPU_ON` DO work (4 cores boot via PSCI). #51 hang = mainline 7.1 fires the
  SYSTEM_RESET `smc`, the monitor no-ops it, and with no SP805 fallback it halts.
- **Why the handler can't be read:** the resident EL3 monitor is installed by the
  **boot ROM / preboot secure firmware**, NOT U-Boot. On this box U-Boot runs at **EL2**
  (dmesg "All CPU(s) started at EL2"), its EL3-install path never executes, its `vbar`
  is the stock panic vectors, and there are **0 `smc` instructions** in the 674 KB
  U-Boot image. The monitor is absent from every recovered blob (U-Boot, al_boot, S2),
  so SYSTEM_RESET's code is unreadable — but the vendor bypass is dispositive.
- **`fabric_software_reset` @ `0xf007003c` is a dead end** (the "cleaner reset" candidate):
  it's a **sub-block** reset — `LEVEL` field 0=fabric/1=GIC/2=SMMU, not the SoC. Present
  on V2 (nb-service `0xf0070000`, same offset as V3) but **no code anywhere writes it**.
  `cpus_software_reset` (`0xf0070024`) is per-core SMP bring-up only. No Alpine generation
  (V1/V3/V4) uses anything but SP805 for whole-SoC reset.

## Files

- **`/mnt/2tb/unvr-port-refs/linux-alpine-v2/modules/al_reboot/al_reboot.c`** — the driver.
- **`.../al_reboot/Makefile`** — OOT build.

Ported from the Ubiquiti GPL `linux-arm64-unvr-4.1.37-ubnt/drivers/power/reset/alpine-reboot.c`.
**Only port change:** the stock driver set `arm_pm_restart = ...` (that hook was removed after
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
`scripts/build-linux-fedora.py` (the `for m in ("al_eth","al_dma","al_ssm","al_sgpo","al_thermal")`
loop → add `"al_reboot"`), so it ships in the module tree and autoloads. Then a NAND
reflash carries it. (Or build it in via `CONFIG_POWER_RESET_ALPINE` if we import the
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
  (the manual SP805 reset we validated), the Ubiquiti GPL `alpine-reboot.c` source cited above.
