# Vendor code modernisation plan

Audit of the out-of-tree `al_*` modules against the generic kernel facilities they duplicate.
Measured 2026-10-11 against `/mnt/2tb/unvr-port-refs/linux-v7.3-fresh` (7.3.0-rc6) and woomera (`7.3.0-rc6-bac8230+`).
Umbrella issue: #284. No driver code changed by this document.

## Principle

- SoC driver owns only the bus/interface that is not generic: register maps, FIFO/descriptor mechanics, clock/reset, DMA plumbing, interrupt wiring, ECC/crypto engine programming.
- The attached standard device (ONFI NAND, SFF-8472 module, PHY, SATA drive, crypto algorithm) is handled by the generic subsystem.
- Template: #274. `al_nand_oob_is_empty()` (2013) vs `nand_check_erased_ecc_chunk()`, used by 15 mainline drivers, fixed in `bac8230`.
- Detection recipe: for every vendor function, name the core function that does the same job; if one exists and the vendor one differs, assume the vendor one is wrong until shown otherwise.

## Ranking (bugs prevented / effort)

Status as of 2026-10-11, HEAD `bc79402`, box `7.3.0-rc6-c352697+`. Per-issue disposition: `docs/issue-reconciliation.md`.

| # | Work item | Bugs prevented | Effort | Risk | Issue | Status |
|---|---|---|---|---|---|---|
| 1 | al_dma: dmaengine descriptor contract | silent leak/corruption on md RAID5 (XOR advertised, `MD_RAID456` built) | S | low | #278 | done (`1d6b480`, `5455587`, `c2d432c`, `c073b10`); dmatest 20000 ops, 0 failures. IRQ completion open, parked (#241) |
| 2 | al_thermal: report correct millidegrees, fix remove path, drop PBS HAL | critical trip can never fire (confirmed); double free on rmmod | S | low | #279, #280 | partial: #279, #294 done; #280 fixed (`580b279`), unverified; PBS HAL (~3,500 lines) not removed |
| 3 | al_ssm: crypto-API contract (flags, GFP, backlog, BH) + test via dm-crypt/EXTRA_TESTS | #267 closed by reading (AF_ALG mask); contract gaps feed #182 | S flags/tests, M engine | med | #282, #267, #182 | not started: #282, #291, #300, #301 open (umbrella #318); #267 residual is a bench-all label |
| 4 | al_nand: `setup_interface()` for ONFI timing, then exec_op | wrong SET FEATURES address/payload | M (+L for exec_op) | med | #281, #268 | partial: `setup_interface()` done (#281, `6f94327`); exec_op open (#268) |
| 5 | Retire unreferenced vendor state machines to `debris/` | removes ~14k lines nothing executes (8.5k conditional on SerDes group) | S, mechanical | low | #325 | done: 31.5k lines to `debris/code/` (23.0k Linux modules, 8.5k U-Boot); evidence table under "Item 5 evidence"; `git log --grep '#325'`. Function-level leftovers listed there, not retired |
| 6 | al_eth: ethtool completeness, tunables, `dimlib` | missing `ethtool -m`/FEC; sysfs knobs duplicate ethtool | S | low | #326 | not started |
| 7 | al_eth 1G: phylib `adjust_link` -> phylink | one link path for both ports | M | med | #327 | not started |
| 8 | SerDes as `drivers/phy` provider + DT tuning | #207, `BUG()`s, per-module copies | L | med | #283, #207 | partial: #283 done (`92077bb`); phy provider and DT taps open (#207) |
| 9 | al_reboot -> `sp805_wdt` restart | one less module; unproven | S (needs reboot) | med | #328 | not started |
| 10 | One HAL source tree | 4 diverging UDMA copies | L | med | #256, #264 | partial: U-Boot and EDK2 stage from `modules/al_eth` (#256); Linux still has separate al_eth/al_ssm/al_dma/al_nand copies (#264) |

- 1-4 are the ones that can hide a #274: silent data-path divergence from a generic contract.
- 5 is first in line for effort/benefit on audit time: nothing in it can be wrong at runtime, but it is ~14k lines every future review has to rule out.

## Item 5 evidence (#325)

- Method: `scripts/find-dead-code.py` (ctags spans + name reachability; roots = `.c` file-scope initialisers and `#define` bodies) over `modules/*`; `outside_refs` greps tests/, scripts/, U-Boot, EDK2; every move built (9 `.ko`, 0 warnings, `CONFIG_WERROR=y`).
- 10G port SerDes group: **3 = HSSP group D**, lane 0. DT `serdes-grp = <3>`; woomera debugfs `board_params` for `0000:00:02.0`: `serdes grp 3 lane 0`. 1G port: no SerDes. Group E (25G, offset 0x2000) unused; U-Boot (`al_serdes_10g.c`): board-cfg SKIP, external-aborts on first access.
- Retired code is in `debris/code/<dir>/` with a README per directory.

| file | lines | why dead | disposition |
|---|---|---|---|
| `al_eth/al_init_eth_lm.c` | 2,564 -> 262 | detect/establish step machines, SFP/QSFP detect, Auto-FEC, retimer drivers: no caller outside themselves; phylink + sfp.c own the port; `retimer_exist = false` | cut to `al_eth-vendor-lm/`; kept `al_eth_lm_init`, static SerDes tables + `_params_set`, `static_parameters_{override,get}`, `debug_mode_set` |
| `al_eth/al_init_eth_lm.h` | 546 -> 210 | context/params fields only the cut code read | trimmed |
| `al_eth/al_init_eth_kr.[ch]` | 830 + 78 | sole caller `al_eth_an_lt_execute()` in the cut establish step | moved, `al_eth-kr-an-lt/` |
| `al_eth/al_hal_eth_kr.[ch]` | 1,029 + 372 | sole caller is `al_init_eth_kr.c` | moved |
| `al_eth/al_hal_an_lt_wrapper_regs.h` | 263 | included only by `al_hal_eth_kr.c` | moved |
| `al_eth/al_hal_serdes_25g.{c,h}`, `_regs.h`, `_internal_regs.h` | 3,395 + 79 + 434 + 4,616 | only reference: `al_serdes.c` group-init table (fills pointers, no register access) for group 4; board has no group 4 | moved, `al_eth-serdes-25g/`; `AL_SRDS_NUM_GROUPS` 5 -> 4 |
| `uboot-port/.../al_hal_serdes_25g.{c,h}`, `_regs.h`, `_internal_regs.h` | 3,393 + 79 + 434 + 4,616 | linked `u-boot` ELF has no `al_serdes_25g*` symbol; `al_serdes_10g.c` only included the header | moved, `uboot-serdes-25g/`; syntax-checked, full U-Boot build not run (it resets the shared tree) |
| `al_ssm/al_hal_ssm_crypto_cfg.c` | 2,382 | 7 exported symbols, callers = header declarations; GMAC/inline-CRC/compression config | moved, `al_ssm-unbuilt-hal/`; 7 prototypes cut from `al_hal_ssm_crypto.h` |
| `al_ssm/al_err_events*.{c,h}`, `al_err_events_internal.h` | 166 + 383 + 239 + 1,403 + 223 | never in the Makefile, included by nothing | moved |
| `al_ssm/al_hal_ssm_crc_regs.h`, `al_hal_unit_adapter_gen_regs.h` | 409 + 1,460 | included by nothing built | moved |
| `al_dma/al_hal_unit_adapter_regs.h` | 433 | included by no al_dma source | moved, `al_dma-unbuilt-hal/` |
| `al_ssm/al_hal_ssm_crc_memcpy.[ch]` | 789 + 344 | only caller `al_ssm_error_ints_unmask()`, itself uncalled; `al_ssm_main.c` is crypto-only | moved, `al_ssm-crc-memcpy/` |
| `al_ssm/al_hal_ssm.c`, `al_hal_ssm_crypto.c` (+ `.h`) | 23 + 125 (+ 35) | `al_ssm_error_ints_unmask`, `al_crypto_error_ints_{mask_get,unmask}` | cut |
| `al_ssm/al_hal_unit_adapter.c`, `.h` | 443 + 261 | 25 of 30 functions uncalled | cut, `al_ssm-unit-adapter/`; kept `_init`, `_snoop_enable`, `_error_track_enable`, `al_pcie_perform_flr`, `_rob_cfg` |

- Module sizes (unstripped `.ko`, bytes): `al_eth_1g` 8,575,008 -> 7,655,152; `al_eth_10g` 8,575,064 -> 7,655,200; `al_ssm` 3,165,488 -> 2,816,128; others unchanged.
- `scripts/hal-forks.json`: the four `al_hal_serdes_25g*` entries dropped (no longer 2+ trees); `al_hal_unit_adapter_regs.h` loses al_dma; `al_hal_ssm.c`, `al_hal_ssm.h`, `al_hal_unit_adapter.h` re-hashed (al_ssm copy).

Kept (dead by the scan, not retired):

| file | lines | reason kept |
|---|---|---|
| `al_eth/al_hal_eth_main.c` | 2,051 of 7,919 (59 functions) | staged into U-Boot and EDK2 (#256); U-Boot calls `al_eth_flr_rmn_restore_params`; the RX/TX parser tables are annotated by `scripts/designate-al-eth-tables.py` |
| `al_nand/al_hal_nand.c` | 909 of 2,131 (13 functions) | `al_nand_properties_encode` is used by `tests/c/al_nand_host.c`; page read/write command generators belong to item 3/4 (exec_op) |
| `al_thermal/al_hal_pbs_utils.c`, `al_hal_thermal_sensor.c` | 77 + 103 | item 2 (PBS HAL removal) |
| `al_*/al_hal_udma_*`, `al_hal_iofic*`, `al_hal_ssm_raid*`, `al_hal_m2m_udma*` | ~5,800 dead across copies | item 10 (one HAL tree) is merging these |
| `al_eth/alpine_serdes.c` lock/trylock/unlock | 21 | uncalled; tied to #310 |

## Alpine-specific code that stays (all modules)

- UDMA M2S/S2M queue, descriptor, completion-ring and IOFIC register programming (`al_hal_udma_*`, `al_hal_iofic*`).
- al_eth MAC/EC programming: `al_hal_eth_main.c` (MAC modes, RFW/RSS tables, checksum/TSO descriptor metadata, flow-control regs, clear-on-read stats, Alpine MDIO controller).
- SerDes HSSP register programming: PLL, CDR, TX/RX equalisation, eye measurement (`al_hal_serdes_hssp.c`).
- 10GBASE-R PCS status read; SerDes static TX/RX parameter tables (`al_init_eth_lm.c`, now the whole file).
- al_nand: command FIFO, code-word buffers, BCH engine programming, boot-time config decode from PBS (ECC layout must match the bootloader), IRQ wait.
- al_ssm/al_dma: SSM crypto SA format (`al_hal_ssm_crypto*.c`), RAID/XOR/PQ transaction build and GF tables (`al_hal_ssm_raid.c`).
- al_thermal: sensor enable/power-down, T_VALID/T_INIT_DONE polling, readout conversion (V2 offset 1090, multiplier 3520).
- Board-params decode from stock NOR (`al_eth_boardparams.c`), MAC-from-NOR derivation.
- al_sgpo (`gpio_chip` over the serial-GPO block), `ubnt_hdd_pwrctl` (LED class + gpiod): already generic-framework drivers; no vendor HAL.

## al_dma (12,162 lines, 88% HAL)

- Bus/interface boundary: UDMA RAID queue + SSM RAID opcodes (keep). dmaengine registration is already in place.
- Drop / replace:
  - Hand-rolled txd lifecycle (`al_dma_drv.c:121-133,194-290,443-740`), the ioat-style ring, with no unmap, no re-init, no ACK check (#278).
  - Polling tasklet (`:262-286`) in place of the completion interrupt (IOFIC already configured at probe).
  - Private copy of `dma_cookie_*` (`:34-80`) is unavoidable out of tree (`drivers/dma/dmaengine.h` is private); keep, mirror upstream.
- Generic facility: `dma_descriptor_unmap()` (`include/linux/dmaengine.h:668`, 23 drivers use it), `dmaengine_desc_get_callback_invoke()`, `async_tx_test_ack()`, virt-dma rules (`drivers/dma/virt-dma.h`, private).
- Deprecated/odd APIs: `module_param int` for sizes; `pr_err` in `issue_pending`; sysfs knobs (`al_dma_sysfs.c`, 62 lines) beside devlink (`al_dma_devlink.c`, 300).
- Open: #169 PQ wrong output (53% in dmatest, `DMA_PQ` gated off in `fbdb1a4`). Not explained by anything found here; PQ contract (`scf`, `DMA_PREP_PQ_DISABLE_*`, `max_pq`) is satisfied on read, so the cause is in the HAL transaction or GF setup.
- HAL copies: `iofic.c`, `udma_debug.c`, `udma_regs_{m2s,s2m}.h` byte-identical to al_eth; `udma_main.c`/`udma_config.c` differ (629/1211 vs 604/1214 lines).
- Risk: low (all changes are inside the cleanup tasklet and `prep` functions). Effort: S for items 1-3 of #278, M for the IRQ.
- Test: `dmatest` (memcpy, xor) with `grep dmaengine-unmap /proc/slabinfo` before/after; baseline now `-2`: 60, `-16`: 60, `-128`: 12, `-256`: 12 active.

## al_thermal (4,670 lines, 95% HAL)

- Bus/interface boundary: ~300 lines. Sensor registers at `fd860a00` (enable/power-down, status, trigger, readout) and the V1/V2 conversion constants.
- Drop:
  - `al_hal_pbs_regs.h` (2,794 lines) + `al_hal_pbs_utils.*` + `al_hal_plat_*`, `types`, `reg_utils` (about 3,500 lines): vendored only to read one device-ID register. Replace with a DT compatible per revision.
  - `get_temp` 100-C quirk (`al_thermal_main.c:77-80`): critical trip unreachable (#279).
  - Manual `thermal_zone_device_unregister` + `thermal_add_hwmon_sysfs` (`:195,212`): double free on remove (#280).
- Mainline `thermal_mmio.c` (`amazon,al-thermal`, a single byte at `0x05002860`) is a different register model; cannot replace the sensor access, only the zone/hwmon plumbing it already has.
- Generic facility: `devm_thermal_of_zone_register()` (2022-08), `devm_thermal_add_hwmon_sysfs()` (2019-12), `thermal_of` trips and `hw_protection_shutdown`, `DEFINE_SIMPLE_DEV_PM_OPS`.
- Deprecated/odd: `SIMPLE_DEV_PM_OPS` (`:124`), `.owner = THIS_MODULE` (`:229`), `of_find_compatible_node`+`of_iomap` leak (`:148-155`), `pr_err` without device, `udelay(1000)` x1000 inside `get_temp` (`:40-70`).
- Cross-module: `alpine_serdes.c:60-80` reads this zone and `BUG()`s if absent (#283).
- Risk: low. Effort: S (about 1 day). Result: about 250 lines of driver plus about 650 lines of sensor HAL (`al_hal_thermal_sensor.{c,h}`, `_regs.h`).
- DT note: `dts/alpine-v2-ubnt-unvr-ea16.dts:206` lists `annapurna-labs,al-thermal` and `al,alpine-thermal`; the driver matches only the second.

## al_ssm (30,121 lines, 88% HAL)

- Bus/interface boundary: UDMA queue + SSM crypto SA/descriptor build (keep); `xts_cfg` swap register (all-zero is the only passing config, #70).
- Root cause of #267: kernel >= 7.2 `AF_ALG_CRYPTOAPI_MASK` (`include/crypto/if_alg.h:269`, commit `7524070f26d8`, v7.2-rc1) excludes `ASYNC`, `ALLOCATES_MEMORY`, `KERN_DRIVER_ONLY`. al_ssm is excluded by design. AF_ALG can never reach it; bench-all crypto figures are CE numbers.
- #182 cannot be reproduced through AF_ALG on 7.2+; use dm-crypt over a loop file or `CONFIG_CRYPTO_MANAGER_EXTRA_TESTS`.
- Contract gaps in driver glue (`al_ssm_main.c`), see #282:
  - no `CRYPTO_ALG_ALLOCATES_MEMORY` while allocating 4x `dma_alloc_coherent(GFP_KERNEL)` per request (`:75-86`);
  - `MAY_SLEEP` ignored; `MAY_BACKLOG` semantics wrong (`:473-488`); completion without `local_bh_disable()` (`:307,375,381`);
  - bounce copies instead of `dma_map_sg()` (`:90,288`); SA rebuilt per request instead of at `setkey`;
  - `-EINVAL` for zero length and for > 256 KiB, no fallback (`:73`);
  - hand-rolled pending/backlog/poll queue and 500 us inline spin under `irqsave` (`:148,240`) in place of `crypto_engine` (`crypto/crypto_engine.c`).
- Dead in the module: `al_err_events.c` + `al_err_events_ssm.c` (1,569 lines) are not in the Makefile; `al_hal_ssm_crc_memcpy.c`, `al_hal_ssm_raid.c`, `al_hal_ssm_crypto_cfg.c` (2,381 lines, IPsec/SSL config) are linked but only AES-CBC/XTS is used.
- Generic facility: `crypto_engine`, `CRYPTO_ALG_ALLOCATES_MEMORY` (2020-07), `CRYPTO_ALG_NEED_FALLBACK`, testmgr extra tests.
- HAL generation: newer than al_eth/al_dma; see #264 and #256.
- Risk: med (data path under dm-crypt). Effort: S for flags + tests, M for `crypto_engine` + `dma_map_sg`.

## al_nand (9,128 lines, 85% HAL)

- Bus/interface boundary: command FIFO, code-word buffers, BCH engine, IRQ wait, PBS-decoded boot config (ECC region must match the bootloader).
- Drop / replace:
  - `chip->legacy.*` (`cmd_ctrl`, `read_byte`, `read_buf`, `write_buf`, `dev_ready`, `select_chip`, `waitfunc`, plus `legacy.cmdfunc` inside `ecc_read_page`) -> `exec_op` (#268, `docs/nand-study.md` section 6).
  - Hand-rolled ONFI timing negotiation: SET FEATURES address `0xfa` and payload 6 (`al_nand_main.c:49,879-912`), table `ns[6][7]` (`:159`). Replace with `nand_controller_ops.setup_interface()` (`include/linux/mtd/rawnand.h:1103`), which sends feature `0x01` and verifies with GET FEATURES (#281). Needs no exec_op port.
  - `read_byte` 4-byte word-cache emulation (`:405-470`) disappears with exec_op.
  - ECC OOB geometry derived by hand (`ecc_offset`, `ecc_bytes`): partially DT-declarable (`nand-ecc-strength`, `nand-ecc-step-size`); the offset itself is the bootloader's and stays.
- Done: erased-page test (`nand_check_erased_ecc_chunk`, `bac8230`); bitflip reporting (#274).
- Deprecated: whole driver on `chip->legacy` (core marks it for removal); `nand_prog_page_begin_op/end_op` already used.
- `ecc.read_subpage` returning `-ENOTSUPP` (`:711`) is unreachable: core sets `NAND_SUBPAGE_READ` only for soft ECC (`nand_base.c:6409`).
- Coded-properties handoff (`al_hal_nand_coded_properties.h`, `al_nand_properties_decode/encode`, about 500 lines) exists for the stock bootloader's PBS scratch; with awto-uboot owning the chain, decide whether DT replaces it. Not established.
- Risk: med (write path). Effort: M for `setup_interface`, L for exec_op (`docs/nand-study.md` section 6).

## al_eth (56,079 lines, 76% HAL)

- Bus/interface boundary (keep): UDMA queues, MAC/EC programming, MDIO controller, PCS status, SerDes HSSP programming, SerDes static tables, board params.
- Already generic:
  - 10G port: phylink + `drivers/net/phy/sfp.c` (`al_eth_phylink.c`, only the PCS and three MAC ops are ours); `page_pool` RX; `pci_enable_msix_exact` only because IOFIC needs a sparse entry set (#177).
  - 1G port: phylib + mii_bus.
- Duplicates a generic subsystem:
  - Retired in item 5 (`debris/code/al_eth-vendor-lm/`, `al_eth-kr-an-lt/`, `al_eth-serdes-25g/`): SFF-8472/8436 parser, link detection/establish state machines, retimer drivers, KR AN/LT, 25G SerDes HAL. Duplicate generic: `sfp-bus.c:sfp_parse_support()`, phylink.
  - Open (#237/#128): sfp.c reports 10GBASE-R for optic and DAC alike; the discriminator is `struct sfp_eeprom_id` (`base.sfp_ct_passive/active`) inside sfp.c, which phylink does not hand to the MAC driver.
  - 1G `al_eth_adjust_link` (`al_eth_main.c:2537-2620`): hard-codes `PHY_INTERFACE_MODE_RGMII_ID` (`:2652-2653`), PHY address from board params instead of DT `phy-handle`/`phy-mode`, `al_eth_down()/al_eth_up()` from phylib's state-machine callback, hand-masked `supported` (`:2660-2672`). Generic: `phylink_create()` + `phylink_of_phy_connect()` with `mac_capabilities`, `phy_support_asym_pause()`.
  - Adaptive interrupt moderation (`al_eth_main.c:275-340,1560-1600`, sysfs tables in `al_eth_sysfs.c`): `lib/dim` (`include/linux/dim.h`, 2018-11) + `ethtool -C`.
  - sysfs knobs (`al_eth_sysfs.c`, 790 lines): `small_copy_len` = ethtool tunable `ETHTOOL_RX_COPYBREAK` (2014-09); `udma_dump` -> devlink region/`get_regs`; `intr_moderation` -> `ethtool -C`; `serdes_*` -> DT/phy (below); `link_poll_interval`, `link_training_enable`, `force_1000_base_x` -> phylink.
  - ethtool gaps: no `get_module_info`/`get_module_eeprom_by_page` (`phylink_ethtool_get_module_*`, 2021-04), no `get_fecparam`, no standard stats (`get_eth_mac_stats`, `get_pause_stats`).
  - IRQ affinity: `cpumask` hints built by hand (`:1973-2031`) -> `pci_alloc_irq_vectors_affinity()` (2016-11) or `irq_set_affinity_and_hint()`.
- SerDes: `alpine_serdes.c` is vendor `drivers/soc/alpine/` with module-global state and `BUG()` (#283). Generic: `drivers/phy` provider (`phy_ops`, `phy_set_mode_ext(PHY_MODE_ETHERNET, ...)`, `phy_configure()` 2018-12), MAC consumes `phys = <&serdes N>`; TX/RX tuning moves to DT (#207).
- Already dead on this board (`docs/al_eth-modernization.md`): `CONFIG_ARCH_ALPINE`-else branches, SRIOV, `IS_NIC` checks.
- Risk: med (shared with U-Boot HAL, #256). Effort: S for ethtool/tunables/dim, S for retiring dead state machines, M for 1G phylink, L for the PHY provider.
- Verify before retiring: build `al_eth_10g` with the candidate files removed (link must succeed); symbol list from `nm` on the built `.ko`.

## al_reboot, al_sgpo, ubnt_hdd_pwrctl (no vendor HAL)

- al_reboot (121 lines): writes SP805 `WDTLOAD=1`, `WDTCONTROL` at priority `SYS_OFF_PRIO_HIGH`. Mainline `sp805_wdt` has `.restart` since 2018-05 (`drivers/watchdog/sp805_wdt.c:137`, priority 128, below PSCI 129 which returns and falls through). Differences: mainline writes `WDTLOAD=0` and the module comment says a 1-tick load is the validated sequence. Not established whether mainline's restart resets this SoC. Test needs a reboot (not run): remove `al_reboot`, `reboot`, observe.
- al_sgpo (342 lines): `gpio_chip` over the Alpine SGPO block; boundary is correct.
- ubnt_hdd_pwrctl (232 lines): LED class + gpiod polling; boundary is correct. Poll interval 2 s is justified in the file (no IRQ wired).

## Risks and ordering constraints

- Items 1-4 touch data paths: land each as a separate commit, bench before/after (`scripts/bench-all.py`, `dmatest`, `fio --direct=1`, dm-crypt over loop).
- Item 5 is mechanical: move to `debris/` (project rule), rebuild, `./dev.py gate`, boot, `./dev.py verify-versions`.
- Item 7 changes the 1G link path (RGMII_ID bug history: 1G was RX, `30e7c65`); keep the phy-mode in DT and re-run the 1G RX test.
- Item 10 (#256/#264) comes last: every module change above is cheaper before the HAL copies are merged.

## Not established

- Why #169 (PQ) gives wrong Q; nothing in the dmaengine contract explains it.
- Whether mainline `sp805_wdt` restart resets AL-324 (needs a reboot).
- Hardware effect of the SET FEATURES defect (#281): no GET FEATURES read-back run.
- Whether `dmaengine-unmap-*` leaks in practice (#278): `dmatest` not run, box in use.
- Whether the PBS coded-properties handoff in al_nand can be replaced by DT under awto-uboot.

## Reference drivers

Method: pick the mainline reference first, diff the contract second (lesson of #285; `724094f` reverted by `22ae409`).
Measured 2026-10-11 against `/mnt/2tb/unvr-port-refs/linux-v7.3-fresh` (v7.3-rc6), repo at `22ae409`. Static analysis only: no box access, no flash, dmatest was running.

### Reference table

| Module | Reference (7.3 tree) | Lines | Why |
|---|---|---|---|
| al_dma | `drivers/dma/ioat/dma.c` | 1062 | ring + head/issued/tail + cleanup tasklet = al_dma's ring model (the vendor code is ioat-derived); only ref with `prep_lock`, `DMA_TRANS_*`, abort/reset |
| al_dma | `drivers/dma/fsl_raid.c` | 892 | XOR/PQ RAID offload + cleanup tasklet; list-based so it shows the submit-queue shape. #284 comment says 741 |
| al_dma (2nd) | `drivers/dma/mv_xor_v2.c` | 896 | XOR engine + tasklet; ACK-gated recycle (`:355`). #284 comment says 733 |
| al_ssm | `drivers/crypto/qce/skcipher.c` + `qce/core.c` | 434 + 353 | async `cbc(aes)`/`xts(aes)`, SG DMA, fallback, `crypto_queue` with backlog; same dm-crypt use case |
| al_nand | `drivers/mtd/nand/raw/fsl_ifc_nand.c` | 1155 | legacy hooks + on-host HW BCH + `nand_check_erased_ecc_chunk` + `ecc.read_page` using `cmdfunc` = al_nand's exact shape |
| al_nand (exec_op, `setup_interface`) | `qcom_nandc.c`, `marvell_nand.c` | 2448, 3194 | too large to diff whole; used as patterns only (#268, #281) |
| al_eth | `drivers/net/ethernet/amazon/ena/ena_netdev.c` | 4398 | same vendor lineage, UDMA-style rings, NAPI, page-based RX; used as a contract checklist, not read whole |
| al_eth phylink | none | - | `al_eth_phylink.c` (441) checked against `include/linux/phylink.h` only; no small PCS+SFP MAC reference chosen |
| al_thermal | `drivers/thermal/kirkwood_thermal.c`, `armada_thermal.c` | 116, 969 | valid-bit `-EIO`; bounded valid poll + `devm_thermal_of_zone_register` |
| al_sgpo | `drivers/gpio/gpio-74x164.c` | 209 | output-only shift-register `gpio_chip` |
| al_reboot | `drivers/watchdog/sp805_wdt.c` (`wdt_restart` `:137-150`) | 375 | same SP805 restart |
| ubnt_hdd_pwrctl | `drivers/input/keyboard/gpio_keys_polled.c`, `drivers/leds/leds-gpio.c` | 383, 345 | polled gpio presence; gpio-driven LED |

### New findings, ranked (issues filed 2026-10-11)

| # | Issue | Class | One line |
|---|---|---|---|
| 1 | #286 | fabricate | al_dma fills the HW ring at prep; `issue_pending` doorbells unsubmitted slots; ring order != cookie order; cookie copy lost its `BUG_ON`. Fits the #285 residual |
| 2 | #287 | fabricate | al_dma completion error status completes as success; `callback_result` ignored |
| 3 | #288 | swallow | al_dma failed doorbell / 50 ms stall: no abort; ring wedges; `async_xor` spins, `async_tx_quiesce` panics |
| 4 | #295 | fabricate | hdd bay: gpio read error = "empty" + hot-unplug uevent |
| 5 | #294 | fabricate | al_thermal `ever_valid` latch: stale RESULT converted as temperature (feeds critical trip) |
| 6 | #292 | fabricate | al_nand erased-page re-read ignores error latch: poisoned `0xff` page returned as success |
| 7 | #291 | fabricate | al_ssm: CBC/XTS lengths the mode cannot do reach the engine |
| 8 | #296 | swallow | al_eth drops uncounted; `rx_dropped` constant 0 |
| 9 | #299 | swallow | al_eth probe unwind: live netdev after failed probe; phylink leak |
| 10 | #289 | swallow | al_dma `free_chan_resources`/remove never reset the queue: second alloc `-EIO` |
| 11 | #297 | fabricate | al_eth csum/TSO offload for unsupported L4 (no `skb_checksum_help`) |
| 12 | #298 | swallow | al_eth `al_udma_q_reset()` result discarded; HAL copy lacks `__must_check` |
| 13 | #290 | fabricate | al_dma prep accepts lengths the descriptor cannot encode |
| 14 | #293 | mixed | al_nand: clk fallback, ISR `IRQ_NONE`, `oob_required` WARN_ON |

### al_dma (vs ioat/dma.c, fsl_raid.c)

- Verified matching:
  - unmap before callback, then clear `callback` (`:255-265`; ioat `:625-631`).
  - `dma_cookie_complete` before the callback (ioat `:626`, fsl `:138`).
  - `dma_cookie_status` copy is equivalent to `dma_async_is_complete()` (`include/linux/dmaengine.h:1626`).
- Divergences:
  - Prep/submit ordering: #286. ioat `prep_lock` `:463-466`/`:309-334`, completion guard `if (tx->cookie)` `:625`.
  - Error result: #287. ioat `:721-872`. fsl_raid has the same hole (`:218-222`).
  - Wedge, no abort/reset: #288. ioat `:721-772`, timer `:902`.
  - Queue reset on free/remove: #289. ioat `init.c:596-640`; al_ssm `al_ssm_main.c:842-854`.
  - Length validation: #290. fsl `:331`, `:418`, `:539`.
  - `-ENOSPC` from `al_raid_dma_prepare()` is back-pressure but counts as `stat_errors` + devlink `AL_DMA_FAULT_SUBMIT` (`:509-510`, `:598-599`, `:724-725`).
  - PQ (gated off, #169): `DMA_PREP_CONTINUE` ignored. `dma_maxpq()` reserves 3 slots when the device lacks `DMA_HAS_PQ_CONTINUE`, so the driver must add P/Q as sources (fsl `:341-344`, `:458-459`, `:501-512`). `src_cnt == 1` not handled (fsl `:429-444`). Neither explains #169 (dmatest PQ uses small `src_cnt`).
  - `:397` comment "dma_cookie_init removed in 6.12 - handled automatically" is wrong: `dma_cookie_init()` is still in `drivers/dma/dmaengine.h:15-19` (43 references under `drivers/dma`) and `dmaengine.c` never calls it. Cookies start at 0, not `DMA_MIN_COOKIE`. No effect found.
  - Latent: `completed` (the ring free pointer) advances at `:243`, before unmap/callback/clear (`:260-265`); ioat advances `tail` after the batch (`:651`). Reachable only with about 1023 outstanding ops (the HW tx ring takes one descriptor per source buffer and memcpy/xor have no meta descriptors, `al_hal_ssm_raid.c:76-80`, `:189-195`); not seen.
  - No `device_terminate_all`/`device_synchronize`: same as ioat and fsl_raid, so not a divergence; `dmaengine_terminate_sync()` returns `-ENOSYS`, dmatest ignores it.
  - `dmaengine_desc_get_callback_invoke()` is in the private `drivers/dma/dmaengine.h`, not an exported facility: copy it like the cookie helpers.
- Corrections to earlier statements:
  - "no ACK check" (#278, section above): ioat recycles ring slots without an ACK test (`dma.c:624-631`); fsl_raid and mv_xor use ack lists because they keep descriptors on lists. Not a defect for a ring driver.
  - `22ae409` comment "under the lock, as `fsl_raid.c:185` and `ioat/dma.c:625`": ioat completes under `cleanup_lock`, assigns under `prep_lock` (different locks). The operative change is complete-before-callback; the lock is harmless.
  - `22ae409` leaves the #285 residual (12/20000 xor) unexplained; #286 is the candidate.
- Alpine-specific, keep:
  - `al_raid_dma_prepare/action/completion` and the SSM RAID opcode build (`al_hal_ssm_raid.c`).
  - GF log/antilog table load (`al_raid_init`).
  - `AL_DMA_RAID_RX_CDESC_SIZE` 8, no TX completion ring, `flags = 0` (no NO_SNOOP) - hardware facts from #23.
  - `al_udma_iofic_config(LEGACY ...)` + `al_iofic_abort_mask_clear(S2M_NO_DESC_TO)` at probe.
  - RX completion coalescing disable (`:375-388`).

## Per-module contract divergences (other modules)

### al_ssm (vs qce/skcipher.c + core.c)

- Known: #282, #267. New: #291 (lengths).
- Verified matching: setkey + `xts_verify_key`; CBC output IV written back (`:290-297`; qce `:59`); completion status mapped to `-EIO` (`:262`, `:267`, `:341`).
- Probe fails on queue-init error (`:649-653`): the check al_dma lacks.
- Alpine-specific, keep: SA build (`al_crypto_hw_sa_init`), `xts_cfg` all-zero swap register (`:767-770`, #70), separate cdesc rings (`:620-631`), FIFO completion assumption on a single queue.
- Not a defect: `submit_and_poll` returns `0` synchronously from an `ASYNC` alg (allowed).

### al_nand (vs fsl_ifc_nand.c)

- Known: #268, #274, #275, #281. New: #292, #293.
- Verified matching:
  - erased check via `nand_check_erased_ecc_chunk` with `ecc_stats.failed`/`corrected` bookkeeping (fsl `:639-666`).
  - `legacy.waitfunc` wrapped to carry the error latch (fsl `:605` returns status).
  - `legacy.cmdfunc` inside `ecc.read_page` is a pattern fsl_ifc uses too, so exec_op is not forced by the shape of the read path.
- Alpine-specific, keep: command FIFO, data-buffer FIFO read (`al_nand_data_buff_read`, not memcpy), code-word config, BCH engine control, OOB-before-data ECC read order (`:655-663`), PBS decode, `ecc_offset` from the bootloader layout, WP/TX toggling around PAGEPROG.

### al_eth (vs ena_netdev.c checklist)

- Known: #170, #177, #237, #283. New: #296, #297, #298, #299.
- Verified matching: `napi_gro_receive`/`napi_gro_frags`, `skb_mark_for_recycle` with `page_pool`, BQL (`netdev_tx_sent_queue`/`completed_queue`), `u64_stats` on rings, queue stop/wake with `smp_mb()` on the wake side (`:3593-3602`), `dma_mapping_error` unwind (`:5620-5638`).
- Style, not defects: `napi_complete()` instead of `napi_complete_done()` (`:3605`, `:3762`); `NETDEV_TX_BUSY` after stopping the queue (`:5502-5507`); no re-check after `netif_tx_stop_queue` at `:5604` (ENA re-checks after `smp_mb()`); lost wake needs every in-flight packet to complete inside the window, not reachable with a nearly full ring.
- `al_eth_pcs_get_state` maps a failed `al_eth_link_status_get()` to link down without logging (`:174-175`); `pcs_pre_config` is void so a failed SerDes mode set (`:103-110`) cannot fail the `pcs_config` that follows.
- Alpine-specific, keep: MAC/EC programming, UDMA queue mechanics, checksum/TSO descriptor metadata build (the metadata fields; the fallback decision is #297), MDIO controller, PCS status read, board-params decode.

### al_thermal (vs kirkwood/armada)

- Known: #279, #280, #283. New: #294.
- Alpine-specific, keep: sensor enable/power-down, `T_INIT_DONE`/`T_PWR_OK` readiness, readout conversion (V2 offset 1090, multiplier 3520).
- `get_temp` busy-waits up to 1 s (`:63-72`); armada waits 100 ms at probe and never in `get_temp` (#280).

### al_sgpo (vs gpio-74x164.c)

- Boundary correct; no vendor HAL. Divergences, none filed (no failure scenario on this board):
  - `get_direction` reports OUT for pins still in HW mode (`:146-150`); `set()` on such a pin is a silent no-op while `get()` reads the written value back (`:181-215`). Reachable only by a consumer that skips `direction_output` (`GPIOD_ASIS`); the DT users request OUT.
  - `direction_input` returns `-ENOTSUPP` (errno 524, kernel-internal) (`:217-221`).
  - No `request`/`free` hook returns the pin to HW mode after a consumer releases it.
  - `ngpios` rounds up to whole 8-pin groups (`:283-294`).
- Alpine-specific, keep: `conf_vec` indexed-write scheme (`:88-99`, `:173-174`), `hw_init` sequence.

### al_reboot (vs sp805_wdt.c `wdt_restart`)

- Divergences from `:137-150`: mainline writes `WDTCONTROL = 0` before `WDTLOAD` (ours does not), loads 0 (ours 1, the validated value), and reads `WDTLOCK` back to flush posted writes (ours uses `mdelay(50)`).
- Not established whether mainline's sequence resets AL-324 (needs a reboot).
- Alpine-specific, keep: priority above PSCI (`SYS_OFF_PRIO_HIGH`, PSCI SYSTEM_RESET returns on this firmware, #51); non-exclusive mapping of the shared wdt0 region.

### ubnt_hdd_pwrctl (vs gpio_keys_polled.c, leds-gpio.c)

- New: #295.
- Boundary correct. Poll interval justified in-file (`:34-41`). Bay power stays with the regulators (`:9-13`).

## Not established (this pass)

- Whether #286 is the whole #285 residual: needs xor-only single-thread dmatest on the box (prediction: 0 failures) or a tasklet counter of `txd.cookie == 0` completions.
- Whether a second `alloc_chan_resources()` really fails `-EIO` (#289) and whether the missing reset is part of "UDMA state survives reload" (#23).
- Hardware behaviour for any #291 length, for `l4_proto_idx == 0` + `L4_CSUM` (#297), and for `T_VALID` low (#294).
- What sets a nonzero RX completion status (#287).
- Whether al_nand overwriting the PBS-decoded BCH strength with the ONFI requirement (`al_nand_main.c:945-958`) can differ from the bootloader on another board (equal on this one: 8 bit / 512 B).

## al_ssm port review

Method: three-way diff of the vendor 4.1.37 `drivers/crypto/al`, the 6.12 port (`linux-alpine-v2` `72f1817` before, `e9762bc` after, 2026-04-15, same author) and `modules/al_ssm/al_ssm_main.c` at `6bd513f`. Question per change: did the rewrite keep an invariant the vendor relied on (the question al_dma failed, #285/#286). Static only; box not touched; reviewed 2026-10-11.

### What `e9762bc` changed
- Sync driver -> async: one bounce-buffer op per call, 1000 ms spin (`72f1817` `:62`, `:379-403`) -> `ASYNC` alg (`e9762bc` `:597`), 64-deep in-flight cap (`:69`), `pending` list, 4096-deep private backlog, inline 500 us spin (`:224`), delayed-work poller at 100 us (`:59`).
- Rings: `72f1817` passed `cdesc_base = desc_base` for TX and RX (`:646`, `:655`), so the engine wrote completions over submission descriptors. `e9762bc` allocates four separate rings (`:641-652`). Vendor always had separate rings: this was a port-introduced defect, fixed here.
- `72f1817` (same day) moved `hw_sa` from the stack to `dma_alloc_coherent` (`:323`).
- Commit message says "Retry with backoff (10 attempts)". No such code exists in the diff; `AL_SSM_SUBMIT_RETRIES` was defined and unused.
- Everything after `e9762bc` in the port repo is al_eth / dts / reboot. Our `al_ssm_main.c` is `e9762bc` plus: CBC IV write-back (`:284-298`), stall detector (`:404-417`), devlink counters, xts cfg (#70), queue reset on remove (#167).

### Invariants kept
- Ring order == completion order, one queue:
  - `prepare` + `action` + `list_add_tail(pending)` in one `chan->lock` section (`:205-233`); completion pop + `list_first_entry(pending)` in one section (`:245-257`, `:320-334`). The 1:1 FIFO pairing cannot skew. This is the opposite of al_dma's cookie split.
  - Vendor relied on the same assumption (`desc = get_ring_ent(chan, idx + i)`, `al_crypto_core.c:1089`).
  - Vendor published `head` before the doorbell (`:1418-1426`); the port reaches the same result through the lock.
- Every transaction carries its own SA, IV, src, dst; nothing shared between in-flight ops except SA slot 0 (below).
- Completion error status is mapped to `-EIO` (vendor completes with `0`, `al_crypto_alg.c:635`).
- `72f1817` had the al_dma bug and `e9762bc` fixed it: submit and each completion poll were separate `chan->lock` sections with no per-request ownership, so thread A could pop thread B's completion and return success before its own output was written, while B timed out. The `pending` list ("FIFO-safe") is the fix.

### Invariants lost or weakened
| Invariant | Vendor | `e9762bc` / ours | Effect |
|---|---|---|---|
| Ring-full decision and backlog insert in one critical section | `al_crypto_alg.c:747-752` (`prep_lock`) | `:205-216` under `chan->lock`, insert `:476-489` under `backlog_lock` | #300 |
| One function consumes every completion and then drains the backlog | `al_crypto_core.c:1047-1127`, gate `:1126` | two consumers: worker (counted, `:398-400`) and inline sweep (`:251-268`, uncounted, no drain) | #300 |
| Bounded wait | IRQ-driven; no timeout needed | `72f1817` failed after 1000 ms (`-ETIMEDOUT`); `e9762bc` removed it: a lost completion is a permanent wait. Stall detector (`:409`) only logs and needs `total_pending > 0` | #182 unbounded form |
| Completion wake-up | MSI-X + tasklet (`al_crypto_core.c:982-1040`, `devm_request_irq` `:465`) | poll only; no `request_irq` | #182 |
| Reap before free on teardown | `al_crypto_free_chan_resources`: `tasklet_disable` + final cleanup (`:852-856`) | frees in-flight buffers before queue reset (`:833-856`) | #301 |
| Per-session SA slot, `sa_in` only on miss | LRU cache (`al_crypto_alg.c:793-799`) | `sa_indx = 0`, SA re-sent every request (`:199-201`) | correct only if the engine orders SA update vs earlier in-flight ops; vendor's LRU eviction has the same dependence; not established |
| Metadata descriptor (enc length) | `src_size = enc_in_len` (`al_crypto_alg.c:693`) | `enc_in_len` unset: no meta desc, 3 TX descs/op (devlink: 180 ops -> `tx_next_desc_idx` 28 = 540 mod 256) | works by engine default (selftest passes); behaviour for odd lengths not established, see #291 |

### Divergences with no lost invariant
- TX completion ring: vendor none (`al_crypto_core.c:757`, `NO_COMP_UPDATE`); port allocates one and acks it lazily in `al_crypto_dma_prepare()` (`al_hal_ssm_crypto.c:1176-1183`). Extra DMA write per descriptor; consistent.
- cdesc stride: vendor 8 (`al_crypto.h:44-45`), port 16 (`sizeof(union al_udma_cdesc)`-sized rings). Stride is programmed into the engine (`al_hal_udma_main.c:217`); devlink 2026-10-09: `rx_next_cdesc_idx == rx_hw_comp_head == 180`, consistent.
- Buffers: vendor `dma_map_sg` on request sg; port bounce copies (#282).
- Alg flags `ASYNC | KERN_DRIVER_ONLY`: identical to vendor (`al_crypto_alg.c:1397`).
- `al_crypto_dma_action()` cannot fail (`al_hal_ssm.c:289-300` always returns 0); the `action` error branches in `submit_and_poll` (`:218-224`) are dead.
- Vendor has no XTS; nothing to diff for `xts(aes)`.

### Does `e9762bc` explain the open issues
| Issue | Verdict |
|---|---|
| #267 | No. `KERN_DRIVER_ONLY` is in the vendor and in `42d10e9`; `72f1817` (sync) is excluded by the 7.2 mask just the same. `ASYNC` and missing `ALLOCATES_MEMORY` are both in the mask; kernel change `7524070f26d8` is the cause |
| #182 | In part. Poll-only completion and the unbounded wait are `e9762bc` (`72f1817` returned `-ETIMEDOUT` at 1000 ms). The lost completion itself is not explained: single-thread AF_ALG cannot reach the backlog (#300) or remove (#301) paths. 2026-08-30 sighting ran this code on a kernel with mask 0 |
| #282 | Items 4 (BH), 5 (MAY_BACKLOG), 7 (hand-rolled queue, inline spin) and the `ALLOCATES_MEMORY` gap becoming consequential: `e9762bc`. Items 1-3, 6 (GFP_KERNEL per request, MAY_SLEEP, bounce copies, length limit): inherited from `72f1817`/`42d10e9` |
| #291 | No. Same `nbytes == 0 || > 256 KiB` check in `72f1817`; vendor never validated lengths either |

### New findings
- #300: backlog insert split from the ring-full decision; drain gated on the worker's own completions; backlog-only hang invisible to the stall detector. Vendor has a narrower form of the same race (`al_crypto_core.c:1126` reads `qlen` outside `prep_lock`).
- #301: remove frees in-flight DMA buffers before quiescing the engine, walks lists unlocked, leaves live tfms on a freed `dev` (`crypto/algapi.c:503` only warns).

### Is the al_dma split-invariant pattern present in al_ssm
- Ring/pending/cookie-style ordering state: no. One lock covers claim, doorbell and list append; one covers pop and list remove.
- Same class (state decided in one critical section, acted on in another): yes, in the backlog path only (#300). Lower impact: needs 64 requests in flight and a microsecond window; nothing like al_dma's 0.245%.
- Pre-rewrite `72f1817` had the ordering form (stolen completions). `e9762bc` fixed it.
- al_dma is therefore not a one-off, but in al_ssm the ordering core is sound and the defects sit in the queueing glue the rewrite added.

### Not established
- Cause of the 2026-08-30 #182 wedge: nothing in this review reaches it from a single AF_ALG thread.
- Whether the engine tolerates SA slot 0 being overwritten while earlier ops are in its pipeline (needs a multi-key concurrent test; dm-crypt over two loop devices).
- Engine behaviour with no metadata descriptor for lengths not a multiple of 16 or above 64 KiB (#291).
- Whether 64 in-flight ops ever occur on real load (#300 reachability).
- Anything dynamic: no module was loaded, no counter read.

## al_nand / al_thermal / al_sgpo / al_reboot / ubnt_hdd_pwrctl port review

Method: three-way diff where a vendor original exists. Vendor = UBNT GPL 4.1.37 (`UNVR-1.3.35-GPL/linux-arm64-unvr-4.1.37-ubnt/`). Port = whatever the module actually came from (below). Ours = `modules/<name>/` at `6bd513f`. Question per change: did the port/rework keep an invariant the vendor relied on (the al_dma question, #285/#286). Static only; no box, no flash; reviewed 2026-10-11.

### Provenance: only one of the five is from `linux-alpine-v2`
| Module | Origin | Evidence |
|---|---|---|
| al_sgpo | `linux-alpine-v2` `42d10e9` (KevinKickass, 2026-03-21), unchanged to `e9762bc` | ours differs only by `set()` returning `int` (`4a188c6`) |
| al_thermal | `mornepousse/al_thermal-standalone` `5a6ba59` (2026-03-20), **not** Kevin | `modules/al_thermal/` = that tree byte-for-byte (glue and HAL; `al_hal_plat_services.h` differs by the `al_err` removal, `46af96a`); vendored `b0f84ff` (2026-08-28) |
| al_nand | delroth HAL copy (`21e45bf`) + glue ported by us (`926fb37`..`bac8230`, 2026-10-07..11) | no NAND driver in `linux-alpine-v2` (DT node only, `dts/alpine-v2-ubnt-unvr.dts:580`, disabled) |
| al_reboot | ours (`e3380fd`), from vendor `drivers/power/reset/alpine-reboot.c` | `linux-alpine-v2` declares the DT node (`dts/alpine-v2-ubnt-unvr.dts:182`) with **no driver**; the clone's `al_reboot` commits are ours (`f6b4cde`, `176ef7c`) |
| ubnt_hdd_pwrctl | ours (`725adaf`), not a port of vendor `drivers/power/ubnt-hdd-pwrctl.c` | commit message |

- al_thermal chain: vendor `drivers/thermal/al/` (`al_thermal.c`, HAL byte-identical to the GPL drop) -> mornepousse (README: "extracted from a Linux kernel source drop from QNAP", tested on QNAP TS-431P AL-212, 6.12.77) -> `b0f84ff`. The AL-212 is Alpine V1; the V2 branch (`al_hal_thermal_sensor.c:73-78`, our AL-324) was never exercised by the port's test (`test/run_tests.sh`).
- `linux-alpine-v2` also ships a thermal-zone with a 105 C critical trip (`dts/alpine-v2-ubnt-unvr.dts:593-607`) and a thermal node, but no thermal driver; same for the reboot node. Both are port omissions: protection declared, nothing behind it.
- The task note "al_thermal was excluded from the original vendoring" matches `b0f84ff`: first added 2026-08-28 from mornepousse, not from the vendor tree or Kevin.

### Split-invariant count (headline)
| Module | Strict (vendor one section -> port/ours two) | Adjacent (ordering now conditional/timing) | Where |
|---|---|---|---|
| al_nand | 0 | 1 | #302: `al_nand_main.c:367` latches, `:549-550` consumes without the ready wait (ours, `0bb1fbc`) |
| al_thermal | 0 | 0 | see below |
| al_sgpo | 0 | 0 | one hardware-order note, below |
| al_reboot | 0 | 0 | |
| ubnt_hdd_pwrctl | 0 | 0 | |
| **Total** | **0** | **1** | |

- Vendor-original, carried over unchanged, not counted: unlocked `nfc_int_en` read-modify-write shared by ISR and process context (#303; vendor `irq_lock` declared `al_nand.c:78`, never taken).

### al_nand (vendor `drivers/mtd/nand/al/al_nand.c` -> `al_nand_main.c`)
- Error latch (`op_err`; the vendor has none: `wait_for_irq()` returns `-EINVAL` as `uint32_t` and every caller drops it, `al_nand.c:330,408,628-644`):
  - Set in `cmd_ctrl` (`:367,376`), `read_words` (`:457`), `write_buf` (`:499,518,528`); consumed in `ecc_read_page` (`:672`), `ecc_write_page` (`:738,758,772`), `prog_page_finish` (`:805`), raw/oob hooks (`:785,792`), `waitfunc` (`:549-552`), probe (`:1299`).
  - Not one critical section by construction, but the MTD chip lock (`nand_get_device()`) spans every entry (`nand_erase_nand` `nand_base.c:4524`, read/write/oob paths), so single-slot sharing is sound. Probe runs single-threaded.
  - No stale-latch producer found: every hook that can latch is followed by a consumer in the same op, except the nested raw re-read in `al_nand_erased_or_bad()` (#292).
- Waitfunc early return (#302): the guard comment (`:542-543`) holds only for a ready-wait timeout. `nand_prog_page_end_op`/`nand_erase_op` treat `ret < 0` as final (`nand_base.c:1669-1671,1993-1995`), so a latched CMD-FIFO timeout (#276) on ERASE2 returns `-ETIMEDOUT` while tBERS is running. `0bb1fbc` made that live; before it the result of `:367` was dropped.
- `nand-write-test.py` cannot see it: `:178` `| tail -1`, `:235` `>/dev/null 2>&1`, verdict from a content hash (`:254`).
- Write sequence kept as vendor: `write_buf` (TX on, WP off) -> RNDIN -> SPARE_WRITE_COUNT -> FIFO drain -> WP/TX/ECC restore -> PAGEPROG + WAIT_FOR_READY (`al_nand.c:556-584` vs `:723-773`). The restore happens before PAGEPROG in both; `cmd_ctrl` repeats it after.
- Our HAL edits (`2d202d5`, `88b0c7a`) are consistent: `data_buff_write` honours `data_buff_base` and stores `cw_*_remaining` back as the read path does (`al_hal_nand.c:1636-1650,1726-1741`). Vendor `data_buff_write` hard-coded the no-wrapper register and never stored the accounting; both are fixes, not regressions.
- Attach ordering (not a defect): SET FEATURES (`:928`) runs before the controller timings are reprogrammed (`:1070`); device told first, controller slower than the device in between. Safe direction.
- Vendor-original, already filed: SET FEATURES `0xfa`/payload (`al_nand.c:54,703-715`, #281), `is_empty_oob` (`:427-450`, #274), clk fallback / ISR `IRQ_HANDLED` / `BUG_ON(oob_required)` (#293).
- Whether the vendor's `wait_for_irq` stale-completion behaviour matters: ours re-reads the status each wake (`:253-263`), so a stale `complete()` only costs one loop.

### al_thermal (vendor `drivers/thermal/al/al_thermal.c` -> mornepousse `al_thermal_main.c` -> ours)
- Diff vendor -> mornepousse, glue only (HAL identical):
  - `__thermal_get_temp` (void* devdata, `*temp = 1000 * readout`, `al_thermal.c:54-73`) rewritten to the 6.x ops signature with a new branch: `readout >= 100 -> *temp = 100000 + readout` (mornepousse `:74-80`, comment `:75` "alway return 100"). Vendor, and UBNT 4.19 `drivers/thermal/al_thermal_v2.c:70`, both use `1000 * readout`. **#279 is port-introduced** (fixed `92077bb`). The origin of the quirk is not in any local reference tree (`linux-qnap-tsx32x` has no al_thermal).
  - Registration moved to `devm_thermal_of_zone_register()` but the manual `thermal_zone_device_unregister()` kept, plus new `thermal_add_hwmon_sysfs()`/`thermal_remove_hwmon_sysfs()` calls with hand-written prototypes (mornepousse `:28-29,184,191,207-208`). **#280 (double unregister) is port-introduced.** The vendor pairing was non-devm register + manual unregister (`al_thermal.c:193,219`), which was correct.
  - The vendor's legacy fallback ops divide by 1000 (`thermal_get_temp` `:75-88`, degrees in a millidegree API) and the port dropped them; the vendor `of_ops` path was already millidegrees.
  - Compatible strings changed (`"annapurna-labs,al-thermal"` -> `"al,alpine-thermal"`, `"annapurna-labs,al-pbs"` -> `"al,alpine-pbs"`); `b0f84ff` aligned the DT.
- Not port-caused: #294 (`ever_valid` latch, HAL `al_hal_thermal_sensor.c:280-290`, identical in the GPL drop); the 1000 x `udelay(1000)` poll (`:42,62` vendor), the `of_iomap` leak (`:164` vendor), `SIMPLE_DEV_PM_OPS`.
- Split invariant: `readout_is_valid()` stores `readout_raw` and `readout_get()` converts it (two calls, shared handle state, `al_hal_thermal_sensor.c:263-307`). Both vendor and port call them back to back from `get_temp`. Serialised by the zone lock in both: vendor `thermal_zone_get_temp` takes `tz->lock` (4.1.37 `thermal_core.c:437`), 7.3 `guard(thermal_zone)` (`thermal_helpers.c:134`). A second caller (`alpine_serdes.c`) goes through the same function. Sound by construction.
- Suspend/resume (`:89-120`) are unserialised against `get_temp`: `enable_set(0)` can race a poll. The core stops zone polling around suspend (`thermal_pm_notify`, `thermal_core.c:1364` comment); a userspace hwmon read in the window is the only reachable case.

### al_sgpo (vendor `drivers/gpio/gpio-alpine-sgpo.c` + `drivers/soc/alpine/al_hal_sgpo.c` -> Kevin `42d10e9`)
- Not a port: a from-scratch rewrite (header: "reverse-engineered from the Annapurna HAL", author "secfirst contributors") replacing the vendor driver's `al_hal_sgpo` calls with raw register writes. 340 lines vs 106 + HAL.
- Constants checked against the vendor headers (`al_hal_sgpo_common_regs.h:99-169`, `al_hal_sgpo_pergroup_regs.h:59-128`, `al_hal_sgpo_regs.h:57`): control fields, `OUT_EN_*`, `CPU_VDD_CTRL` mask 0x7, group stride 0x1000, `conf_vec` at +0x400 indexed `1 << bit`, mode 2 bits/pin, USER = 3. All match.
- Behavioural differences from the vendor driver:
  - Vendor never calls `al_sgpo_hw_init()`, never changes pin mode, and has no `get` (`gpio-alpine-sgpo.c:21-36`); it relies on the bootloader's `sgpo_init`. Port runs `hw_init` at probe (`al_sgpo.c:297`, body `:232-258`) and switches a pin to USER in `direction_output` (`:152-179`).
  - Port programs only CNTR_SCALE, `OUT_EN`, `CPU_VDD_EN`, `SGI_ENABLE`; group mode, SATA LED select, invert, blink, stretch stay as the bootloader left them (`dt-gaps-hardware-of-record.md` C1).
- Strict split: none. `direction_output` mode RMW and value write share one `spin_lock_irqsave` (`:163-176`); `set()` is a single indexed write with no RMW, so lock-free is correct.
- Hardware-order note, not filed (no failure scenario): `direction_output` switches to USER (`:166-168`) before writing the value (`:173-174`), so the pin drives whatever `conf_vec` held for one serial frame. Value-then-mode would avoid it. LED pins only; no effect observed or expected.

### al_reboot (vendor `drivers/power/reset/alpine-reboot.c`)
- Register sequence identical to the vendor handler: unlock, `LOAD=1`, `CONTROL=INT|RESET` (`al_reboot.c:54-56` vs `alpine-reboot.c:38-41`). Validated on hardware (#51 fixed 2026-09-03).
- One behavioural change: vendor spins forever after arming (`while (1);`, `:44-45`); ours `mdelay(50)` then returns `NOTIFY_DONE` (`:58-59`), so the chain continues to PSCI if the SP805 did not reset. Intended and stated at `:49-53`; the SP805 resets after two WDOGCLK periods (LOAD=1: interrupt, then reset), so 50 ms covers any WDOGCLK above ~100 Hz; a failed reset falls through rather than hanging.
- Port-caused? Yes, by omission: `linux-alpine-v2` carries the DT node and no driver, so #51 (hang at "Restarting system") exists in the port as shipped.
- Split invariant: none (single function, no shared state).

### ubnt_hdd_pwrctl (vendor `drivers/power/ubnt-hdd-pwrctl.c`)
- Independent rewrite. Vendor coupled presence to power: on a presence change `msleep(pwren_delay)` then `gpio_direction_output(pwren, present)`, then update `present_state` (`:58-66`). Ours drops the power half deliberately (header `:8-12`, regulators own pins 0-3, #104).
- Strict split: none. `present_state` is written by the poll work and read by `present_show()` with no lock; each is a single `bool` and the uevent is raised after the loop (`:101-114`), so a reader cannot see a half-updated set that matters.
- Not port-caused: #295 (new code).
- Vendor extras not carried: sysfs `hdd_force_poweron/off`, `hdd_fault_led<N>` store, exported `ubnt_hdd_fault_led_enabled()`. No caller of the export in the vendor tree.

### Do the filed issues trace to the port
| Issue | Port-caused | By whom / where |
|---|---|---|
| #279 al_thermal 100 C quirk | Yes | mornepousse `5a6ba59` `al_thermal_main.c:74-80`; absent in vendor and UBNT 4.19 |
| #280 al_thermal double unregister | Yes | mornepousse: devm registration + kept manual unregister |
| #294 al_thermal `ever_valid` | No | vendor HAL |
| #281 SET FEATURES `0xfa`/6 | No | vendor `al_nand.c:54,703-715` |
| #274 erased-page test | No | vendor `is_empty_oob` |
| #292, #276, #302 | No | ours (`0bb1fbc` and later); #276's bound is ours, `:367` is made live by `0bb1fbc` |
| #293 | No | vendor (clk, ISR, `BUG_ON`) |
| #295 | No | ours |
| #51 reboot hang | Yes (omission) | `linux-alpine-v2` DT node with no driver |

### Verdict on "systemic across the 6.12 port"
- In these five modules: no. Strict split-invariants: 0. The one adjacent instance (#302) is in our own rework, not the port.
- Port footprint here is small: Kevin contributed only al_sgpo (a faithful rewrite). The thermal defects come from a second third-party port (mornepousse), and the reboot hang is an omission.
- Defect shape in this group: not a split critical section; it is the new-glue-over-old-hardware kind (an invented unit quirk, a kept manual teardown beside devm, a latch consumed too early).

### Not established
- Whether the `command FIFO still busy` warning is ERASE2 and not another entry (#302 rests on count, not identity).
- That a write to a busy chip is dropped silently (#302 consequence 2): ONFI rule only.
- Origin of the `>= 100` branch: mornepousse cites a QNAP drop that is not among the local references.
- Whether `nfc_int_en` races have ever fired (#303): no counter exists.
- al_sgpo `get()` on a pin still in HW mode returns the last written `conf_vec`, not the driven level (not checked against hardware).
- Whether mainline `sp805_wdt` restart resets AL-324 (unchanged from the plan above).

## al_eth port review (vendor 4.1.37 vs `linux-alpine-v2` @ `42d10e9` vs `modules/al_eth/`)

Question: is the al_dma "split invariant" regression (#284/#285/#286) systemic across the 6.12 port, or was al_dma unlucky.

### Verdict

- **al_eth ring/UDMA paths are not split.** TX submit, TX completion and RX refill/poll are the vendor code verbatim apart from API renames; the vendor already ran each in one critical section (netif_tx_lock for submit, single NAPI context for completion).
- 0 split-invariant instances in the data path. 0 are attributable to the port. The 4 instances below arrived later, from replacing the vendor link manager (#113) and from our own fixes.
- What the port did regress is **defaults and fail-stop guarantees**, where it edited rather than copied (table below). All but one are already fixed in our tree.
- Trust calibration: the port is trustworthy where it left code alone (HAL, rings, completion); check every place it collapsed an `#ifdef` or deleted a call.

### Method

- Three-way diff, `diff -w` after normalising API churn: vendor `al_eth.c` vs port `al_eth_main.c` (1458 diff lines, ~216 hunks), HAL files, `al_init_eth_lm.c`, sysfs.
- Vendor UDMA HAL lives outside the driver dir: `drivers/soc/alpine/al_hal_udma_*.c`, `include/soc/alpine/al_hal_udma*.h`.
- Diff sizes, port vs vendor: `al_hal_udma_main.c` 37 lines (debug print + two locals), `al_hal_udma_config.c` 8 (includes), `al_hal_eth_main.c` 40 (see HAL skew), `al_hal_eth_kr.c`/`al_init_eth_kr.c` 0.
- Ours vs port at `c919d82`: 231 diff lines in `al_eth_main.c`; the port is the direct ancestor.
- Every `CONFIG_ARCH_ALPINE` conditional in vendor `al_eth.c` checked for which arm the port kept.

### Data path: one critical section or two

| step | vendor `al_eth.c` | ours `al_eth_main.c` | verdict |
|---|---|---|---|
| ring slot read, skb recorded | `:5547-5549` | `:5519-5521` | one section: whole `start_xmit` under the core's `HARD_TX_LOCK` (no `NETIF_F_LLTX`, `:6021-6034`) |
| `next_to_use++`, descriptors built, `tx_descs` recorded | `:5602-5605` | `:5586-5589` | same section |
| queue stop, doorbell | `:5609-5616` | `:5601-5608` | same section |
| completion walk | `:3417-3558` | `:3502-3608` | single NAPI instance per queue; reads `tx_descs` only for slots the HW has completed |
| stop/wake | `:3499-3507` | `:3590-3602` | `smp_mb()` + re-check under txq lock, same as ENA `ena_netdev.c:887` |
| RX refill: slot claim and post | `refill_rx_bufs` | `:3291-3332` | claim (`next_to_use`) and `al_eth_rx_buffer_add` in one loop, doorbell once, no other writer |
| RX poll: HAL cdesc index vs `next_to_clean` | `:3567-3690` | `:3659-3769` | both advanced in the same pass; error and alloc-fail paths advance by `descs` (`:3697`, `:3707`) |

- The al_dma shape (claim in `prep`, record in `submit`, two lock acquisitions) does not exist in al_eth: no separate prep/submit callbacks, no cookie.
- Ours changed the data path in 4 places, all safe: ring-space check at entry (`:5502-5507`, unreachable: queue stops at `:5601` first and wakes at 256 free, `al_eth.h:60-63`); `WARN_ON_ONCE(tx_descs == 0)` (`:5597`); `smp_wmb()` enabled (`:5585`, vendor had it commented out); TX stats accumulated once per poll (`:3582-3583`, vendor accumulated N(N+1)/2).
- HAL descriptor code is unchanged: `al_eth_tx_pkt_prepare`, `al_eth_comp_tx_get`, `al_eth_pkt_rx`, `al_udma_q_*` have no semantic diff vs vendor.

### Split-invariant instances (strict: atomic in vendor, interleavable now)

| # | where | invariant | broke at | issue |
|---|---|---|---|---|
| 1 | `alpine_serdes.c:154-175` (lock, 0 callers); `al_eth_phylink.c:103-112`, `:146-147`, `:167-174` | vendor held the SerDes group lock across mode set + lane params + establish + status (`al_eth.c:4486-4493`, `:4577-4584`; `al_eth_group_lm.c:503-717`); now each is a separate phylink callback | `9d4680e` (moved group-LM to debris, inside an unrelated commit message) | #310 |
| 2 | `al_hal_eth_main.c:734-770` (addr/data pair); callers `:1585-1590`, `:4400`, `:4497`; poll `al_eth_phylink.c:167,174`; `al_eth_main.c:2749` | KR PCS indirect access is a two-write pair; poll and `reset_task` -> `mac_config` can interleave | `c5dd27d` (PCS reset RMW now on every `up()`); vendor reset_task was already unlocked (`al_eth.c:2497-2505`) | #310 |
| 3 | `al_init_eth_lm.c:2412-2425` (writer), `:739-784`, `:786-830` (reader); `al_eth_phylink.c:116-118`, `:146-147`; `al_eth_sysfs.c:257-276`, `:331-348` | override struct copy, `dirty`, `valid` must be observed together; writer sets `dirty` before `valid`, reader clears `dirty` before applying and holds no lock | `3a1fdb1` (made the vendor no-op store live) | #308 |
| 4 | `al_eth_main.c:2749-2769` (no running check), `:4289` (flag cleared before rtnl release), `:1412` (queue site) | "no reset against a closed interface" holds only if the worker starts during close | `db0b860` (#171 fix) | #307 |

Same shape, not strictly lock-splits:

- `al_eth_main.c:6056` vs `:6072-6075`: netdev published before the moderation setup `ndo_open` reads (vendor `al_eth.c:6124-6136`, same order) -> #306.
- `al_eth_main.c:3920-3941`: `up_complete` failure unwound as if only rings existed; latent, `al_eth_mac_start()` cannot fail on 1G/10G (`al_hal_eth_main.c:1977-2004`) -> #309.

### Port-introduced differences (vendor -> `42d10e9`)

| item | vendor | port | ours now |
|---|---|---|---|
| UDMA error IRQ | `panic()` "to prevent potential user data corruption" (`al_eth.c:1150`) | `//panic(...)`, log only (`al_eth_main.c:1085`): fail-stop became ignore | reset via `reset_task` + re-unmask (`al_eth_main.c:1371-1428`, #175); a reset is a weaker guarantee than the vendor's panic |
| `AL_ETH_DEFAULT_ADAPTIVE_INT_MODERATION` | FALSE on Alpine (`al_eth.c:274-278`) | TRUE (`:257`) | TRUE (`:302`), unfixed -> #304 |
| PHY lookup | `phy_map[adapter->phy_addr]` (`al_eth.c:886,2274,2402,4918`) | `phy_find_first()`: wrong PHY on a bus with more than one | `mdiobus_get_phy(bus, phy_addr)` (`:2478,2643,2698,4541`) |
| UDMA state change | silent | `pr_info` + extra MMIO read per change (`al_hal_udma_main.c:480-492`) | `pr_debug` |
| i2c SFP access | `i2c_put_adapter()` after each transfer (`al_eth.c:3975,4020`) | `i2c_get_adapter()` with no put (`al_eth_main.c:3514,3543`): adapter refcount leak | code removed (sfp.c owns i2c) |
| `select_queue` | per-CPU `smp_processor_id()` | unchanged | `netdev_pick_tx()` (`:5656`, #121) |
| `TX_TIMEOUT` | 5 s | 30 s (`:82`); no recorded reason | 30 s (`:97`); 6x slower TX-hang recovery |
| `al_eth_down`/`up` ordering, napi/irq setup | - | unchanged | unchanged |
| `HIGHEST` moderation interval | assigned `[HIGHEST]` twice (vendor typo, `al_eth.c:1315-1318`) | assigns `[HIGH]` then `[HIGHEST]` (fixed) | fixed |

### HAL version skew (not edits)

- Port HAL/LM is the older UDM-era Alpine drop; the UNVR 4.1.37 tree is newer. Differences are skew, not port edits.
- `al_hal_eth.h`: no `AL_ETH_RETIMER_DS_125`; one merged `gpio_spd_1g_10g` field (vendor decodes the same mask into two fields, `al_hal_eth_main.c:5175-5182`, so equivalent).
- `al_init_eth_lm.c` (479 diff lines): no `lm_context->lock`, no DS125 retimer table, no 1G-DA SerDes tables, byte-wise SFP EEPROM reads, `signal_is_detected` early exit, link-state priority `local_fault` before `link_up` (`al_eth_lm_link_state_get`). Moot: LM is retired for the SFP+ port (phylink, #113); `board-params-handoff.md:73` has `retimer_exist = false` for this board.
- V2 `ec_c_mask` merged into `AL_ETH_V1_V2_...` (`al_hal_eth_main.c:141-144`) is bit-identical to vendor V2 (`AL_ETH_V2_EC_IOFIC_GROUP_C_ERROR_INTS`): no change on AL-324.

### Filed al_eth issues vs the port

| issue | origin |
|---|---|
| #296 rx_dropped always 0 | vendor: `al_eth.c:4798` `stats->rx_dropped = 0` |
| #297 csum/TSO no fallback | vendor: `al_eth_tx_csum` `al_eth.c:5447-5512` identical |
| #298 `al_udma_q_reset` result discarded | vendor: `al_eth.c:393,397` |
| #299 probe leaves live netdev | vendor: `al_eth.c:6124-6131`; phylink leak is ours |
| #283 `alpine_serdes` `BUG()` | vendor: `drivers/soc/alpine/alpine_serdes.c:55-75`, port copy identical |
| #237 SFP media type | ours (phylink) |
| #253 | U-Boot al_eth, not the Linux driver |
| #188 AXI snoop comment | port wrote a comment citing a non-existent quirk; vendor `hw_init_adapter` has no per-driver snoop either, so no behaviour regression (coherence is DT `dma-coherent`, verified `yes` on both ports) |

None traces to the port.

### New findings in code the port left alone

- #305: `al_eth_rx_poll()` returns under budget without `napi_complete()` when refill falls short (`al_eth_main.c:3752-3759`); NAPI stuck, RX queue dead until down/up. Vendor-original (`al_eth.c:3686`). ENA completes unconditionally (`ena_netdev.c:1491`) and has `check_for_empty_rx_ring` (`:3587`).
- Style, not filed: `napi_complete()` vs `napi_complete_done()` (`:3605`, `:3762`); `NETDEV_TX_BUSY` entry check is dead code (above).

### Alpine-specific, must stay

- UDMA queue/descriptor mechanics and completion accounting in descriptor units (`al_hal_udma_*`, `al_eth_tx_pkt_prepare`, `al_eth_comp_tx_get`, `al_eth_pkt_rx`, `tx_descs`).
- TSO MSS fix for the HW minimum frame (`al_eth_tx_tso_mss_fix`, `:5345-5383`) and explicit `eth_skb_pad` (MAC pad not zeroed, `:5512`).
- EC/MAC/forwarding/RSS-thash table programming, IOFIC moderation registers, board-params scratchpad decode and FLR save/restore (`al_eth_function_reset`, `:2772-2829`).
- KR PCS indirect window, HSSP/25G SerDes drivers, shared MDIO controller, `AL_ETH_RX_OFFSET`/small-copy RX layout.
- Replaced or replaceable by mainline: buffer management (page_pool, done), link management (phylink/sfp, done for SFP+), MDIO bus glue, ethtool plumbing, sysfs knobs.

### Not established

- Whether the #305 refill-fail branch has fired on the box, and the width of the #306 open-vs-probe window (no instrumented run).
- Hardware effect of a misdirected KR PCS `pcs_addr` write (#310).
- Whether #304's flip is part of #184 (the thread only shows the hypothesis).
- Static analysis only; no box access this pass.
