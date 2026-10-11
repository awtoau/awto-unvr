#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Issue clusters: source of truth for docs/issue-clusters.md and the issue comments.

Data lives in CLUSTERS below. One run can:
  --write-doc         render docs/issue-clusters.md
  --file-umbrellas    create the per-cluster umbrella issues (skips existing titles)
  --post-comments     one cross-reference comment per member (skips already-posted)
  --post-dups         duplicate notices (DUPS); never closes anything
Default is a dry run: prints what would be done. --apply performs it.

Idempotent: comments carry an HTML marker and are not posted twice.
Closed issues are skipped. Log: tmp/logs/issue-clusters.log.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
LOG = REPO / "tmp/logs/issue-clusters.log"
DOC = REPO / "docs/issue-clusters.md"
GH_REPO = "awtoau/awto-unvr"
MARK = "<!-- issue-clusters -->"
MARK_DUP = "<!-- issue-clusters-dup -->"
MARK_UMB = "<!-- issue-clusters-umbrellas -->"

# members: issue -> (step in `order`, role). related: issue -> role.
# umbrella: title of the umbrella issue, or None.
CLUSTERS: list[dict] = [
    {
        "id": "ssm",
        "name": "al_ssm / crypto",
        "umbrella": "Umbrella: al_ssm - AF_ALG unreachable, async-queue port, remove and length gaps",
        "disp": "LINK",
        "root": (
            "Port e9762bc replaced the sync driver with a hand-rolled async queue "
            "(poll-only completion, nothing bounds a pending request, private backlog); "
            "vendor 4.1.37 used MSI-X + tasklet + one sw_queue under one lock. Separately, "
            "kernel >= 7.2 masks ASYNC/KERN_DRIVER_ONLY out of AF_ALG (7524070f26d8), so the "
            "only path that reaches al_ssm is dm-crypt."
        ),
        "members": {
            267: (
                1,
                "AF_ALG cannot reach al_ssm on >= 7.2 (by design); every bench-all crypto number is xts-aes-ce. Fix is in bench-all, see cluster `bench`",
            ),
            282: (
                2,
                "crypto-API contract gaps (7 items); item 7 = hand-rolled queue vs crypto_engine",
            ),
            291: (
                3,
                "CBC/XTS lengths the mode cannot process reach the engine and report success",
            ),
            301: (
                4,
                "remove frees in-flight DMA buffers before quiescing; UAF on tfm->dev",
            ),
            300: (
                2,
                "backlog insert split from ring-full decision; request can be lost (obsolete if crypto_engine adopted)",
            ),
            182: (
                5,
                "D-state wedge; lost completion unexplained; not reachable via AF_ALG on 7.3",
            ),
        },
        "related": {
            264: "U-Boot-side HAL generation shim for al_ssm (cluster `hal`)",
            312: "bench-all `--ssm` is the dm-crypt path",
            284: "modernisation umbrella, item 3",
        },
        "order": [
            "Make the path testable: dm-crypt on a loop file (`bench-all --ssm`) + CONFIG_CRYPTO_MANAGER_EXTRA_TESTS=y. #267 closes by relabelling bench-all, not by a driver change.",
            "Decide #282 item 7: adopt crypto_engine or keep the hand-rolled queue. Adopt -> #300 and the pending/backlog half of #301 are obsolete.",
            "#291 length validation: independent, small, needed under either design.",
            "#301 remove path: quiesce the engine before freeing buffers (independent half); list walk depends on step 2.",
            "#182 last: lost completion unexplained, a single AF_ALG thread cannot reach #300's path. Retest only after 2-4, via dm-crypt, with the stall detector and `devlink health diagnose`.",
        ],
        "blockers": "`al-ssm-stress --unbind-under-load` can crash the box (#301 test); #182 needs exclusive box + console (#243).",
    },
    {
        "id": "dma",
        "name": "al_dma",
        "umbrella": "Umbrella: al_dma - one hardware pass for the landed completion-path fixes, then PQ",
        "disp": "LINK",
        "root": (
            "al_dma_drv.c completion path (cleanup tasklet, issue_pending, recovery) "
            "reimplemented the dmaengine contract. Landed, host-tested, not hardware-verified: "
            "1d6b480 (#278), 5455587 (#287), c2d432c (#288), c073b10 (#311), 71272b8 (#169 partial)."
        ),
        "members": {
            311: (1, "ring slot freed before its callback ran; landed c073b10"),
            287: (
                1,
                "completion error reported as success, callback_result never called; landed 5455587",
            ),
            288: (
                1,
                "failed doorbell / 50 ms stall leave descriptors that never complete; landed c2d432c",
            ),
            278: (
                2,
                "no dma_descriptor_unmap(), stale txd fields; landed 1d6b480, leak NOT demonstrated",
            ),
            169: (
                3,
                "PQ engine wrong output, 53% data error; PQ withdrawn unless enable_pq=1",
            ),
        },
        "related": {
            32: "md RAID baseline: the array step 2 needs",
            241: "XOR/PQ offload vs 15374 MB/s software XOR is a residue there",
            284: "modernisation umbrella, item 1",
        },
        "order": [
            "One hardware pass, no flash writes: `bench-all` dma subtest (dmatest copy+xor, many threads on one channel = #311's trigger; `--dma-reload`). Verifies #311 #287 #288 by absence of regression.",
            "md RAID5 load with DMA_XOR to look for #278's unmap leak (slab counts cannot show it, per #278).",
            "#169 last: `modprobe al_dma enable_pq=1`. Q ops are limited to 8 KiB (AL_RAID_OP_Q_MAX_BLOCK_SIZE 1<<13; status 0x10 = CMD_DECODE_ERROR) vs the 16 KiB test size; 53.3% against ~50% expected is 2.5 sigma, so a second cause is possible.",
        ],
        "blockers": "Exclusive box; step 2 needs the array from #32.",
    },
    {
        "id": "nand",
        "name": "al_nand",
        "umbrella": "Umbrella: al_nand - verify the landed fixes in one flash session, then OOB layout, oob_required, exec_op",
        "disp": "LINK (+ MERGE #276 into #302)",
        "root": (
            "Legacy cmd_ctrl driver on 2013 vendor code that reimplements nand_base contracts "
            "(erased-page test, waitfunc, ECC reporting, OOB layout). No mainline Alpine NAND "
            "driver exists (#268 survey). Landed, not hardware-verified: 9a2c4ff + bac8230 (#274), "
            "e9bf6be (#276 #302), a2e0f98 (#292), 2a3c5b2 (#293), 403feff (#303), 6f94327 (#281)."
        ),
        "members": {
            274: (1, "bitflip reporting + erased-page test; landed 9a2c4ff, bac8230"),
            276: (
                1,
                "64 us FIFO bound fires on erase; landed e9bf6be together with #302 (duplicate)",
            ),
            302: (
                1,
                "waitfunc returns a latched FIFO timeout without nand_wait(); landed e9bf6be",
            ),
            292: (
                1,
                "failed erased-page re-read returns an all-0xff page; landed a2e0f98",
            ),
            293: (1, "clock fallback / IRQ_NONE / oob_required; landed 2a3c5b2"),
            303: (1, "nfc_int_en read-modify-write unlocked; landed 403feff"),
            275: (2, "OOB layout claims 220 ECC bytes, hardware uses 128; untouched"),
            317: (
                3,
                "write_page with caller OOB refused (-ENOTSUPP); decision needed, follow-up to #293 item 3",
            ),
            268: (4, "exec_op port; checklist items 1-2 done (36cb313, 2d202d5)"),
        },
        "related": {
            208: "driver audit; write path verified once 2026-10-10",
            166: "flash policy / recovery",
            30: "MTD layout",
            284: "modernisation umbrella, item 4",
        },
        "order": [
            "ONE hardware session verifies #274 #276 #302 #292 #293 #303 together (one file; #302 says they compound). Needs the owner's flash-write approval: `nand-write-test.py --write` on the erased mtd1 `device_tree`.",
            "#275 decision (OOB 220 vs 128 B; unread `max-onfi-timing-mode`) before #317: #317's free-OOB range [2, ecc_offset) depends on the layout.",
            "#317 decision: second partial PAGEPROG (needs Micron MT29F8G08 NOP count check + a flash write) or keep -ENOTSUPP (nothing here writes OOB through the ECC path).",
            "#268 exec_op port last: it rewrites cmd_ctrl, so step 1's verified legacy path is its regression baseline. Effort L (#284 item 4).",
        ],
        "blockers": "Every write step needs explicit owner approval (CLAUDE.md, #166). No bad block exists on the chip (0/4096) to exercise bad-block marking.",
    },
    {
        "id": "thermal",
        "name": "al_thermal",
        "umbrella": None,
        "disp": "LINK",
        "root": (
            "Port-introduced (mornepousse al_thermal, validated on Alpine V1 not V2): millidegree "
            "conversion, double unregister, busy-wait. Landed: 92077bb (#279), 580b279 (#280), "
            "e17bb76 (#294). #316 is the one measurement left."
        ),
        "members": {
            316: (
                1,
                "conversion cadence unmeasured; 100 ms T_VALID bound is armada_thermal's, not ours",
            ),
            279: (2, "critical trip could never fire; landed 92077bb"),
            280: (2, "remove path / PBS leak / hwmon / busy-wait; landed 580b279"),
        },
        "related": {284: "modernisation umbrella, item 2"},
        "order": [
            "#316: read-only ssh measurement (command in its body), set AL_THERMAL_VALID_TIMEOUT_US to 1.25x the worst conversion period.",
            "Verify #279 and #280 on the box in the same session; close all three.",
        ],
        "blockers": "None; read-only.",
    },
    {
        "id": "eth",
        "name": "al_eth lifecycle and datapath (port review)",
        "umbrella": "Umbrella: al_eth lifecycle and datapath invariants from the port review (#284)",
        "disp": "LINK (+ MERGE #306 into #299)",
        "root": (
            "al_eth_main.c breaks init-before-publish and reverse-order teardown; reference "
            "is ena_netdev.c. Not systemic to the 6.12 port: #309 (bb0f1cd) and #307 (db0b860) "
            "are ours, #305/#306 are vendor-original. Landed: e96acdf (#299, also resolves #306), "
            "2cb8f10 (#305), 17cd220 (#296 #313), f0ccaec (#314), 7e07475 (#297)."
        ),
        "members": {
            306: (
                1,
                "probe publishes netdev before moderation setup; resolved by e96acdf (duplicate of #299)",
            ),
            299: (
                3,
                "probe failure after register_netdev leaves a live netdev; landed e96acdf",
            ),
            309: (
                2,
                "al_eth_up() unwinds an up_complete() failure as if only rings existed; NOT fixed",
            ),
            307: (
                2,
                "reset_task has no running check; close guard cleared before rtnl released; NOT fixed",
            ),
            305: (3, "rx_poll stalls the queue on a short refill; landed 2cb8f10"),
            313: (
                3,
                "start_xmit head-map failure leaves a freed skb in the slot; landed 17cd220",
            ),
            314: (3, "rx_poll zero-budget guard; landed f0ccaec"),
            298: (
                3,
                "al_udma_q_reset() failure discarded; al_eth HAL lacks __must_check",
            ),
            297: (
                4,
                "TX csum fallback; premise wrong on 7.3, defensive fix landed 7e07475",
            ),
            272: (
                4,
                "three classification IDs have no enumerator (l4_proto_idx=2); needs chip/docs",
            ),
        },
        "related": {
            184: "unattributed hang under UDP TX load; none of these is established as its cause",
            254: "same UDMA reset/state class (cluster `ueth`)",
            301: "same quiesce-before-free class (cluster `ssm`)",
            288: "same quiesce-before-free class (cluster `dma`)",
            284: "modernisation umbrella, item 6/7",
        },
        "order": [
            "Close #306 as resolved by e96acdf (moderation setup now precedes register_netdev; the commit names only #299).",
            "#309 then #307, together: reset_task is down();up(), so it inherits #309's bad unwind. Neither has a fix or a reproduction (static analysis only).",
            "One hardware pass for the landed batch (#299 #305 #313 #314 #298): bench-all health subtest (`ethtool -S` drop/refill counters) + ifdown/ifup cycling + rmmod/modprobe.",
            "#297: close, premise wrong on 7.3 (core falls back, net/core/dev.c:4007; 100/100 on the old build); keep the defensive fix + bench check c352697. #272 shares `l4_proto_idx`; low priority, needs the chip's EPE entry 2 or Annapurna docs.",
        ],
        "blockers": "#307/#309 unreproduced. Hardware passes need the box exclusively (a down/up cycle drops both ports). Tests are source-shape only (tests/test_al_eth.py).",
    },
    {
        "id": "sfp",
        "name": "SFP media + SerDes link configuration (Linux and U-Boot)",
        "umbrella": "Umbrella: SFP media type and SerDes link configuration (Linux and U-Boot)",
        "disp": "LINK",
        "root": (
            "Nothing derives link configuration from the module or the board: the DT properties "
            "are decorative (4 of_property_read sites in modules/al_eth), TX taps are compile-time "
            "statics (al_init_eth_lm.c:714, al_serdes_10g.c:44), and the vendor group-LM that "
            "serialised SerDes access was retired without carrying its lock over."
        ),
        "members": {
            207: (
                1,
                "decision: DT home for SerDes TX taps (presence property + static fallback)",
            ),
            237: (
                2,
                "media type never derived from the module; DT declares a DAC, SR optic fitted",
            ),
            127: (
                2,
                "1G optics in the SFP+ cage rejected (mac_capabilities = MAC_10000FD)",
            ),
            133: (2, "no maximum-power-milliwatt; host cap 1.0W; latent"),
            310: (
                3,
                "SerDes group lock + KR PCS addr/data serialisation dropped with the group-LM",
            ),
            308: (
                3,
                "serdes_tx_*/rx_* override handshake unlocked, dirty set before valid (live since 3a1fdb1)",
            ),
            239: (
                4,
                "indirect SerDes register read has no settling wait; returns 0xFF",
            ),
            215: (
                5,
                "serdes-tune write path (--set, bounce, sweep) never exercised on hardware",
            ),
            108: (6, "U-Boot SFP EEPROM read + media decode; landed f72ed7a"),
            118: (
                6,
                "U-Boot SerDes group-D PLL relies on stock preboot; guard landed, cold boot untested",
            ),
        },
        "related": {
            221: "board params dump/self-seed (cluster `ueth`)",
            284: "modernisation umbrella, items 5 and 8",
        },
        "order": [
            "#207 decision first. Its outcome decides whether the sysfs serdes_tx_*/rx_* override path (#308, #215) survives; if it retires, #308 closes as moot.",
            "#237 media derivation from the module EEPROM (byte 2 connector 0x07 = optical, 0x21 = DAC). The FS SFP-10GSR-85 SR optic is fitted again, so it is retestable. Then #127 (needs a 1G SFP module) and #133 (latent; needs the cage power budget).",
            "#310 + #308 together: one locking design for SerDes group access and override state (static analysis only).",
            "#239: no vendor tree has a busy/valid bit or a wait; decide a bounded read-until-stable before trusting #215's readback-verify (hypothesis: it reads through the same indirect path, hssp.c:1771; not checked).",
            "#215 last: needs the write path on hardware and a peer for the bounce.",
            "U-Boot side, independent: verify #108 (f72ed7a); #118 needs a boot path that skips stock preboot's SerDes init.",
        ],
        "blockers": "#127 needs a 1G SFP module; #118 needs a non-chainload path; any SerDes bounce drops the 10G link (exclusive box).",
    },
    {
        "id": "ueth",
        "name": "U-Boot ethernet bring-up and board params",
        "umbrella": "Umbrella: U-Boot ethernet bring-up - UDMA state, 10G TX completion, board params",
        "disp": "LINK",
        "root": (
            "awto-uboot alu_* (on NAND since 2026-10-07, awto-d1bd850): UDMA activation/teardown "
            "and FLR side effects. No measurement before 2026-10-07 exercised the current code (#253). "
            "HAL fixes land once in modules/al_eth/al_hal_* and reach U-Boot and EDK2 by staging (#256)."
        ),
        "members": {
            254: (
                1,
                "al_udma_state_set_wait() IDLE wait is a quiescence check; timeout now names the state (219eb9a); U-Boot re-activation retest open",
            ),
            253: (
                2,
                "10G TX completion timeout; leading hypothesis: FLR wipes board params",
            ),
            226: (
                3,
                "TX completion poll bound ~8000x expected; per-frame bound committed, not on hardware",
            ),
            270: (4, "silent ethact rotation; landed ac359f9"),
            221: (
                5,
                "inspect/dump/validate board params in all layers; Linux half verified (2ca22be), U-Boot/EDK2 half open",
            ),
            223: (6, "what NOR 0x1f0006 actually is; needs the stock U-Boot binary"),
        },
        "related": {
            118: "U-Boot SerDes cold boot (cluster `sfp`)",
            108: "U-Boot SFP EEPROM (cluster `sfp`)",
            298: "al_eth ignores q_reset failures (cluster `eth`)",
            256: "single HAL (cluster `hal`)",
        },
        "order": [
            "#254 first, to remove the confound: the bug now sits in modules/al_eth/al_hal_eth_main.c:529 (and modules/al_ssm/al_hal_udma_main.c:1034); uboot-port/.../hal/ was deleted by #256.",
            "#253 retest per its recipe (10G is fitted again): dhcp, then setenv serverip; peer tcpdump first, filter on the 10G MAC; dump board params after the FLR before blaming the datapath.",
            "#226: compare the committed per-frame bound to the measured TX completion (no longer blocked on #90, which is closed).",
            "#270: verify the ETHACT ROTATED message on a forced start() failure.",
            "#221: U-Boot inspect-then-write and the EDK2 side (EDK2 has no ethernet code).",
            "#223: independent; settle with the stock U-Boot reader of NOR 0x1f0000.",
        ],
        "blockers": "U-Boot tests use the shared console (do not touch a tio you did not start); #223 needs the stock U-Boot binary.",
    },
    {
        "id": "hal",
        "name": "Single HAL (Linux / U-Boot / UEFI)",
        "umbrella": None,
        "disp": "LINK",
        "root": (
            "#256 made modules/ the single HAL source (U-Boot 56-file and EDK2 61-file copies "
            "deleted, 54,574 lines; hal-drift-check enforces). Two gaps remain; HAL fixes in "
            "other clusters land in modules/al_eth/al_hal_* once."
        ),
        "members": {
            256: (
                1,
                "structurally complete and proven on hardware; its own recommendation is to close it and split two gaps",
            ),
            264: (
                2,
                "al_ssm still on uboot-port/drivers/crypto/al_ssm/al_hal_compat shim (last #256 scaffolding)",
            ),
        },
        "related": {
            254: "HAL fix: al_hal_eth_main.c:529",
            298: "HAL fix: restore __must_check in modules/al_eth/al_hal_udma.h:353",
            239: "HAL fix: al_hal_serdes_hssp.c:1005",
            272: "HAL tables in al_hal_eth_main.c",
        },
        "order": [
            "Close #256 as achieved. Gap 1 (al_hal_plat_contract.h names no pr_*/fallthrough/__must_check) has a proposed body in #256's comment and is not filed as its own issue.",
            "#264: rebase al_ssm onto the shared HAL, delete al_ssm_hal_compat.h and the hal-drift-check.py:80 entry.",
        ],
        "blockers": "None.",
    },
    {
        "id": "bench",
        "name": "Measurement validity (bench-all, shared box)",
        "umbrella": None,
        "disp": "LINK",
        "root": (
            "A bench-all PASS asserts a number, not the path it travelled (the #170 shape): the "
            "box->host leg left via another NIC (864 Mbit/s on a 100 Mb/s link); xts(aes) "
            "resolved to xts-aes-ce. --idle compares with bench-history/baseline.json, so an "
            "impossible value logs as +820%, not a failure."
        ),
        "members": {
            315: (
                1,
                "box->host leg not port-bound (--bind-dev); 2 false-positive FAILs",
            ),
            267: (
                2,
                "crypto numbers are CE, labelled al_ssm; the driver-side facts are in cluster `ssm`",
            ),
            243: (
                3,
                "a reboot is indistinguishable from a hang; console banner cross-check not in CLAUDE.md",
            ),
            312: (4, "bench-all is the one harness; records what was folded in"),
        },
        "related": {
            241: "parked perf umbrella; 10G re-baselined 2026-10-11 (9.38 Gbit/s, 0 retr)",
            184: "its repro depends on a valid sustained-load measurement",
        },
        "order": [
            "#315: --bind-dev on the box->host leg; FAIL any reading above the negotiated link speed; fix the 2 false-positive FAILs.",
            "#267: label crypto by the driver that ran (al_ssm `submitted` counter delta or /proc/crypto refcnt); relabel the existing CE numbers.",
            "#243: put the boot-banner cross-check where agents read it (CLAUDE.md `Verifying, not assuming`).",
            "#312: update when 1-2 land.",
        ],
        "blockers": "1G is capped at 100 Mb/s by the link partner (verified on three NICs, two drivers): not our bug, but it is why #315's reading was impossible.",
    },
    {
        "id": "tftp",
        "name": "Dev-host TFTP path",
        "umbrella": None,
        "disp": "LINK",
        "root": "Dev-host firewalld/conntrack in front of scripts/tftpd.py. #136 = UDP:69 dropped; #185 = long-lived daemon stalls at block 19355 / ~31 s (conntrack hypothesis, untested).",
        "members": {
            136: (
                1,
                "firewalld zone drops UDP:69; detection + opt-in fix landed (scripts/_firewall.py)",
            ),
            185: (
                2,
                "standalone daemon stalls deterministically mid-transfer; ram-boot-deploy.py's one-shot server is unaffected",
            ),
        },
        "related": {},
        "order": [
            "#136: verify detection + opt-in fix on the dev host.",
            "#185: test the conntrack hypothesis (31 s is near a default UDP timeout); cheapest fix is the one-shot embedded server that already works.",
        ],
        "blockers": "None.",
    },
    {
        "id": "wdt",
        "name": "Watchdog / reset / hardening",
        "umbrella": None,
        "disp": "LINK (+ #45's watchdog bullet is #60's systemd half)",
        "root": "Plain `reboot` works (SP805 restart handler, #51). Open: reset after a corrupting Oops, an early-armed hardware watchdog, and nothing petting it in userland.",
        "members": {
            259: (
                1,
                "reset after a corrupting Oops untested; narrowed to one hardware test",
            ),
            60: (
                2,
                "early SP805 watchdog: U-Boot arms, sp805_wdt takes over, systemd pets",
            ),
            45: (
                3,
                "Fedora hardening: root creds/SSH, SELinux, sysctl; its watchdog bullet duplicates #60",
            ),
        },
        "related": {
            1: "security analysis; cloud-identity exposure now historical",
            284: "modernisation umbrella, item 9",
        },
        "order": [
            "#259 hardware test first (panic=15 is live from awto-uboot CONFIG_BOOTARGS).",
            "#60: arm early AND pet, together; arming without a petter is a reset loop.",
            "#45: drop the watchdog bullet (RuntimeWatchdogSec=30s, RebootWatchdogSec=2min come with #60); the 10G sysctl tuning waits on #241.",
        ],
        "blockers": "#259 can corrupt the box; needs power-cycle access (Sonoff so-th-1).",
    },
    {
        "id": "uefi",
        "name": "UEFI",
        "umbrella": None,
        "disp": "LINK (tracker: #39)",
        "root": (
            "The P4 Linux boot from the Shell has never been run; the EDK2 platform code was "
            "carried across, not rewritten (awto-uefi fork is pristine upstream 2970e5699b), so "
            "#263 and #255 are defects in Unvr.dsc (PcdSystemMemorySize 0xC0000000 = bank 0 only; "
            "PcdCoreCount 1)."
        ),
        "members": {
            39: (5, "UEFI tracker"),
            261: (1, "uefi-p4-boot.py: 3 of 5 defects fixed in passing, 2 remain"),
            251: (
                2,
                "P4: Image is PE/COFF, EFI stub loads the DTB; bootedk2 is in the running U-Boot, retestable",
            ),
            263: (3, "EDK2 hands Linux only DRAM bank 0 (3 GiB vs 4 GiB)"),
            255: (3, "PSCI CPU_ON returns SUCCESS for cores that never start"),
        },
        "related": {
            30: "resident FD slot decision",
            221: "EDK2 has no ethernet/board-params code",
            256: "EDK2 HAL staging done",
        },
        "order": [
            "#261: fix the two remaining script defects.",
            "#251: run P4 (boot Linux from the Shell); confirms `bootedk2`, SATA/ESP, USB, AlEth1gSnpDxe binds 1c36:0001.",
            "#263 + #255: same file, Platform/Ubiquiti/UNVR/Unvr.dsc. Verify with `free -g` (4 GiB) and a secondary-core signature written by the secondary itself (a PSCI SUCCESS means nothing here).",
            "Resident FD placement is #30's open design decision.",
            "Close #39 when P3 TX/RX, P4 and the FD slot are done.",
        ],
        "blockers": "Box + console exclusive; #30 decision for the resident FD.",
    },
    {
        "id": "mtd",
        "name": "MTD layout and recovery",
        "umbrella": None,
        "disp": "LINK",
        "root": "Flash layout and recovery policy for a box that no longer supports UniFi OS. `run bootnand` is not a recovery path (#166).",
        "members": {
            30: (
                1,
                "MTD layout redesign; docs/mtd.md landed; DTS cleanup + recovery-kernel/FD-slot decision remain",
            ),
            166: (
                3,
                "recovery NAND partition is not genuine stock firmware; docs half fixed, flash-script trap remains",
            ),
        },
        "related": {
            39: "resident FD slot (its P-list item 3)",
            208: "NAND write path verified once",
            268: "NAND write path",
        },
        "order": [
            "#30 item 2 (DTS only, no flash write): drop dead partitions chike, device_tree, cksum; dead chosen/bootargs; vcc-supply on the pca9575 nodes.",
            "#30 item 3: decide the 16 MiB NOR recovery-kernel slot and a resident EDK2 FD slot (the one open design decision, shared with #39).",
            "#166: remove the live trap in the flash script (see its latest comment).",
        ],
        "blockers": "Reclaiming flash space is a flash write: owner approval. No JTAG; backstop is a 1.8 V NOR clip.",
    },
    {
        "id": "dtnodes",
        "name": "DT nodes without a driver / peripherals",
        "umbrella": None,
        "disp": "LINK (#123's sata-leds question is #208's sata-leds line)",
        "root": "Driver audit: DT nodes that bind nothing (sata-leds `annapurna-labs,al-sata-sw-leds`, EDAC_AL_MC) and a driver whose error path was never reviewed.",
        "members": {
            208: (
                1,
                "audit tracker; modules and al_nand done, sata-leds and EDAC remain",
            ),
            123: (2, "LED activity: blue ulogo trigger + what binds sata-leds"),
            295: (
                3,
                "ubnt_hdd_pwrctl: a gpio read error is treated as 'bay empty' and emits a hot-unplug uevent",
            ),
        },
        "related": {},
        "order": [
            "#208: settle what binds sata-leds and the EDAC_AL_MC status.",
            "#123 reduces to a trigger change on `led-ulogo-blue` (ledtrig-disk is enabled) once the sata-leds binding is known.",
            "#295 independent: on a negative gpiod read keep the previous state, rate-limit the log.",
        ],
        "blockers": "None.",
    },
    {
        "id": "upstream",
        "name": "DT / PCIe / upstreaming chain",
        "umbrella": None,
        "disp": "LINK (tracker: #36); #34's clock half is already #16's patch 6",
        "root": "Dependency chain, not a shared defect: binding -> SoC dtsi -> PCIe snoop split -> shared DT source -> submission.",
        "members": {
            15: (
                1,
                "ARM64 DT binding; gates dtbs_check (enum only has al,alpine-v2-evp)",
            ),
            213: (2, "decision: move al_* in-tree; unblocked by #256"),
            16: (
                3,
                "SoC blocks into alpine-v2.dtsi; absorbs #12; patch 6 = fixed-clocks",
            ),
            74: (
                4,
                "PCIe snoop fixup: U-Boot half done, Linux half (idempotent al_snoop_fix) not started",
            ),
            75: (5, "shared DT source; blocked on #74 (blocker 1)"),
            36: (6, "per-patch ledger exists (a24bf06); submission last"),
            34: (
                7,
                "clock half resolved by design (= #16); keep for reset-controller + PM only",
            ),
        },
        "related": {
            29: "overclock; separate by #34's own scope",
            256: "single HAL",
        },
        "order": [
            "#15 binding (blocks #16 via dtbs_check).",
            "#213 decision (gates #36's submission order).",
            "#16 SoC blocks into alpine-v2.dtsi.",
            "#74 Linux half (idempotent al_snoop_fix).",
            "#75 shared source, after #16 and #74.",
            "#36 submission last.",
            "#34 re-scoped to reset-controller + PM.",
        ],
        "blockers": "#75 <- #74 Linux half. Upstream posting is a public action (three-check rule).",
    },
]

# duplicate -> (survivor, why). The user closes; this only posts the notice.
DUPS: dict[int, tuple[int, str]] = {
    306: (
        299,
        "e96acdf moved the moderation setup (`adaptive_intr_rate`, the interval table, "
        "`restore_default()`) and `al_eth_sysfs_init()` ahead of `register_netdev()` "
        "(al_eth_main.c:6262-6274). That is this issue's fix; the commit names only #299.",
    ),
    276: (
        302,
        "Both landed in e9bf6be as one change (#302: 'they compound; one commit'). #276 is the "
        "symptom (64 us FIFO bound on erase), #302 the consumer (waitfunc skipping nand_wait()); "
        "the hardware check is the same erase on mtd1.",
    ),
}

NOTES: dict[int, str] = {
    254: (
        "Fix location moved: `uboot-port/.../hal/` no longer exists (#256). The wait is in "
        "`modules/al_eth/al_hal_eth_main.c` (static) and `modules/al_ssm/al_hal_udma_main.c`; "
        "U-Boot and EDK2 stage the al_eth copy. Target state left as IDLE (quiescence): see 219eb9a."
    ),
    297: (
        "Disposition: premise wrong on 7.3 - the core already routes IPv6 extension "
        "headers to software checksum (net/core/dev.c:4007); verified 100/100 on the old build. "
        "7e07475 and the c352697 bench check are defensive. Suggest close."
    ),
    300: "If #282 item 7 (crypto_engine) is adopted this issue is obsolete; otherwise independent.",
    226: "Body says blocked on #90; #90 is closed and the per-frame bound is committed (see latest comment).",
    108: "Body's 'GLC-SX-MM 1000BASE-SX' module is stale: the fitted module is an FS SFP-10GSR-85 SR optic (#237).",
    127: "Body's 'fitted module is 10GBASE-LR' is stale (SR optic). Remaining blocker: a 1G SFP module on the bench.",
    184: (
        "10G 4-stream TCP TX read 9.38 Gbit/s, 0 retr on 2ca22be (#241) - different traffic "
        "from UDP -R at 1000M, so it does not close this. #304 does not explain it "
        "(its only caller is rx_poll; this is TX-only load). Check the console for a boot "
        "banner before recording a hang (#243)."
    ),
    215: "The readback-verify may read through the indirect SerDes read #239 describes; not checked.",
    45: "Watchdog bullet duplicates #60's systemd half; the sysctl tuning waits on #241.",
    34: "Clock inventory is #16's patch 6 (fixed-clocks); re-scope this to reset-controller + PM.",
    123: "The sata-leds binding question is the same line as in #208.",
    267: "The fix is a bench-all change (cluster `bench`); the driver-side work is in cluster `ssm`.",
}


def log(msg: str) -> None:
    LOG.parent.mkdir(parents=True, exist_ok=True)
    with LOG.open("a", encoding="utf-8") as fh:
        fh.write(msg + "\n")
    print(msg)


def gh(*args: str) -> str:
    r = subprocess.run(["gh", *args], capture_output=True, text=True, check=False)
    if r.returncode != 0:
        log(f"gh {' '.join(args[:3])} failed: {r.stderr.strip()}")
        raise SystemExit(1)
    return r.stdout


def load_issues() -> dict[int, dict]:
    out = gh(
        "issue", "list", "--repo", GH_REPO, "--state", "all", "--limit", "500",
        "--json", "number,title,state",
    )  # fmt: skip
    return {i["number"]: i for i in json.loads(out)}


def umbrella_numbers(issues: dict[int, dict]) -> dict[str, int]:
    by_title = {i["title"]: n for n, i in issues.items()}
    return {
        c["id"]: by_title[c["umbrella"]]
        for c in CLUSTERS
        if c["umbrella"] and c["umbrella"] in by_title
    }


def short(issues: dict[int, dict], n: int) -> str:
    i = issues.get(n)
    return i["title"] if i else "?"


def umbrella_body(c: dict, issues: dict[int, dict]) -> str:
    lines = [
        f"Cluster `{c['id']}` of `docs/issue-clusters.md`. Disposition: **{c['disp']}**.",
        "",
        "## Shared cause",
        f"- {c['root']}",
        "",
        "## Order (what blocks what)",
    ]
    lines += [f"{k}. {s}" for k, s in enumerate(c["order"], 1)]
    lines += ["", f"Blockers: {c['blockers']}", "", "## Members"]
    for n, (step, role) in sorted(
        c["members"].items(), key=lambda kv: (kv[1][0], kv[0])
    ):
        done = "x" if issues.get(n, {}).get("state") == "CLOSED" else " "
        lines.append(f"- [{done}] #{n} (step {step}) - {role}")
    if c["related"]:
        lines += ["", "## Related (not checklist items)"]
        lines += [f"- #{n} - {r}" for n, r in c["related"].items()]
    dup = [f"#{d} -> #{s}" for d, (s, _) in DUPS.items() if d in c["members"]]
    if dup:
        lines += ["", f"Duplicates (owner closes): {', '.join(dup)}."]
    return "\n".join(lines)


def member_comment(n: int, issues: dict[int, dict], umb: dict[str, int]) -> str:
    own = [c for c in CLUSTERS if n in c["members"]]
    rel = [c for c in CLUSTERS if n not in c["members"] and n in c["related"]]
    out = [MARK]
    for c in own:
        step, role = c["members"][n]
        u = f"; umbrella #{umb[c['id']]}" if c["id"] in umb else ""
        out += [
            f"**Cluster: {c['name']}** (`{c['id']}`, {c['disp']}{u}; `docs/issue-clusters.md`)",
            f"- This issue: {role}",
            f"- Order: step {step} of {len(c['order'])}: {c['order'][min(step, len(c['order'])) - 1]}",
            f"- Shared cause: {c['root']}",
        ]
        sib = [f"#{m} ({r})" for m, (_, r) in sorted(c["members"].items()) if m != n]
        if sib:
            out.append("- Siblings: " + "; ".join(sib))
        out.append(f"- Blockers: {c['blockers']}")
        out.append("")
    for c in rel:
        u = f" (umbrella #{umb[c['id']]})" if c["id"] in umb else ""
        mem = ", ".join(f"#{m}" for m in sorted(c["members"]))
        out.append(
            f"Related to cluster **{c['name']}**{u}: {c['related'][n]}. Members: {mem}."
        )
        out.append("")
    if n in DUPS:
        out.append(f"Duplicate of #{DUPS[n][0]} (see the next comment). Owner closes.")
        out.append("")
    if n in NOTES:
        out += [f"Note: {NOTES[n]}", ""]
    return "\n".join(out).rstrip() + "\n"


def dup_comment(n: int) -> str:
    s, why = DUPS[n]
    return (
        f"{MARK_DUP}\nDuplicate of #{s}. Survivor: **#{s}**.\n\n- {why}\n"
        f"- Not closed here; close #{n} when #{s} is closed or now.\n"
    )


def umbrella_comment(umb: dict[str, int], issues: dict[int, dict]) -> str:
    rows = [
        f"- #{u}: {next(c['name'] for c in CLUSTERS if c['id'] == k)}"
        for k, u in umb.items()
    ]
    return (
        f"{MARK_UMB}\nPer-cluster umbrellas filed from `docs/issue-clusters.md` "
        "(this umbrella stays the per-module modernisation plan):\n"
        + "\n".join(rows)
        + "\n"
    )


def render_doc(issues: dict[int, dict], umb: dict[str, int]) -> str:
    L = [
        "# Issue clusters",
        "",
        "- Generated by `scripts/issue-clusters.py` (data is in that file); do not hand-edit.",
        "- Orthogonal to `docs/issue-reconciliation.md` (per-issue disposition). This file groups.",
        "- LINK = all members distinct, cross-referenced, fixed together. MERGE = close the duplicate into the survivor.",
        "- Facts assumed: box `7.3.0-rc6-c352697+`; 1G is capped at 100 Mb/s by the link partner (not our bug); 10G 8.59-9.38 Gbit/s, 0 retr; 6.12 port audit complete (1 split-invariant, #286, fixed; not systemic).",
        "",
        "## Table",
        "",
        "| cluster | members | root cause | LINK / MERGE | order | blockers |",
        "|---|---|---|---|---|---|",
    ]
    for c in CLUSTERS:
        mem = ", ".join(f"#{n}" for n in sorted(c["members"]))
        u = f" (umbrella #{umb[c['id']]})" if c["id"] in umb else ""
        order = "<br>".join(f"{k}. {s}" for k, s in enumerate(c["order"], 1))
        cells = [f"**{c['name']}**{u}", mem, c["root"], c["disp"], order, c["blockers"]]
        L.append("| " + " | ".join(x.replace("|", "/") for x in cells) + " |")
    L += [
        "",
        "## Duplicates (owner closes)",
        "",
    ]
    for d, (s, why) in DUPS.items():
        L.append(f"- #{d} -> survivor #{s}: {why}")
    L += [
        "",
        "## Conditional or partial overlaps (not duplicates)",
        "",
        "- #300 -> #282: obsolete only if #282 item 7 (crypto_engine) is adopted.",
        "- #45 watchdog bullet -> #60. #34 clock half -> #16 (patch 6). #123 sata-leds line -> #208.",
        "- #297 closable on premise (wrong on 7.3), not a duplicate.",
        "- #182 vs #267: distinct. #267 is by design and its fix is in bench-all; #182 is an unexplained lost completion.",
        "",
        "## Unclustered",
        "",
        "- #1 (security analysis) related to `wdt`. #29 (overclock) related to `upstream`. #32 (md baseline) related to `dma`.",
        "- #284 is the modernisation umbrella; the per-cluster umbrellas are listed on it.",
        "",
        "## Not established",
        "",
        "- Whether #307 and #309 reproduce (static analysis only, no commit).",
        "- The cause of #184 and #182.",
        "- Hardware behaviour of every 'landed' fix above except #278 (dmatest) and #274 part 2 (reproduced before the fix).",
        "- Whether #254's wrong expected state is reachable on the Linux copy.",
        "- Whether #308 survives the #207 decision.",
        "",
        "## Issue titles",
        "",
    ]
    seen = sorted({n for c in CLUSTERS for n in (*c["members"], *c["related"])})
    L += [f"- #{n} {short(issues, n)}" for n in seen]
    return "\n".join(L) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--write-doc", action="store_true")
    ap.add_argument("--file-umbrellas", action="store_true")
    ap.add_argument("--post-comments", action="store_true")
    ap.add_argument("--post-dups", action="store_true")
    ap.add_argument("--apply", action="store_true", help="act; default is a dry run")
    a = ap.parse_args()

    issues = load_issues()
    missing = sorted(
        {n for c in CLUSTERS for n in (*c["members"], *c["related"])} - set(issues)
    )
    if missing:
        log(f"unknown issue numbers: {missing}")
        return 2
    umb = umbrella_numbers(issues)

    if a.file_umbrellas:
        for c in CLUSTERS:
            if not c["umbrella"] or c["id"] in umb:
                continue
            body = umbrella_body(c, issues)
            log(f"umbrella: {c['umbrella']} ({len(body)} chars)")
            if a.apply:
                url = gh(
                    "issue", "create", "--repo", GH_REPO,
                    "--title", c["umbrella"], "--body", body,
                ).strip()  # fmt: skip
                log(f"  filed {url}")
        if a.apply:
            umb = umbrella_numbers(load_issues())

    if a.post_comments:
        targets = sorted({n for c in CLUSTERS for n in (*c["members"], *c["related"])})
        for n in targets:
            if issues[n]["state"] != "OPEN":
                log(f"#{n}: closed, skipped")
                continue
            body = member_comment(n, issues, umb)
            have = gh("issue", "view", str(n), "--repo", GH_REPO, "--json", "comments")
            if MARK in have:
                log(f"#{n}: already commented")
                continue
            log(f"#{n}: comment ({len(body)} chars)")
            if a.apply:
                gh("issue", "comment", str(n), "--repo", GH_REPO, "--body", body)
        if umb and issues[284]["state"] == "OPEN":
            if MARK_UMB in gh(
                "issue", "view", "284", "--repo", GH_REPO, "--json", "comments"
            ):
                log("#284: already commented")
            else:
                log("#284: umbrella list comment")
                if a.apply:
                    gh("issue", "comment", "284", "--repo", GH_REPO,
                       "--body", umbrella_comment(umb, issues))  # fmt: skip

    if a.post_dups:
        for n in DUPS:
            if issues[n]["state"] != "OPEN":
                log(f"#{n}: closed, skipped")
                continue
            if MARK_DUP in gh(
                "issue", "view", str(n), "--repo", GH_REPO, "--json", "comments"
            ):
                log(f"#{n}: duplicate notice already posted")
                continue
            log(f"#{n}: duplicate notice -> #{DUPS[n][0]}")
            if a.apply:
                gh(
                    "issue",
                    "comment",
                    str(n),
                    "--repo",
                    GH_REPO,
                    "--body",
                    dup_comment(n),
                )

    if a.write_doc:
        text = render_doc(issues, umb)
        log(f"doc: {len(text)} chars -> {DOC.relative_to(REPO)}")
        if a.apply:
            DOC.write_text(text, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
