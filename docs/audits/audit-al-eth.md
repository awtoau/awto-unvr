# Adversarial source-of-truth audit — al_eth U-Boot DM port

Scope: `uboot-port/drivers/net/al_eth/{al_eth_dm.c,al_eth_stubs.c,Kconfig,Makefile}`
Compared against: vendored HAL `drivers/net/al_eth/hal/` (== delroth-alpine_hal @ eb6b9f1, byte-identical on the constants checked), and vendor U-Boot glue `UNVR-1.3.35-GPL/u-boot/drivers/net/al_eth.c`.

Method: every magic constant, every HAL call arg, and the init order treated as wrong until a source line proves it. Line cites are `file:line` on both sides.

## Headline
- The originally-shipped fabricated constant (`AL_ETH_DESCS_PER_Q = PKTBUFSRX+1 = 5`) is **already fixed** in current source → now `32`. Confirmed correct (≥32, power-of-2). See row 5.
- **0 hard mismatches** that would break ping in the remaining constants/args.
- **0 fabricated-no-source** values that are wrong. Two values have no HAL source but are board-of-record (PHY addr, ref clk) and are confirmed elsewhere.
- The post-phy MAC re-sync (`al_eth_dm.c:270`) is an **improvement over the vendor**, which left it as a TODO (`al_eth.c:931-933`). Without it a 100M link would silently drop frames. Keep it.
- Findings are all **cosmetic/doc** severity. Detailed below.

## Full table

| # | our symbol/value | our file:line | source-of-truth value | source file:line | VERDICT | recommended fix |
|---|---|---|---|---|---|---|
| 1 | `AL_ETH_PCI_VENDOR 0x1c36` | al_eth_dm.c:47 | Annapurna vendor id; eth1 = `1c36:0001` | vendor al_eth.c / hal AL_ETH_DEV_ID_STANDARD | match | — |
| 2 | `AL_ETH_PCI_DEV_1G 0x0001` | al_eth_dm.c:48 | `AL_ETH_DEV_ID_STANDARD 0x0001` | hal/include/al_hal_eth.h:110 | match | — |
| 3 | `AL_ETH_BAR_UDMA = 0` | al_eth_dm.c:49 | `AL_ETH_UDMA_BAR 0` | hal/include/al_hal_eth.h:118 | match | — |
| 4 | `AL_ETH_BAR_EC = 4` | al_eth_dm.c:50 | `AL_ETH_EC_BAR 4` | hal/include/al_hal_eth.h:119 | match | — |
| 5 | `AL_ETH_BAR_MAC = 2` | al_eth_dm.c:51 | `AL_ETH_MAC_BAR 2` | hal/include/al_hal_eth.h:120 | match | — |
| 6 | `AL_ETH_RX_BUFFERS = PKTBUFSRX` | al_eth_dm.c:57 | vendor `NUM_RX_BUFFERS = PKTBUFSRX` (=4) | vendor al_eth.c:64; net.h:36 | match | — |
| 7 | `AL_ETH_DESCS_PER_Q = 32` | al_eth_dm.c:58 | `AL_UDMA_MIN_Q_SIZE 32`; must be pow2 | hal/include/al_hal_udma.h:77; udma_main.c:600,612 | **match (fix applied)** | none — vendor's `PKTBUFSRX+1` is rejected by this HAL's min-size + pow2 checks; 32 satisfies both |
| 8 | `AL_ETH_CDESC_SIZE = 16` | al_eth_dm.c:59 | vendor `COMPLETION_DESC_SIZE 16`; HAL asserts `cdesc_size<=32` | vendor al_eth.c:70; eth_main.c:1878 | match | — (NB `union al_udma_cdesc` is only 4B in the struct decl at udma.h:157, but the HW completion stride the vendor programs is 16; 16 is the right value, not an assumption) |
| 9 | `AL_ETH_Q_DESCS_SIZE = 32*16 = 512` | al_eth_dm.c:60 | used for BOTH sdesc and cdesc ring regions | — | match (cosmetic) | sdesc region should be sized by `sizeof(union al_udma_desc)` (=16, udma.h:96-121), not by CDESC_SIZE. Works only because both are 16. Add a `_Static_assert(sizeof(union al_udma_desc)==AL_ETH_CDESC_SIZE)` or split the two sizes. |
| 10 | desc block offsets 0/512/1024/1536, block=2048 | al_eth_dm.c:62-66 | internally consistent, 4 non-overlapping 512B regions | — | match | — |
| 11 | `AL_ETH_TX_POLL_MAX = 100000` (~100ms @ udelay(1)) | al_eth_dm.c:72 | no HAL/vendor source; heuristic. 1518B@1G drains ~12us → ~8000x | (frame arithmetic) | fabricated-no-source (cosmetic) | Per repo timeout rule this is orders-of-magnitude too generous. Tighten to ~1.25x worst case once measured; the HARDWARE-TODO already flags it. Not a correctness bug (self-names on expiry, then flushes TX). |
| 12 | `AL_ETH_DEFAULT_PHY_ADDR = 4` | al_eth_dm.c:78 | board-of-record (no HAL source; vendor reads it from MAC scratchpad board params) | vendor al_eth.c:296; README/MEMORY | match (board-confirmed) | — |
| 13 | `AL_ETH_MDIO_CLK_KHZ = 1000` | al_eth_dm.c:79 | vendor `AL_ETH_BOARD_MDIO_FREQ_1_MHZ → 1000` | vendor al_eth.c:338-339 | match | — |
| 14 | `AL_ETH_REF_CLK = AL_ETH_REF_FREQ_500_MHZ` | al_eth_dm.c:80 | enum value `=3` valid → 500000 kHz | hal/include/al_hal_eth_defs.h:29; mac_internal.c:269-270 | match (board-confirmed) | — |
| 15 | `ap.rev_id = AL_ETH_REV_ID_2` | al_eth_dm.c:140 | `AL_ETH_REV_ID_2 = 2` "Alpine V2 basic"; dispatches to v1_v2 | hal/include/al_hal_eth.h:113; eth_mac.c:120-123 | match | — (this is what keeps v3/v4 stubs off the live path) |
| 16 | `ap.dev_id = AL_ETH_DEV_ID_STANDARD` | al_eth_dm.c:141 | `0x0001`, matches PCI dev | hal/include/al_hal_eth.h:110 | match | — |
| 17 | `ap.common_mode = AL_ETH_COMMON_MODE_INVALID` | al_eth_dm.c:152 | enum exists; consulted only for rev_id≥4 | al_hal_eth_common.h:126-128; eth_main.c:892 | match | — (unused for rev2; safe) |
| 18 | `ap.unit_adapter = NULL` | al_eth_dm.c:151 | NULL → `al_unit_adapter_init` NOT called (only warns) | eth_main.c:1415-1419 | match | — (stub genuinely unreached; see stubs section) |
| 19 | `txp/rxp.size = 32`, `cdesc_size = 16`, `adapter_rev_id = REV_ID_2` | al_eth_dm.c:161-163,171-173 | `al_udma_q_init` validates size≥32 & pow2; cdesc_size≤32 | udma_main.c:600-616; eth_main.c:1878 | match | — |
| 20 | `al_eth_queue_enable(...)` (ret ignored) | al_eth_dm.c:181,183 | HAL impl is a stub returning `-EPERM`; queue already enabled inside `al_udma_q_init` | eth_main.c:1887-1892; udma_main.c:652 | match (informational) | Harmless no-op; vendor also calls+ignores it (al_eth.c:684,686). Optionally drop the call or add a comment that q_init does the enable. |
| 21 | `al_eth_mac_config(RGMII)` | al_eth_dm.c:188 | board is RGMII; `AL_ETH_MAC_MODE_RGMII` handled by v1_v2 | mac_v1_v2.c:75,333; vendor al_eth.c:365,768 | match | — |
| 22 | `mac_link_config(FALSE,TRUE,1000,TRUE)` (dma_init default) | al_eth_dm.c:189-192 | sig `(force_1000_base_x, an_enable, speed, full_duplex)`; for RGMII an_enable+force_1000_base_x are ignored, sets MAC to fixed 1G | eth_mac.c:348-351; mac_internal.c:203-257, link_config_1g_mac 109-141 | match (cosmetic) | Comment "let PHY autoneg drive it" is misleading — an_enable is a no-op in the RGMII path. Call is redundant (overridden post-phy) but harmless. Vendor SKIPS this call for external-PHY RGMII (al_eth.c:770-772). Consider dropping it, or fix the comment. |
| 23 | `mac_link_config(FALSE,FALSE,phy->speed,fd)` (post-phy re-sync) | al_eth_dm.c:270-272 | correct: clears ENA_AUTO, programs 1G_SPD/10M_SPD from `speed` | mac_internal.c:109-141 | **match (improvement)** | Keep. Vendor left this as an unfilled TODO (al_eth.c:931-933) → their driver never retargets the MAC to the negotiated 100M, exactly the silent-drop bug the code comment (#1) describes. |
| 24 | `al_eth_rx_pkt_limit_config(30, 1518)` | al_eth_dm.c:193 | identical to vendor; assert `max<=AL_ETH_MAX_FRAME_LEN(10240)` | vendor al_eth.c:779; eth_main.c:1901-1905; al_hal_eth.h:122 | match | — |
| 25 | `al_eth_rx_buffer_add(..., AL_ETH_RX_FLAGS_INT, NULL)` | al_eth_dm.c:203-204 | identical flags to vendor | vendor al_eth.c:788-789 | match | — |
| 26 | `al_eth_rx_buffer_action(rx_q, AL_ETH_RX_BUFFERS)` | al_eth_dm.c:210 | vendor rings doorbell for `NUM_RX_BUFFERS` | vendor al_eth.c:798 | match | — (we primed exactly AL_ETH_RX_BUFFERS descriptors first) |
| 27 | `al_eth_mdio_config(CLAUSE_22, AL_TRUE, ref_clk, 1000)` | al_eth_dm.c:215-216 | sig `(type, shared_mdio_if, ref_clk_freq, mdio_clk_khz)`; shared=TRUE matches vendor | al_hal_eth_mac.h:1021-1023; vendor al_eth.c:877-883 | match | — |
| 28 | supported-speed mask (10/100/1000) | al_eth_dm.c:245-247 | identical mask to vendor | vendor al_eth.c:859-863 | match | — |
| 29 | `al_eth_mac_addr_store(ec_regs, 0, enetaddr)` | al_eth_dm.c:377 | sig `(ec_base, idx, addr)` | al_hal_eth.h:1478 | match | — |
| 30 | `udelay(10)` in stop ("MAC FIFO drain") | al_eth_dm.c:367 | no source; heuristic | — | fabricated-no-source (cosmetic) | Weak justification ("~10KB FIFO" ≠ 10us at any rate). Not on the ping path (teardown only). State a real reason or drop. |
| 31 | `pkt.num_of_bufs = 1` single-buf TX | al_eth_dm.c:285 | matches vendor single-buffer send | vendor al_eth.c:963 | match | — |

## Init SEQUENCE vs vendor

Our order (probe → start → dma_init → send/recv) vs vendor `al_eth_dev_init`+`al_eth_init`:

| step | our order | vendor order (al_eth.c) | verdict |
|---|---|---|---|
| adapter_init | dma_init:154 | 682 | match |
| queue_config TX / enable | 180-181 | 683-684 | match |
| queue_config RX / enable | 182-183 | 685-686 | match |
| q_handle_get tx/rx | 184-185 | (vendor keeps its own q ptrs) | equivalent |
| mac_config | 188 | 768 | match |
| mac_link_config (initial) | 189-192 | 773-777 (SKIPPED for external-PHY RGMII) | benign deviation — we call, vendor skips; overridden post-phy |
| rx_pkt_limit_config | 193 | 779 | match |
| rx_buffer_add loop | 197-209 | 782-794 | match |
| rx_buffer_action | 210 | 798 | match |
| mac_start | 212 | 800 | match |
| mdio_config | 215 | 877 (in al_eth_init, after dev_init) | match (both after mac_start, before phy_connect) |
| phy_connect/config/startup | start:239-259 | al_eth_init:893-923 | match |
| **post-phy mac_link_config re-sync** | start:270 | **absent (TODO al_eth.c:931-933)** | **our addition — correct, keep** |

No missing HAL step, no wrong-arg step. The only order divergence (initial mac_link_config present vs vendor-skipped) is benign because it is superseded by the post-phy re-sync.

## al_eth_stubs.c — v3/v4 + unit_adapter off the live path?

- `al_eth_mac_handle_init` dispatches by `params->eth_rev_id`: rev 1/2 → `al_eth_mac_v1_v2_handle_init`; rev 3 → v3; rev 4 → v4 (`eth_mac.c:120-131`). We pass `rev_id = AL_ETH_REV_ID_2` (dm.c:140,163,173), so **v3/v4 handle-init are never reached**. Stubs return `-ENOTSUPP`; if ever hit they fail loudly rather than mis-init. Safe. ✅
- `al_unit_adapter_init`: called only in the `params->unit_adapter != NULL` branch (`eth_main.c:1415-1417`). We pass `unit_adapter = NULL` (dm.c:151) → branch not taken, the `else if (!unit_adapter)` path just warns (`eth_main.c:1418-1419`). Stub **never called**. Safe. ✅
- Prereq for both: the #74 board_late_init snoop/PCI init must genuinely be done before probe (the reason unit_adapter can be NULL). Runtime-verify on the box; not checkable statically here.

## MUST FIX before hardware
None. No constant or call-arg would break ping. The one would-break-ping issue (MAC not retargeted to negotiated speed) is already handled by the post-phy re-sync at dm.c:270 — verify it actually fires on a 100M link on the box.

## Cosmetic / doc (ranked)
1. **dm.c:60 `AL_ETH_Q_DESCS_SIZE`** conflates submission-desc size with cdesc size. Correct only because both are 16. Add `_Static_assert(sizeof(union al_udma_desc)==AL_ETH_CDESC_SIZE, ...)` so a future cdesc bump can't silently undersize the sdesc ring.
2. **dm.c:72 `AL_ETH_TX_POLL_MAX=100000`** ~8000x worst case — violates the repo aggressive-timeout rule. Tighten to ~1.25x once drain is measured (HARDWARE-TODO already present).
3. **dm.c:189-192 initial mac_link_config** — comment "let PHY autoneg drive it" is wrong for RGMII (an_enable ignored); call is redundant vs the post-phy re-sync. Drop it or fix the comment.
4. **dm.c:367 `udelay(10)`** stop-path drain — heuristic with no derivable basis; teardown-only. State a real reason or remove.
5. **dm.c:181,183 `al_eth_queue_enable`** return ignored; it's a `-EPERM` stub and the real enable is inside `al_udma_q_init`. One-line comment would prevent a future reader thinking enabling is missing.
