# awto-unvr — project rules

Ubiquiti UNVR (Annapurna Labs Alpine V2, AL-324, sysid `ea16`), hostname **woomera**.
Mainline Linux + our own U-Boot + out-of-tree `al_*` drivers.

Global rules: `/home/dan/.claude/CLAUDE.md`. This file is the project layer on top.

**House LAN IPs come from the registry:** `awto-dan` repo (private, clone alongside this one)
-> `docs/network/ip-registry.toml`, rules in `docs/network/README.md`. Use woomera's `[[block]]`
(`.40-.47`); never ping-scan for a free IP, never static inside the DHCP pool `.100-.179`.
Static values: `/24`, gateway/DNS `.254`, domain `dnt.com.au`. Multi-NIC-on-one-subnet
weak-host/ARP-flux notes: `docs/agent-memory/four-nics-one-subnet-weak-host.md`.

## Hard "do not" list — every one of these has cost hours

- ~~NEVER scan i2c~~ — **fixed 2026-09-04 (#86).** Scanning is safe on a kernel whose DTB
  carries `snps,no-enable-abort`; the bus also runs at 400 kHz now. On any *older* build the
  old rule still applies: a ch0 scan wedges the controller, recoverable only by a **cold power
  cycle**, and it drops the 10G link. `docs/rtc-s35390a-fault.md`.
  - `i2cget 0x30` is an SMBus READ_BYTE and is NOT what the RTC driver does. The
    driver-shaped test is `i2ctransfer -y <bus> r1@0x30`.
- **NEVER run `./dev.py flash` casually.** NAND `0x1300000` holds awto-uboot since #216;
  flashing a kernel there destroys the bootloader. It refuses without an explicit flag.
- **Flash writes need NO permission — the box is ours to flash (Dan, 2026-10-14).**
  Don't ask; do it. The care is in the target, not the asking:
  - Address partitions by label, re-read `/proc/mtd` / the offset first — mtd numbers moved when
    the NAND driver landed.
  - Size + CRC-check the image before erasing (`flash-uboot` does). A wrong partition has no
    recovery path (#166).
- **NEVER `ext4write` (or any U-Boot filesystem write) to the Fedora root partition.**
  U-Boot reads it fine but cannot write Fedora's ext4 (`metadata_csum`): a *failed* write
  on 2026-10-14 still corrupted the superblock and the box would not mount root (#347).
  No network? Boot Linux with the bad module blacklisted and use `scripts/console-put.py`.
- **Never kill or restart a `tio` you did not start** — it is the user's console.
- **Never `pkill` Chrome** (shared, and it loses the cleared Turnstile session).
- **NEVER re-test snoop/SMCC/axcache/ROB/cache-maintenance/FLR for the U-Boot eth TX
  failure (#253).** Read `docs/eth-uboot-tx-ruled-out.md` first. Snoop alone has been
  tested **5 times** across 3 sessions; three commits each claimed to be "THE
  TX-completion fix" (`92f5706` cache, `092fcde` FLR, `d69b711` snoop) and all failed.
  The 1G and 10G program byte-identical values in that whole register group and one
  works, so it cannot be the discriminator. Untested leads are listed in that file —
  start with where `crhp` is written vs where the completion ring was allocated.

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
`192.168.25.0/24`, all with DHCP leases. Linux's default weak-host behaviour made
the box reply on whichever interface the ROUTE picked, not the one owning the
address — three wrong conclusions in one day.

**FIXED 2026-10-10: `scripts/fix-multihome-routing.py --persist`.** Strict ARP
(`arp_ignore=1`, `arp_announce=2`) plus one route table per NIC with
`ip rule from <addr>`. Persisted via
`/etc/NetworkManager/dispatcher.d/90-awto-multihome`, re-applied on every
interface up, so a DHCP renew or link flap cannot undo it. Verified: `.133`
answers from `a8:11` (1G), `.127` from `a8:12` (10G) — each address on its own
port. `-B <addr>` now selects the right port and read 6.64 Gbit/s on the 10G.

On a box WITHOUT that fix applied, all of the following still holds:

- **`ip neigh show <ip>` and CHECK THE MAC.** An IP that pings proves nothing
  about which port carried it.
- **iperf3 needs `--bind-dev <iface>`; `-B <addr>` is not enough** — it selects a
  source address, not an egress port. `-B` on the 10G address measured **94 Mb/s**
  because the traffic left via a USB NIC. `--bind-dev` is still the safer habit:
  it constrains the port directly rather than relying on the rules being in place.
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
- `modules/al_hal/` — the one copy of every HAL file two or more modules share; the modules
  link to it (`modules/<m>/<f> -> ../al_hal/<f>`). Fix a shared file once. A same-named regular
  file in another module is a fork and needs a reason in `scripts/hal-forks.json`
  (`./dev.py hal-drift`).
- `dts/reference/`, `docs/hw-reference/*/live.dts` — vendor hardware-of-record, never built.
- Reference trees (20 sources, ~9.9 GB) under `/mnt/2tb/unvr-port-refs/` — QNAP, MikroTik
  CCR2004 UEFI, the UBNT GPL drop with the **working 4.1.37 kernel**, delroth's HAL.
  See `docs/reference-sources.md`. Search these before reverse-engineering anything.

## Filing

Findings go in a GitHub issue **when found**, with evidence — chat is not a record.
The repo is public and awtoau-owned: scrub private paths, then confirm before posting.
