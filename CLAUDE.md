# awto-unvr — project rules

Ubiquiti UNVR (Annapurna Labs Alpine V2, AL-324, sysid `ea16`), hostname **woomera**.
Mainline Linux + our own U-Boot + out-of-tree `al_*` drivers.

Global rules: `/home/dan/.claude/CLAUDE.md`. This file is the project layer on top.

## Hard "do not" list — every one of these has cost hours

- ~~NEVER scan i2c~~ — **fixed 2026-09-04 (#86).** Scanning is safe on a kernel whose DTB
  carries `snps,no-enable-abort`; the bus also runs at 400 kHz now. On any *older* build the
  old rule still applies: a ch0 scan wedges the controller, recoverable only by a **cold power
  cycle**, and it drops the 10G link. `docs/rtc-s35390a-fault.md`.
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

## Watching the console

- `./dev.py console` starts `tio` (socket + log). One instance owns the port;
  everything else attaches to the socket.
- **`./dev.py console-attach` to watch live** — same socket the scripts drive, so you
  see exactly what they see. Ctrl-C detaches; `tio` survives.
- `./dev.py console-peek [-n N]` for a bounded tail without attaching.
- Log: `tmp/logs/unvr-console.log`, rolled at 20 MB (`.log.1`, `.log.2`…). A run that
  spans a roll needs both files — `cat tmp/logs/unvr-console.log.1 tmp/logs/unvr-console.log`.
- Timestamps in the log lag the command; use `time` on the box for real durations.

## Reaching the box

- `./dev.py ssh [-- cmd]` — resolves by **MAC**, never a hardcoded IP. DHCP has moved
  this box across .129/.136/.140. All four NICs (both al_eth, both USB) are in
  `scripts/_box.py`, so it finds the box even when only a USB NIC is up — no IP
  fallback needed. A guard test fails on any IP/MAC literal elsewhere (#260).
- If it reports "No route to host": `ping -c1 <ip>` to re-ARP, then retry. Both al_eth
  ports are on one subnet, so neighbour entries go stale (#170).

### All four NICs are on ONE subnet — this invalidates the obvious measurement (#170)

The box has 4 NICs (`enp0s1` 1G, `enp0s2` 10G, two USB r8152) all on
`192.168.25.0/24`, all with DHCP leases. **Weak-host routing means the box replies
on whichever interface the ROUTE picks, not the one the address belongs to.** It has
cost this project three wrong conclusions in one day. Before concluding anything
about a port:

- **`ip neigh show <ip>` and CHECK THE MAC.** `.133` is `enp0s1`'s address but answers
  from `74:ac:b9:41:a8:12` — the **10G** MAC. An IP that pings proves nothing about
  which port carried it.
- **iperf3 needs `--bind-dev <iface>`. `-B <addr>` is NOT enough** — it selects a
  source address, not an egress port. `-B` on the 10G address measured **94 Mb/s**
  because the traffic left via a USB NIC.
- **`tcpdump -e 'ether host <mac>'` seeing nothing does NOT mean the port is dead.**
  Zero frames from the 1G MAC is the *expected* result of weak-host routing. Confirm
  with the port's own counters (`ip -s link show <iface>`) and its own neighbour table
  (`ip neigh show dev <iface>`) before calling a link broken.
- Per-interface MACs are in `scripts/_box.py`; `./dev.py ssh` resolves by MAC, so it
  finds the box regardless of which port answers.
- Recovery if it will not boot: `<Esc><Esc>` at stock, then netboot - stock is the
  only stage with working networking. **`run bootnand` is NOT a recovery path**: the
  NAND recovery image at `0x300000` is not genuine stock firmware and does not boot
  (#166). Pristine MTD backups: `git_debris/woomera-mtd/...-164356/`.

## Verifying, not assuming

- **`./dev.py gate` before calling anything done.** It runs fmt, lint, pytest, `hal-drift`
  (3 vendored HAL copies, #218) and `dt-drift` (board facts shared by the Linux and U-Boot
  DTs, #221).
- **A config symbol is not a working driver.** `CONFIG_RTC_DRV_S35390A=y` did nothing
  because the DTS node is `status="disabled"` (#232). `CONFIG_EDAC_AL_MC=m` bound nothing
  because our compatible string did not match the driver's. Check it *bound*, on the box.
- **Device letters are not stable.** `/dev/sdX` shifts with probe order across boots.
- **`hdparm -t` understates by ~4x.** Use `fio` with `--direct=1`.
- **An IP that answers does not tell you which NIC answered.** All 4 ports share one
  subnet; check the MAC in `ip neigh`. See the #170 block above — it has produced
  three wrong conclusions (a 94 Mb/s "10G" reading, a phantom TX asymmetry, and a
  "1G is on an unreachable segment" that was simply weak-host routing).
- **A true observation is not a conclusion.** `tcpdump` really did show zero frames
  from the 1G MAC; the port was fine. State what was measured, then what it does and
  does not establish.
- **Console timestamps lag the command.** Use `time` on the box for real durations — I
  read 250 MB/s off timestamps where `time` said 43.

## Where things live

- `dts/alpine-v2-ubnt-unvr-ea16.dts` — Linux DT, built to `/boot/unvr.dtb` on the SSD.
- `uboot-port/arch/dts/awto-alpine-v2-unvr-uboot.dts` — compiled **into** `u-boot.bin`.
- `dts/reference/`, `docs/hw-reference/*/live.dts` — vendor hardware-of-record, never built.
- Reference trees (20 sources, ~9.9 GB) under `/mnt/2tb/unvr-port-refs/` — QNAP, MikroTik
  CCR2004 UEFI, the UBNT GPL drop with the **working 4.1.37 kernel**, delroth's HAL.
  See `docs/reference-sources.md`. Search these before reverse-engineering anything.

## Filing

Findings go in a GitHub issue **when found**, with evidence — chat is not a record.
The repo is public and awtoau-owned: scrub private paths, then confirm before posting.
