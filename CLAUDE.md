# awto-unvr — project rules

Ubiquiti UNVR (Annapurna Labs Alpine V2, AL-324, sysid `ea16`), hostname **woomera**.
Mainline Linux + our own U-Boot + out-of-tree `al_*` drivers.

Global rules: `/home/dan/.claude/CLAUDE.md`. This file is the project layer on top.

## Hard "do not" list — every one of these has cost hours

- **NEVER scan i2c.** No `i2cdetect`, no full-bus probe. Probing empty addresses on mux
  ch0 wedges the whole controller, **recoverable only by a COLD POWER CYCLE** — a warm
  reboot comes back still wedged. It drops the 10G link (SFP polling fails → "module
  removed"). `docs/rtc-s35390a-fault.md`, #86.
  - `i2cdetect` exiting 0 proves nothing; the wedge surfaces on the *next* access.
  - `i2cget 0x30` is an SMBus READ_BYTE and is NOT what the RTC driver does. The
    driver-shaped test is `i2ctransfer -y <bus> r1@0x30`.
- **NEVER run `./dev.py flash` casually.** NAND `0x1300000` holds awto-uboot since #216;
  flashing a kernel there destroys the bootloader. It refuses without an explicit flag.
- **Never kill or restart a `tio` you did not start** — it is the user's console.
- **Never `pkill` Chrome** (shared, and it loses the cleared Turnstile session).

## Boot chain (since #216)

```
stock U-Boot → NAND 0x1300000 (awto-uboot, raw, `go`) → ext4load /boot/uImage from SSD → Linux
```

- Deploy = `./dev.py publish-fedora` then `./dev.py deploy-ssd [--reboot]`. It is an scp,
  not a flash. `./dev.py flash` (NAND) is recovery-only.
- `./dev.py uboot-test --cold` reliably lands at the `awto-nas#` prompt. `catch-uboot.py`
  stops at **stock's** prompt, not ours — different tool, different job.
- The `awto-nas#` prompt is **not sticky**: its autoboot countdown continues, so a slow
  follow-up command lands in Linux instead.
- Stock's console is 115200 and cannot change (closed binary). Ours can, but a stale saved
  env in mtd3 overrides compiled `CONFIG_*` — `env default -a; saveenv` (#229).

## Reaching the box

- `./dev.py ssh [-- cmd]` — resolves by the **1G MAC**, never a hardcoded IP. DHCP has
  moved this box across .129/.136/.140.
- If it reports "No route to host": `ping -c1 <ip>` to re-ARP, then retry. Both al_eth
  ports are on one subnet, so neighbour entries go stale (#170).
- Fallback when al_eth is down: `root@192.168.25.100` (a USB NIC).
- Recovery if it will not boot: `<Esc><Esc>` at stock, then `run bootnand`.

## Verifying, not assuming

- **`./dev.py gate` before calling anything done.** It runs fmt, lint, pytest, `hal-drift`
  (4 vendored HAL copies, #218) and `dt-drift` (board facts shared by the Linux and U-Boot
  DTs, #221).
- **A config symbol is not a working driver.** `CONFIG_RTC_DRV_S35390A=y` did nothing
  because the DTS node is `status="disabled"` (#232). `CONFIG_EDAC_AL_MC=m` bound nothing
  because our compatible string did not match the driver's. Check it *bound*, on the box.
- **Device letters are not stable.** `/dev/sdX` shifts with probe order across boots.
- **`hdparm -t` understates by ~4x.** Use `fio` with `--direct=1`.
- **Console timestamps lag the command.** Use `time` on the box for real durations — I
  read 250 MB/s off timestamps where `time` said 43.

## Where things live

- `dts/alpine-v2-ubnt-unvr-ea16.dts` — Linux DT, built to `/boot/unvr.dtb` on the SSD.
- `uboot-port/arch/dts/awto-alpine-v2-unvr-uboot.dts` — compiled **into** `u-boot.bin`.
- `Platform/Ubiquiti/UNVR/DeviceTree/unvr.dts` — UEFI's, **referenced by nothing** (#39).
- `dts/reference/`, `docs/hw-reference/*/live.dts` — vendor hardware-of-record, never built.
- Reference trees (20 sources, ~9.9 GB) under `/mnt/2tb/unvr-port-refs/` — QNAP, MikroTik
  CCR2004 UEFI, the UBNT GPL drop with the **working 4.1.37 kernel**, delroth's HAL.
  See `docs/reference-sources.md`. Search these before reverse-engineering anything.

## Filing

Findings go in a GitHub issue **when found**, with evidence — chat is not a record.
The repo is public and awtoau-owned: scrub private paths, then confirm before posting.
