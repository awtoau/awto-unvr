# Issue reconciliation, 2026-10-11

Every open issue against the code, the box and `docs/vendor-code-modernisation.md`. Nothing was closed in this pass; the owner closes. Each issue recommended for closing carries a disposition comment with the evidence.

## Basis

- Tree HEAD `bc79402`; woomera `7.3.0-rc6-c352697+` (rebooted 22:17 +11:00, read-only ssh at 22:23-22:34 +11:00).
- 83 issues were dumped; #296 was closed during the pass, so **82** are tabulated.
- Evidence tags in reasons:
  - `[hw]` checked on the box or in a stored bench run.
  - `[hw-regression]` the hardware check available is a regression run; the failure path is fake-engine/host tested only (it cannot be provoked on the box).
  - `[src]` fix is correct by construction or compile-time enforced; the defect is not provokable; a source-shape test fails on the parent.
- Newest stored bench run, on the running kernel: `bench-history/bench-20261011-223448.json` (`7.3.0-rc6-c352697+`, 22:34:48 +11:00, started by another session): dma copy 20000 tests 0 fail, xor 20000 tests 0 fail; oversize fails only with `prep error` (#290); `health queue drop counters` no growth; `health adaptive moderation off` ok. Its two health FAILs are bench-all false positives (#315).
- Earlier run: `bench-history/bench-20261011-220943.json`, kernel `7.3.0-rc6-a2e0f98-dirty+`: dma copy 20000 tests 0 fail, xor 20000 tests 0 fail; oversize run fails only with `prep error` (#290). `c073b10`, `5455587`, `c2d432c`, `71272b8` are ancestors of `a2e0f98`; `c2d432c` is the last `modules/al_dma` commit. The `-dirty` tag means uncommitted changes at build time; their content was not recorded.

## Counts

| disposition | n | issues |
|---|---|---|
| CLOSE-DONE | 19 | #314 #313 #311 #303 #299 #298 #297 #293 #292 #288 #287 #279 #278 #274 #256 #215 #208 #136 #123 |
| CLOSE-STALE | 2 | #239 #1 |
| CLOSE-DUPLICATE | 2 | #306 #276 |
| CLOSE-SUPERSEDED | 3 | #267 #34 #29 |
| CLOSE-WONTFIX | 7 | #317 #272 #213 #133 #127 #36 #15 |
| VERIFY-ONLY | 10 | #305 #302 #280 #270 #226 #184 #182 #169 #118 #108 |
| PLAN-ITEM | 7 | #301 #300 #291 #282 #268 #264 #207 |
| REAL-OPEN | 32 | #316 #315 #312 #310 #309 #308 #307 #295 #284 #275 #263 #261 #259 #255 #254 #253 #251 #243 #241 #237 #223 #221 #185 #166 #75 #74 #60 #45 #39 #32 #30 #16 |
| total | 82 | |

- Close now (owner): CLOSE-DONE + CLOSE-STALE + CLOSE-DUPLICATE + CLOSE-SUPERSEDED = 26. CLOSE-WONTFIX = 7 more, each needs the owner's call (#317 and #213 are decisions).
- #15 and #36 are WONTFIX only if #213 closes as 'stay out-of-tree'.

## Where this pass differs from the brief

- **#291 is not fixed.** No commit; `al_ssm_main.c:72` still has only the zero / 256 KiB check. Listed as fixed-and-deployed in the brief. Filed under PLAN 3.
- #215, #136, #123, #208, #274 were reported closed but are still open. Tabulated CLOSE-DONE.
- #297: the brief's own correction applied (premise wrong on 7.3), so CLOSE-DONE rather than VERIFY-ONLY.
- #287, #288, #311: the one hardware command (bench-all dma) had already been run on a build containing the fixes; see Basis. They are CLOSE-DONE `[hw-regression]`, not VERIFY-ONLY.
- **#170's fix is stale** (#324): the dispatcher hardcodes DHCP-era addresses; `ip route get 192.168.25.30 from 192.168.25.40/.42/.43` all return `dev enp0s2`. Probable cause of #315's 864 Mbit/s reading.
- Another session had uncommitted edits to `modules/al_ssm/al_ssm_main.c` and `tests/test_al_ssm_review.py` while this ran. The al_ssm rows describe HEAD.
- Another session filed umbrellas #318-#323 (clusters) while this ran. Rows cite them; the dispositions here are per issue, theirs are per cluster. Agreeing: #276 -> #302, #306 -> #299.

## CLOSE-DONE (fix committed and verified)

| issue | reason | note |
|---|---|---|
| #314 | `[src]` zero-budget guard in `al_eth_rx_poll` (`f0ccaec`); netpoll path not provokable here (no netconsole); source-shape test fails on the parent; box runs it; bench health `queue drop counters`: no growth (`bench-20261011-223448.json`) |  |
| #313 | `[src]` `tx_info->skb = NULL` before the free (`17cd220`); needs an injected `dma_mapping_error`, not provokable; source-shape test fails on the parent; `tx_*` rows present and 0 on the box |  |
| #311 | `[hw]` `c073b10`; bench dma on the running kernel `7.3.0-rc6-c352697+` (22:34:48): copy 20000/0 fail, xor 20000/0 fail (`bench-history/bench-20261011-223448.json`; same on `-a2e0f98-dirty+` in `-220943.json`); the many-thread full-ring trigger is host-fake-engine only | re-run on the current kernel: `./scripts/bench-all.py --skip-health --skip-eth --skip-crypto --skip-disk --skip-usb` |
| #303 | `[hw]` `403feff`: `nanddump -l 4194304 /dev/mtd2` rc=0, `ecc_failures` 0, `/proc/interrupts` al-nand 4704 -> 6805, no `irq timeout`/`nobody cared` in dmesg (2026-10-11) | lost-update never counted, so the old race is not measurable |
| #299 | `[hw+src]` `e96acdf`: both ports probe on the new kernel (`/sys/class/net/enp0s2/device/small_copy_len` present, `ip -br link` up); failure unwinds are source-shape only (no safe fault injection) |  |
| #298 | `[src]` `__must_check` on the al_eth UDMA HAL (file now identical to al_dma's) + errors propagated; WERROR build clean, `hal-drift` updated; failure not provokable; box dmesg has no `queue N reset failed` |  |
| #297 | premise refuted on 7.3: core sends IPv6 extension headers to software checksum (`net/core/dev.c:4007`); 100/100 hop-by-hop datagrams on the pre-fix build; defence in depth landed (`7e07475`) plus a bench-all guard | optional: `./scripts/bench-all.py --skip-health --skip-crypto --skip-disk --skip-usb --skip-dma` -> `eth 10G v6 hop-by-hop udp csum` PASS |
| #293 | `[hw]` `2a3c5b2`: boot probe clean (no clock warning, one al-nand IRQ line), 4 MiB ECC-path read ok, no `nobody cared`; item 3 (write with OOB) moved to #317; item 4 unchanged by design |  |
| #292 | `[hw]` `a2e0f98`: `nand-survey.py ecc --label linux_kernel --pages 64` -> parity ok 64, mismatch 0; ECC-path read ok; the failing re-read needs an injected IRQ timeout, host test only |  |
| #288 | `[hw-regression]` `c2d432c`: dmatest copy+xor 20000/0 fail on the running kernel (`bench-20261011-223448.json`); bench dma check also fails on any `queue reset`/`never completed` kernel-log line; a stall cannot be provoked safely, recovery path is fake-engine tested | re-run: `./scripts/bench-all.py --skip-health --skip-eth --skip-crypto --skip-disk --skip-usb` |
| #287 | `[hw-regression]` `5455587`: same run; bench dma fails on any `completion error` line and passed; error statuses are fake-engine tested (engine cannot be made to fail on demand) |  |
| #279 | `[hw+src]` `92077bb`: `*temp = 1000 * readout` over the whole range (`al_thermal_main.c:95`); box reads 58000-59000 mC, 200 reads no `EIO`; >100 C not reachable on hardware, host harness covers the conversion |  |
| #278 | `[hw-regression]` items 1-2 fixed (`1d6b480`); item 3 retracted (ring drivers recycle without an ACK test, plan doc 'Corrections'); dmatest copy+xor 20000/0 fail on the running kernel; leak itself not demonstrable via slab counts; item 4 (polling completion) parked | IRQ completion -> #241 |
| #274 | `[hw]` part 1 `9a2c4ff` (`-EUCLEAN` bound), part 2 `bac8230` (`nand_check_erased_ecc_chunk`); owner-verified 2026-10-11; this pass: survey parity ok 64/64, 4 MiB read ecc_failures 0 |  |
| #256 | structurally complete and on hardware: U-Boot 56-file and EDK2 61-file HAL copies deleted (54,574 lines), `check_no_edk2_hal_copy()` enforced, `alu_*` U-Boot DHCPs in 8 ms; residuals filed: contract header #331, al_ssm shim #264 | plan item 10 stays partial |
| #215 | `[hw]` owner-verified on hardware 2026-10-11 (task statement); `c54d479` hardening + `1b1caf8` gate test (`tests/test_serdes_tune.py`); not re-run in this pass |  |
| #208 | `[hw]` all four gaps answered: EDAC bound, hdd-pwrctl bound, al-nand reads byte-identical (16 MiB, ECC 0/0) and write path PASS 5/5 incl. parity vs independent BCH 64/64 (2026-10-10); sata-leds residual -> #329 |  |
| #136 | `[hw]` `ea93f96`, `c59fb88`: `python3 scripts/_firewall.py` -> `firewall: ok - UDP:69 permitted in zone trusted` (2026-10-11); `tests/test_firewall.py` in the gate |  |
| #123 | `[hw]` `a09b93b`: `cat /sys/class/leds/ulogo_blue/trigger` -> `[disk-activity]` (2026-10-11); bay fault LEDs from `ubnt_hdd_pwrctl`; dead `sata-leds` node -> #329 |  |

## CLOSE-STALE

| issue | reason | reopen if |
|---|---|---|
| #239 | not reproducible: 6 back-to-back `eth diag 2` reads correct; the one `0xFF` capture was on the Sep-04 U-Boot whose HAL copy is deleted; register map has no busy bit, no vendor tree waits, no bound can be derived | reopen if a BIST/serdes-tune read ever disagrees with itself |
| #1 | the live exposure is gone: no USERDEV, no UniFi OS, nothing registers to a cloud account; remaining material is offline in NOR/backups and controlled by gitignore (`sources/**/*.pdf`, `images/mtd/*`); owner's own 2026-10-10 comment recommends closing |  |

## CLOSE-DUPLICATE

| issue | reason | absorbed by |
|---|---|---|
| #306 | resolved by `e96acdf` (#299): moderation setup `al_eth_main.c:6262-6265` now precedes `register_netdev` `:6274`; commit names only #299 | absorbed by #299 |
| #276 | same commit as #302 (`e9bf6be`): ERASE2 / STATUS now use the 13 ms ready bound while the array is busy | absorbed by #302 |

## CLOSE-SUPERSEDED

| issue | reason | absorbed by |
|---|---|---|
| #267 | root cause found: kernel >= 7.2 `AF_ALG_CRYPTOAPI_MASK` (`7524070f26d8`) excludes al_ssm by design; residual is that `bench_crypto` still labels the row `[xts-aes-al-ssm]` by priority (`scripts/bench-all.py:520-537,646`) | absorbed by #315 (label fix); dm-crypt path is `bench-all --ssm` |
| #34 | clock half resolved by design (fixed clocks); reset-controller lost its motive when #51 was fixed; only the PSCI `idle-states` DT item has payoff, and it is performance/power work | absorbed by #241 |
| #29 | CPU PLL overclock: zero runtime tooling, no cpufreq, parked behind the bug-fix phase; DRAM half already owned by #247 | absorbed by #241 (same precedent as #196, #209) |

## CLOSE-WONTFIX

| issue | reason | reopen if / decision |
|---|---|---|
| #317 | `-ENOTSUPP` is a sound refusal; nothing here writes OOB through the ECC path; implementing needs a second partial PAGEPROG (Micron NOP count unchecked) and a flash write to test | owner picks the number; umbrella #320 step 3 |
| #272 | readability only; the 3 IDs (`l4_proto_idx=2`, `tunnel_control=5`) have no enumerator in any available source and no consumer is blocked | reopen if the chip's EPE entry 2 or Annapurna docs appear |
| #213 | decision recorded in the issue (2026-10-10 comment): stay out-of-tree; `modules/` is the HAL source of truth for U-Boot/EDK2 (#256); measured rebase cost near zero (3177 upstream commits, 8 carried, 0 conflicts); trigger (NET_DEVLINK) fixed by a patch | revisit only if #194-style upstreaming becomes live |
| #133 | latent: fitted module is Power Level 1; `sfp.c` refuses a >1 W module with a clear dmesg line; writing an unmeasured 2000 mW is worse than the refusal | reopen with the cage power budget (U5075/U5076) or a >1 W module |
| #127 | no 1G SFP module to test with; phylink rejects one with a clear error; untested mode-switch code on the only 10G port is worse than the refusal | reopen when a 1G SFP is owned; share the byte-3 mapping with #108 |
| #36 | conditional on #213 closing as 'stay out-of-tree'; ledger half exists (`patches/` headers); SPDX/DCO only matter on a decision to upstream. Do NOT delete `patches/uboot/` on the strength of the 2026-10-10 'stale' comment: `uboot-build.py:285,359` and `docs/build.md:34` still use it | reopen with the decision to upstream |
| #15 | conditional on #213 closing as 'stay out-of-tree': bindings only serve upstream submission; `dtbs_check` cannot pass for `ubnt,unvr` until the `amazon,al.yaml` enum gains it | reopen with the decision to upstream |

## VERIFY-ONLY (fix landed; one hardware command left)

| issue | reason | command |
|---|---|---|
| #305 | `2cb8f10`: NAPI completes on a short refill + 1 s empty-ring watchdog; counters `queue_N_rx_refill_fail/rx_empty_ring` present and 0 (2026-10-11); the stall itself needs memory pressure | `./dev.py ssh -- "stress-ng --vm 2 --vm-bytes 85% -t 60"` while `iperf3 -c <box 10G> -t 60` runs from the dev host; then `./dev.py ssh -- "ethtool -S enp0s2 \| grep -E 'rx_(refill_fail\|empty_ring)'"`: RX recovers after the stress ends |
| #302 | `e9bf6be`: `waitfunc` runs `core_waitfunc()` before reporting the latch; erase path cannot return mid-tBERS; host model only, erase never run since | `python3 scripts/nand-write-test.py --write` (flash write, needs owner OK) -> `[0] [5] [6] PASS`; then `dmesg \| grep -c 'command FIFO still busy'` = 0 |
| #280 | `580b279`: devres-only teardown, PBS mapping released, devm hwmon, sleeping bounded wait; rmmod path never run on the box (critical trip unprotected for the seconds the module is out) | `./dev.py ssh -- 'modprobe -r al_thermal; dmesg \| tail -20; modprobe al_thermal; cat /sys/class/thermal/thermal_zone0/temp'` -> no splat, millidegrees back |
| #270 | `ac359f9` announces the ethact rotation; build clean, never run on hardware | boot `./dev.py uboot-test --cold` with the 1G cable out, `ethrotate` unset, then `ping`/`dhcp` at `awto-nas#` -> `>>> ETHACT ROTATED: al_eth_1g FAILED TO START ...` |
| #226 | `3c259af` derives the TX completion bound per frame length and link rate (27.9 us @1G); never run on hardware | `./dev.py uboot-test --cold`; at `awto-nas#`: `dhcp`, `setenv serverip <dev host>`, `tftpboot` a large file on eth0 -> zero `TX completion timeout` lines |
| #184 | both-NICs-dead under 5 s of 1000 Mbit/s UDP TX seen once on 7.1.8; never reproduced; now runnable (10G fitted, #304 default fixed); #304 does not explain it (TX-only load) | console attached first, then `./dev.py ssh -- 'iperf3 -u -b 1000M -t 5 -p 5701 --bind-dev enp0s2 -c <dev-host 10G addr>'` x5 (dev host `.30`, `ip-registry.toml`); hang -> read the console, check boot ID (#243) |
| #182 | AF_ALG cannot reach al_ssm on >= 7.2 (#267), so the 2026-08-30 wedge cannot be re-driven that way; new candidate cause #332 (128 KiB op truncated by the 16-bit descriptor length; fix `367f876` not on hardware); dm-crypt path exists; pass = not reproducible = STALE | `./scripts/bench-all.py --skip-health --skip-eth --skip-crypto --skip-disk --skip-usb --skip-dma --ssm --ssm-writers 8 --ssm-mib 128` |
| #169 | `71272b8`: Q/PQ over 8 KiB refused (`AL_RAID_OP_Q_MAX_BLOCK_SIZE`, status 0x10 = decode error); PQ off unless `enable_pq=1`; prediction untested | `./scripts/bench-all.py --skip-eth --skip-crypto --skip-disk --skip-usb --dma-reload --dma-pq --i-accept-a-reboot` -> `dma pq-8k` 0 failures; any failure = second cause, keep gated |
| #118 | `1403177` reads group-D synthready/synthstatus before EQ-apply; polarity (bit0 = ready) inferred from the name only | `./dev.py uboot-test --cold`; at `awto-nas#`: `serdes status` -> record `synthready`/`synthstatus` on a good boot; item 2 (preboot-free boot) stays blocked |
| #108 | `f72ed7a` reads the SFP+ EEPROM by phandle and logs the media; never run on hardware | `./dev.py uboot-test --cold`; at `awto-nas#`: `setenv ethact eth1; dhcp` -> console line `SFP module: FS SFP-10GSR-85 -> 10GBASE-SR` |

## PLAN-ITEM (`docs/vendor-code-modernisation.md`)

| issue | reason | plan item |
|---|---|---|
| #301 | remove frees in-flight DMA buffers before quiescing; tests xfail(strict); not fixed | PLAN 3; umbrella #318 step 4 |
| #300 | backlog insert split from the ring-full decision; obsolete if `crypto_engine` is adopted (#282 item 7); not fixed | PLAN 3; umbrella #318 step 2 |
| #291 | NOT fixed (no commit; `al_ssm_main.c:72` still only `nbytes == 0 \|\| > AL_SSM_MAX_DATA_SIZE`): CBC unaligned / XTS < 16 reach the engine and report success; `367f876` (#332) only splits > 64 KiB buffers | PLAN 3; umbrella #318 step 3 |
| #282 | crypto-API contract gaps (flags, GFP, BH, MAY_BACKLOG, queue); decision: adopt `crypto_engine` or keep the hand-rolled queue | PLAN 3; umbrella #318 step 2 |
| #268 | survey done; checklist items 1-2 done (`36cb313`, `2d202d5`); remaining is the exec_op port (L) | PLAN 4; umbrella #320 step 4 |
| #264 | al_ssm HAL generation gap: U-Boot compat shim and a separate al_ssm HAL copy in Linux | PLAN 10 |
| #207 | decision: DT home for SerDes TX taps (presence property `serdes-tx-params` + static fallback); nothing implemented | PLAN 8; umbrella #322 step 1 |

## REAL-OPEN

| issue | reason | needs |
|---|---|---|
| #316 | conversion period measured 2026-10-11: 17.3-18.2 ms over 20 back-to-back reads (includes ~1 ms fork); the 100 ms bound is 5.5x; set `AL_THERMAL_VALID_TIMEOUT_US` ~23000 (1.25x worst) and record the source | driver change, not made in this pass |
| #315 | bench-all: box->host leg not pinned to the port (probable root cause #324), `BUG` substring matches `Debug`/`Firmware Bug`, dead USB NIC should SKIP, `health al_eth module` fails on a matching kernel (`vermagic=modinfo: running=7.3.0-rc6-c352697+` in `bench-20261011-223448.json`); also takes #267's label residual | `scripts/bench-all.py:215-330` |
| #312 | bench-all follow-ups: attended `--save-baseline` (10G was baselined one direction at a time), no crypto/sata/usb baseline, `hdparm` vs `fio --direct=1`, hardcoded disk letters, full `--idle` cycle never run | cluster bench |
| #310 | no per-adapter mutex around the KR PCS addr/data pair; group lock has 0 callers; low likelihood (1 Hz poll vs a reset); static analysis only | umbrella #322 step 3 with #308 |
| #309 | `al_eth_up()` unwinds an `up_complete()` failure as if only rings existed; latent (cannot fail on 1G/10G); two-line fix: call the `al_eth_down()` body | umbrella #321 step 2 with #307 |
| #308 | serdes override handshake: reader unlocked, `dirty` before `valid`; fix = snapshot under `serdes_config_lock`, set `valid` first; moot if #207 retires the sysfs path | umbrella #322 steps 1 and 3 |
| #307 | `reset_task` has no `netif_running && up` check; `CLOSE_ONGOING` cleared before rtnl release; low likelihood; no repro | umbrella #321 step 2 |
| #295 | gpio read error treated as 'bay empty' and emits a hot-unplug uevent; fix = keep previous state on `state < 0`, rate-limited log | `modules/ubnt_hdd_pwrctl/ubnt_hdd_pwrctl.c:103` |
| #284 | umbrella for `docs/vendor-code-modernisation.md`; keep until plan items 2-10 are done or dropped; status column added 2026-10-11 | plan |
| #275 | OOB layout claims 220 ECC bytes, hardware uses 128 (SPARE counts, `oobavail` 2); `max-onfi-timing-mode` DT knob unread; needs a flash write to verify | PLAN 4 (ECC OOB geometry); umbrella #320 step 2 |
| #263 | EDK2 hands Linux DRAM bank 0 only (`Unvr.dsc:256-257`); UEFI track | decision #330 |
| #261 | `uefi-p4-boot.py` fsN discovery and `uefi-esp-stage.py` free-space check remain; UEFI track | decision #330 |
| #259 | reset after a corrupting Oops is untested; destructive, needs exclusive box + console and #243's boot-ID cross-check first | do with #60 and plan item 9 (#328) in one reboot session |
| #255 | PSCI CPU_ON lies; secondaries never start under EDK2; parked requirement on platform code that was carried across, not rewritten; UEFI track | decision #330 |
| #254 | the IDLE wait is a quiescence check, not a wrong target: `state_get()` reads NORMAL while a sub-block is busy (live al_ssm/al_dma enabled-idle read `idle`); waiting for NORMAL would break bring-up. Landed `219eb9a`: timeout warning names the state the engine is stuck in (al_eth and al_ssm copies). Open: why the engine is not quiescent on U-Boot re-activation | umbrella #323 step 1; one U-Boot session: two `tftpboot` in one boot, look for `engine still` |
| #253 | U-Boot 10G TX completion timeout: no measurement ever exercised the `alu_*` code; retest recipe in the 2026-10-09 comment (module is fitted now) | umbrella #323 step 2 |
| #251 | one hardware run left (`bootedk2` on the NAND U-Boot, then the three #256 phase-3 checks); UEFI track | decision #330 |
| #243 | add the boot-ID cross-check rule to CLAUDE.md and a helper that snapshots boot ID + console offset around a test; protects #184, #182, #259, #253 |  |
| #241 | parked performance umbrella; absorbs #29, #34 and the IRQ-completion residual of #278; 10G re-baselined 2026-10-11 (box->host 9.38 Gbit/s, 0 retr; host->box 7.15) |  |
| #237 | media type never derived from the module: DT declares a DAC, SR optic fitted (`dac 1 dac_len 3` in board_params debugfs); byte 2 + byte 18 of the EEPROM is a complete discriminator | umbrella #322 step 2; PLAN 8 (tap table per media); U-Boot half landed in #108 |
| #223 | what NOR `0x1f0006` is: needs the stock U-Boot binary reader; nothing blocked (rewrite uses base+2); low priority | umbrella #323 step 6 |
| #221 | Linux half done (`c71ecb2`, verified 2026-10-11); remaining: exercise the seed path with regs cleared, port the validator into `alu_eth_bp_seed()`, EDK2 side | umbrella #323 step 5 |
| #185 | standalone `tftpd.py` stalls at block 19355 / ~31 s; still on live paths (`dev.py`, `netboot.py`, `flash-awto-uboot.py`, `upgrade-ladder.py`); cheap fix = every caller uses the embedded one-shot server of `ram-boot-deploy.py` |  |
| #166 | `flash-awto-uboot.py` and CLAUDE.md fixed; left: `docs/mtd.md:232` still says U-Boot has 'no NAND driver at all'; restoring the pristine recovery image is a flash write and a choice | one doc line |
| #75 | blocked on #74's Linux half; scope is a three-tree decision (one DTS for Linux/U-Boot/EDK2 vs widen `dt-drift`); Linux board files triplicated (#16) |  |
| #74 | Linux half (idempotent `al_snoop_fix`, generic ECAM for the internal PCIe node) not started; Linux relies on awto-uboot having applied the fixup; the al_eth exclusion must be retested before dropping |  |
| #60 | standing SP805 watchdog: nothing opens `/dev/watchdog0`; no `system.conf` on the box; U-Boot `reset_cpu` is a one-shot | same SP805 block as plan item 9 (#328) |
| #45 | production hardening deferred on purpose (lab box): `root:unvr` + `PermitRootLogin yes` (`build-fedora-rootfs.py:198-200`), SELinux off twice, no sysctl/udev tuning; watchdog item duplicates #60, tuning belongs to #241 |  |
| #39 | UEFI tracker: P0-P2 done, P3 binds untested, P4 booted Fedora with `maxcpus=1` | decision #330 |
| #32 | md RAID baseline never built: sda 7.3T, sdb 7.3T, sdc 5.5T are blank and `/proc/mdstat` is empty (2026-10-11); needs the array shape decision (disk allocation); parity runs in software (neonx8 4.3 GB/s) with PQ gated | umbrella #319 step 2 needs this array |
| #30 | left: drop dead DTS partitions (NAND `chike`, `device_tree`, NOR `cksum`), fate of the 16 MiB NOR recovery slot, resident EDK2 FD slot | decision #330 for the EDK2 slot |
| #16 | populate `alpine-v2.dtsi`; three board files redeclare the same SoC blocks (1108/779/543 lines); has local value without upstreaming; fold the dead `chosen/bootargs` text in the same pass |  |

## Verification batches

- **One flash-write session** (owner OK per run, CLAUDE.md): `python3 scripts/nand-write-test.py --write` covers #302 (and #276). Read-only NAND checks already done: see below.
- **One U-Boot session** (`./dev.py uboot-test --cold`, shared console, do not touch a tio not started): #108, #118, #226, #253 retest, #254, #270 (needs the 1G cable out).
- **One quiet-box pass** (`./scripts/bench-all.py` subsets): #169 (`--dma-reload --dma-pq --i-accept-a-reboot`), #182 (`--ssm`), #305 (stress), #297 optional, #184 (UDP 1000M with the console attached).
- **Fix #324 first**: every per-port bench number taken since the renumbering is suspect until the multihome rules follow the interface addresses.

## Decisions needed

| decision | issue | recommendation |
|---|---|---|
| write-with-OOB: implement or keep `-ENOTSUPP` | #317 | keep (close WONTFIX) |
| stay out-of-tree | #213 | stay; then #15, #36 close |
| DT home for SerDes TX taps | #207 | DT presence property + static fallback (PLAN 8) |
| UEFI track: keep / park / drop | #330 | park (close #255 #261 #263; keep #39 #251) |
| `AL_THERMAL_VALID_TIMEOUT_US` | #316 | 23000 (1.25 x measured 18.2 ms) |
| `sata-leds` node: delete / driver / disable | #329 | delete |
| array shape on the three blank disks | #32 | owner's call |
| al_ssm: adopt `crypto_engine` or keep the queue | #282 item 7 | adopt (makes #300 obsolete) |

## Filed in this pass

| issue | what |
|---|---|
| #325 | plan item 5: retire unreferenced vendor code to `debris/` |
| #326 | plan item 6: ethtool module/FEC/stats, copybreak, dimlib |
| #327 | plan item 7: 1G port to phylink |
| #328 | plan item 9: al_reboot to mainline `sp805_wdt` restart |
| #324 | multihome routing (#170 fix) stale after renumbering |
| #329 | `sata-leds` DT node binds nothing |
| #330 | decision: UEFI track |
| #331 | HAL plat contract lacks `pr_*`, `fallthrough`, `__must_check` |

## Read-only box checks run (woomera, 2026-10-11 22:23-22:34 +11:00)

- `ethtool -S enp0s2`: all `tx_dropped/pad_err/dma_mapping_err/csum_err/frag_err`, `rx_dropped/refill_fail/empty_ring/dma_mapping_err` rows present and 0 (4 queues).
- dmesg since boot: no WARNING/BUG/Oops/SError (6 grep hits are `Firmware Bug` VPD, `sys-kernel-debug`, `ramoops`); no `command FIFO still busy`, `irq timeout`, `nobody cared`.
- `python3 scripts/nand-survey.py ecc --label linux_kernel --pages 64`: parity ok 64, mismatch 0.
- `nanddump -l 4194304 /dev/mtd2`: rc 0, `ecc_failures` 0, `corrected_bits` 0; `/proc/interrupts` al-nand 4704 -> 6805.
- `/sys/class/thermal/thermal_zone0/temp`: 200 reads, 0 `EIO`, values 58000/59000. Conversion period (second read after a consumed `T_VALID`, 20 samples): 17253-18162 us.
- `cat /sys/class/leds/ulogo_blue/trigger`: `[disk-activity]`.
- `/sys/bus/platform/devices/sata-leds`: no `driver` link.
- `lsblk`: sda 7.3T, sdb 7.3T, sdc 5.5T blank; `/proc/mdstat` empty.
- `ip rule`, `ip route get ... from <addr>`: see #324.
- `python3 scripts/_firewall.py` (dev host): `firewall: ok - UDP:69 permitted in zone trusted`.

## Not established

- Whether #324 is the whole of #315's 864 Mbit/s (addresses at the time of that run were not recorded).
- #305, #280, #302, #169, #182, #184: the fixes landed but the failure each guards against has not been exercised on hardware.
- Running kernel (`unvr-ea16-7.3-fedora.config`) has `FAULT_INJECTION`, `FUNCTION_ERROR_INJECTION` and `NETCONSOLE` off, so no failure path is provokable without a rebuild: #313 (`dma_mapping_error`), #314 (zero-budget netpoll), #305 refill failure (real memory pressure only; `stress-ng` not installed on the box), #292 (IRQ timeout), #293 items 1-2 (no DT clock, spurious shared IRQ), #288 doorbell (`al_ssm_dma_action()` always returns 0: dead code).
- #335 items 1, 3, 6 have no callers in any consumer (`no_desc_cfg_set`, DWRR config, `q_sc_reset`); item 2 is unproven (ifdown under traffic).
- #287, #288, #311 failure paths (error status, stall, full-ring callback race) are proven only against the fake engine; the box cannot produce them on demand.
- The 10G figures in `bench-20261011-220943.json` (host->box 3.40, box->host 4.83 Gbit/s) sit below the 01:31 re-baseline in #241 (7.15 / 9.38): routing (#324), the `-dirty` build, or load on the box; not investigated.
- al_ssm rows describe HEAD; uncommitted edits by another session were not read.
