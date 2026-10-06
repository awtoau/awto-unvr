---
name: eth-udma-tx-hang
description: "awto-uboot al_eth — 1G FIXED (was RX, RGMII_ID, not TX at all); 10G TX still fails (#253, FLR wipes board params). #90 closed. Eliminated-hypotheses list, do not re-test."
metadata: 
  node_type: memory
  type: project
  originSessionId: f681cd02-005d-427b-ae71-5abdae07a3b5
  modified: 2026-09-04T23:24:16.522Z
---

#90 ("UDMA M2S descriptor read hangs, 1G+10G") is CLOSED and was wrong in headline. Split into #234 (1G, closed) and #253 (10G, open).

**1G — never a TX bug.** `30e7c65` (2026-09-04): the AR8033 was connected `PHY_INTERFACE_MODE_RGMII`, not `RGMII_ID`, so no internal delay and the MAC sampled mid-transition. TX always reached the wire (tcpdump on a peer saw every ARP, peer replied); RX rejected every frame — `if_in_errors` = `mac.pkts`, `frames_rxed_ok 0`, `fcs_errors 0`. "TX completion timeout" was a red herring for "no ARP reply came back". Linux uses `RGMII_ID` (`al_eth_main.c:2667`). tftpboot works, 859424 B @ 4.2 MiB/s.

**10G — real TX failure (#253).** Wire test: 0 frames leave the MAC. Lead: `al_eth_dm_10g.c` calls raw `al_eth_flr_rmn()`, which wipes board params + EC MAC (`eth diag 2` reads both UNSET after start). Linux's `al_eth_function_reset()` (`al_eth_main.c:2786`) saves/restores around the same FLR. Unproven on hardware as of 2026-09-05. Explains `092fcde`: adding FLR was half-right, it added the destructive variant.

**Symptom moved, so old captures mislead.** Original (d41ed23): `m2s.state=0x2222`, `drhp` frozen. Later: `drhp/dcp/crhp` advancing, `m2s.state=0x1000` = `desc_pref=1` Normal, a healthy idle engine. Not a hang.

**DO NOT re-test — eliminated with evidence:**
- Cache/coherency incl. stock's `CONFIG_SYS_DCACHE_OFF`: `al_udma_cdesc_get_all()` reads the completion head from the `crhp` REGISTER, not DRAM. Structural.
- SMCC snoop/axcache/ROB: read live off Linux with `setpci`, byte-identical to what awto-uboot programs.
- Low-DMA placement, FLR-as-fix, error-track, `al_udma_axi_set`, BME, CCU: on-box, no effect (d41ed23).
- Single-core/SMP: 1G works single-core with caches on; `CCU_SLAVE4_SNOOP=0` on a working boot (`eb4aecf`).
- Three commits each claimed "THE TX-completion fix" and all failed: `92f5706` cache, `092fcde` FLR, `d69b711` snoop. Do not add a fourth without a real tftpboot.

**#254 contaminates captures:** `al_udma_state_set_wait()` requests NORMAL but waits for IDLE → `-110`, and U-Boot silently falls through to another port. First transfer per session works, second fails 3/3. Verify which port is active for every capture. Linux carries the same latent bug.

**Why:** three sessions chased the wrong half of the datapath because this memory and #90's title said TX/DMA. The cheap wire test (peer tcpdump) split it in minutes. Compare against STOCK U-Boot's driver as well as Linux — same environment, no OS DMA API in the way.
**How to apply:** for any "TX never completes", capture on a peer first. See [[dont-blame-hardware-assume-our-code]], [[test-every-change-never-assert]].
