# ddr-config-reverse — working notes (raw)

Session 2026-08-17. Raw offsets/dumps behind `docs/ddr-config-reverse.md`.
Artifacts used: `tmp/sections/01-uboot.bin` (5.1.25 fw U-Boot section, 1,395,740 B),
`docs/nor-reference/s2-loader-stage2_v2.22.3-25044B.bin`,
`docs/nor-reference/preboot-s2-{decompiled.c,disassembly.asm}`,
`docs/nor-reference/preboot-alboot-decompiled.c`, `tmp/dtb-current/dtb0*.dts`.

## Container / TOC (5.1.25)

`scripts/parse-al-toc.py tmp/sections/01-uboot.bin` → TOC @0x80000, 9 entries:

```
[0] preboot  type 0x0000000d off 0x000000 size 0x080000
[1] dt       type 0x00000002 off 0x081000 size 0x007000  payload 0x66f6
[2] dt_pro   type 0x10000002 off 0x088000 size 0x008000  payload 0x6b65
[3] dt_ai    type 0x20000002 off 0x090000 size 0x008000  payload 0x6844
[4] dt_bt    type 0x30000002 off 0x098000 size 0x00a000  payload 0x674e
[5] dt_hd    type 0x40000002 off 0x0a2000 size 0x00a000  payload 0x65d9
[6] uboot    type 0x00000005 off 0x0ac000 size 0x114000  payload 0xa8bd0 load/entry 0x1100000
[7] uboot-en type 0x00000007 off 0x1c0000 size 0x010000
[8] uboot-re type 0x00000008 off 0x1d0000 size 0x230000
```

NOR mtd00 (post-5.1.25 dump) has 8 entries — **no `dt_hd`**, uboot at 0x0a1000.

### preboot region data extents (0x100 granularity, non-0xff)

```
000000..006200   S2 / stage2_loader (exact 0x0..0x61d4)
020000..020100   al_boot img header (magic 0x000b9ec7, +0x28 = 0x6a6b4)
021000..06b700   u32 length @0x21000 = 0x0004a6b0, then payload
```

- `0x21000 + 4 + 0x4a6b0 = 0x6b6b4`; last non-0xff byte 0x6b6b7 → 4 trailing bytes
  `a3 34 a4 03` (checksum).
- Header field @+0x28 (0x6a6b4) = 0x4a6b4 + 0x20000; it is NOT the payload length.

## S2 = `stage2_loader v2.22.3` — it does the DDR init

Strings (`strings -a -t x` on the carved S2), file offsets:

```
4784 al_flash_toc_search failed!
4892 stage2_loader v2.22.3
48a9 NB PLL
48b0 eeprom_read failed!
48c5 invalid ref clock = %d!
48de al_ddr_init(%d) succeeded after %d attempts!
490c ddr_init_ch(%d) failed!
4925 al_ddr_init(%d) failed!
493e pll_fixup
4948 ddr_freq_change_according_to_spd
4969 ddr_init_dimm_params_get
4982 %s: ddr_init_spd_get failed!
49a0 al_ddr_spd_parse failed!
49c2 %s: unsupported min_ck_ps = %u!
49e3 %s: not supported sdr_pll_freq %u
4a74 %s: ddr_freq_change_according_to_spd failed!
4aa2 invalid early init info!
4abc ddr_init_spd_read_early_init_paramsa failed!
4aea SPD I2C Address: %02x
4b01 no valid dimms!
4b12 ddr_init_read_spd failed!
4b2d SPD scan failed!
4b3f set_dram_impedance_ctrl_from_eeprom
4b63 %s: invalid DRAM ODT_DYN from EEPROM (%d)
4b8e %s: invalid DRAM ROUT from EEPROM (%d)
4bb6 %s: invalid PHY ROUT from EEPROM (%d)
4bdd %s: invalid PHY ODT from EEPROM (%d)
4c03 %s: invalid DRAM ODT from EEPROM (%d)
4c82 dram voltage gpio pin is out of range!
4caa spd dram voltage support is not properly set
4cd7 spd dram voltage supported 1.25[V] is invalid
4d05 al_ddr_spd_compute_cas_write_latency_ddr3
4d2f al_ddr_spd_compute_cas_write_latency_ddr4
4d59 al_ddr_spd_compute_cas_latency
4d78 al_ddr3_spd_parse
4d8a al_ddr4_spd_parse
4d9c al_ddr_spd_parse
4dad %s: 4 bit chips not supported
4dcc %s: invalid DDR data width field (0x%02x)
4e13 DDR clock is faster than the DIMM can support.
4ea6 %s: 32 bit chips not supported
4f4e %s: couldn't find supported CAS latency!
5033 al_ddr_mode_register_set
5111 al_ddr_ctrl_wait_for_normal_operating_mode
513c al_ddr_phy_vt_calc_disable
5157 al_ddr_ctrl_dfi_init
516c al_ddr_phy_wait_for_init_done
51c5..52fa  invld CL/t_wr/DIC/ODT/AL/CWL/ODT_DYN/CRC_DM_WCL/PL/RTT_PARK/CCD_L/PHY ROUT/PHY ODT
53a3..54eb  Write Leveling Error / Read DQS Gate Error / Read Leveling Error /
            Write Level Adjust Error / Static Read / Read Deskew / Write Deskew /
            Read Eye Centering / Write Eye Centering / DRAM Vref / Host Vref
5519 PASS
5e14 al_i2c_perform_write ...
```

Function map (S2, VA = 0xf2200000 + file offset):

| VA | span | role |
|---|---|---|
| 0xf2200124 | main | bootstrap/PLL/UART, then DDR, then TOC/flash load of al_boot |
| 0xf22002c8 | | `al_bootstrap_parse(0xfd8a8000, &bs)` + NB PLL init (0xfd860c00) |
| 0xf2200360 | | `eeprom_read(off, len)` → buf 0xf220606c + off |
| 0xf22003b0/b8/c0/c8 | | reads of 7B@0x00, 3B@0x0b, 0x16B@0x0e, 2B@0x24 |
| 0xf22003d8 | | **DDR bring-up orchestrator** |
| 0xf220061c | | `cfg+0x104 = 0xf220602c` (static table, unidentified) |
| 0xf2200628 | 0x1ec | `ddr_freq_change_according_to_spd` — SPD get + retry ladder |
| 0xf2200816 | 0x126 | addrmap builder (col/bank/bg/row/cs index tables) |
| 0xf220093c | 0x11c | `ddr_init_spd_get` — early-init record + SPD scan 0x50..0x58 |
| 0xf2200a58 | 0x2b8 | **`set_dram_impedance_ctrl_from_eeprom`** |
| 0xf2200d10 | 0xbc | DRAM voltage GPIO |
| 0xf2200dcc | | JEDEC CRC-16 |
| 0xf2200df8/e74/ed8 | | CWL DDR3 / CWL DDR4 / CAS latency |
| 0xf2200f30 | 0x210 | `al_ddr3_spd_parse` |
| 0xf2201140 | 0x2a8 | `al_ddr4_spd_parse` |
| 0xf22013e8 | 0xe0 | `al_ddr_spd_parse` — CRC check + DDR3/DDR4 dispatch |
| 0xf22018f4 | | `al_ddr_cfg_init(nb, ctrl, phy, cfg)` |
| 0xf2201a90 | **0x20c8** | **`al_ddr_init(cfg)`** (8,392 B, everything inlined) |
| 0xf22044b8 | | `*(u32*)0xfbff4150 = 0x31415926` |
| 0xf22044c8 | | returns 0xfbff4150 |
| 0xf22044d0 | | `i2c_eeprom_read(addr, off, len, dst)`; 2-byte offset if off > 0xff |
| 0xf2204694 | | `al_i2c_init(&if, cfg@0xf2205df8, regs 0xfd880000)` |

Literal pool values (resolved from the binary):

```
f22005dc -> f2205eb0   struct al_ddr_init_cfg (bss)
f22005e0 -> f22061a4   struct al_bootstrap  (bss)
f22005e4 -> f0080000   AL_NB_DDR_CTL_BASE
f22005e8 -> f0088000   AL_NB_DDR_PHY_BASE
f22005ec -> f0070000   AL_NB_SERVICE_BASE
f22005f0 -> 000f4240   1e6
f22005f4 -> f2206004   ddr desc {&org, &tmg, col, bg, bank, cid, .., min_ck_ps, volt}
f22005f8 -> fbff4100   SRAM_DEV_INFO_ADDRESS
f22005fc -> fbff410b   DEV_INFO early-init addr MSB (LSB at 0xfbff410a)
f2200600 -> f2205ef4   cfg+0x44  (addrmap)
f2200604 -> f2205f68   cfg+0xb8  (impedance_ctrl)
f2200608 -> 3b9aca00   1e9
f2200624 -> f220602c   static table -> cfg+0x104
f22007dc -> 37a18955   933,333,333 Hz
f22007e4 -> 27bc86aa   666,666,666 Hz
f22007e8 -> 1fca0555   533,333,333 Hz
f22007f4 -> 2faf0800   800,000,000 Hz
f22007f8 -> fd860c00   NB PLL regs
f220080c -> f22061a4   bootstrap
f2200810 -> 3b9aca00   1e9
f2200a2c -> f2206098   i2c scan counter (init 0x50)
f2200a30 -> f220619c   spd descriptor {addrA, _, offA16, addrB, _, offB16}
f2200a40 -> f22061a4   bootstrap (i2c_preload_addr at +0x27)
f2200a44 -> f220609c   SPD buffer (0x100 B)
f22003a4/f22003d4 -> f220606c   early-init record buffer
f2204510 -> fd880000   AL_I2C_PLD_BASE
f2204514 -> f2205df8   i2c config
f2200dc0 -> fd897400 ; f2200dc4 -> fd897000   GPIO regs for pins 0x28..0x2f
```

i2c config @0xf2205df8 (28 B): `01 00 00 00 | 00*16 | e8 03 00 00 | 00 02 00 00`
→ mode=1, no timing override, +0x14=1000, +0x18=0, +0x19=2 → IC_CON = 0x63 (master,
standard 100 kHz, restart en). Matches DT `i2c-pld ... clock-frequency = <0x186a0>`.

## al_ddr_init_cfg observed offsets (this build)

| off | member | evidence |
|---|---|---|
| 0x00 | `ddr_cfg` (nb, ctrl, phy, rev u8) | `al_ddr_cfg_init` writes [0..3] |
| 0x10 | `org` | desc[0] = 0xf2205ec0 = cfg+0x10 |
| 0x10+0x00 | `org.data_width` | `*org` → 1=64b, 0=32b, else 16b |
| 0x10+0x04 | `org.ranks` | `((spd[12]>>3)&7)+1` |
| 0x10+0x08 | `org.dimms` | set to 1/2 by SPD scan; scales ranks |
| 0x10+0x0c | `org.rdimm` | from SPD module_type mask |
| 0x10+0x10 | `org.udimm_addr_mirroring` | bit0 of spd[0x88] |
| 0x10+0x14 | `org.ecc_is_supported` | `spd[13] & 0x18` |
| 0x10+0x18 | `org.ddr_type` | 0=DDR3 (spd[2]==0x0b), 1=DDR4 (0x0c) |
| 0x44 | `addrmap` | DAT_f2200600 |
| 0x6c | `tmg` | desc[1] = 0xf2205f1c |
| 0x6c+0x00 | `tmg.ref_clk_freq_mhz` | `bs.ddr_pll_freq / 1e6` |
| 0x6c+0x04 | `tmg.ddr_freq` | tCK-ps → enum, written as byte at cfg+0x70 |
| 0xb8 | `impedance_ctrl` | DAT_f2200604 |
| 0xdc | (`misc`?) `= (cfg+0x24 != 0)` | drives the 32 MiB clear loop |
| 0x104 | pointer = 0xf220602c | unidentified |

`org` has **no `cids`** field in this build (published header has it between `ranks`
and `dimms`).

### impedance_ctrl byte layout (cfg+0xb8), 1-byte enums, no hv_min/hv_max

```
+0x00 dic            +0x01 odt          +0x02 odt_dyn      +0x03 rtt_park
+0x04 host_initial_vref = 0x28 (40)     +0x05 vrefdq = 0x19 (25)
+0x06..07 phy_rout_pu[2] = 0x0d,0x0b    +0x08..09 phy_rout_pd[2] = 0x0d,0x0d
+0x0a..0b phy_pu_odt[2] = 0x07,0x07
+0x0c..13 wr_odt_map[4] (u16)           +0x14..1b rd_odt_map[4] (u16)
+0x1c..1d phy_rout[2] = 0x0a,0x0a       +0x1e..1f phy_odt[2] = 0x08,0x08
+0x20 dqs_res        +0x21 dqsn_res
```

dimms==1: dic=1, odt=3, odt_dyn=0, wr_odt_map={1,2,0,0}, rd_odt_map={0,0,0,0}
dimms>=2: dic=1, odt=5, odt_dyn=2, wr_odt_map={5,10,5,10}, rd_odt_map={4,8,1,2}

## EEPROM decode tables in S2 (value → HAL enum)

```
DRAM ODT   @f2204c34 (10) [(0,0),(2,1),(4,2),(6,3),(8,4),(12,5),(1,6),(5,7),(3,8),(7,9)]
ODT_DYN    @f2204c2a (5)  [(0,0),(2,1),(4,2),(1,3),(153,4)]
DRAM ROUT  @f2204c48 (3)  [(6,0),(7,1),(5,2)]
PHY ODT    @f2204c4e (15) [(200,0),(133,1),(100,2),(77,3),(66,4),(56,5),(50,6),(44,7),
                           (40,8),(36,9),(33,10),(30,11),(28,12),(26,13),(25,14)]
PHY ROUT   @f2204c6c (11) [(80,0),(68,1),(60,2),(53,3),(48,4),(44,5),(40,6),(37,7),
                           (34,8),(32,9),(30,10)]
```

All five match `enum al_ddr_odt` / `al_ddr_odt_dyn` / `al_ddr_dic` /
`al_ddr_phy_odt` / `al_ddr_phy_rout` in
`urnvr-kernel-4.19.152/drivers/soc/alpine/HAL/ddr/include/al_hal_ddr_init.h`
and `al_hal_ddr.h` exactly. EEPROM stores RZQ dividers (DRAM side) and ohms (PHY side).

## tCK-ps → al_ddr_freq (S2 FUN_f22003d8)

`tck_ps = 1e9 / (bs.ddr_pll_freq / 1000)`

| tck_ps | enum | rate |
|---|---|---|
| 0x753 (1875) or unmatched | 1 | 1066 |
| 0x5dc (1500) | 2 | 1333 |
| 0x4e2 (1250) | 3 | 1600 |
| 0x442 (1090) / 0x42f (1071) | 4 | 1866 |
| 0x3a9..0x42d (937..1069) | 5 | 2133 |
| 0x341..0x3a8 (833..936) | 6 | 2400 |

## NOR EEPROM partition (mtd04) — identity only, NOT the DDR EEPROM

```
0000: 74acb941a811 76acb941a811 ea16 0777 000b101d      MACs, sysid, hwrev
8000: "UBNT" ... 0777 ea16 000b101d 74acb941a811        UBNT identity block
9000..c000  RSA/caps region        d000  TlvInfo (113-02832-29, 20200524)
e000..e330  ssh-rsa private key region
```

## Boot log / live cross-checks already on record

- U-Boot boot print `SPD I2C Address: 57` (docs/hardware.md:26) = S2 string @0xf2204aea.
- `DRAM: 4 GiB`; live.dts banks 0x0 +3 GiB, 0x200000000 +1 GiB.
- DT `i2c-pld` = `snps,designware-i2c` @0xfd880000 = the S2's bus, Linux `i2c-0`.
  PCA9546 mux at 0x71 hangs off it (chan 0 = s35390a@0x30, chan 3 = adt7475@0x2e).
