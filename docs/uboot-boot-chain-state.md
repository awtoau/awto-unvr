# Boot chain — live env/bootdelay state (as of 2026-08-26)

What actually happens on a cold power-on, verified live via genuine
power-cycles (not chainload-from-an-already-running-session tests, which
inherit state a real cold boot wouldn't have — see #140 for where that
distinction mattered). Two separate U-Boot instances, two separate
persisted environments, on the same physical SPI-NOR chip.

## Chain

```
preboot (S2 + al_boot, 0x21000 in mtd05) — always runs, proprietary
  -> stage3/agent_wakeup -> stock U-Boot 2015.07 (0xa1000 in mtd05)
       bootdelay=2, bootcmd='run bootchain'
       -> tftpboot + go 0x1100000 -> our U-Boot (uboot-port, chainloaded, RAM only)
            bootdelay=2, bootcmd=(CANARY check) -> scsi scan -> bootm -> Fedora
                 -> woomera login: (automatic)
```

Confirmed end-to-end via `./dev.py power-cycle` + watching the raw serial
stream (not the log file — see gotcha below) twice in a row, zero manual
intervention either time.

## Stock U-Boot persisted env (SPI-NOR `u-boot-env`, `0x1c0000`)

| var | value |
|---|---|
| `bootdelay` | `2` (was `-1` = never autoboot, until 2026-08-26) |
| `bootcmd` | `run bootchain` |
| `bootchain` | `setenv ipaddr 192.168.25.140; setenv serverip 192.168.25.147; if tftpboot 0x1100000 u-boot-chainload.bin; then go 0x1100000; fi` |

**No Linux fallback if tftp fails** (deliberate, carried over from the
original `linux-to-autochain.tcl` design) — if our dev host's tftpd is down,
the `if` just falls through and the box sits at the stock prompt.
Recoverable, never silently boots old vendor Linux.

**`serverip` is baked in, not detected at boot** — it's *this dev host's*
current IP, saved into persisted env. Drifts with DHCP lease changes; a
stale value here means auto-chainload silently retries against nobody
forever. Re-run the `setenv bootchain ...` + `saveenv` sequence below after
any host IP change. Same class of bug `scripts/_net.py:detect_server_ip()`
already fixes for every *interactive* chainload path this session — this is
the one place it can't apply, since it has to be a static string baked into
U-Boot's own persisted env, not something U-Boot can call out to Python for.

To refresh `serverip` after a host IP change:
```
setenv bootchain 'setenv ipaddr 192.168.25.140; setenv serverip <NEW_IP>; if tftpboot 0x1100000 u-boot-chainload.bin; then go 0x1100000; fi'
saveenv
```

## Our U-Boot (uboot-port) persisted env

| var | value |
|---|---|
| `bootdelay` | `2` (compiled default `-1` in `alpine_v2_unvr_defconfig`, was ALSO overridden by a stale persisted `-1` until explicitly `setenv`+`saveenv`'d — see gotcha) |
| `bootcmd` | `if itest.l *0x10000000 == 0xb0075709; then echo == CANARY set - staying in U-Boot ==; else scsi scan; ext4load scsi 0:2 0x02000000 /boot/uImage-unvr-ea16-7.1-fedora-gz && ext4load scsi 0:2 0x04078000 /boot/alpine-v2-ubnt-unvr-ea16-7.1-fedora.dtb && bootm 0x02000000 - 0x04078000; fi` |

**CANARY escape hatch**: a script that wants to stop at `awto-nas#` for
manual testing (instead of auto-continuing to Fedora) pre-sets
`0x10000000 = 0xb0075709` before triggering the boot. Nothing else touches
that address, so it's a clean, deliberate override — no tooling currently
sets it automatically; add this if a test script needs to guarantee landing
at the prompt now that bootdelay is non-negative.

**`CONFIG_BOOTDELAY=-1` was a deliberate safety choice** (original comment:
"prevents auto-running bootcmd, which can hang on a bad bootm before we have
OF_LIBFDT + a working sysreset"). Changed to `2` on 2026-08-26 only after
confirming the originally-motivating bug (#97, AHCI/SATA handoff hang) is
fixed and closed. If `bootm` ever becomes unreliable again on some future
build, this is the first thing to revert.

## Gotcha: persisted env overrides the compiled defconfig default

Changing `CONFIG_BOOTDELAY` in `alpine_v2_unvr_defconfig` and rebuilding is
**not enough on its own** — if a valid environment is already saved in
flash (from any prior `saveenv`, on either U-Boot), that persisted value
wins over the new compiled default every time. Confirmed live: rebuilt with
`CONFIG_BOOTDELAY=2`, chainloaded, `printenv bootdelay` still read `-1`
until an explicit `setenv bootdelay 2; saveenv` was run *on the live box*.
Any defconfig env-default change needs this same live follow-up, on both
U-Boots independently (they don't share an environment — see next section).

## Gotcha: our U-Boot's env offset targets the wrong partition (#158)

`CONFIG_ENV_OFFSET=0x1E0000` in `alpine_v2_unvr_defconfig` (in place since
2026-08-19, not from this work) targets the vendor's **`factory`**
partition (read-only per the real Linux DTS), not the dedicated
`u-boot-env` partition at `0x1C0000` stock uses. `saveenv` on our U-Boot has
been erasing+writing `0x1e0000` for a week+ without observed damage, and a
full pre-existing backup of the original factory sector exists
(`/mnt/2tb/git_debris/woomera-mtd/.../mtd03-Factory-*.img`, captured before
any of this env work began) — not urgent, but a real mismatch. See #158.

## USB / PCIe status (#140) — as of this doc

**Not working yet.** Substantial real progress this session (link training,
enumeration, config-space access all now solid — the original SError crash
is permanently fixed), but the actual xHCI device-context DMA allocation
(`Cannot allocate device context to get SLOT_ID`) — the *original* symptom
this issue opened with — is still open. See #140 for the full blow-by-blow;
short version: six-plus rounds of PCIe-HAL-level fixes closed every gap
found in the Alpine-specific bring-up code, and what's left is generic
mainline xHCI/USB territory, not yet chased.

## Recovery / re-catching an interactive prompt

Autoboot is no longer a no-op on either U-Boot (both `bootdelay=2` now) —
`scripts/catch-uboot.py` (ESC-bashing during the boot window) still works
exactly as before to interrupt and land at a prompt for manual testing;
there's just now a real (if short) race against autoboot where there
previously wasn't one, since `bootdelay=-1` meant it always dropped to the
prompt unconditionally. `./dev.py uboot-test` / `./dev.py chainload` are
unaffected — they drive this same catch mechanism.

## Log-tailing gotcha (unrelated to env, hit repeatedly tonight)

U-Boot's own prompt string (`ALPINE_UBNT_NAS_ALL>` / `awto-nas#`) has no
trailing newline. `tio`'s line-buffered file logging
(`tmp/logs/unvr-console.log`) can leave it unflushed for a long time even
though the box is genuinely sitting there responsive — `tail`-ing the log
file made a working, already-at-prompt box look hung for several minutes
during this session's testing. `./dev.py check` / `console-send` read the
live stream directly and are the reliable way to check actual state;
don't trust an apparently-stalled log file alone.
