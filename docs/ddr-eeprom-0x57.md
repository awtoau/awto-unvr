# DDR config EEPROM 0x57 — full byte map + the from-scratch DDR init recipe (#67)

The 8 KiB 24C64 at I²C `0x57` (Linux `i2c-0`, `i2c-pld` @`0xfd880000`). Every byte of the
device is accounted for. This is the "what" for a from-scratch DDR bring-up; the "how" is
the GPLv2 `al_hal_ddr_init_alpine_v2.c` already in the kernel tree
([uboot-ddr-port.md](uboot-ddr-port.md)).

Companion docs: [ddr-config-reverse.md](ddr-config-reverse.md) (how the format was
reversed), [ddr-s2-parser-analysis.md](ddr-s2-parser-analysis.md) (the S2 parser,
byte-exact), [overclock-and-caps.md](overclock-and-caps.md) (the RSA caps blob).

Dump: `docs/nor-reference/ddr-config-eeprom-0x57-8k.bin`
Decoders: `scripts/decode-ddr-records.py` (0x400+), `scripts/decode-eeprom-preload.py` (0x000-0x2e3)
On-box reads: `scripts/read-ddr-bootstrap.py` (PBS strap), `scripts/read-nb-pll.py` (the running DDR PLL)

---

## 1. Dump provenance — two independent paths, byte-identical

| date | path | sha256 |
|---|---|---|
| 2026-08-17 | `scripts/read-ddr-spd.py`, 16-bit `i2ctransfer` over the serial console, 100 kHz | `743408d2…6772fe` |
| 2026-09-04 | `at24` driver → `/sys/bus/i2c/devices/0-0057/eeprom`, 400 kHz bus (post-#86) | `743408d2…6772fe` |

`cmp` clean over all 8192 B. The i2c fix (#86, `snps,no-enable-abort`) and the 4x clock
change did not perturb the read, and the driver path confirms the hand-rolled one.

`dmesg`: `at24 0-0057: 8192 byte 24c64 EEPROM, writable, 32 bytes/write`.

---

## 2. Byte map — 8192 / 8192 B attributed

| range | size | content |
|---|---|---|
| `0x0000-0x000b` | 12 | opening encapsulation marker `36 1c` ×6 |
| `0x000c-0x00d7` | 204 | **17 preload register-write records** (§3) |
| `0x00d8-0x00e3` | 12 | closing encapsulation marker (same pattern) |
| `0x00e4-0x01e3` | 256 | **RSA-2048 modulus** (§6) |
| `0x01e4-0x02e3` | 256 | **RSA-2048 signature** (§6) |
| `0x02e4-0x03ff` | 284 | zero fill |
| `0x0400-0x0406` | 7 | `0xAA` pointer record (§4) |
| `0x0407-0x040a` | 4 | unused (zero) |
| `0x040b-0x040d` | 3 | `0xBB` DRAM-voltage GPIO record — **disabled** |
| `0x040e-0x0423` | 22 | `0xCC` impedance override record (§5) |
| `0x0424-0x043f` | 28 | `0xDD` UART-divisor slot — **absent** (`0xff`/`0x00`) |
| `0x0440-0x053f` | 256 | **JEDEC DDR4 SPD image** (§4) |
| `0x0540-0x05ff` | 192 | zero fill |
| `0x0600-0x1fff` | 6656 | erased (`0xff`) — unused |

Only the first 1.5 KiB is written. Nothing unattributed.

**`0x57` holds DDR + caps config only.** Board identity (sysid, MAC, P/N, host key) lives in
SPI-NOR mtd04, a different device — settles dt-gaps M1.

---

## 3. Preload region `0x000-0x02e3` — previously unattributed, now decoded

Format from al_boot `eeprom_preload_parser` (`FUN_01003230`,
`docs/nor-reference/preboot-alboot-decompiled.c:1857`): walk the device in 12-byte records
with a 2-byte offset; compare each to the 12-byte marker; the closing marker's end is the
RSA blob offset. (Ghidra renders the literal 12 as `AL_I2C_TAR_10BIT_ADDR_SHIFT` — it is a
plain integer, not an I²C field.)

Record = `{value:LE32, addr:LE32, pad:LE32}`.

| # | addr | value | target |
|---|---|---|---|
| 0 | `0x7f896100` | `0x20000000` | **?? not valid MMIO** — see below |
| 1 | `0xfd8b400c` | `0x00000007` | SGPO +0x0c |
| 2 | `0xfd8a8118` | `0x1c000000` | PBS regfile +0x118 (`boot_strap` is +0x110) |
| 3 | `0xfd8a811c` | `0x1c000000` | PBS regfile +0x11c |
| 4 | `0xf00700a4` | `0x00007171` | NB service +0xa4 |
| 5 | `0xfd860c04` | `0x1d4cc00d` | **NB PLL** +0x04 |
| 6 | `0xfd860c00` | `0x8000001b` | **NB PLL** +0x00 — the DDR clock PLL |
| 7 | `0xfd860c20` | `0x000c5003` | NB PLL +0x20 |
| 8 | `0xfd860c20` | `0x000c7003` | NB PLL +0x20 (second write, bit 13 set) |
| 9-16 | `0xfbff4100`..`0x411c` | — | **SRAM DEV_INFO** image (§3a) |

Open items:
- **Record 0's address is not decoded.** `0x7f896100` is not a valid MMIO address; with bit
  31 set it would be `0xff896100`, and masked to `0xfd`-space it is `0xfd896100` — OTP base
  `0xfd896000` + 0x100. Not confirmed; the record may carry a flag in the top bit, or a
  different encoding. `pad = 0xf5` here and `0` in every other record, so record 0 is
  structurally different.
- **NB PLL register field layout is not decoded** (no spec in the GPL HAL). The values are
  captured verbatim above; that is enough to replay them, not to re-derive them.

**Who applies these writes:** not al_boot — `eeprom_preload_parser` only *scans* the records
to locate the closing marker, and never writes the targets. The targets (NB PLL, PBS regfile,
SRAM DEV_INFO) must all be set before al_boot runs, and DEV_INFO must exist before the S2
reads its record base from it, so the applier is the **mask BootROM**. Not provable from the
carved NOR blobs — the BootROM is not in NOR.

### 3a. SRAM DEV_INFO image — confirms the record base by data

Reconstructed from records 9-16, at `SRAM_DEV_INFO_ADDRESS 0xfbff4100`:

```
24 a3 39 ff fd fd ff bf fb ff 00 04 00 00 00 00 00 00 00 00 76 32 73 69 6c 2d 33 39 2d 72 63 31
                                                             v  2  s  i  l  -  3  9  -  r  c  1
```

Field names from GPL U-Boot `board/annapurna-labs/alpine_ubnt/dev_info_layout.h`:

| offset | field | value |
|---|---|---|
| `0x00` | `DEV_INFO_DEV_ID_0` | `0x24` (nonzero) |
| `0x0a`/`0x0b` | `DEV_INFO_EARLY_INIT_ADDR_LSB/MSB` | `0x00`/`0x04` → **`0x0400`** |
| `0x0c` | `DEV_INFO_RSVD` | `0x00` (xmodem_load=0, spi_load=0) |
| `0x14-0x1f` | (ASCII, not in the GPL header) | `v2sil-39-rc1` — silicon/board revision tag |

**The 0x400 record base was previously an assumption (the hardcoded default). It is now
confirmed by data:** `DEV_INFO[0] != 0`, so the S2 takes the base from
`DEV_INFO[0x0a] | DEV_INFO[0x0b]<<8` — which is `0x0400`. Both paths agree, but the
agreement is now observed rather than assumed.

---

## 4. SPD — JEDEC DDR4, both CRCs pass

At `spd_off = 0x0440` on the same device (`0xAA` record: `spd_i2c_addr = 0xff` ⇒ strap
fallback ⇒ `0x57`; `aux_i2c_addr = 0x00` ⇒ `dimms = 1`).

CRC gate: block1 over `0x00-0x7d` = `0xfbbc` (stored `0xfbbc`); block2 over `0x80-0xfd` =
`0x0000` (stored `0x0000`). Both **OK** — genuine JEDEC image.

**Gotcha:** SPD byte 4 bits 3:0 (density code) is **0**. A stock decoder that trusts it
reports 256 Mb. The Annapurna parser ignores it and derives density from row/col/bank/bg
geometry. Decode with `scripts/decode-ddr-records.py`, not `decode-dimms`.

### Organisation

| SPD | field | value |
|---|---|---|
| byte 2 = `0x0c` | `ddr_type` | DDR4 |
| byte 3 = `0x02` | `module_type` | UDIMM (⇒ `rdimm = 0`) |
| byte 12 = `0x02` | `ddr_device` / `ranks` | **x16** / **1 rank** |
| byte 13 = `0x03` | `data_width` / ECC | **64-bit** / **no ECC** |
| byte 4 = `0x40` | bank / bank-group bits | 2 banks bits (4 banks) / 1 bg bit (2 groups) |
| byte 5 = `0x21` | row / col bits | **16 row / 10 col** |
| byte 0x88 | `udimm_addr_mirroring` | 0 |

Derived density = 2^33 bits = **8 Gbit** per device. Matches the photo-confirmed
Samsung K4A8G165WB (`docs/chips/dram-ddr4.md`).

Size = `ranks(1) << (16+10+2+1 = 29) × 8 B` = **4 GiB** ✅ — matches live `DRAM: 4 GiB`,
`free -m` 3879 MiB, and the DT split (3 GiB @0 + 1 GiB @`0x200000000`, the MMIO hole).

### Timings (MTB 125 ps, FTB 1 ps)

| SPD bytes | param | ps |
|---|---|---|
| 0x12/0x7d | tCKAVGmin | 1071 (⇒ SPD caps the part at **1866** MT/s) |
| 0x14-0x17 | CAS support map | `0x000000f8` ⇒ CL {10,11,12,13,14} |
| 0x18/0x7b | tAAmin | 13920 |
| 0x19/0x7a | tRCDmin | 13920 |
| 0x1a/0x79 | tRPmin | 13920 |
| 0x1b/0x1c | tRASmin | 34000 |
| 0x1b/0x1d | tRCmin | 47920 |
| 0x1e/0x1f | tRFC1 | 350000 (8 Gbit JEDEC) |
| 0x20/0x21 | tRFC2 | 260000 |
| 0x22/0x23 | tRFC4 | 160000 |
| 0x24/0x25 | tFAWmin | 30000 |
| 0x26/0x77 | tRRD_S | 5300 |
| 0x27/0x76 | tRRD_L | 6400 |
| 0x28/0x75 | tCCD_L | 5355 |

The K4A8G165WB-**BCRC** marking is a 2400 bin, but **this SPD caps the part at 1866**.

---

## 5. Impedance — `0xCC` record present

`cc 01 | 04 00 07 38 22 | 04 00 07 38 22 | ff × 10`

`dqs_sel = 1` ⇒ `dqs_res = PULL_DOWN_500OHM`, `dqsn_res = PULL_UP_500OHM`.

20-byte table indexed `(dimms-1)*10 + (ranks/dimms-1)*5`; ours (1,1) → index 0 →
`04 00 07 38 22`. The EEPROM stores **human units** — RZQ dividers DRAM-side, ohms PHY-side —
decoded through five tables carved from the S2 blob that match
`enum al_ddr_odt`/`al_ddr_odt_dyn`/`al_ddr_dic`/`al_ddr_phy_odt`/`al_ddr_phy_rout` in the GPL
kernel HAL element-for-element.

| byte | field | EEPROM | decoded | enum |
|---|---|---|---|---|
| +0 | `odt` (RTT_NOM) | `0x04` | RZQ/4 = **60 Ω** | `AL_DDR_ODT_RZQ4` (2) |
| +1 | `odt_dyn` (RTT_WR) | `0x00` | **disabled** | `AL_DDR_ODT_DYN_DIS` (0) |
| +2 | `dic` | `0x07` | RZQ/7 = **34 Ω** | `AL_DDR_DIC_RZQ7` (1) |
| +3 | `phy_odt` | `0x38` | **56 Ω** | 5 |
| +4 | `phy_rout` | `0x22` | **34 Ω** | 8 |

Everything else in `impedance_ctrl` is stage2-hardcoded, not from EEPROM: `rtt_park = DIS`,
`host_initial_vref = 40`, `vrefdq = 25`, `phy_rout_pu = {13,11}`, `phy_rout_pd = {13,13}`,
`phy_pu_odt = {7,7}`, `wr_odt_map = {1,2,0,0}`, `rd_odt_map = {0,0,0,0}` (the `dimms = 1`
defaults).

The shipping stage2 was built with 1-byte enums (`-fshort-enums`) and an older HAL (no
`hv_min`/`hv_max`) — a port against the current header must not assume the struct offsets.

---

## 6. RSA-2048 capabilities blob

`0x00e4` modulus (0x100 B) + `0x01e4` signature (0x100 B), located by the closing marker.

- Read by al_boot `eeprom_preload_parser`, verified by `FUN_01002f38`: SHA-256 the modulus,
  compare against the eFuse hash at OTP `0xfd89608c`-`0xfd8960ac`, then RSA-verify.
- Gates SoC capability flags in preboot only. The `DRAM frequency violation!` /
  `CPU frequency violation!` checks in `stg3_board_init` log and set a flag; **neither
  aborts boot** (the kernel is unsigned).
- Not forgeable — needs Annapurna's private key ([overclock-and-caps.md](overclock-and-caps.md) §3).

[overclock-and-caps.md](overclock-and-caps.md) predicted this object's existence and format
from the decompile but never located it. These are its bytes.

---

## 7. Running frequency — the last open value, now read (and it is not the strap)

The doc set treated `al_bootstrap.ddr_pll_freq` as the running point. **It is not.** The
strap is a reset-time default; the EEPROM preload script (§3, records 5-8) reprograms the
NB PLL before the S2 runs. Read both.

### The strap (reset default)

`scripts/read-ddr-bootstrap.py`, `/dev/mem` at PBS regfile `0xfd8a8000` + `0x110`:

```
boot_strap = 0x0fffdef5
  NB_PLL field (bits 6:4, AL_HAL_BOOTSTRAP_NB_PLL_FREQ_MASK) = 7
  -> al_bootstrap_ddr_pll_freq_get() case 0x7 = 800 MHz    <-- default only, NOT running
```

Decode verified against the GPL HAL source
(`delroth-alpine_hal/drivers/pbs/al_hal_bootstrap.c:192`).

Same register, for the record: `cpu_pll_freq` 1700 MHz, `pll_ref_clk` 100 MHz,
`sb_pll` 3000 MHz, `sb_clk` 500 MHz, `boot_device` SPI(M0), 4 cores. `sb_clk` 500 MHz
matches the DT `sbclk` exactly, so the strap decode itself is sound.

### The NB PLL (running)

`scripts/read-nb-pll.py`, `/dev/mem` at `0xfd860c00` (unclaimed in `/proc/iomem`, so no
conflicting-mapping risk):

```
+0x00 0x8000001b   +0x04 0x1d4cc00d   +0x20 0x000c7003
```

**Byte-identical to what the 0x57 preload script writes** — proving the preload writes are
applied to hardware, and that nothing reprograms the PLL afterwards.

`reg+0x00` low byte = 27; with the usual NF+1 convention, `100 MHz × 28 / 3` =
**933.33 MHz** → **DDR4-1866**.

Three independent confirmations of 933.33 MHz:

| source | value |
|---|---|
| NB PLL `+0x00` low byte 27 → 100 × 28/3 | 933.333 MHz |
| vendor `live.dts` `nbclk` = `0x37a18808` | 933.333 MHz |
| measured arch timer, `dmesg` "cp15 timer running at 58.33MHz" (= 933.33/16) | 933.333 MHz |

`tCK = 1e9 / (933333333/1000)` = **1071 ps** → `AL_DDR_FREQ_1866`, and the SPD's
`tCKAVGmin` is also 1071 ps: **the board runs the DIMM at exactly its fastest declared
bin.** No `ddr_freq_change_according_to_spd` downshift, and no SPD headroom.

Through the S2's own functions at tCK 1071:

- `ddr_cl_solve` (`FUN_f2200ed8`) = ceil(tAA/tCK) then scan up the CAS mask
  → ceil(13920/1071) = 13, in the mask → **CL = 13**.
- `ddr4_cwl_from_tck` (`FUN_f2200e74`) is a descending tCK ladder — Ghidra renders the
  return values as `AL_I2C_*` symbols, they are plain integers:

  | tCK ≥ | CWL | ≈ MT/s |
  |---|---|---|
  | 1250 | 9 | 1600 |
  | 1070 | **10** | 1866 |
  | 935 | 11 | 2133 |
  | 833 | 12 | 2400 |
  | 750 | 14 | 2667 |
  | 681 | 16 | 2933 |

  tCK = 1071 → **CWL = 10**. (JEDEC DDR4-1866 CWL is 10 — the ladder matches the standard.)

Timings in clocks at tCK 1071:

| param | ps | clk |
|---|---|---|
| tRCD | 13920 | 13 |
| tRP | 13920 | 13 |
| tRAS | 34000 | 32 |
| tRC | 47920 | 45 |
| tFAW | 30000 | 29 |
| tRRD_S | 5300 | 5 |
| tRRD_L | 6400 | 6 |
| tCCD_L | 5355 | 5 |

This **confirms** the DDR4-1866 CL13 CWL10 already in
[ddr-config-reverse.md](ddr-config-reverse.md) and
[ddr-s2-parser-analysis.md](ddr-s2-parser-analysis.md), which computed it from the SPD cap
alone. The cap and the running point coincide here — but only by coincidence, and reading
the strap alone would have given the wrong answer (1600 CL12).

**Headroom:** the parts are marked `-BCRC` (a 2400 bin) but the SPD declares 1866 as the
maximum, and the board already runs there. Going faster means overriding the SPD as well as
the PLL, and the caps gate logs `DRAM frequency violation!` without blocking (#29).

## 8. `al_ddr_init_cfg` for ea16 — the from-scratch recipe

Everything a standalone SPL needs. Feed to `al_ddr_init()` from the GPLv2
`al_hal_ddr_init_alpine_v2.c`.

```
ddr_cfg   nb 0xf0070000, ctrl 0xf0080000, phy 0xf0088000, rev V2 (0x120120)

org       ddr_type            = AL_DDR_TYPE_DDR4
          ddr_device          = AL_DDR_DEVICE_X16
          data_width          = AL_DDR_DATA_WIDTH_64_BITS
          ranks               = 1
          dimms               = 1
          rdimm               = 0
          udimm_addr_mirroring= 0
          ecc_is_supported    = 0

tmg       ref_clk_freq_mhz    = 800          <- al_bootstrap.ddr_pll_freq / 1e6
          ddr_freq            = AL_DDR_FREQ_1600
          cl                  = 12
          cwl                 = 9
          al                  = 0
          t_ras_min_ps        = 34000
          t_rc_ps             = 47920
          t_rcd_ps            = 13920
          t_rp_ps             = 13920
          t_rrd_ps            = 6400        (tRRD_L)
          t_rrd_s_ps          = 5300
          t_ccd_ps            = 5355        (tCCD_L)
          t_faw_ps            = 30000
          t_rfc_min_ps        = 350000
          t_rfc2_ps           = 260000
          t_rfc4_ps           = 160000

addrmap   col  -> sys bits 7..13
          bank -> sys bits 17,18
          bg   -> sys bit 6
          row  -> sys bits 14,15,16,19..31
          cs   -> NC (single rank)

impedance dic = RZQ7 (34 Ω)      odt = RZQ4 (60 Ω)      odt_dyn = DIS
          rtt_park = DIS         host_initial_vref = 40  vrefdq = 25
          phy_rout = 34 Ω        phy_odt = 56 Ω
          phy_rout_pu = {13,11}  phy_rout_pd = {13,13}   phy_pu_odt = {7,7}
          wr_odt_map = {1,2,0,0} rd_odt_map = {0,0,0,0}
          dqs_res  = PULL_DOWN_500OHM
          dqsn_res = PULL_UP_500OHM

misc      training_en = 1, phy_dll_en = 1, ecc_is_enabled = 0,
          crc/parity/ocpar/dbi = 0
perf      header suggested defaults
calc      leave zeroed (HAL fills)
```

Then publish the handoff the downstream stages read:
`shared_parameters { u32 magic = 0x31415926; u64 ddr_size = 0x100000000; }` at SRAM
`0xfbff4150` (+0x04 is Annapurna's retry count, an extension over the GPL struct).

### Two ways to build it

1. **Replay the constants above.** Simplest. Valid only for this unit at this strap.
2. **Re-derive at runtime**, the way stock does — read `0x57`, parse the records + SPD, read
   the strap. Survives a board/DRAM respin. This is the portable option and every step of it
   is now byte-documented in this doc.

Also required, and *not* in this EEPROM:
- The **NB PLL setup** (§3 records 5-8) — this is what actually sets the DDR rate, and the
  strap does not. Values captured and live-verified identical; the multiplier/divider fields
  are partly decoded (low byte of +0x00 = NF-1: 27 -> 100 x 28/3 = 933.33 MHz), the rest not.
- The **DRAM remap** (`al_addr_map_dram_remap_set`) for the 3 GiB + 1 GiB split around the
  MMIO hole. al_boot's job, not the DDR HAL's.
- Nothing from the `0xBB` GPIO record (disabled) or `0xDD` UART record (absent).

---

## 9. What remains open

| item | impact |
|---|---|
| Preload record 0 address `0x7f896100` | one write of 17; the other 16 decode cleanly and are live-verified |
| Full NB PLL field layout (`0xfd860c00`) | the frequency-setting fields are decoded well enough to explain 933.33 MHz; the remaining bits (and `+0x08`..`+0x1c`) are not. Enough to replay, not enough to synthesise another rate — so #29 (overclock) still needs this |
| Live `al_ddr_cfg_init` readback cross-check | the `tmg`/`org`/`impedance` above are EEPROM-derived. Reading MR0/1/2/5/6 + MSTR/ADDRMAP off the trained controller would confirm them independently. `0xf0080000` is claimed by `al_mc_edac`, so a raw `/dev/mem` map risks the conflicting-mapping crash seen before — do it from U-Boot, or via a kprobe on the driver's own pointer |

None of these blocks the SPL: option 1 (replay) needs only what is in §8.

---

## 10. Corrections this work makes to the existing docs

- **The strap is not the running DDR frequency.** `docs/ddr-config-reverse.md` §7 and
  `docs/preboot-decompile.md` list `al_bootstrap.ddr_pll_freq` as the one open value that
  would pin the running point. Reading it gives **800 MHz (DDR4-1600)**, which is wrong —
  the EEPROM preload script reprograms the NB PLL to **933.33 MHz** before the S2 runs.
  Reading the strap alone would have produced a CL12/CWL9 config that does not match the
  hardware. Read `0xfd860c00`, not the strap.
- **The DDR4-1866 CL13 CWL10 already in the docs is confirmed**, now from the PLL and the
  measured timer rather than from the SPD cap alone.
- **`docs/overclock-and-caps.md` §2 says the strap NB_PLL field is `0x4`** (933.33 MHz).
  The live field is **`0x7`** (800 MHz). The doc's *frequency* is right and its *strap value*
  is wrong — it was back-derived from the DT rather than read. The strap table itself
  (`al_bootstrap_ddr_pll_freq_get`) is correct; it just is not what is in force.
  Consequence for #29: overclocking is a **PLL** change, not a strap change — which is
  better news, since the strap was described there as OTP/irreversible.
- **The 0x400 record base is now data-confirmed**, not the hardcoded default being assumed
  (§3a).
- **The preload region and RSA blob are located.** `docs/overclock-and-caps.md` §3 described
  the caps object's format from the decompile but never found its bytes; they are at
  `0x00e4`/`0x01e4` (§6).
