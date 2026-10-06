---
name: no-plain-reboot-use-watchdog
description: "woomera plain `reboot` WORKS as of 2026-09-03 (#51 closed); SP805 is still the only whole-SoC reset, PSCI SYSTEM_RESET is dead"
metadata:
  node_type: memory
  type: reference
  originSessionId: 0edb0769-b7b5-4d90-bc3a-84dd99588559
  modified: 2026-09-03T21:14:18.320Z
---

**Plain Linux `reboot` on woomera WORKS as of 2026-09-03** (#51 closed, verified: back at a login prompt in ~50 s, uptime confirming a real restart). Two things were needed, both now in tree:

1. **`al_reboot` must actually be built.** `modules/al_reboot/` existed for weeks but was never in the OOT module list in `scripts/build-linux-fedora.py`, so the .ko was never produced or shipped — the box's `extra/` had every other `al_*` and not this one. Check `lsmod | grep al_reboot` before believing any reboot claim.
2. **It must outrank PSCI.** Our DT declares `arm,psci-0.2` (`/proc/device-tree/psci/compatible`), so the kernel registers `psci_sys_reset_nb` at **notifier priority 129**. `devm_register_restart_handler()` uses `SYS_OFF_PRIO_DEFAULT` = **0** and would never run. `al_reboot` now calls `devm_register_sys_off_handler(..., SYS_OFF_MODE_RESTART, SYS_OFF_PRIO_HIGH, ...)` (192).

**Still true: the AL-324 has no working whole-SoC reset except the SP805 watchdog.**
- **PSCI `SYSTEM_RESET` (0x84000009) is non-functional** — the resident EL3 monitor no-ops the `smc`. PSCI `VERSION`/`CPU_ON` DO work (cores boot). Don't use `CONFIG_SYSRESET_PSCI` in our U-Boot.
- `fabric_software_reset` @`0xf007003c` is a **sub-block** reset (fabric/GIC/SMMU), not the SoC — dead end.
- **Canonical reset = SP805 `wdt0` @`0xfd88c000`:** unlock `WDTLOCK(0xC00)=0x1ACCE551`, `WDTLOAD(0x000)=1`, `WDTCONTROL(0x008)=INT|RESET`, spin. Used by stock U-Boot `reset_cpu` (`al_board.c:190`), stock Linux `alpine-reboot.c:40`, our `al_reboot`, and our U-Boot `reset_cpu()`. Writeup: docs/reboot-driver-handover.md.

**The watchdog trick is now for a WEDGED box, not for a normal reboot:**
```
python3 -c "import fcntl,struct; f=open('/dev/watchdog','r+b',buffering=0); fcntl.ioctl(f,0xC0045706,struct.pack('I',1)); exec('while True: pass')"
```
Arms 1 s + busy-holds the fd → SoC reset in ~2 s. To catch stock U-Boot after reset, spam ESC and expect `ALPINE_UBNT_NAS_ALL>`.

**But that path is unreliable exactly when needed (#106):** it runs SSH → shell → python3 → `/dev/watchdog` → ioctl, and most of our Oopses are in the *network* driver — so the recovery shares a dependency with what crashed. Taint is a marker, not a mechanism. Out-of-band fallback that depends on nothing inside the box: `./dev.py power-cycle` (HA outlet, [[unvr-power-cycle-via-hass]]).

Testing: [[box-testing-over-ssh]], [[test-every-change-never-assert]].
