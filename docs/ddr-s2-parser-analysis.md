# S2 stage2_loader DDR parser — Ghidra analysis + portable port

Rigorous reverse of the DDR config path in `docs/nor-reference/preboot-s2-decompiled.c`
(stage2_loader v2.22.3, Thumb-2 @ `0xf2200000`). Goal: reuse the vendor's exact algorithm
in the U-Boot SPL. Every claim cites `function@addr` + decompile line; table data recovered
from `s2-loader-stage2_v2.22.3-25044B.bin`, never guessed.

Decode of THIS unit's live dump: `scripts/decode-ddr-records.py` →
`tmp/logs/decode-ddr-records.log`.

## Open (reused as-is) vs reversed glue

- **OPEN — input (JEDEC DDR4 SPD parse).** The S2 reads standard JEDEC SPD byte offsets and
  validates the standard JEDEC CRC-16. Same offsets/formulas as GPL
  `UNVR-1.3.35-GPL/u-boot/drivers/ddr/fsl/ddr4_dimm_params.c` (`n_ranks` :163, `device_width`
  :173, `n_row_addr` :213, `n_col_addr` :214, `bank_addr_bits` :215, `bank_group_bits` :216,
  `caslat_x = b1<<7|b2<<15|b3<<23` :265). Reproduce from that, do not re-derive.
- **OPEN — output (`struct al_ddr_init_cfg`).** Every S2 output offset maps to a named field
  in `delroth-alpine_hal/ddr/include/al_hal_ddr_init.h` (`org`:165, `addrmap`= `al_hal_ddr.h`:656,
  `tmg`:453, `impedance_ctrl`:234, assembled :844). Consumed unchanged by
  `al_hal_ddr_init_alpine_v2.c` `al_ddr_init` :5552.
- **REVERSED GLUE (the only novel part).** (a) Ubiquiti's `0xAA/0xBB/0xCC` EEPROM record
  wrapper + `0xAA` pointer indirection; (b) the Annapurna SPD-field→cfg-field mapping and the
  freq-tier / impedance lookup tables (`DAT_*`). Recovered below and verified against the two
  open anchors.

**S2 struct build differs from the delroth header** in two ways (both established, §2b of
`ddr-config-reverse.md`): 1-byte enums (`-fshort-enums`), and an older `impedance_ctrl` with
no `hv_min`/`hv_max`. Field *names/order* still match; byte offsets in `impedance_ctrl` are
the packed 0x22-byte layout, not the delroth C offsets. `org` has no `cids` (dimms at +8).

---

## Orchestrator call chain

`FUN_f22003d8` @0xf22003d8 (`preboot-s2-decompiled.c:375`, "DDR bring-up orchestrator"),
called from main `FUN_f2200124`. Assembles `struct al_ddr_init_cfg` @ `0xf2205eb0` (from the
binary literal pool) and hands it to `al_ddr_init`. Scratch `dimm_params[]` @ `0xf2206004`:
`[0]=&cfg.org (cfg+0x10)`, `[1]=&cfg.tmg (cfg+0x6c)`.

| seq | line | call | fills |
|---|---|---|---|
| 1 | :402 | `FUN_f22018f4(nb,ctrl,phy,cfg)` = `al_ddr_cfg_init` | `cfg.ddr_cfg` (bases/rev) |
| 2 | :403 | `cfg.tmg.ref_clk_freq_mhz = ddr_pll_freq/1e6` | tmg+0x00 |
| 3 | :405 | `FUN_f2200628(dimm_params)` = `ddr_freq_change_according_to_spd` | drives 4→6 below |
| 3a | :562 | ↳ `FUN_f220093c(idx,&spd,&aux)` = SPD-get / `0xAA` reader | resolves SPD+aux location |
| 3b | :573 | ↳ `FUN_f22013e8(spd,tCK,dimm_params,&flag)` = `al_ddr_spd_parse` | CRC + DDR3/DDR4 dispatch |
| 3c | — | ↳↳ `FUN_f2201140` = `al_ddr4_spd_parse` | `cfg.org`, `cfg.tmg` timings, CL/CWL |
| 4 | :428 | `FUN_f2200816(dimm_params,&cfg.addrmap)` = addrmap builder | `cfg.addrmap` |
| 5 | :429 | `FUN_f2200a58(dimms,ranks,&cfg.impedance_ctrl)` = `set_dram_impedance_ctrl_from_eeprom` | `cfg.impedance_ctrl` |
| 6 | :430 | `FUN_f2200d10(module_voltage)` = DRAM-voltage GPIO (`0xBB`) | drives voltage rail |
| 7 | :455-481 | size math + speed-bin enum from running tCK | `cfg.tmg.ddr_freq`, ddr_size |
| 8 | :482-491 | `FUN_f2201a90(cfg)` = `al_ddr_init`, retried ≤1000× | trains DDR |

Size (`:439-452`): `ddr_size = ranks << (row+col+bank+bg) × bus_bytes`. Speed-bin enum
(`:456-481`, stored `cfg.tmg.ddr_freq` = cfg+0x70): running `tCK_ps = 1e12/ddr_pll_freq`
switch → `enum al_ddr_freq`.

---

## FUNCTIONS

| addr | name (role) | input | output | callers |
|---|---|---|---|---|
| 0xf22003d8 | orchestrator (assemble cfg, train) | globals | `al_ddr_init_cfg` @0xf2205eb0 | main 0xf2200124 |
| 0xf22044d0 | `i2c_eeprom_read(addr,off,len,dst)` — 2-byte offset when off>0xff | I²C | buf | all readers |
| 0xf22003b0 | record reader: 7 B @ base+0x00 (`0xAA`) | base | rec buf +0x00 | 0xf220093c |
| 0xf22003b8 | record reader: 3 B @ base+0x0b (`0xBB`) | base | rec buf +0x0b | 0xf2200d10 |
| 0xf22003c0 | record reader: 0x16 B @ base+0x0e (`0xCC`) | base | rec buf +0x0e | 0xf2200a58 |
| 0xf22003c8 | record reader: 2 B @ base+0x24 (`0xDD` UART) | base | rec buf +0x24 | UART init |
| 0xf220093c | `ddr_init_spd_get` / `0xAA` reader — resolve SPD+aux, scan 0x50→0x58 | rec buf | spd desc, `dimm_params[0/1]` | 0xf2200628 |
| 0xf2200628 | `ddr_freq_change_according_to_spd` — SPD parse + PLL retry ladder | dimm_params | cfg.org/tmg | orchestrator |
| 0xf22013e8 | `al_ddr_spd_parse` — CRC-16 gate + type dispatch | spd, tCK | dispatch | 0xf2200628 |
| 0xf2200dcc | CRC-16 XMODEM (init 0, poly 0x1021, MSB-first) | buf,len | u16 | 0xf22013e8 |
| 0xf2200f30 | `al_ddr3_spd_parse` (byte2==0x0b) | spd | org/tmg | 0xf22013e8 |
| 0xf2201140 | `al_ddr4_spd_parse` (byte2==0x0c) — **our path** | spd,tCK | org, tmg timings | 0xf22013e8 |
| 0xf2200ed8 | CL solver: `ceil(tAA/tCK)` → next set bit in CAS map | dimm,map,num,tCK | `tmg.cl` (+0x38) | ddr4 parse |
| 0xf2200e74 | CWL from tCK (DDR4) → 9/10/11/12/14/16 | dimm,tCK | `tmg.cwl` (+0x3c) | ddr4 parse |
| 0xf2200df8 | CWL from tCK (DDR3) | dimm,tCK | tmg.cwl | ddr3 parse |
| 0xf2200816 | addrmap builder — geometry → col/bank/bg/row/cs maps | dimm_params | `al_ddr_addrmap` | orchestrator |
| 0xf2200a58 | `set_dram_impedance_ctrl_from_eeprom` — defaults + `0xCC` override | dimms,ranks | `impedance_ctrl` | orchestrator |
| 0xf2200d10 | DRAM-voltage GPIO — `0xBB` pin, PL061 @0xfd887000 | module_voltage | GPIO | orchestrator |

---

## MAGIC-RECORDS (Ubiquiti EEPROM wrapper — REVERSED GLUE)

Record base = **0x400** default, or `DEV_INFO[0x0a]|DEV_INFO[0x0b]<<8` when `DEV_INFO[0]!=0`
(`FUN_f22003b0:275`). Device = `al_bootstrap.i2c_preload_addr` (straps), live = **0x57**.

| off | len | magic | reader@ | layout → fields |
|---|---|---|---|---|
| +0x00 | 7 | `0xAA` | 0xf220093c:799 | +1 `spd_i2c_addr`(0xff⇒strap); +2..3 `spd_off` u16 LE; +4 `aux_i2c_addr`(0⇒dimms=1); +5..6 `aux_off` u16 LE |
| +0x0b | 3 | `0xBB` | 0xf2200d10:1081 | +1 `gpio_pin`(0xff⇒disabled, else <0x38); +2 `polarity` |
| +0x0e | 0x16 | `0xCC` | 0xf2200a58:958 | +1 `dqs_sel`(==1 special); +2.. 20-byte impedance table (4 cases × 5 B) |
| +0x24 | 2 | `0xDD` | 0xf22003c8 | +1 UART divisor override |

`0xAA` parse (`FUN_f220093c:799-814`): magic `*p==0xAA`; if `spd_i2c_addr==0xff` use
`al_bootstrap.i2c_preload_addr`; SPD read = 0x100 B from `(spd_i2c_addr, spd_off)` on the SAME
device (:837); aux 1-byte probe at `(aux_i2c_addr, aux_off)` sets dimms 1/2 (:848). On I²C
failure the address is incremented and rescanned to 0x58 (:826-847), printing `SPD I2C
Address: %02x`.

Impedance-table index (`FUN_f2200a58:967-971`): `idx = (dimms-1)*10 + (ranks/dimms-1)*5`,
5 bytes per `(dimms, ranks-per-dimm)` case → `{odt, odt_dyn, dic, phy_odt, phy_rout}`.

---

## STRUCTS

### Input — SPD image (OPEN, JEDEC DDR4; offsets = `spd_off` + n)

Parsed by `FUN_f2201140`; identical offsets to GPL `ddr4_dimm_params.c`.

| off | field | S2 read (line) | formula |
|---|---|---|---|
| 0x02 | ddr_type | :1565 | 0x0b⇒DDR3, 0x0c⇒DDR4 |
| 0x03 | module_type | :1451 | `1<<(b3&0xf)`; mask 0x132⇒rdimm, mirror←b[0x88]; else b[0x83] |
| 0x04 | density/banks | :1470 | bank_bits=`((b&0x3f)>>4)+2`; bg_bits=`b>>6` (density code b[3:0] IGNORED) |
| 0x05 | addressing | :1467 | row=`((b&0x3f)>>3)+12`; col=`(b&7)+9` |
| 0x0c | organization | :1406,1430 | device=`b&7`(0/1/2⇒x4/x8/x16); ranks=`((b&0x3f)>>3)+1` |
| 0x0d | bus width | :1431 | data_width: `b&7` 2/3/1 ⇒ enum 0/1/2 (32/64/16-bit); ecc=`b&0x18` |
| 0x11 | timebase | :1473 | 0 ⇒ MTB=125 ps, FTB=1 ps |
| 0x12 / 0x7d | tCKAVGmin (MTB/FTB) | :1478 | fastest supported tCK |
| 0x14-0x17 | CAS latencies | :1484 | `map = b14|b15<<8|b16<<16` then `<<7` |
| 0x18 / 0x7b | tAAmin | :1487 | → CL solver |
| 0x19 / 0x7a | tRCDmin | :1497 | tmg+0x14 |
| 0x1a / 0x79 | tRPmin | :1503 | tmg+0x1c |
| 0x1b,0x1c | tRASmin | :1507 | `((b1b&0xf)<<8\|b1c)×MTB` |
| 0x1b,0x1d / 0x78 | tRCmin | :1508 | `((b1b&0xf0)<<4\|b1d)×MTB` |
| 0x1e-0x1f | tRFC1min (u16) | :1511 | tmg+0x20 |
| 0x20-0x21 | tRFC2min (u16) | :1512 | tmg+0x24 |
| 0x22-0x23 | tRFC4min (u16) | :1514 | tmg+0x28 |
| 0x24,0x25 | tFAWmin | :1516 | `((b24&0xf)<<8\|b25)×MTB` |
| 0x26 / 0x77 | tRRD_Smin | :1500 | tmg+0x48 |
| 0x27 / 0x76 | tRRD_Lmin | :1502 | tmg+0x18 |
| 0x28 / 0x75 | tCCD_Lmin | :1496 | tmg+0x44 |
| 0x7e-0x7f | CRC-16 block1 (over 0..0x7d) | :1560 | JEDEC CRC gate |
| 0xfe-0xff | CRC-16 block2 (over 0x80..0xfd) | :1563 | JEDEC CRC gate |

### Output — `al_ddr_init_cfg` @ cfg=0xf2205eb0 (offsets from binary literal pool)

`org`=cfg+0x10, `addrmap`=cfg+0x44, `tmg`=cfg+0x6c, `impedance_ctrl`=cfg+0xb8 (all confirmed:
`addrmap_out`=0xf2205ef4 ⇒ +0x44; `imped_out`=0xf2205f68 ⇒ +0xb8).

**org** (`struct al_ddr_init_cfg_org`, no `cids` in this build):

| off | field | S2 write |
|---|---|---|
| +0x00 | data_width (`enum al_ddr_data_width`) | :1448 |
| +0x04 | ranks | :1430 |
| +0x08 | dimms | 0xf2200628:578 |
| +0x0c | rdimm | :1457 |
| +0x10 | udimm_addr_mirroring | :1464 |
| +0x14 | ecc_is_supported | :1466 |
| +0x18 | ddr_type (0=DDR3,1=DDR4) | 0xf22013e8:1573 |
| +0x19 | ddr_device (`enum al_ddr_device`) | :1429 |
| +0x1c | (valid flag =1) | :1522 |

**addrmap** (`struct al_ddr_addrmap`, `al_hal_ddr.h:656`; NC=0xff), built by `FUN_f2200816`:

| off | field | size |
|---|---|---|
| +0x00 | col_b3_9_b11_13 | 10 |
| +0x0a | bank_b0_2 | 3 |
| +0x0d | bg_b0_1 | 2 |
| +0x0f | row_b0_17 | 18 |
| +0x21 | cs_b0_1 | 2 |
| +0x23 | cid_b0_1 / swap (`*(u32)+0x24=0`) | — |

**tmg** (`struct al_ddr_init_cfg_tmg`) — S2 offset ↔ named field is **1:1** with the header:

| tmg+off | field | S2 write |
|---|---|---|
| +0x00 | ref_clk_freq_mhz | orch :403 |
| +0x04 | ddr_freq (`enum al_ddr_freq`) | orch :481 |
| +0x08 | t_faw_ps | :1516 |
| +0x0c | t_ras_min_ps | :1507 |
| +0x10 | t_rc_ps | :1508 |
| +0x14 | t_rcd_ps | :1497 |
| +0x18 | t_rrd_ps (tRRD_L) | :1502 |
| +0x1c | t_rp_ps | :1503 |
| +0x20 | t_rfc_min_ps (tRFC1) | :1511 |
| +0x24 | t_rfc2_ps | :1512 |
| +0x28 | t_rfc4_ps | :1514 |
| +0x38 | cl | 0xf2200ed8:1273 |
| +0x3c | cwl | 0xf2200e74:1252 |
| +0x44 | t_ccd_ps | :1496 |
| +0x48 | t_rrd_s_ps | :1500 |

**impedance_ctrl** (`struct al_ddr_init_cfg_impedance_ctrl`, packed 0x22-byte
short-enum build; names from header, offsets from S2 `FUN_f2200a58`):

| off | field | source |
|---|---|---|
| +0x00 | dic | `0xCC`[2] via `dic` table |
| +0x01 | odt | `0xCC`[0] via `odt` table |
| +0x02 | odt_dyn | `0xCC`[1] via `odt_dyn` table |
| +0x03 | rtt_park | default 0 (never written) |
| +0x04 | host_initial_vref | hardcoded 0x28 (:931) |
| +0x05 | vrefdq | hardcoded 0x19 (:931) |
| +0x06-07 | phy_rout_pu[2] | hardcoded 0x0b0d (:932) |
| +0x08-09 | phy_rout_pd[2] | hardcoded 0x0d0d (:933) |
| +0x0a-0b | phy_pu_odt[2] | hardcoded 0x0707 (:934) |
| +0x0c-13 | wr_odt_map[4] (u16) | dimms default (:909/919) |
| +0x14-1b | rd_odt_map[4] (u16) | dimms default |
| +0x1c-1d | phy_rout[2] | `0xCC`[4] via `phy_rout` table (:1049) |
| +0x1e-1f | phy_odt[2] | `0xCC`[3] via `phy_odt` table (:1046) |
| +0x20 | dqs_res | `0xCC`.dqs_sel (:962) |
| +0x21 | dqsn_res | `0xCC`.dqs_sel |

`misc.training_en` = 1 (S2 always trains). Other `misc`/`perf` fields not set by S2 (defaults).

---

## LOOKUP-TABLES (recovered from the .bin — `key → enum`)

`odt`/`odt_dyn`/`dic` keys are **RZQ dividers** (RZQ=240 Ω); `phy_*` keys are **ohms**.
Every table matches the GPL HAL enums element-for-element.

| table | VA | key→enum pairs | enum |
|---|---|---|---|
| odt | 0xf2204c34 | 0→0,2→1,4→2,6→3,8→4,12→5,1→6,5→7,3→8,7→9 | `al_ddr_odt` DIS/RZQ2/4/6/8/12/1/5/3/7 |
| odt_dyn | 0xf2204c2a | 0→0,2→1,4→2,1→3,153(0x99)→4 | `al_ddr_odt_dyn` DIS/RZQ2/4/1/HI_Z |
| dic | 0xf2204c48 | 6→0,7→1,5→2 | `al_ddr_dic` RZQ6/7/5 |
| phy_odt | 0xf2204c4e | 200→0,133→1,100→2,77→3,66→4,56→5,50→6,44→7,40→8,36→9,33→10,30→11,28→12,26→13,25→14 | `al_ddr_phy_odt` (ohms) |
| phy_rout | 0xf2204c6c | 80→0,68→1,60→2,53→3,48→4,44→5,40→6,37→7,34→8,32→9,30→10 | `al_ddr_phy_rout` (ohms) |

Speed-bin (orch `:456-481`, running tCK ps → `enum al_ddr_freq`): 0x753⇒1066, 0x5dc⇒1333,
0x4e2⇒1600, 0x442/0x42f⇒1866, [0x3a9,0x42e)⇒2133, [0x341,0x3a9)⇒2400, else 1066.
CWL (`FUN_f2200e74`, tCK ps → nCK): ≥0x4e2⇒9, [0x42e,0x4e2)⇒10, [0x3a7,0x42e)⇒11,
[0x341,0x3a7)⇒12, [0x2ee,0x341)⇒14, [0x2a9,0x2ee)⇒16.

---

## Decoded ea16 result (live dump) + cross-check

Records at 0x400: `AA ff 40 04 00 00 00` · `BB ff 00` · `CC 01 04 00 07 38 22 …`.
SPD at 0x440: CRC block1 `0xfbbc` **OK**, block2 `0x0000` **OK** → genuine JEDEC DDR4 SPD.

| cfg field | decoded ea16 value | cross-check |
|---|---|---|
| org.ddr_type | DDR4 (byte2=0x0c) | ✅ |
| org.ddr_device | x16 (byte12&7=2) | ✅ K4A8G165WB x16 |
| org.data_width | 64-bit (byte13&7=3⇒enum1) | ✅ 4×x16 = 64-bit bus |
| org.ranks | 1 (byte12=0x02) | ✅ single rank |
| org.dimms | 1 (aux_i2c_addr=0) | ✅ soldered |
| org.rdimm / mirror / ecc | 0 / 0 / 0 (UDIMM) | ✅ no ECC in BOM |
| addrmap | 16 row / 10 col / 2 bank / 1 bg; col→sys7-13, bank→17-18, row→14-16,19-31 | ✅ 8 Gbit geometry |
| **ddr_size** | ranks<<(16+10+2+1=29) × 8 B = **4 GiB** | ✅ live `DRAM: 4 GiB` |
| tmg.t_rfc_min / t_rfc2 / t_rfc4 | 350 / 260 / 160 ns | ✅ JEDEC 8 Gbit exactly |
| tmg.t_ras / t_rc | 34.0 / 47.92 ns | ✅ DDR4-1866 |
| tmg.t_rcd / t_rp | 13.92 / 13.92 ns | ✅ CL13@1866 |
| tmg.t_faw / t_rrd_s / t_rrd_l / t_ccd | 30 / 5.3 / 6.4 / 5.355 ns | ✅ DDR4-1866 |
| tmg.cl / cwl | 13 / 10 (@tCK 1071) | ✅ supported CL set {10-14} |
| **tmg.ddr_freq** | **AL_DDR_FREQ_1866** (SPD tCKmin=1071 ps) | ⚠ **contradicts prior "2400 expected"** |
| impedance.dic | AL_DDR_DIC_RZQ7 (34 Ω) | via `0xCC`[2]=7 |
| impedance.odt (RTT_NOM) | AL_DDR_ODT_RZQ4 (60 Ω) | via `0xCC`[0]=4 |
| impedance.odt_dyn (RTT_WR) | AL_DDR_ODT_DYN_DIS | via `0xCC`[1]=0 |
| impedance.phy_rout[2] | 34 Ω (enum 8) | via `0xCC`[4]=0x22 |
| impedance.phy_odt[2] | 56 Ω (enum 5) | via `0xCC`[3]=0x38 |
| impedance.host_initial_vref / vrefdq | 40 / 25 | stage2 hardcoded |
| impedance.phy_rout_pu/pd / phy_pu_odt | {13,11}/{13,13}/{7,7} | stage2 hardcoded |
| impedance.wr/rd_odt_map | {1,2,0,0} / {0,0,0,0} | dimms=1 default |
| impedance.dqs_res / dqsn_res | PULL_DOWN_500 / PULL_UP_500 | `0xCC`.dqs_sel=1 |
| DRAM-voltage GPIO (`0xBB`) | pin 0xff ⇒ disabled | ✅ DDR4 fixed 1.2 V |

**CONTRADICTION flagged:** the SPD encodes **DDR4-1866** (tCKAVGmin=1071 ps, CAS map tops out
at CL14). The K4A8G165WB-**BCRC** marking is a 2400 bin, but this SPD **caps the part at
1866** — a 2400 request (tCK 833) fails the `param_2+1 < tCKmin` gate (`FUN_f2201140:1480`)
and the loader downshifts the NB PLL. Prior `ddr-config-reverse.md` §2a "2400 expected" is
**wrong**; ea16 runs ≤1866. Running freq itself = `al_bootstrap.ddr_pll_freq` (live read still
open) but is SPD-bounded to 1866.

No other value contradicts the known 4 GiB / K4A8G165WB (8 Gbit x16 DDR4) facts.

---

## Portable C — reversed glue only (open pieces reused by reference)

Drop-in for the SPL. `jedec_ddr4_spd_parse()` and `struct al_ddr_init_cfg` are the OPEN
pieces (use `ddr4_dimm_params.c` + `al_hal_ddr_init.h` verbatim); only the wrapper +
lookup-glue below is reversed. 1:1 with the decompile.

```c
/* ---- Ubiquiti EEPROM record wrapper (REVERSED, FUN_f220093c/d10/a58) ---- */
struct ubnt_aa_rec {          /* 7 B @ base+0x00, magic 0xAA */
    uint8_t  magic;           /* 0xAA */
    uint8_t  spd_i2c_addr;    /* 0xff => al_bootstrap.i2c_preload_addr */
    uint16_t spd_off_le;      /* SPD image offset, same device */
    uint8_t  aux_i2c_addr;    /* 0 => no probe => dimms=1 */
    uint16_t aux_off_le;      /* 1-byte probe => dimms=2 if it ACKs */
} __attribute__((packed));
struct ubnt_bb_rec { uint8_t magic /*0xBB*/, gpio_pin /*0xff=off,<0x38*/, polarity; } __attribute__((packed));
struct ubnt_cc_rec { uint8_t magic /*0xCC*/, dqs_sel; uint8_t table[20]; } __attribute__((packed));

/* CRC-16 XMODEM, FUN_f2200dcc @0xf2200dcc:1132 */
static uint16_t jedec_crc16(const uint8_t *p, int n) {
    uint16_t crc = 0;
    for (int i = 0; i < n; i++) {
        crc ^= (uint16_t)p[i] << 8;
        for (int b = 0; b < 8; b++)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}

/* Impedance decode tables recovered from s2 .bin (key -> enum) */
static const uint8_t T_ODT[10][2]     = {{0,0},{2,1},{4,2},{6,3},{8,4},{12,5},{1,6},{5,7},{3,8},{7,9}};
static const uint8_t T_ODT_DYN[5][2]  = {{0,0},{2,1},{4,2},{1,3},{0x99,4}};
static const uint8_t T_DIC[3][2]      = {{6,0},{7,1},{5,2}};
static const uint8_t T_PHY_ODT[15][2] = {{200,0},{133,1},{100,2},{77,3},{66,4},{56,5},{50,6},
                                         {44,7},{40,8},{36,9},{33,10},{30,11},{28,12},{26,13},{25,14}};
static const uint8_t T_PHY_ROUT[11][2]= {{80,0},{68,1},{60,2},{53,3},{48,4},{44,5},{40,6},{37,7},{34,8},{32,9},{30,10}};
static int tbl_lookup(const uint8_t t[][2], int n, uint8_t key) {
    for (int i = 0; i < n; i++) if (t[i][0] == key) return t[i][1];
    return -1;   /* caller: "invalid ... from EEPROM" */
}

/* set_dram_impedance_ctrl_from_eeprom, FUN_f2200a58 @0xf2200a58:864 */
void ubnt_impedance_from_eeprom(unsigned dimms, unsigned ranks,
                                const struct ubnt_cc_rec *cc,
                                struct al_ddr_init_cfg_impedance_ctrl *z) {
    /* defaults (dimms selects, NOT ranks) */
    z->dic = AL_DDR_DIC_RZQ7;
    z->odt = (dimms < 2) ? AL_DDR_ODT_RZQ6  : AL_DDR_ODT_RZQ12;
    z->odt_dyn = (dimms < 2) ? AL_DDR_ODT_DYN_DIS : AL_DDR_ODT_DYN_RZQ4;
    z->host_initial_vref = 0x28; z->vrefdq = 0x19;
    z->phy_rout_pu[0]=13; z->phy_rout_pu[1]=11;
    z->phy_rout_pd[0]=13; z->phy_rout_pd[1]=13;
    z->phy_pu_odt[0]=7;   z->phy_pu_odt[1]=7;
    if (dimms < 2) { static const uint16_t w[4]={1,2,0,0},r[4]={0,0,0,0};
        memcpy(z->wr_odt_map,w,sizeof w); memcpy(z->rd_odt_map,r,sizeof r); }
    else           { static const uint16_t w[4]={5,10,5,10},r[4]={4,8,1,2};
        memcpy(z->wr_odt_map,w,sizeof w); memcpy(z->rd_odt_map,r,sizeof r); }
    z->phy_rout[0]=z->phy_rout[1]=10;   /* 30 ohm */
    z->phy_odt[0]=z->phy_odt[1]=8;      /* 40 ohm */
    z->dqs_res = AL_DDR_DQS_RES_PULL_DOWN_500OHM;   /* default u16=1 */

    if (!cc || cc->magic != 0xCC) return;           /* no override */
    if (cc->dqs_sel == 1) { z->dqs_res = AL_DDR_DQS_RES_PULL_DOWN_500OHM;
                            z->dqsn_res = AL_DDR_DQSN_RES_PULL_UP_500OHM; }
    else                  { z->dqs_res = AL_DDR_DQS_RES_PULL_UP_611OHM;
                            z->dqsn_res = AL_DDR_DQSN_RES_PULL_UP_458OHM; }
    const uint8_t *s = &cc->table[(dimms-1)*10 + (ranks/dimms-1)*5];  /* 5 B case */
    z->odt     = tbl_lookup(T_ODT,     10, s[0]);
    z->odt_dyn = tbl_lookup(T_ODT_DYN,  5, s[1]);
    z->dic     = tbl_lookup(T_DIC,      3, s[2]);
    z->phy_odt[0] = z->phy_odt[1]   = tbl_lookup(T_PHY_ODT, 15, s[3]);
    z->phy_rout[0]= z->phy_rout[1]  = tbl_lookup(T_PHY_ROUT,11, s[4]);
}

/* addrmap builder, FUN_f2200816 @0xf2200816:653 (DDR4 path) */
void ubnt_build_addrmap(enum al_ddr_data_width dw, int is_ddr4,
                        int row, int col, int bank_bits, int bg_bits, int ranks,
                        struct al_ddr_addrmap *am) {
    memset(am, AL_DDR_ADDRMAP_NC, sizeof *am);      /* 0xff */
    int iv = (dw == AL_DDR_DATA_WIDTH_64_BITS) ? 6 : (dw == AL_DDR_DATA_WIDTH_32_BITS) ? 5 : 4;
    int u5;
    if (is_ddr4) {
        if (iv != 6) { am->col_b3_9_b11_13[0] = iv; if (iv != 5) am->col_b3_9_b11_13[1] = 5; }
        u5 = 9 - iv;
        if (bg_bits) { am->bg_b0_1[0] = 6; if (bg_bits > 1) am->bg_b0_1[1] = 7; }
        iv = bg_bits + 6;
    } else u5 = 3;
    for (; u5 < col; u5++) am->col_b3_9_b11_13[u5-3] = iv++;
    int last = iv;                                  /* cVar1 pre-row */
    /* cs map: ranks 1 -> NC/NC (default) ; ranks 2/4 handled in full port */
    int r = iv;                                     /* row fill part 1: while r<=0x10 */
    while (r <= 0x10) { am->row_b0_17[r - iv] = r; r++; }
    last = r;                                       /* =0x11 */
    for (int j = 0; j < bank_bits; j++) am->bank_b0_2[j] = last + j;
    int u5b = r - iv;                               /* =3 */
    int base = last + bank_bits - u5b;
    for (; u5b < row; u5b++) am->row_b0_17[u5b] = base + u5b;
}
```

`al_ddr4_spd_parse` / `al_ddr_spd_parse` are the OPEN JEDEC path — reuse `ddr4_dimm_params.c`
(add: MTB=125/FTB=1 scaling, `caslat<<7` CL solve, tRFC2/tRFC4). The S2's timing writes land
on the named `tmg`/`org` fields per the STRUCTS table; the speed-bin + size math is in the
orchestrator (glue, small — port directly from `:439-481`).

**Portability status:** wrapper + tables + addrmap are complete and drop-in. The SPD parse is
open (cited). Remaining live inputs before the SPL trains hardware: `al_bootstrap.ddr_pll_freq`
(sets running tCK / `ddr_freq` / CL) and confirmation that the record base is 0x400
(DEV_INFO[0]==0). All static config for ea16 is resolved above.
