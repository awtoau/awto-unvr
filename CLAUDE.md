# awto-unvr — project rules

Ubiquiti UNVR (Annapurna Labs Alpine V2, AL-324, sysid `ea16`), hostname **woomera**. Mainline Linux + our U-Boot + out-of-tree `al_*` drivers. Public, awtoau-owned → scrub private paths, confirm before posting.

IPs, subnet, gateway/DNS, domain → `awto-dan/docs/network/ip-registry.toml` (woomera block). Multi-NIC-on-one-subnet weak-host/ARP-flux: `docs/agent-memory/four-nics-one-subnet-weak-host.md`.

## Hard "do not" — each has cost hours
- **i2c scan now safe** (fixed 2026-09-04 #86) on a kernel whose DTB carries `snps,no-enable-abort`; bus also runs 400 kHz now. On any *older* build: a ch0 scan wedges the controller, recoverable only by a **cold power cycle**, and drops the 10G link (`docs/rtc-s35390a-fault.md`).
  - `i2cget 0x30` is an SMBus READ_BYTE, NOT what the RTC driver does. Driver-shaped test: `i2ctransfer -y <bus> r1@0x30`.
- **NEVER `./dev.py flash` casually.** NAND `0x1300000` holds awto-uboot since #216; flashing a kernel there destroys the bootloader. Refuses without an explicit flag.
- **Flash writes need NO permission — the box is ours (Dan 2026-10-14).** Care is in the target:
  - Address partitions by label; re-read `/proc/mtd` / the offset first — mtd numbers moved when the NAND driver landed.
  - Size+CRC-check the image before erasing (`flash-uboot` does). A wrong partition has no recovery path (#166).
- **NEVER `ext4write` (or any U-Boot FS write) to the Fedora root.** U-Boot reads it fine but can't write Fedora's ext4 (`metadata_csum`): a *failed* write on 2026-10-14 still corrupted the superblock, box wouldn't mount root (#347). No network? Boot Linux with the bad module blacklisted, use `scripts/console-put.py`.
- **Never kill/restart a `tio` you didn't start** — it's the user's console.
- **NEVER re-test snoop/SMCC/axcache/ROB/cache-maintenance/FLR for the U-Boot eth TX failure (#253).** Read `docs/eth-uboot-tx-ruled-out.md` first. Snoop alone tested **5x** across 3 sessions; three commits each claimed "THE TX-completion fix" (`92f5706` cache, `092fcde` FLR, `d69b711` snoop), all failed. 1G and 10G program byte-identical values in that whole register group and one works → not the discriminator. Untested leads are in that file — start with where `crhp` is written vs where the completion ring was allocated.

## Boot chain (since #216)
```
stock U-Boot → NAND 0x1300000 (awto-uboot, raw, `go`) → ext4load /boot/uImage from SSD → Linux
```
- Deploy = `./dev.py publish-fedora` then `./dev.py deploy-ssd [--reboot]` — an scp, not a flash. `./dev.py flash` (NAND) is recovery-only.
- Kernel variant: `--kernel debug|perf` on build-fedora / publish-fedora / deploy-ssd. **debug is slow** (10G CPU-bound); perf = line rate. `docs/build.md`, #353.
- `./dev.py uboot-test --cold` reliably lands at `awto-nas#`. `catch-uboot.py` stops at **stock's** prompt, not ours.
- `awto-nas#` prompt is **not sticky**: its autoboot countdown continues, so a slow follow-up command lands in Linux.
- Stock console is 115200, can't change (closed binary). Ours can, but a stale saved env in mtd3 overrides compiled `CONFIG_*` — `env default -a; saveenv` (#229).

## Watching the console
- `./dev.py console` starts `tio` (socket + log). One instance owns the port; everything else attaches to the socket.
- **`./dev.py console-attach` to watch live** — same socket the scripts drive. Ctrl-C detaches; `tio` survives.
- `./dev.py console-peek [-n N]` = bounded tail without attaching.
- Log `tmp/logs/unvr-console.log`, rolled at 20 MB (`.log.1`, `.log.2`…). A run spanning a roll needs both: `cat tmp/logs/unvr-console.log.1 tmp/logs/unvr-console.log`.
- Console timestamps lag the command; use `time` on the box for real durations.

## Reaching the box
- `./dev.py ssh [-- cmd]` — resolves by **MAC**, never a hardcoded IP. DHCP has moved this box across .129/.136/.140. All four NICs (both al_eth, both USB) are in `scripts/_box.py`, so it finds the box even when only a USB NIC is up. A guard test fails on any IP/MAC literal elsewhere (#260).
- "No route to host": `ping -c1 <ip>` to re-ARP, then retry. Both al_eth ports on one subnet → neighbour entries go stale (#170).
- Recovery if it won't boot: `<Esc><Esc>` at stock, then netboot — stock is the only stage with working networking. **`run bootnand` is NOT a recovery path**: the NAND recovery image at `0x300000` isn't genuine stock firmware and doesn't boot (#166). Pristine MTD backups: `git_debris/woomera-mtd/...-164356/`.

### All four NICs on ONE subnet (#170)
4 NICs (`enp0s1` 1G, `enp0s2` 10G, two USB r8152) all on `192.168.25.0/24`, all with DHCP leases. Linux weak-host made the box reply on whichever interface the ROUTE picked, not the one owning the address — three wrong conclusions in one day.
- **FIXED 2026-10-10: `scripts/fix-multihome-routing.py --persist`.** Strict ARP (`arp_ignore=1`, `arp_announce=2`) + one route table per NIC with `ip rule from <addr>`. Persisted via `/etc/NetworkManager/dispatcher.d/90-awto-multihome`, re-applied on every interface up, survives a DHCP renew/link flap. Verified: `.133` answers from `a8:11` (1G), `.127` from `a8:12` (10G). `-B <addr>` now selects the right port, read 6.64 Gbit/s on the 10G.
- On a box WITHOUT the fix applied:
  - **`ip neigh show <ip>` and CHECK THE MAC.** An IP that pings proves nothing about which port carried it.
  - **iperf3 needs `--bind-dev <iface>`; `-B <addr>` isn't enough** — it selects a source address, not an egress port. `-B` on the 10G address measured **94 Mb/s** (traffic left via a USB NIC). `--bind-dev` constrains the port directly.
  - **`tcpdump -e 'ether host <mac>'` seeing nothing does NOT mean the port is dead.** Zero frames from the 1G MAC is the *expected* result of weak-host routing. Confirm with port counters (`ip -s link show <iface>`) + its neighbour table (`ip neigh show dev <iface>`).
  - Per-interface MACs are in `scripts/_box.py`; `./dev.py ssh` resolves by MAC.

## Verifying, not assuming
- **`./dev.py gate` before calling anything done.** Runs fmt, lint, pytest, `hal-drift` (3 vendored HAL copies #218), `dt-drift` (board facts shared by the Linux + U-Boot DTs #221).
- **A config symbol is not a working driver.** `CONFIG_RTC_DRV_S35390A=y` did nothing — DTS node is `status="disabled"` (#232). `CONFIG_EDAC_AL_MC=m` bound nothing — our compatible string didn't match the driver's. Check it *bound*, on the box.
- **Device letters are not stable.** `/dev/sdX` shifts with probe order across boots.
- **`hdparm -t` understates by ~4x.** Use `fio --direct=1`.
- **An IP that answers doesn't tell you which NIC answered** — check the MAC in `ip neigh` (see #170 block: produced a 94 Mb/s "10G" reading, a phantom TX asymmetry, and a "1G on an unreachable segment" that was simply weak-host routing).
- **A true observation is not a conclusion.** `tcpdump` really showed zero frames from the 1G MAC; the port was fine. State what was measured, then what it does and does not establish.
- **A box that stops responding is not a reproduction** (#243). Another agent may have rebooted it.
  - Before recording a hang/crash/disappearance: `./dev.py boot-id snapshot` before the test, `./dev.py boot-id check` after (0 same boot, 1 rebooted, 2 unreachable).
  - Exit 2: look for a boot banner in `tmp/logs/unvr-console.log` first (`U-Boot 2015.07`, `U-Boot 2026.07`, `Starting kernel`).
  - Unrelated buses failing together (USB NIC + al_eth) = reboot, not a driver fault.
- **Console timestamps lag the command** — use `time` on the box (read 250 MB/s off timestamps where `time` said 43).

## Where things live
- `docs/improvements-over-stock.md` — ledger of every deliberate difference from stock firmware. Any setting, patch or tuning that differs from stock gets a row (issue + measurement file).
- `dts/alpine-v2-ubnt-unvr-ea16.dts` — Linux DT, built to `/boot/unvr.dtb` on the SSD.
- `uboot-port/arch/dts/awto-alpine-v2-unvr-uboot.dts` — compiled **into** `u-boot.bin`.
- `modules/al_hal/` — the one copy of every HAL file ≥2 modules share; modules symlink to it (`modules/<m>/<f> -> ../al_hal/<f>`). A same-named regular file in another module is a fork, needs a reason in `scripts/hal-forks.json` (`./dev.py hal-drift`).
- `dts/reference/`, `docs/hw-reference/*/live.dts` — vendor hardware-of-record, never built.
- Reference trees in `tmp/refs/` (gitignored symlink) — QNAP, MikroTik CCR2004 UEFI, the UBNT GPL drop with the **working 4.1.37 kernel**, delroth's HAL. See `docs/reference-sources.md`. Search before RE'ing anything.
- Kernel trees are worktrees of the machine Linux store (`./dev.py kernel-store`, `docs/kernel-rebase.md`, #350); never `git clone`/`git gc` there.
