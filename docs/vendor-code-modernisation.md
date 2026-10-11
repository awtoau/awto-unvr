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

| # | Work item | Bugs prevented | Effort | Risk | Issue |
|---|---|---|---|---|---|
| 1 | al_dma: dmaengine descriptor contract | silent leak/corruption on md RAID5 (XOR advertised, `MD_RAID456` built) | S | low | #278 |
| 2 | al_thermal: report correct millidegrees, fix remove path, drop PBS HAL | critical trip can never fire (confirmed); double free on rmmod | S | low | #279, #280 |
| 3 | al_ssm: crypto-API contract (flags, GFP, backlog, BH) + test via dm-crypt/EXTRA_TESTS | #267 closed by reading (AF_ALG mask); contract gaps feed #182 | S flags/tests, M engine | med | #282, #267, #182 |
| 4 | al_nand: `setup_interface()` for ONFI timing, then exec_op | wrong SET FEATURES address/payload | M (+L for exec_op) | med | #281, #268 |
| 5 | Retire unreferenced vendor state machines to `debris/` | removes ~14k lines nothing executes (8.5k conditional on SerDes group) | S, mechanical | low | umbrella |
| 6 | al_eth: ethtool completeness, tunables, `dimlib` | missing `ethtool -m`/FEC; sysfs knobs duplicate ethtool | S | low | umbrella |
| 7 | al_eth 1G: phylib `adjust_link` -> phylink | one link path for both ports | M | med | umbrella |
| 8 | SerDes as `drivers/phy` provider + DT tuning | #207, `BUG()`s, per-module copies | L | med | #283, #207 |
| 9 | al_reboot -> `sp805_wdt` restart | one less module; unproven | S (needs reboot) | med | umbrella |
| 10 | One HAL source tree | 4 diverging UDMA copies | L | med | #256, #264 |

- 1-4 are the ones that can hide a #274: silent data-path divergence from a generic contract.
- 5 is first in line for effort/benefit on audit time: nothing in it can be wrong at runtime, but it is ~14k lines every future review has to rule out.

## Alpine-specific code that stays (all modules)

- UDMA M2S/S2M queue, descriptor, completion-ring and IOFIC register programming (`al_hal_udma_*`, `al_hal_iofic*`).
- al_eth MAC/EC programming: `al_hal_eth_main.c` (MAC modes, RFW/RSS tables, checksum/TSO descriptor metadata, flow-control regs, clear-on-read stats, Alpine MDIO controller).
- SerDes HSSP register programming: PLL, CDR, TX/RX equalisation, eye measurement (`al_hal_serdes_hssp.c`).
- 10GBASE-R PCS status read; SerDes static TX/RX parameter tables (`al_init_eth_lm.c:739-833`).
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
  - `al_init_eth_lm.c:400-720`: SFF-8472/SFF-8436 byte-offset parser (`SFP_I2C_HEADER_*`, `QSFP_*`) = `sfp-bus.c:sfp_parse_support()`. Reachable only from the unreferenced establish path. Keeping it does not solve #237/#128 (sfp.c reports 10GBASE-R for optic and DAC alike): the discriminator is `struct sfp_eeprom_id` (`base.sfp_ct_passive/active`) inside sfp.c, which phylink does not hand to the MAC driver.
  - `al_init_eth_lm.c:1849-2410` link detection/establish state machine, retimer lock checks (`:1424-1600`), `al_eth_lm_link_check`: phylink's job. No caller outside the state machine itself (`al_eth_lm_link_establish`, `al_eth_lm_link_detection`, `al_eth_lm_link_check`, `al_eth_lm_pause_check` have none).
  - KR AN/LT: `al_init_eth_kr.c` (830) + `al_hal_eth_kr.c` (1,029) + `al_hal_eth_kr.h` (372); only caller is `al_eth_an_lt_execute()` from the dead establish path. Mainline has no clause 72 framework; keep as register-level HAL only if 10GBASE-KR backplane is ever needed.
  - 25G SerDes (`al_hal_serdes_25g.c` 3,395 + internal regs 4,616 + regs 434 + `.h` 79 = 8,524 lines): selected only for SerDes group 4 (`al_serdes.c`); the 1G port has no SerDes (dmesg: "no serdes on this port"). Confirm the 10G port's group from board params before retiring.
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
- Which SerDes group the 10G port uses (25G HAL retirement depends on it).
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
