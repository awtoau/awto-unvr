# 10G SFP+ (eth2) U-Boot bring-up plan — UNVR / AL-324 (Alpine V2)

Scope: the **10G-specific** bring-up for eth2 (PCI `1c36:0002`) — SerDes lane, PCS,
MAC mode, SFP detection. The UDMA/EC datapath and send/recv are **shared with the 1G
driver** (`al_eth_dm.c`) and are NOT re-implemented here. 10G passes no traffic until
the shared UDMA TX fix (M2S never completes a TX descriptor) lands — see §8.

Hardware-of-record: live 1.3.35/1.4.9 dmesg (`docs/hardware.md` §Ethernet), stock DTB
board-cfg (`docs/porting-roadmap.md` Phase 5), stock U-Boot `al_eth.c` + `board.c`.

## 1. What the port is

| Property | Value | Evidence |
|---|---|---|
| PCI id | `1c36:0002` = al_eth `DEV_ID_ADVANCED` (0x0002) | `al_hal_eth.h:111`; hardware.md:39 |
| eth rev_id | 2 (Alpine V2 basic) — 1.4.9 driver re-labels port2 `rev_id 3`, same silicon | hardware.md:417 |
| External PHY | **none** (`phy No, addr 0`) — no MDIO/phylib needed | hardware.md:423 |
| MAC mode | `AL_ETH_MAC_MODE_10GbE_Serial` | mac.h:210; board flow al_eth.c:373-382 |
| Media | SFP optic, `media 5` (auto-detect / 10g-serial, `sfp_plus_module_exist`) | hardware.md:424; board.c:720-740 |
| LM mode | `AL_ETH_LM_MODE_10G_OPTIC` — **fixed 10.3125 Gbps, no KR AN/LT** | hardware.md:425; roadmap:256 |
| SerDes | **group 3, lane 0, 156.25 MHz** ref | roadmap:254-255 |
| Retimer | `br410` present-but-**disabled** → skip | roadmap:256 |
| SFP EEPROM | i2c `0x50`, behind pld PCA9546 mux `0x71` **ch1** | i2c-map.md; al_eth.c:82 `SFP_I2C_ADDR 0x50` |

Boot-log "al_eth2 [PRIME]" (hardware.md:38): `[PRIME]` is U-Boot's *active* NIC
marker, not a mode — the 1G port carries it in our current boot. No 10G meaning.

## 2. Datapath — same UDMA/EC as 1G, different MAC front-end

- Three BARs, identical mapping to eth1: UDMA=BAR0, EC=BAR4, MAC=BAR2
  (`al_eth_dm.c:53-55`, README table). Bind by PCI id `0x0002`, not DT.
- UDMA rings, `al_eth_tx_pkt_prepare` / `al_eth_comp_tx_get` / `al_eth_pkt_rx`,
  FLR, snoop-enable, cache maintenance: **reuse `al_eth_dm.c` verbatim in pattern**.
  Only the front-end differs (MAC mode + SerDes/PCS + no-PHY link).
- `ap.dev_id = AL_ETH_DEV_ID_ADVANCED`, `ap.rev_id = AL_ETH_REV_ID_2`,
  `ap.serdes_lane = 0`.

## 3. MAC mode 10GbE_Serial is reachable with the CURRENT compiled HAL subset

Key feasibility result: **no `mac_v3`/`mac_v4`, no `al_hal_eth_kr.c`, no serdes
closure is needed** for fixed-10G optic.

- rev_id 2 → `al_eth_mac_v1_v2_handle_init` (`al_hal_eth_mac.c:120-123`), already
  compiled (Makefile).
- `al_hal_eth_mac_v1_v2.c` fully implements `AL_ETH_MAC_MODE_10GbE_Serial`:
  - `mac_config` case — `al_hal_eth_mac_v1_v2.c:257-276` (self-contained register
    writes: `mac_10g.cmd_cfg=0x01022810`, `gen.cfg=5`, `gen.mux_sel` KR input
    `0x00073910`, `gen.clk_cfg=0x10003210`, `sd_fifo_ctrl`, LED). **No serdes/KR
    calls.**
  - `mac_start_stop_adv` — `:352-378` (10G TX/RX enable).
  - `link_status_get` — `:554-568` reads KR-PCS BASE-R Status2 (`kr.pcs_addr/data`)
    for block-lock + local/remote fault. **This is the fixed-10G link-up gate.**
  - `tx_flush_config`, `rx_pkt_limit_config`, `loopback_config` — all 10G-aware.
- The stubs (`al_eth_stubs.c`) for v3/v4 stay as-is; 10G never calls them.

## 4. Exact HAL call order for 10G bring-up (mirror of `al_eth_dm_dma_init`)

Front-end deltas vs the 1G path are **bold**. Everything else is copied from
`al_eth_dm.c`.

1. `al_eth_flr_rmn(cfg_read, cfg_write, dev, mac_regs)` — clean-state FLR (dm.c:235).
2. `al_eth_adapter_init(&adapter, &ap)` — **`ap.dev_id=ADVANCED`,
   `ap.serdes_lane=0`** (dm.c:237).
3. `al_eth_dm_snoop_enable(dev)` — AXI SMCC snoop, post-FLR (dm.c:245). Shared.
4. **SerDes + PCS: `al_serdes_10g_init()`** — `drivers/phy/al_serdes/al_serdes_10g.c:150`.
   Group-3 fixed-10G electrical (`mode_set_kr`), lane-0 optic EQ, 10GBASE-R PCS.
   Order: before `mac_start`; reconcile PCS-vs-MAC mux ownership (§7).
5. `al_eth_queue_config/enable` TX+RX, `al_udma_q_handle_get` (dm.c:267-272). Shared.
6. **`al_eth_mac_config(&adapter, AL_ETH_MAC_MODE_10GbE_Serial)`** (dm.c:278 uses RGMII).
7. `al_eth_rx_pkt_limit_config(&adapter, 30, 1518)` (dm.c:279). Shared (10G-aware).
8. Prime RX ring + `al_eth_rx_buffer_action` + cache maintenance (dm.c:281-303). Shared.
9. `al_eth_mac_start(&adapter)` (dm.c:305). Shared.
10. **NO `al_eth_mdio_config`, NO `phy_connect`/`phy_startup`** — no external PHY.
11. **NO `al_eth_mac_link_config`** — that path is SGMII AN; fixed-10G optic does not
    use it (stock skips it too for 10G_OPTIC). Link = PCS block-lock, §5.

### Link-up detection (replaces phylib for the no-PHY 10G port)

- SerDes lane: `obj.pll_lock_get`, `signal_is_detected(lane0)`, `cdr_is_locked(lane0)`,
  `rx_valid(lane0)` — via the `al_serdes_grp_obj` vtable (interface.h:1059-1221),
  already wired in `al_serdes_10g_status()`.
- PCS block-lock: KR-PCS BASE-R Status2 (Clause-45 dev3 reg `0x21`) bit 15 — either
  `al_eth_link_status_get` (mac_v1_v2:554) or the direct read in
  `al_serdes_10g_status()`. **Good link = PLL LOCKED + signal + CDR LOCKED + rx_valid
  + pcs_block_lock LOCKED.**
- No KR Clause-73 AN / Clause-72 LT: the optic/DAC comes up at a fixed rate by
  construction. (KR AN/LT would need `al_hal_eth_kr.c` + the LM FSM — deliberately
  excluded, matching stock's `10G_OPTIC` behaviour.) Stock confirms LT/AN + rx-equal
  engage **only** for `10G_DA` (passive DAC) / `25G` media, never for a 10G **optical**
  SFP (subagent map §5-6).

### If a full LM layer is wanted later (auto-detect / DAC / rx-equal)

- The stock u-boot GPL drop ships al_eth's SerDes/LM/KR HAL as a **binary blob** — the
  only readable source is the **kernel tree**:
  `/mnt/2tb/unvr-port-refs/UBNT-source-code/UNVR-1.3.35-GPL/linux-arm64-unvr-4.1.37-ubnt/drivers/net/ethernet/al/`
  (`al_init_eth_lm.c`, `al_init_eth_kr.c`, `al_hal_eth_kr.c`) + `drivers/soc/alpine/`.
- Stock establish loop (for reference): `al_eth_lm_link_detection` +
  `al_eth_lm_link_establish`, ≤30 iters × 100 ms, break after 5 consecutive successes
  (`al_eth.c:807-835`). Our fixed-10G bring-up needs only a PCS block-lock poll, not
  this FSM.
- We already vendor the delroth HAL, so a future LM add would port these kernel files,
  not the blob.

## 5. SFP module detection (I2C)

- Path: pld i2c bus (`i2c_pld` @ `0xfd880000`, Linux i2c-0) → PCA9546 mux `0x71`
  **ch1** → SFP EEPROM `0x50` (SFF-8472). U-Boot: `i2c dev 0; i2c mw 0x71 0 2 1`
  (select ch1); read `0x50`.
- Presence/type bytes (SFF-8079/8472): byte 0 = identifier (`0x03` = SFP), byte 3 =
  10G compliance codes (10GBASE-SR/LR bits), bytes 12/8 = nominal bitrate. Stock reads
  these via `al_eth_i2c_byte_read(bus, 0x50, addr, &val)` (`al_eth.c:440-457`).
- The `s35390a` RTC (`0x30`) is on ch0, a separate bus from the SFP EEPROM - the RTC
  ch0 wedge (`rtc-s35390a-fault.md`) never actually blocked SFP EEPROM access.
- Gating: stock sets `sfp_detect_needed=TRUE` only for **auto-detect** media
  (`AUTO_DETECT`/`AUTO_DETECT_AUTO_SPEED`/`25G*`), then LM reads the EEPROM and returns
  `-ENETDOWN` if the read fails (`al_eth.c:697-700`). For plain **`10g-serial`
  (`10GBASE_SR`) media, `sfp_detect_needed=FALSE`** — stock does **no EEPROM probe at
  all** and uses `default_mode=10G_OPTIC` directly (subagent map §4).
- **Recommendation: configure our port as fixed `10g-serial`, NOT auto-detect.** This
  makes bring-up i2c-independent → sidesteps the RTC ch0 wedge entirely, and matches
  the observed `10G_OPTIC` behaviour. Bring the lane up unconditionally, report PCS
  block-lock. Add the i2c EEPROM read as a **diagnostic only**, gated behind the RTC
  sda-hold fix.
- Stock u-boot LM does **not** use `gpio-sfp-present` for presence (`gpio_get`/
  `gpio_present` left NULL) — presence is purely the i2c EEPROM read (subagent §4). A
  presence GPIO on PCA9575 `0x20` (SFP straps/LED) is a possible future add; bit
  mapping is a HW-TODO.
- **No tx_disable/mod-def0/tx-fault/los pins exist on this board — verified, not just
  undeclared (#134).** Checked the live stock DTB pulled from the running box's own
  firmware, the UNVR GPL U-Boot source's board-params parser (`dt_based_init_eth_params_port`,
  field-identical struct to ours), and the PCA9575/SoC GPIO maps — none carry a laser-
  control pin. The Alpine V2 board-params ABI itself has no field for it (same across
  the whole family, not UNVR-specific). Module laser stays on for as long as it has
  power; no software kill-switch, full stop — this is a hardware limitation, not a
  driver/DT gap to fix.

**Open question:** SFP+ front-panel LED reads off under our U-Boot even with the
module confirmed powered+correctly-identified via EEPROM (below) — presumed a
link-state indicator that hasn't lit because the 10G link never fully
establishes (see #90/#132), not a power tell. Not yet confirmed which GPIO
actually drives that specific LED or what condition lights it.

### 5a. Reading the SFP EEPROM by hand from the U-Boot prompt

```
i2c dev 0                  # pld i2c bus
i2c mw 0x71 0.0 2 1        # select PCA9546 mux ch1 (SFP EEPROM) - see gotcha below
i2c probe                  # 0x50 (+ 0x51 if DDM page present) should now show
i2c md 0x50 0 1            # byte 0 - identifier, expect 0x03 (SFP)
i2c md 0x50 3 1            # byte 3 - 10G compliance codes, expect 0x10 (10GBASE-SR)
```

Two gotchas that look like "module not present/powered" but aren't:

- **The PCA9546 mux has no internal register addressing — it's a single control
  byte, not a register+value device.** `i2c mw 0x71 0 2 1` (default 1-byte
  address width) sends TWO bytes on the wire (`0x00` then `0x02`); the mux
  latches the `0x00` and deselects all channels, so the write silently doesn't
  do what it looks like it does. Always use the **`.0` zero-width address**
  form (`i2c mw 0x71 0.0 2 1` / `i2c md 0x71 0.0 1`) for the mux specifically.
  The SFP EEPROM itself (`0x50`) is a normal 1-byte-addressed device — don't
  use `.0` there.
- **A failed/NAK'd read to ANY device on this bus resets the mux's channel
  selection as a side effect** (confirmed: `i2c md 0x50 0 20` — a 20-byte burst
  read, which fails — left the mux reading back `0x00` on the next check, even
  after a **correct** `.0`-addressed select). Re-select the mux
  (`i2c mw 0x71 0.0 2 1`) after every failed read before probing again, or a
  vanished `0x50`/`0x51` will look like the module dropped off the bus when
  it's actually just the mux. Also: **read one byte at a time** — a 20-byte
  burst (`i2c md 0x50 0 20`) reliably fails ("Error reading the chip: 1") where
  single-byte reads (`i2c md 0x50 0 1`) succeed; looks like a DW i2c controller
  burst-length limit on this device, not investigated further.

## 6. Device-tree — already present, no additions required for bring-up

- `serdes@fd8c0000` compatible `annapurna-labs,al-serdes-25g`, reg-names
  `serdes` (`0xfd8c0000`/`0x2400` PMA) + `pcs` (`0xfe120000`/`0x10000` MAC-adapter).
  Consumed by `CONFIG_AL_SERDES` via ofnode (dts:185-191). **Sufficient.**
- eth2 MAC datapath binds by PCI id (no DT node) — same model as eth1.
- Optional future props (only if a DM_ETH-integrated driver reads them instead of
  compile-time constants; stock reads board-cfg, not these): `serdes-group=3`,
  `serdes-lane=0`, `phy-mode="10gbase-r"`, `managed="in-band-status"`,
  SFP i2c bus handle. Not needed for the constant-driven bring-up below.
- `pcs` base disagreement to resolve on box: hardware.md/DTS say `0xfe120000`; an
  earlier task brief said `0xfc200000`. DTS value is current best (roadmap/live.dts).

## 7. Ownership / reconciliation with the shared MAC+UDMA work

- Two overlapping writers of the KR/PCS mux: `al_eth_mac_v1_v2.c` 10GbE_Serial case
  writes `gen.mux_sel` (KR input) and touches `kr.pcs_*`; `al_serdes_10g.c` writes
  `pcs_10g_ll_cfg` + reads block-lock. **Decide one owner at merge.** Cleanest:
  MAC `config` owns the MAC-adapter/PCS-mode writes; the serdes driver owns the PMA
  lane + EQ + a read-only status view. The drafted serdes PCS write is a standalone
  crutch (guarded by the optional `pcs` reg) — drop it once the MAC path runs.
- Do NOT edit `al_eth_dm.c` or `hal/udma/*` / `al_hal_eth_main.c` (other agents).
  The 10G driver is a NEW file that calls the same public HAL entry points.

## 8. Dependency on the shared UDMA TX fix

- The M2S engine never completing a TX descriptor is a **shared datapath bug** (both
  ports use the same UDMA). 10G inherits it: link can come up (PCS block-lock) but
  ping/tftp will fail identically until that fix lands.
- **Independently testable now (no traffic needed):** `serdes init` + `serdes status`
  → PLL/CDR/PCS block-lock with an optic/DAC + link partner inserted. This validates
  the entire 10G-specific path (SerDes + PCS + MAC mode) without the UDMA.

## 9. Config symbols

- `CONFIG_AL_SERDES=y` — already in `alpine_v2_unvr_defconfig:120` (SerDes+PCS+`serdes` cmd).
- 10G MAC DM_ETH driver: fold into `CONFIG_AL_ETH` (same HAL subset already builds it)
  or add `CONFIG_AL_ETH_10G` gating the new file. No new HAL objects to compile —
  `al_hal_eth_mac_v1_v2.o` (already listed) carries the 10GbE_Serial path.

## 10. Test hardware (deliverable B)

- **SFP+ module:** a 10GBASE-SR optic (matches live `10G_OPTIC`) — e.g. generic
  850 nm SR SFP+ — plus a matching optic on the link partner and an LC-LC OM3/OM4
  duplex fibre. A 10G passive DAC (SFP+ direct-attach) also works (`10G_DA`) and needs
  no fibre — simplest for a bench link partner.
- **Link partner:** any 10G SFP+ switch/NIC port set to fixed 10G, no-AN (the AL side
  is fixed 10.3125G, KR AN off). If the partner insists on AN, link may flap (live
  dmesg showed `link established / wasn't established` flapping).
- **Cabling:** DAC twinax (bench) OR 2× SR optics + duplex LC fibre.
- **Serial console** for `serdes status`; box reachable over the 1G port / SSH for
  driving tests without disturbing the SFP link.
- Optional: capture the SFP EEPROM (`0x50`) once the RTC sda-hold fix lands, to
  confirm module type/rate (currently uncaptured — hardware.md:62).

## 11. Open questions / risks (deliverable D)

- **SFP i2c blocked by the RTC bus wedge** (§5) — highest-friction item; fix
  `i2c-sda-hold-time-ns` first or presence-detect via GPIO strap.
- **SerDes lane index** — assumed lane 0 (UNVR DT explicitly sets `serdes-grp 3
  serdes-lane 0`, roadmap:255). Note the stock **code default** is `3 - port_idx` =
  lane **1** for port2 (board.c:768) — only the explicit DT value gives lane 0. Our
  constant uses lane 0; the physical TX/RX pad wiring on AL-324 is unverified on our
  board (HW-TODO in `al_serdes_10g.c`).
- **Optic TX/RX EQ params** — vendor optic defaults; may need retune per the real
  module (rx_equalization sweep).
- **PCS base + PCS-vs-MAC reset ordering** — `0xfe120000` to confirm; single-owner
  decision (§7).
- **Fresh (non-chainload) SerDes** — `mode_set_kr` may need prior serdes-group PLL /
  PBS clock init that the stock bootrom/earlier stage did; chainloaded bring-up hides
  this. Watch PLL-lock failure on a cold path.
- **No-AN link partner** — a partner forcing AN may not link; document required
  partner config.
- **Shared UDMA TX bug** (§8) — no traffic until it lands; not a 10G-specific defect.
- **Retimer** — br410 present-but-disabled; assumed skippable. Confirm it is not in
  the SFP TX/RX path electrically.
