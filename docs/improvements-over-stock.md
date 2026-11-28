# Improvements over stock

Stock = vendor kernel 4.1.37 (UNVR 1.3.35) / 4.19.152 (5.1.25) with U-Boot 2015.07-alpine_db-2.21-HAL. Ours = mainline Linux 7.4.0-rc4 + awto-uboot (chained from NAND, #216). Same hardware.

## Headline: speeds
| Path | Stock | Ours | Source |
|---|---|---|---|
| 10G Ethernet (iperf3, TCP) | **TBA** (measuring; vendor ships the same optic equalisation value that gave us 34-594 Mbit/s) | **9.3-9.4 Gbit/s**, 0 retransmits | #121, bench-20261117 |
| 1G Ethernet | **TBA** (measuring) | **941 Mbit/s** | #354 baseline 2026-11-13 |
| SATA SSD sequential read | **492 MB/s** (stock 4.19.152, fio 1M direct, 15 s, 100% of one core; 2026-11-28, #381) | **~500-550 MB/s** | bench-history/overhead-20261128-* |
| USB 3 SSDs (3 behind one hub) | **393 MB/s** for ONE M.2 SSD on stock 4.19.152 (usb-storage, no UAS; combined run not completed; 2026-11-28, #381) | **413 MB/s total** (stock cmdline has the same `pci=pcie_bus_perf`; our Fedora args had dropped it, 373 without) | #384, 2026-11-28 |

Stock numbers: same iperf3 / fio tests on the stock 4.19 kernel on this unit, in progress (#381). Public stock numbers: none found.

## Headline: bugs squashed (present in the vendor code, confirmed in source)
| Bug | Effect on stock | Fix |
|---|---|---|
| 10G optic equalisation never calibrated for optics (stock board config also assumes a 3 m DAC) | 10G link collapses to hundreds of Mbit/s with retransmits | #121, #199, #200 |
| RX poll completes without `napi_complete` (4.1.37) | receive queue stalls under memory pressure until ifdown/ifup | #305 |
| Ethernet stop does not flush RX before waiting for idle | teardown fails -110, poisons next init | #343 |
| Crypto engine truncates buffers over 64 KiB | silently corrupt output | #332 |
| NAND treats parity-less pages as erased | corrupt pages read back clean, UBI never scrubs | #274 |
| NAND timing mode set without read-back | chip may sit in the wrong timing mode | #281 |
| Thermal sensor keeps converting a stale reading | wrong temperature after the first sample | #294 |
| Thermal driver leaks a mapping and busy-waits up to 1 s | CPU held on a sysfs read | #280 |
| ~62 hardware drop/error counters never collected (empty stub) | no visibility into drops | #212, #235 |

9 groups confirmed in vendor source with file:line (#381 comment of 2026-11-24); about 85 further bugs were ours from the port and are not counted.

## Detail

Every deliberate difference from stock, one row each: what, why, evidence, effect.

- Add a row whenever a setting, patch or config differs from stock. Link the issue and the measurement file.
- "Stock" column: `same` = vendor has the identical value/code, `differs` = shown in vendor source, `NM` = not measured on stock (no stock benchmark exists yet, #381).
- Effects are measured values with date and conditions in the linked file, never estimates, unless marked `est`.
- Not an improvement list: bugs we introduced during the port are not here (see #381 class 2).
- Audit of recommended-but-not-done items: [improvements-audit.md](improvements-audit.md).

## Network (10G SFP+ and 1G)
| Change | Stock | Evidence / effect |
|---|---|---|
| Optic TX equalisation `c_plus_1` 0x2 -> 0x5 (and U-Boot copy) | `same` (0x2 in `optic_tx_params`, vendor `al_init_eth_lm.c:849-855`, table never calibrated for optics) | #121: 34-594 Mbit/s with thousands of retransmits -> 9.42 Gbit/s, 0 retransmits. QNAP moved the same parameter on the same SoC 0x2 -> 0x4. Stock throughput NM |
| TX queue selection `netdev_pick_tx()` (stable per flow, XPS-aware) | `differs` (vendor `qid = smp_processor_id()`, `al_eth.c:5644`) | docs/network-throughput-testing.md |
| RX NAPI poll completes under budget (`napi_complete`) | `differs` (4.1.37 `al_eth.c:3683-3686` reschedules without completing; 4.19 fixed) | #305: RX queue stall under memory pressure removed, fix 2cb8f10 |
| UDMA stop flushes RX before waiting for idle | `differs` (gen A HAL waits without stopping RX, `al_hal_eth_main.c:539-565`) | #343: teardown no longer fails with -110 |
| Hardware stats (~62 EC/UDMA counters) collected | `differs` (`al_udma_stats_get()` is an empty stub in the vendor tree) | #212/#235 |
| SFP via phylink/sfp.c, module EEPROM read | `differs` (vendor LM hardcodes optic) | #127, #128, #133 |

## USB
| Change | Stock | Evidence / effect |
|---|---|---|
| `pci=pcie_bus_perf`: xHCI link payload 128 -> 256 B | `same` (stock cmdline `pci=pcie_bus_perf`, docs/hw-reference/20260816-104601/live.dts:14): parity, not an improvement; restored after our bootargs omitted it | #384, 2026-11-28 (3 USB SSDs behind one hub, read-only fio 1M QD32): each 355-372 -> 407-412 MB/s, total 373 -> 413 MB/s (+11 %) |
| UAS on both SSD bridges (JMicron, ASMedia) at 5 Gbps | NM | #384: both enumerate 5000M driver `uas` |

## Storage, thermal, power
| Change | Stock | Evidence / effect |
|---|---|---|
| Bay power as per-bay `regulator-fixed` so AHCI probes after the rails are up | NM | #104, docs/sata-bay-power-ordering.md |
| All 8 AHCI ports probed (4 per controller); port mask no longer in default bootargs | `same` (stock implements all 8) | #377, verified 2026-11-28: 4 populated ports 6.0 Gbps, 4 empty link down, no boot-time cost |
| Fans follow drive temperature (on at 60 C, off at 55 C) and SoC (on 80 C, off 50 C) | NM | `scripts/unvr-fand.py`, #373 |
| SoC die temperature (al_thermal) with the critical trip working | `differs` (the 105 C trip cannot fire in the imported fork; vendor returns `1000*temp`) | #279 (our regression, fixed 92077bb), #294 vendor `ever_valid` latch fixed e17bb76 |
| Btrfs RAID1 mirror, bees dedupe, per-file SHA-256 xattr cache | new capability | #372, #373, #378 |
| White logo LED on at power-up, blue = disk activity | NM | #123 |

## Crypto, DMA, NAND (vendor defects fixed)
| Defect present in the vendor tree | Fix | Effect |
|---|---|---|
| al_ssm truncates buffers over 64 KiB (16-bit descriptor length; vendor `al_crypto.h:428-461`) | 367f876 | corrupt output for large requests; vendor dm-crypt sizes stayed below it |
| NAND `is_empty_oob()` treats parity-less pages as erased (vendor `al_nand.c:427-448`) | bac8230, a2e0f98 | corrupt pages no longer read back clean; UBI can scrub |
| NAND ONFI SET FEATURES 0xfa with controller-enum payload (vendor `al_nand.c:54,707-762`) | 6f94327 | timing mode negotiation |
| al_thermal PBS mapping leak and 1 s busy-wait (vendor `al_thermal.c:57-63,157-164`) | 580b279 | mapping released; sysfs temperature read no longer holds a CPU up to 1 s |

Full verified list with file:line: issue #381 comment of 2026-11-24.

## Boot and platform
| Change | Stock | Evidence |
|---|---|---|
| Linux 4.x -> mainline 7.4-rc4 | differs | #4, #6, #13, #41; branch awto-au/linux `unvr/port-7.4-rc4` |
| awto-uboot 2026.x chained from NAND; boot profiles from SSD without a console | differs | #216, docs/boot-select.md |
| RTC works at 400 kHz I2C (no ABORT wedge) | n/a (4.19 has no abort code) | #86 |
| Reboot via SP805 restart handler | differs | #51 |

## Pending (agent campaign, results added here when measured)
- 10G receive costs 2-2.6 of 4 cores at ~1.2 GB/s with ~100k interrupts/s (interrupt moderation off, MTU 1500); with SATA + USB running together 10G receive falls 1218 -> 884 MB/s (2026-11-28 baselines, bench-history/overhead-*.json).
- AHCI and xHCI interrupts all land on CPU0.
- Payload/read-request size on the internal endpoints (all 128 B, capability 256 B).
- Measured 2026-11-28, #386 (bench-history/overhead-20261128-*-T-rxus*.json, 1 run each, not yet repeated 3x): `ethtool -C enp0s2 rx-usecs 128` takes 4-stream 10G receive from ~98k to ~50k IRQ/s and 264% to 243% of 400% busy at the same 1218 MB/s; rx-usecs 8-32 no gain. GRO off: 398% busy (worse). RPS, netdev_budget 150/600, busy_poll, larger tcp_rmem: no gain (busy_poll worse, 342%).
- Measured 2026-11-28, #386: DevCtl MaxPayload/MaxReadReq bits on 00:05.0 (al_dma) do not stick (write 0x1020, 0x2000, 0x0020 read back 0000; only bit 11 NoSnoop-enable sticks); 00:04.0 same. A setpci/PCI-fixup for MPS/MRRS on these endpoints is not a lever. 00:08.0/00:01.0/00:02.0/00:09.0 not written.
