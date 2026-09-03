# SerDes TX equalisation — research record

Everything established while root-causing #121 (10G SFP+ TX at 34–594 Mbit/s) and
answering "how *should* the tap value be chosen?" (#207).

Scope: Alpine V2 / AL-324, HSSP SerDes group D lane 0, 10GBASE-SR via
FS SFP-10GSR-85. Related: #121 #193 #195 #196 #199 #200 #207 #209.

---

## 1. The mechanism

- TX equaliser = FFE pre-distorting the waveform to cancel channel low-pass response.
- Taps: `c_minus_1` (pre-cursor), main, `c_plus_1`/`c_plus_2` (post-cursor).
- **`total_driver_units` is a shared budget, not an independent knob:**
  `main = tdu − (c_minus_1 + c_plus_1 + c_plus_2)`.
  U-Boot `al_serdes_10g.c:295`; `al_serdes_tx_deemph_set()` writes `LEVN = c0+c+1+c−1`.
- Raising a tap **steals amplitude from the main cursor** → the upper cliff in #121
  (at `c_plus_1=10`, main is 9 of 19 units and the lane dies).
- `amp` = 3-bit swing select, 1 = 952 mV … 7 = 1331 mV diff pk-pk.
- `slew_rate` 0–3 = 31 / 33 / 68 / 170 ps.

### The channel, and why it is a board constant

```
SoC pad → PCB trace → SFP+ cage → module input
```
Fixed per board. SFF-8431 defines a **host output compliance mask at point B**; a
host meeting it works with any compliant module. Per-module tuning ⇒ host out of spec.

| media | channel | mechanism |
|---|---|---|
| optic | fixed, ends at module input | static equalisation |
| DAC | **variable — includes the cable** | Clause 72 link training adapts |

That is why the vendor DT carries `dac-length` and gates LT on `10G_DA`.

---

## 2. Why there is no feedback on SR

- TX quality is observable **only at the far end**. Locally nothing moves — #121
  surfaced as TCP retransmits, a layer-4 symptom.
- **10GBASE-SR has no link training and no auto-negotiation.** "-LR, -SR and -ER are
  all just -R … raw 64b/66b without link auto-negotiation or link training. -KR is
  special." <https://electronics.stackexchange.com/questions/196965/>
- Clause 72 LT is **10GBASE-KR, media Cu-Backplane** (802.3ap-2007, CL49/72).
  SFP+ direct attach is **SFF-8431**, an MSA, not an IEEE clause — there is no
  IEEE 10GBASE-CR at 10G (CR appears at 25/40/100G).

### IEEE is actively retrofitting LT to optics — 802.3dj

- Ghiasi et al., *Extending Link Training to Optics*, 802.3dj Jul 2023 Berlin —
  <https://www.ieee802.org/3/dj/public/23_07/ghiasi_3dj_01_2307.pdf>
- Ghiasi et al., *Benefits of Transmitter Adaptation for Optical Links*, Nov 2023 —
  <https://grouper.ieee.org/groups/802/3/dj/public/23_1128/ghiasi_3dj_01a_2311.pdf>
- <https://www.ieee802.org/3/dj/public/23_09/ghiasi_3dj_01a_2309.pdf>
- OIF CMIS-LT white paper — <https://www.oiforum.com/wp-content/uploads/OIF-WP-CMIS-LT-01.0.pdf>

**Load-bearing warnings from those papers — read before building #209:**

- "Traditional in-band or out-band link training where the receiver adjusts the
  partner TX FFE tap values **unlikely to work for optical transmitters**":
  autonomous TX FFE tuning may drive optical **TX OMA into non-compliance**;
  VCSEL/DML asymmetric turn-on/off, MZM cosine compression, EA non-linear transfer.
- The 802.3dj proposal is therefore **limited to presets only**, deliberately.
  "All presets must be known good settings that operate with the reference
  equalizer, otherwise receiver may lose lock."
- **Over-emphasis creates an ERROR FLOOR, not a plateau.** "Pre-emphasis/overshoot
  can both improve and degrade the link BER"; beyond ~20 % overshoot the link
  develops an error floor (≈1 order of magnitude BER change). "Transmitter with
  large amount of pre-emphasis used with high BW receiver may result in clipping."
  Backed by 802.3cu data — <https://grouper.ieee.org/groups/802/3/cu/public/March20/rodes_3cu_01a_031720.pdf>
- ⇒ **A monotone hill-climb on BER can walk into the floor. Sweep past the apparent
  optimum in both directions.**
- Their algorithm grades by far-end BER: "request the setting with best BER",
  30 s timeout.

---

## 3. Metrics — ranked

### 3.1 PCS Clause 49 counters — **implemented, best local option**

- `ETH_MAC_KR_PCS_BASE_R_STATUS2` = `0x21` = MDIO **3.33**, `al_hal_eth_mac_regs.h:2077`.
- Clause 49 layout, matching `include/uapi/linux/mdio.h:327-328`:
  **bits 7:0 = errored-block counter, bits 13:8 = BER counter.**
- Was read at `al_hal_eth_main.c` using **only bit 15** — both counters discarded.
- **Now harvested and exposed** as `pcs_errored_blocks` / `pcs_ber_events` in
  `ethtool -S`. Verified live, both 0 on a healthy link.
- **Gotchas:** both are **clear-on-read**, so exactly one reader is allowed — the
  phylink poll. A userspace sweep reading MDIO 3.33 directly **races the 1 Hz poll**
  and both get partial counts. Both **saturate** (255 / 63) — a pegged value means
  "at least this many", so they go flat exactly where the link is worst.
- Memory-mapped siblings: `kr.pcs_stat` BLOCK_LOCK bit 8 / HI_BER bit 9,
  `al_hal_eth_mac_regs.h:1459-1461`.

### 3.2 Eye measurement (HSSP) — richest, currently dead code

| API | returns | ref |
|---|---|---|
| `eye_measure_run()` | 0 (closed) … 0xffff (open), ≤1 s | `al_hal_serdes_hssp.c:1208` |
| `eye_diag_sample(x,y)` | 2D scan, x 0–63 over 2 UI, y 0–62 over ±500 mV, BER-like 0x0000–0xffff | `hssp.c:1445`, regs 33-38 |
| `rx_equalization()` | integer score from `RXEQ_BEST_EYE_MSB/LSB` | `hssp.c:2665` |
| `calc_eye_size()` | width / height | `hssp.c:3075,3166` |

- **Bug found and fixed:** `*width =+ reg_value` / `*height =+ …` — a transposed
  `+=` that parses as assignment of unary plus, discarding the MSB read on the
  previous line. Every eye size would have read ≤63.
- **Dead twice over:** `al_eth_rx_equal_run()` is gated on `10G_DA` *and* on
  `lm_context->rx_equal`, which the driver never sets.
- Eye measurement **re-runs the RX adaptive equaliser**, so `optic_rx_params.override`
  must be cleared first — moving RX away from production settings.

### 3.3 PRBS BIST — viable, one correct loopback mode

- Ops: `bist_pattern_select`, `bist_tx_enable`, `bist_rx_enable`,
  `bist_rx_status(is_locked, err_cnt_overflow, err_cnt)` —
  `al_hal_serdes_interface.h:866-903`. Implemented for HSSP at `hssp.c:864`.
- **Patterns on HSSP: PRBS7, PRBS23, PRBS31, CLK1010, USER. NOT PRBS9** — no case,
  hits `al_assert(0)`. (SFF-8431 names PRBS9 normative; use PRBS31, more demanding.)
- Working recipe: `delroth-alpine_hal/samples/serdes.c:102`.
- **Caveats:** `bist_overrides_enable()` forces datawidth/divrate on **all 4 lanes of
  the group**; `bist_overrides_disable()` only restores LOCWREN bits — a lane
  re-init is required afterwards.

Loopback modes, `al_hal_serdes_interface.h:288-327` (all compiled in —
`AL_SERDES_BASIC_SERVICES_ONLY` undefined):

| mode | includes | verdict |
|---|---|---|
| `PMA_SERIAL_TX_IO_TO_RX_IO` | "loop back done directly from TX to RX pads" — TX driver + its FIR/amp/slew | **the one to use** (excludes package, trace, cage, optics) |
| `PMA_INTERNALLY_BUFFERED_SERIAL_TX_TO_RX` | "bypasses IO Driver" | **useless for TX eq** |
| `PMA_IO_UN_TIMED_RX_TO_TX`, `PMA_PARALLEL_RX_TO_TX` | RX→TX | wrong direction |
| `PCS_PIPE` / `PCS_NEAR_END` / `PCS_FAR_END` | — | **not implemented in HSSP**, `default:` → `al_assert(0)` (`hssp.c:832-834`) |

### 3.4 Throughput — dead end

Saturates. `c_plus_1` 4/5/6/7 all read 9.42 Gbit/s. A 10GBASE-R link at BER 1e-9
still does full line rate. Finds window edges only, cannot rank within.

### 3.5 UDP fixed-rate loss — usable proxy through a switch

Store-and-forward switches **drop bad-CRC frames**, so corruption on hop 1 arrives
as *missing frames*, not errors. Offer a fixed rate well below line rate ⇒ loss
scales with BER instead of clipping. Implemented as
`scripts/serdes-tx-sweep.py --metric udp-loss`.

---

## 4. Measurement-time arithmetic — drives everything

At 10.3125 Gb/s, to observe 100 errors:

| BER | dwell per sweep point |
|---|---|
| 1e-9 | ~10 s |
| 1e-12 | **~2.7 hours** |

⇒ **BER only discriminates while the link is marginal.** Once clean it has no
gradient, and eye/margin is the only metric with resolution. Start sweeps from a
deliberately bad tap so there is an error floor to climb out of.

Corroborated by the Altera/CERN flow: "if multiple settings lead to 0 BER → use
eye viewer to compare eye width and height to identify the best settings."

---

## 5. Far-end instrumentation

### Our topology
`box (10G SR) → TP-Link switch → dev host enp7s0 (fibre)`. The dev host's counters
measure **hop 2**, not the link under test — useful only as a control that must
read zero.

### Dev host NIC — dead end, tested
- `07:00.0 Mellanox MT26448 [ConnectX EN 10GigE, PCIe 2.0] [15b3:6750]` = **ConnectX-2**.
- `mlxlink -d 07:00.0` → **"Operation not supported"**. MFT 4.30.0-139 installed.
- `mlx4_en` exposes **no** `*_phy` counters, no PCS statistical group, no standard
  `eth-phy` stats group. Only `blueflame` / `phv-bit` priv flags.
- ⇒ **A ConnectX-4 or later is required** to use any of §5.1–5.2.

### 5.1 `mlxlink` (needs CX-4+)
- `-c | --show_counters` → `Raw Physical Errors Per Lane`, `Raw Physical BER`
  (pre-FEC), `Effective Physical Errors/BER` (post-FEC). `--pc` clears.
  **Use Raw** — Effective is masked by FEC, and 10GBASE-SR has none.
- `-e | --show_eye` → `Physical Grade`, `Height Eye Opening [mV]`,
  `Phase Eye Opening [psec]`. Confirmed on a real ConnectX-4 network port:
  <https://forums.developer.nvidia.com/t/how-to-evaluate-the-eye-information-of-mellanox-connectx-4-network-card/206063>
- `--margin [--measure_time N] [--lane N]`, `-b/--ber_collect <csv>`,
  `--amber_collect <csv>` (includes **`Conf_Level_Raw_BER`** — whether a step ran
  long enough to trust), `--rx_fec_histogram`.
- PRBS checker: `--test_mode EN --rx_prbs PRBS31 --rx_rate 10G`; `--test_mode TU`
  = "Perform PRBS Tuning". Allows **different TX and RX patterns/rates**, i.e.
  designed for one-direction-at-a-time. `--force_tx_allowed` if the module refuses.
- At 10GbE with `FEC : No FEC`, plain `mlxlink -d <dev>` prints a firmware verdict:
  `Status Opcode : 15 / Recommendation : Bad signal integrity` —
  <https://forums.developer.nvidia.com/t/mlxlink-bad-signal-integrity-on-mellanox-connectx-4-lx/299434>
- Raw vs Effective — <https://enterprise-support.nvidia.com/s/article/mlxlink-Understanding-Effective-vs-Raw-Bit-Error-Rate-BER>
- Reference — <https://networking-docs.nvidia.com/mftswum/4350/mlxlink-utility.md>
- **Install: `dnf install mstflint`.** `mlxlink` is in the open-source
  `Mellanox/mstflint` tree (`mlxlink/modules/`, built with
  `--enable-adb-generic-tools`), and Fedora's spec passes it. **No NVIDIA MFT, no
  `kernel-mft-dkms`, no `mst start`.** Address as `-d mlx5_0` or `-d <pci bdf>`.
  Do **not** use `-p` on a NIC — port 2 is a separate mst device.

### 5.2 `ethtool -S` PHY counters by driver

| driver / NIC | PHY-layer RX error counters | verdict |
|---|---|---|
| **mlx5** (CX-4/5/6) | `rx_pcs_symbol_err_phy` (post-FEC), `rx_corrected_bits_phy` (raw pre-correction), `rx_err_lane_N_phy`, `rx_crc_errors_phy`, `rx_symbol_err_phy` + mlxlink | **best** |
| **bnxt_en** (Broadcom) | `rx_pcs_symbol_err`, **`rx_bits`** (a real BER denominator), `rx_fec_corrected_blocks`, `rx_fec_uncorrectable_blocks`, `rx_fcs_err_frames` | usable; no eye (niccli is OEM-restricted) |
| **i40e** (X710) | `port.illegal_bytes`, `port.rx_crc_errors`, `port.mac_local/remote_faults` | weak but usable |
| **ixgbe** (X520/82599) | `rx_crc_errors` only, a netdev stat | **dead end** |
| **mlx4** (CX-2/3) | none | **dead end — this is ours** |

- mlx5 PPCNT groups: 802.3 (0x0) and phy-statistical (0x16). Group 0x16 also carries
  **`phy_received_bits`** — the BER denominator — **not exposed by ethtool**, only
  via mlxlink/PPCNT.
- Gated on `MLX5_CAP_PCAM_FEATURE(ppcnt_statistical_group)` and
  `per_lane_error_counters`; absent firmware caps ⇒ counters simply do not appear.
- Driver-agnostic: `ethtool -S <if> --groups eth-phy eth-mac rmon`.
  `ethtool -I --show-fec` is useless here — 10GBASE-R has no FEC.

---

## 6. Loopback options

| option | includes | status |
|---|---|---|
| SerDes `PMA_SERIAL_TX_IO_TO_RX_IO` | TX driver + FIR | viable; excludes trace/cage/optic |
| **Physical fibre loop** on our own module | **trace + cage + optic** | **viable — the real channel** |
| SFP+ i2c module loopback | — | **not standardised at all** |
| Far-end (OAM remote loopback) | whole path both ways | needs peer support + implementation |

### Physical fibre loopback — the numbers

From the FS SFP-10GSR-85 datasheet (`sources/`):
- **"The transceiver is a limiting module"** — **no CDR, no retimer**, so TX
  jitter/ISI propagates optically and remains measurable at the far end.
- `PAVE` −5.0 … −1.0 dBm · `RSENS1` −11.1 dBm OMA · stressed −7.5 dBm OMA ·
  `PMAX` +0.5 dBm · **saturation −1.0 dBm**.
- **Direct loop lands ≈ −1.75 dBm — at overload.** You would measure RX
  compression, not TX equalisation.
- **5 dB** attenuation puts a typical unit at ≈ −7.65 dBm OMA, on stressed
  sensitivity → best discrimination. **3 dB** is the safe blind choice.
- 300 m OM3 gives only ≈1.05 dB — does **not** fix overload, but adds real modal
  dispersion.
- **Minimum 2 m** cabling per 802.3ae — no short stubs.
- Real metric = **BER-cliff sweep**: vary attenuation until failure; better TX eq
  ⇒ cliff at higher attenuation.

### Electrical loopback plug
FS `10GSFP-LPM` / Cisco `SFP-10G-LB`, selectable **0 / 3.5 / 5 dB** internal
attenuation. Gives SerDes TX → known-loss channel → SerDes RX with a documented
insertion-loss profile: a **repeatable reference channel**, complementing the fibre
loop (which is the one that includes the optic).

### SFP+ i2c loopback — dead end, confirmed three ways
- `grep -i loopback` = **0 hits** in SFF-8472 rev 12.4, SFF-8431, SFF-8636.
- Standardised **only in CMIS** — page 13h bytes 180–183, capability page 13h byte
  128. An SFP+ is not a CMIS module. `sonic-platform-common/.../cmis/pages/page13.py`;
  Björn Töpel kernel RFC <https://lwn.net/Articles/1059525/>.
- FBOSS `SffModule.cpp` hardcodes *"Only Miniphoton has loopback capability"* —
  per-part magic by people who own the module design. That is the state of the art.
- Linux has no write path: `sfp_module_eeprom_by_page()` is read-only,
  `sfp_socket_ops` has no `set_module_eeprom`. Only `/dev/i2c-N` direct
  (**our bus: i2c-2, mux ch1, 0x50**), fighting `sfp.c`.

### Module-side knobs, checked on our unit
- **SFF-8472 A2h byte 114 "Tx Input EQ control"**, 0–10 dB in 1 dB steps (Table
  9-18); capability A2h byte 60 bit 0, max A2h byte 63 bits 3:0.
  **Our module: byte 60 = `0x00`, byte 114 = `0x00` → not implemented.** Expected
  for a limiting module.
- SFF-8472 §10.6 **variable receiver decision threshold**, A2h page 02h bytes
  130–131, advertised A0h byte 65 bit 7 — a real standardised SI knob, unchecked.
- Our module's Enhanced Options A0h byte 93 = `0xf0` (alarm flags, soft TX_DISABLE,
  soft TX_FAULT, soft RX_LOS); bits 3:0 clear. A0h byte 92 = `0x68` (DDM,
  internally calibrated).

---

## 7. Standard practice — how this is done properly

- **SFF-8431 compliance point is B**, not TP2. §3.3.1: "Host system transmitter and
  receiver compliance are defined by tests in which a Host Compliance Board is
  inserted in place of the SFP+ module … The compliance points are B and C."
  B = host TX output at the HCB output; C = host RX input; B'/C' module side;
  D = ASIC/SerDes reference. **TP2/TP3 are the IEEE 802.3 Clause 52 *optical*
  points** — the module's compliance, not ours.
- **Table 12 (host TX at B):** TJ ≤ 0.28 UI p-p · DDJ ≤ 0.1 UI · DDPWS ≤ 0.055 UI ·
  UJ ≤ 0.023 UI RMS · Qsq ≥ 50 · eye mask X1 0.12 / X2 0.33 UI, Y1 95 / Y2 350 mV
  at 5e-5 hit ratio.
- **Equipment for a real point-B run** (Tektronix 55W-29390-0, `sources/`):
  SFP+ Host Compliance Board with DC blocks, **>16 GHz real-time scope** (34 ps
  rise-time), 100 GS/s for TWDPc. 15 defined host-TX measurements.
  **Not reproducible in this lab.**
- **No formal software-only substitute exists.**
- **What vendors actually do: characterise once per board, bake into board config.**
  Microchip VelocityDRIVE is explicit — `board.yaml` carries
  `serdes: tx_eq: [{type: SPEED_10000MBIT, pre: 10, main: 50, post: 15}]`, applied
  at init, no runtime feedback —
  <https://microchip-ung.github.io/velocitydrivesp-documentation/vd-docs/2026.06/doc-dbg-serdes.html>
- SONiC does the same via `media_settings.json` — a **static per-media lookup table**
  supplied by the platform vendor, keyed vendor+PN then media type, holding
  `preemphasis`/`idriver`/`ipredriver` per lane as opaque hex, applied by `xcvrd`
  on media insert via `SAI_PORT_SERDES_*` —
  <https://github.com/sonic-net/SONiC/blob/master/doc/media-settings/Media-based-Port-settings.md>
  Rationale is failure-driven, not optimisation-driven. **How vendors derive the
  numbers is nowhere documented.**

⇒ **This directly validates #207:** a per-board constant in config *is* industry
practice. Our error was the *provenance* of the constant, not its existence.

---

## 8. Published sweep methodologies

| source | method |
|---|---|
| `jueshi/windsurf` `serdes_python_api-4.11/*/lib/tx_eq_sweep.py` — <https://github.com/jueshi/windsurf/blob/master/serdes_python_api-4.11/serdes_python_api/e32g/lib/tx_eq_sweep.py> | 3D grid over pre/main/post → `set_tx_eq()` → `meas_ber(lane, 1e-9.5)`, CSV `lane,pre,main,post,ber,date`. **`tx_eq_sweep_fixed()` constrains `main = 24 − (ceil(pre/4)+ceil(post/4))`** — the shared-budget constraint we also have. `repetitions=2`, `pause=0.25 s`. `meas_ber(..., conf_lvl=95)` dwells to establish the target BER at 95 % confidence. |
| Intel/Altera Transceiver Toolkit "Auto Sweep" — <https://prm-fw-hw-docs.web.cern.ch/03_hw_testing/04_Transceiver/> | "Auto-sweep of PMA settings"; "if BER = 0 → leave DFE off; if BER > 0 → sweep DFE"; **"if multiple settings lead to 0 BER → use eye viewer to compare eye width and height"** |
| AMD/Xilinx ChipScoPy — <https://github.com/Xilinx/chipscopy/blob/master/chipscopy/examples/ibert/versal_gty/link_and_eye_scan.py> | cache originals → `property.report()` for valid values → `itertools.product()` over `TX_PRE_CURSOR × TX_POST_CURSOR × TX_DIFFERENTIAL_SWING × RX_TERMINATION_VOLTAGE` → eye scan per point → score `zero_ber_proportion = count(error_count==0)/len(...)` → restore. **"Requires external loopback; internal loopback typically shows minimal parameter sensitivity."** |
| Broadcom switch SDK — <https://github.com/ManiAm/net-lab-prbs> | `phy diag <port> prbs set p=3` (needs `linkscan off` first, else the checker never locks: `-2 errors`); `phy diag <port> dsc` → per-lane `SD`, `LCK`, `EYE(L,R,U,D)`, `DFE(1–6)`, **`TXEQ(n1,m,p1,p2,p3)`**, `VGA`; `prbsstat start i=60 / ber`; **`berproj`** — statistical extrapolation of eye margin to BER floors 1e-9/1e-12/1e-15 |
| `mvsoliveira/IBERTpy` | Vivado TCL: swing/emphasis config, BER tests, eye scans → horizontal/vertical/area histograms → LaTeX report |
| `tusharpathaknyu/serdes-link-validation` | `src/03_eq_sweep.py` sweeps EQ + line rate, records eye opening and bit errors → CSV. **Simulation only** (PyBERT + scikit-rf) |
| SAI (vendor-neutral) — `inc/saiport.h`, <https://github.com/opencomputeproject/SAI> | `SAI_PORT_ATTR_PRBS_PATTERN`, `SAI_PORT_ATTR_PRBS_CONFIG` with **`ENABLE_TX` and `ENABLE_RX` separately** — exactly the "generate here, check there" case; `PRBS_RX_STATE` (clear-on-read error count), `_PER_LANE_BER_LIST` |

**IBERT eye scan is receiver-side** — PG246: "provides RX margin analysis through
eye scan plots on the RX data". ChipScoPy output is a **per-offset BER heat map**:
`ScanPoint(x, y, ber, errors, samples)`; params Horizontal/Vertical Step 1–16,
Horizontal Range ±0.5 UI, Vertical Range 10–100 %, Target BER 1e-5…1e-19, Dwell 0–60.
<https://xilinx.github.io/chipscopy/2024.1/ibert/eye_scan.html> ·
<https://www.xilinx.com/support/documents/ip_documentation/in_system_ibert/v1_0/pg246-in-system-ibert.pdf>

**PRBS caveat both vendors document:** PRBS replaces normal data, the PCS reports
link-down, and link supervision must be disabled first or the SerDes is
re-initialised and the checker never locks.

### The consensus method

1. PRBS (or live traffic) one way.
2. **Raw pre-FEC error counter at the far-end receiver** as the primary metric.
3. **Eye / margin scan as the tie-breaker once BER floors at zero.**
4. Sweep both directions past the optimum — over-emphasis has an error floor.

---

## 9. Generic Linux — clean negative

- Complete PHY tunable list, `include/uapi/linux/ethtool.h:289-302` = **7 entries**;
  newest three are Micrel KSZ87xx-only; `DSP_EQ_INIT_VALUE` is an **RX** seed on
  100BASE-TX copper.
- **No netlink message for PMA/SerDes TX drive.**
- **Zero in-tree Ethernet drivers do closed-loop TX-eq tuning outside Clause 72/73.**
  `ice` is GET-only (no `ice_aq_set_phy_equalization`); `bnxt`'s `preemphasis` field
  is dead; `octeontx2`'s `CGX_CMD_DISPLAY_EYE` / `CGX_CMD_PRBS` are never issued;
  everything else is a static C table or DT.
- PCIe lane margining is **not in mainline** (v7 series in flight, lkml 2026-08-28);
  no Ethernet equivalent exists.
- ⇒ Our `serdes_tx_*` sysfs attributes are the right and only mechanism, and match
  what mlxlink exposes.

### Our sysfs knob
`al_eth_sysfs.c:313-329` registers `serdes_tx_{amp,total_driver_units,c_plus_1,c_plus_2,c_minus_1,slew_rate}`
on the PCI device → `/sys/class/net/<if>/device/serdes_tx_*`.
- **Values are parsed as HEX** (`kstrtoul(buf, 16, …)`, `al_eth_sysfs.c:206`).
- The write commits via `al_eth_lm_static_parameters_override()` but that only sets
  `tx_param_dirty`; `al_eth_serdes_static_tx_params_set()` runs from phylink's
  `pcs_config` → **each sweep step needs a link bounce to take effect.**
- Upstream note: in the newer vendor generation these attributes are **0444,
  read-only** — no runtime tuning knob at all.

---

## 10. Not retrieved

- **NXP AN5119** (SerDes tuning app note) — `nxp.com/docs/en/application-note/AN5119.pdf`
  is **HTTP 404**, as are the `.cn`/`.jp` mirrors and siblings `AN12572`, `AN12950`,
  `QCVS_SerDes_User_Guide.pdf`. NXP's `/docs/en/**.pdf` tree is retired; the Zoomin
  portal renders nav-only without login. Only copy located is an attachment in
  <https://community.nxp.com/t5/T-Series/QCVS-serdes-tool-where-can-I-find-the-spec-quot-SerDes/m-p/581772>
  behind a signed-in session. **Its metric and method are therefore unknown.**
- Broadcom / Marvell / Microchip SDK docs on how per-port pre-emphasis values are
  *derived* — NDA, nothing public.
- Academic literature on open-loop TX FFE tap search graded by a far-end error
  counter — **nothing on point**; everything found is receiver-side adaptive
  equalisation. The nearest peer-reviewed treatment is the 802.3dj set in §2.
- DesignCon 2026 "Optimizing Host Output (TP1a) Equalization … for Linear Optical
  Links" — JS-only session page, title confirmed:
  <https://dcon26.mapyourshow.com/8_0/sessions/session-details.cfm?scheduleid=191>

---

## 11. Our current position

- `optic_tx_params.c_plus_1` = **0x5** (vendor default 0x2). Clean window 4–7;
  0x5 centres it. **Measured, reproducible, 20× the default — but not a
  characterisation** (#207): one board, one module, 35.5 °C, pass/fail metric.
- QNAP independently moved the **same parameter on the same SoC** (AL-324) from
  0x2 → **0x4**, plus `amp` 0x1 → 0x3, `c_minus_1` 0x2 → 0 — on the *copper* table.
  Convergent, one step from ours.
- **No vendor anywhere has retuned `optic_tx_params`** — 13+ source copies identical.
- **The UNVR has no retimer** (#202, all five i2c buses probed) and no switch chip:
  its SFP+ cage is wired **straight to the SoC SerDes**, so the SoC's TX eq is the
  only conditioning in the path. Annapurna's EVP has a br410 retimer; MikroTik's
  cages sit behind a Marvell 98PX1012. **That is why this defect bites us and
  nobody else.**
