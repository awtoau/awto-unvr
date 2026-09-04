#!/usr/bin/env python3
"""Decode the Ubiquiti-custom DDR config records in the live 0x57 EEPROM dump.

Field layout is taken from the S2 stage2_loader parser
(docs/nor-reference/preboot-s2-decompiled.c):
  - record readers  FUN_f22003b0/b8/c0/c8  (base 0x400, +0x0b, +0x0e, +0x24)
  - 0xAA + SPD read FUN_f220093c            (spd_i2c_addr / spd_off / aux)
  - DDR4 SPD parse  FUN_f2201140            (al_ddr4_spd_parse)
  - SPD dispatch    FUN_f22013e8            (CRC-16 + 0x0b/0x0c => DDR3/DDR4)
  - CRC-16          FUN_f2200dcc            (XMODEM: init 0, poly 0x1021, no reflect)
  - impedance       FUN_f2200a58            (set_dram_impedance_ctrl_from_eeprom)
  - DRAM-volt GPIO  FUN_f2200d10            (0xBB record)

Offline only: reads the .bin, does not touch hardware.
Log -> tmp/logs/decode-ddr-records.log
"""

import os
from pathlib import Path

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(REPO, "docs/nor-reference/ddr-config-eeprom-0x57-8k.bin")
S2 = os.path.join(REPO, "docs/nor-reference/s2-loader-stage2_v2.22.3-25044B.bin")
LOG = os.path.join(REPO, "tmp/logs/decode-ddr-records.log")
os.makedirs(os.path.dirname(LOG), exist_ok=True)
Path(LOG).write_text("")  # truncate; out() appends per-call below


def out(*a):
    s = " ".join(str(x) for x in a)
    print(s)
    with open(LOG, "a") as f:
        f.write(s + "\n")


data = Path(BIN).read_bytes()
s2 = Path(S2).read_bytes()
BASE = 0x400  # record base (DEV_INFO override absent -> default 0x400)


def crc16_xmodem(buf):
    """FUN_f2200dcc: init 0, poly 0x1021, MSB-first, no reflect, no xorout."""
    crc = 0
    for b in buf:
        crc ^= b << 8
        for _ in range(8):
            crc = (
                ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
            )
    return crc & 0xFFFF


# ------------------------------------------------------------------ records
out("=" * 72)
out(f"EEPROM 0x57 DDR config records  (base 0x{BASE:03x})")
out("=" * 72)


def hexs(off, n):
    return " ".join(f"{b:02x}" for b in data[off : off + n])


# ---- 0xAA pointer record : FUN_f22003b0 reads 7 B @ base+0; FUN_f220093c parses
aa = data[BASE : BASE + 7]
out(f"\n[0xAA] pointer record @0x{BASE:03x} (7 B): {hexs(BASE, 7)}")
assert aa[0] == 0xAA, "no 0xAA magic"
spd_addr = aa[1]
spd_off = aa[2] | (aa[3] << 8)  # u16 LE  (CONCAT11(byte3,byte2))
aux_addr = aa[4]
aux_off = aa[5] | (aa[6] << 8)
out(f"  +0 magic        = 0x{aa[0]:02x}")
out(
    "  +1 spd_i2c_addr = 0x{:02x}  {}".format(
        spd_addr,
        "(0xff => use bootstrap i2c_preload_addr, live=0x57)"
        if spd_addr == 0xFF
        else "",
    )
)
out(
    f"  +2 spd_off      = 0x{spd_off:04x}  (u16 LE)  -> SPD image read here on the SAME device"
)
out(
    "  +4 aux_i2c_addr = 0x{:02x}  {}".format(
        aux_addr, "(0x00 => no 2nd-rank/dimm probe => dimms=1)" if aux_addr == 0 else ""
    )
)
out(f"  +5 aux_off      = 0x{aux_off:04x}")

# ---- 0xBB DRAM-voltage GPIO : FUN_f2200d10 checks buf[+0x0b]==0xBB, pin buf[+0x0c]
bb = data[BASE + 0x0B : BASE + 0x0B + 3]
out(f"\n[0xBB] DRAM-voltage GPIO @0x{BASE + 0x0B:03x} (3 B): {hexs(BASE + 0x0B, 3)}")
out(f"  +0 magic    = 0x{bb[0]:02x}")
out(
    "  +1 gpio_pin = 0x{:02x}  {}".format(
        bb[1],
        "(0xff => DISABLED: no GPIO drives DRAM voltage; DDR4 fixed 1.2 V)"
        if bb[1] == 0xFF
        else "(active pin < 0x38)",
    )
)
out(
    "  +2 polarity = 0x{:02x}  {}".format(
        bb[2], "(unused, pin disabled)" if bb[1] == 0xFF else ""
    )
)

# ---- 0xCC impedance override : FUN_f2200a58 checks buf[+0x0e]==0xCC
cc = data[BASE + 0x0E : BASE + 0x0E + 0x16]
out(
    f"\n[0xCC] impedance override @0x{BASE + 0x0E:03x} (0x16 B): {hexs(BASE + 0x0E, 0x16)}"
)
assert cc[0] == 0xCC
dqs_sel = cc[1]
tbl = cc[2 : 2 + 20]  # 20-byte table @ base+0x10
out(f"  +0 magic       = 0x{cc[0]:02x}")
out(
    "  +1 dqs_sel     = 0x{:02x}  {}".format(
        dqs_sel,
        "(==1 => dqs_res=PULL_DOWN_500OHM, dqsn_res=PULL_UP_500OHM)"
        if dqs_sel == 1
        else "(else => dqs_res=PULL_UP_611OHM, dqsn_res=PULL_UP_458OHM)",
    )
)
out("  +2.. 20-byte table: {}".format(" ".join(f"{b:02x}" for b in tbl)))


# impedance decode tables lifted from the S2 binary (VA 0xf22040xx -> file off 0x40xx)
def s2tbl(foff, n):  # n 2-byte (key,enum) pairs
    return [(s2[foff + 2 * i], s2[foff + 2 * i + 1]) for i in range(n)]


t_odt = s2tbl(0x4C34, 10)  # DRAM RTT_NOM   key=RZQ divider
t_odt_dyn = s2tbl(0x4C2A, 5)  # DRAM RTT_WR    key=RZQ divider (0x99=Hi-Z)
t_dic = s2tbl(0x4C48, 3)  # DRAM DIC       key=RZQ divider
t_phy_odt = s2tbl(0x4C4E, 15)  # PHY ODT        key=ohms
t_phy_rout = s2tbl(0x4C6C, 11)  # PHY ROUT       key=ohms
out("\n  S2 decode tables (key=EEPROM value -> enum):")
out(f"    odt      (0x4c34): {t_odt}")
out(f"    odt_dyn  (0x4c2a): {t_odt_dyn}")
out(f"    dic      (0x4c48): {t_dic}")
out(f"    phy_odt  (0x4c4e): {t_phy_odt}")
out(f"    phy_rout (0x4c6c): {t_phy_rout}")


def lookup(tbl, key):
    for k, e in tbl:
        if k == key:
            return e
    return None


# dimms=1, ranks_per_dimm=1 -> idx=(dimms-1)*10+(rpd-1)*5 = 0 -> table[0:5]
sel = tbl[0:5]
out(
    "\n  active 5-byte set (dimms=1,ranks/dimm=1 -> idx 0): {}".format(
        " ".join(f"{b:02x}" for b in sel)
    )
)
odt_v, odt_dyn_v, dic_v, phy_odt_v, phy_rout_v = sel
out(
    f"    [0] odt      EEPROM=0x{odt_v:02x}  RZQ/{odt_v} = {240 // odt_v if odt_v else 0} ohm  -> enum {lookup(t_odt, odt_v)}"
)
out(
    "    [1] odt_dyn  EEPROM=0x{:02x}  {} -> enum {}".format(
        odt_dyn_v,
        (f"RZQ/{odt_dyn_v} = {240 // odt_dyn_v} ohm")
        if odt_dyn_v
        else "divider 0 = DISABLED",
        lookup(t_odt_dyn, odt_dyn_v),
    )
)
out(
    f"    [2] dic      EEPROM=0x{dic_v:02x}  RZQ/{dic_v} = {240 // dic_v if dic_v else 0} ohm  -> enum {lookup(t_dic, dic_v)}"
)
out(
    f"    [3] phy_odt  EEPROM=0x{phy_odt_v:02x}  = {phy_odt_v} ohm         -> enum {lookup(t_phy_odt, phy_odt_v)}"
)
out(
    f"    [4] phy_rout EEPROM=0x{phy_rout_v:02x}  = {phy_rout_v} ohm         -> enum {lookup(t_phy_rout, phy_rout_v)}"
)

# ------------------------------------------------------------------ SPD
out("\n" + "=" * 72)
out(f"SPD image @ spd_off=0x{spd_off:03x}  (parsed as JEDEC DDR4 by FUN_f2201140)")
out("=" * 72)
spd = data[spd_off : spd_off + 512]


def B(i):
    return spd[i]


# ---- CRC-16 gate (FUN_f22013e8) ------------------------------------------
crc_len1 = 0x75 if (B(0) & 0x80) else 0x7E  # bit7 of byte0 selects range
c1 = crc16_xmodem(spd[0:crc_len1])
c1_stored = B(0x7E) | (B(0x7F) << 8)
c2 = crc16_xmodem(spd[0x80 : 0x80 + 0x7E])
c2_stored = B(0xFE) | (B(0xFF) << 8)
out("\nCRC gate:")
out(
    "  block1 CRC over 0..0x{:02x}: calc=0x{:04x} stored=0x{:04x}  {}".format(
        crc_len1 - 1, c1, c1_stored, "OK" if c1 == c1_stored else "MISMATCH"
    )
)
out(
    "  block2 CRC over 0x80..0xfd: calc=0x{:04x} stored=0x{:04x}  {}".format(
        c2, c2_stored, "OK" if c2 == c2_stored else "MISMATCH"
    )
)
out(f"  => image is a genuine CRC-valid JEDEC DDR4 SPD (byte2=0x{B(2):02x})")

# ---- geometry (FUN_f2201140) ---------------------------------------------
out("\nGeometry / organization:")
b2, b3, b4, b5, b12, b13 = B(2), B(3), B(4), B(5), B(0xC), B(0xD)
dev_w = {0: "x4", 1: "x8", 2: "x16", 3: "x32"}[b12 & 7]
ranks = ((b12 & 0x3F) >> 3) + 1
dw_code = b13 & 7
data_width = {2: "32-bit", 3: "64-bit", 1: "16-bit"}.get(dw_code, "?")
rows = ((b5 & 0x3F) >> 3) + 12
cols = (b5 & 7) + 9
banks_bits = ((b4 & 0x3F) >> 4) + 2
bg_bits = b4 >> 6
ecc = (b13 & 0x18) != 0
mtype = {1: "RDIMM", 2: "UDIMM", 3: "SODIMM", 4: "LRDIMM"}.get(
    b3 & 0xF, f"0x{b3 & 0xF:x}"
)
out(f"  byte 0x02 = 0x{b2:02x}  ddr_type      = DDR4")
out(f"  byte 0x03 = 0x{b3:02x}  module_type   = {mtype}")
out(f"  byte 0x0c = 0x{b12:02x}  device_width  = {dev_w} ; ranks = {ranks}")
out(
    f"  byte 0x0d = 0x{b13:02x}  data_width    = {data_width} (raw&7={dw_code}) ; ecc = {ecc}"
)
out(
    f"  byte 0x04 = 0x{b4:02x}  bank_bits     = {banks_bits} ({1 << banks_bits} banks) ; bank_group_bits = {bg_bits} ({1 << bg_bits} groups)"
)
out(f"  byte 0x05 = 0x{b5:02x}  row_bits      = {rows} ; col_bits = {cols}")
dens_bits = (
    rows + cols + banks_bits + bg_bits + {"x4": 2, "x8": 3, "x16": 4, "x32": 5}[dev_w]
)
out(
    f"  derived per-device density = 2^{dens_bits} bits = {(1 << dens_bits) // (1 << 30)} Gbit  (JEDEC byte4[3:0] density code = {b4 & 0xF}, IGNORED by parser)"
)

# ---- timings (FUN_f2201140), MTB=125 ps, FTB=1 ps ------------------------
MTB = 125
tb = B(0x11)


def sc(i):  # signed char
    v = B(i)
    return v - 256 if v >= 128 else v


out(f"\nTiming (MTB={MTB} ps, FTB=1 ps ; byte0x11 timebase=0x{tb:02x}):")
tck_mtb = B(0x12)
tck_ftb = sc(0x7D)
tck = tck_mtb * MTB + tck_ftb
taa = B(0x18) * MTB + sc(0x7B)
trcd = B(0x19) * MTB + sc(0x7A)
trp = B(0x1A) * MTB + sc(0x79)
tras = ((B(0x1B) & 0x0F) << 8 | B(0x1C)) * MTB
trc = ((B(0x1B) & 0xF0) << 4 | B(0x1D)) * MTB + sc(0x78)
trfc1 = (B(0x1E) | B(0x1F) << 8) * MTB
trfc2 = (B(0x20) | B(0x21) << 8) * MTB
trfc4 = (B(0x22) | B(0x23) << 8) * MTB
tfaw = ((B(0x24) & 0x0F) << 8 | B(0x25)) * MTB
trrds = B(0x26) * MTB + sc(0x77)
trrdl = B(0x27) * MTB + sc(0x76)
tccdl = B(0x28) * MTB + sc(0x75)
casmap = B(0x14) | B(0x15) << 8 | B(0x16) << 16 | B(0x17) << 24
cls = [7 + i for i in range(24) if casmap & (1 << i)]
import math


def clk(ps):
    return math.ceil(ps / tck) if tck else 0


out(
    f"  byte 0x12/0x7d  tCKAVGmin = {tck} ps  => {round(2e6 / tck)} MT/s (fastest the SPD allows)"
)
out(f"  byte 0x14..17   CAS map = 0x{casmap:08x}  supported CL = {cls}")
out(
    f"  byte 0x18/0x7b  tAAmin  = {taa:5d} ps  = CL {clk(taa)} (ceil tAA/tCK; next supported)"
)
out(f"  byte 0x19/0x7a  tRCDmin = {trcd:5d} ps  = {clk(trcd):2d} clk")
out(f"  byte 0x1a/0x79  tRPmin  = {trp:5d} ps  = {clk(trp):2d} clk")
out(f"  byte 0x1b/0x1c  tRASmin = {tras:5d} ps  = {clk(tras):2d} clk")
out(f"  byte 0x1b/0x1d  tRCmin  = {trc:5d} ps  = {clk(trc):2d} clk")
out(
    f"  byte 0x1e/0x1f  tRFC1   = {trfc1:6d} ps = {trfc1 // 1000} ns  (8 Gbit JEDEC=350 ns)"
)
out(
    f"  byte 0x20/0x21  tRFC2   = {trfc2:6d} ps = {trfc2 // 1000} ns  (8 Gbit JEDEC=260 ns)"
)
out(
    f"  byte 0x22/0x23  tRFC4   = {trfc4:6d} ps = {trfc4 // 1000} ns  (8 Gbit JEDEC=160 ns)"
)
out(f"  byte 0x24/0x25  tFAWmin = {tfaw:5d} ps  = {clk(tfaw):2d} clk")
out(f"  byte 0x26/0x77  tRRD_S  = {trrds:5d} ps  = {clk(trrds):2d} clk")
out(f"  byte 0x27/0x76  tRRD_L  = {trrdl:5d} ps  = {clk(trrdl):2d} clk")
out(f"  byte 0x28/0x75  tCCD_L  = {tccdl:5d} ps  = {clk(tccdl):2d} clk")

out("\nRaw SPD bytes 0x00..0x2f:")
out("  " + " ".join(f"{B(i):02x}" for i in range(0x30)))
out(
    "Raw SPD FTB bytes 0x75..0x7d: "
    + " ".join(f"{B(i):02x}" for i in range(0x75, 0x7E))
)

# ------------------------------------------------------------------ addrmap
# FUN_f2200816 (0xf2200816) -> al_ddr_addrmap (37 B), NC = 0xff
out("\n" + "=" * 72)
out("addrmap  (FUN_f2200816 -> struct al_ddr_addrmap, cfg+0x44)")
out("=" * 72)
dw_enum = {"64-bit": 1, "32-bit": 0, "16-bit": 2}[data_width]
am = [0xFF] * 0x25
iv = 6 if dw_enum == 1 else (5 if dw_enum == 0 else 4)  # 6/5/4 for 64/32/16-bit
# DDR4 path (ddr_type==1)
if iv != 6:
    am[0] = iv
    if iv != 5:
        am[1] = 5
u6 = bg_bits
u5 = 9 - iv
if u6 != 0:
    am[0xD] = 6
    if u6 > 1:
        am[0xE] = 7
iv = u6 + 6
k = u6
while k < 2:
    am[k + 0xD] = 0xFF
    k += 1
# column map
u6 = cols
while u5 < u6:
    am[u5 - 3] = iv & 0xFF
    iv += 1
    u5 += 1
while u5 < 0xD:
    am[u5 - 3] = 0xFF
    u5 += 1
cVar1 = iv & 0xFF
# cs (rank) map : ranks==1 here -> NC,NC
if ranks == 4:
    am[0x21] = cVar1
    iv += 2
    am[0x22] = (cVar1 + 1) & 0xFF
elif ranks == 2:
    am[0x21], am[0x22] = (0xFF, cVar1) if False else (cVar1, 0xFF)
    iv += 1
else:
    am[0x21] = 0xFF
    am[0x22] = 0xFF
# row map fill part 1 (while iv<=0x10)
iv8 = iv
p = 0xF
while True:
    d = iv8 - iv
    cVar1 = iv8 & 0xFF
    if iv8 > 0x10:
        break
    am[0xF + d] = cVar1
    iv8 += 1
# bank map
bk = banks_bits
for j in range(bk):
    am[0xA + j] = (cVar1 + j) & 0xFF
cVar2 = bk
for j in range(bk, 3):
    am[0xA + j] = 0xFF
# row map fill part 2
u5b = iv8 - iv  # = 3
cVar3 = u5b & 0xFF
base = (cVar1 + cVar2 - cVar3) & 0xFF
while u5b < rows:
    am[0xF + u5b] = (base + u5b) & 0xFF
    u5b += 1
while u5b < 0x12:
    am[0xF + u5b] = 0xFF
    u5b += 1
am[0x24] = 0  # *(u32)(am+0x24)=0


def amslice(lo, hi):
    return " ".join(f"{am[i]:02x}" for i in range(lo, hi + 1))


out(f"  col_b3_9_b11_13[10] @0x00 : {amslice(0x00, 0x09)}")
out(f"  bank_b0_2[3]        @0x0a : {amslice(0x0A, 0x0C)}")
out(f"  bg_b0_1[2]          @0x0d : {amslice(0x0D, 0x0E)}")
out(f"  row_b0_17[18]       @0x0f : {amslice(0x0F, 0x20)}")
out(f"  cs_b0_1[2]          @0x21 : {amslice(0x21, 0x22)}   (single rank -> NC)")
out(
    "  (0xff = AL_DDR_ADDRMAP_NC ; row bits 0-2 -> sys 14-16, bank -> 17-18, row 3-15 -> 19-31)"
)

# ------------------------------------------------------------------ impedance_ctrl (final)
out("\n" + "=" * 72)
out("impedance_ctrl  (FUN_f2200a58 -> struct al_ddr_init_cfg_impedance_ctrl, cfg+0xb8)")
out("=" * 72)
ODT_E = {
    0: "DIS",
    1: "RZQ2",
    2: "RZQ4",
    3: "RZQ6",
    4: "RZQ8",
    5: "RZQ12",
    6: "RZQ1",
    7: "RZQ5",
    8: "RZQ3",
    9: "RZQ7",
}
DYN_E = {0: "DIS", 1: "RZQ2", 2: "RZQ4", 3: "RZQ1", 4: "HI_Z"}
DIC_E = {0: "RZQ6", 1: "RZQ7", 2: "RZQ5"}
odt_e = lookup(t_odt, odt_v)
dyn_e = lookup(t_odt_dyn, odt_dyn_v)
dic_e = lookup(t_dic, dic_v)
podt_e = lookup(t_phy_odt, phy_odt_v)
prout_e = lookup(t_phy_rout, phy_rout_v)
out(
    f"  dic               = {dic_e}  AL_DDR_DIC_{DIC_E[dic_e]}  (RZQ/{dic_v} = {240 // dic_v} ohm)"
)
out(
    f"  odt (RTT_NOM)     = {odt_e}  AL_DDR_ODT_{ODT_E[odt_e]}  (RZQ/{odt_v} = {240 // odt_v} ohm)"
)
out(f"  odt_dyn (RTT_WR)  = {dyn_e}  AL_DDR_ODT_DYN_{DYN_E[dyn_e]}")
out("  rtt_park          = 0  AL_DDR_RTT_PARK_DIS  (default, not overridden)")
out("  host_initial_vref = 0x28 = 40  (stage2 hardcoded)")
out("  vrefdq            = 0x19 = 25  (stage2 hardcoded)")
out("  phy_rout_pu[2]    = {13, 11}   (stage2 hardcoded 0x0b0d)")
out("  phy_rout_pd[2]    = {13, 13}   (stage2 hardcoded 0x0d0d)")
out("  phy_pu_odt[2]     = {7, 7}     (stage2 hardcoded 0x0707)")
out("  wr_odt_map[4]     = {1, 2, 0, 0}   (dimms=1 default)")
out("  rd_odt_map[4]     = {0, 0, 0, 0}   (dimms=1 default)")
out(f"  phy_rout[2]       = {{{prout_e}, {prout_e}}}  (EEPROM {phy_rout_v} ohm)")
out(f"  phy_odt[2]        = {{{podt_e}, {podt_e}}}  (EEPROM {phy_odt_v} ohm)")
out("  dqs_res           = 0  AL_DDR_DQS_RES_PULL_DOWN_500OHM   (dqs_sel=1)")
out("  dqsn_res          = 1  AL_DDR_DQSN_RES_PULL_UP_500OHM    (dqs_sel=1)")

# ------------------------------------------------------------------ size + freq (orchestrator FUN_f22003d8)
out("\n" + "=" * 72)
out("size + speed bin  (orchestrator FUN_f22003d8)")
out("=" * 72)
addr_bits = rows + cols + banks_bits + bg_bits  # per rank
bus_bytes = 1 << ({"64-bit": 6, "32-bit": 5, "16-bit": 4}[data_width] - 3)
size = ranks * (1 << addr_bits) * bus_bytes
out(
    f"  ddr_size = ranks({ranks}) << (row+col+bank+bg = {addr_bits}) * bus_bytes({bus_bytes}) = {size} bytes = {size // (1 << 30)} GiB"
)
# ddr_freq speed bin (tmg+0x04) : orchestrator maps running tCK (from PLL). SPD-fastest = tck.
FREQ_E = {0: "800", 1: "1066", 2: "1333", 3: "1600", 4: "1866", 5: "2133", 6: "2400"}


def freq_enum(tck_ps):
    if tck_ps == 0x753:
        return 1  # 1875 -> 1066 (also the no-match default=1)
    if tck_ps == 0x5DC:
        return 2  # 1500 -> 1333
    if tck_ps == 0x4E2:
        return 3  # 1250 -> 1600
    if tck_ps in (0x442, 0x42F):
        return 4  # 1090/1071 -> 1866
    if 0x3A9 <= tck_ps < 0x3A9 + 0x85:
        return 5  # 2133
    if 0x341 <= tck_ps < 0x341 + 0x68:
        return 6  # 2400
    return 1


fe = freq_enum(tck)
out(
    "  SPD tCKAVGmin = {} ps (0x{:x}) -> speed-bin enum = {} = AL_DDR_FREQ_{} (fastest the DIMM allows)".format(
        tck, tck, fe, FREQ_E.get(fe, "?")
    )
)


# FUN_f2200e74 ddr4_cwl_from_tck: descending tCK ladder. Ghidra renders the
# return values as AL_I2C_* symbols; they are plain integers (0x9, 0xA, 0xB, 0xC).
def cwl_from_tck(t):
    for thr, cwl in (
        (0x4E2, 9),
        (0x42E, 10),
        (0x3A7, 11),
        (0x341, 12),
        (0x2EE, 14),
        (0x2A9, 16),
    ):
        if t >= thr:
            return cwl
    return None  # below 0x2a9 the loader errors out


def cl_solve(t):
    """FUN_f2200ed8 ddr_cl_solve: ceil(tAA/tCK), then scan up the CAS support mask."""
    c = math.ceil(taa / t)
    while c < 0x20 and not (casmap & (1 << (c - 7))):
        c += 1
    return c if c < 0x20 else None


# The RUNNING point is the NB PLL, NOT the boot_strap NB_PLL field.
# The strap is only the reset-time default; the 0x57 preload script reprograms the PLL
# before the S2 runs (docs/ddr-eeprom-0x57.md §7). Live on woomera 2026-09-04:
#   boot_strap @0xfd8a8110 = 0x0fffdef5, NB_PLL field (bits 6:4) = 7 -> 800 MHz  (default)
#   NB PLL     @0xfd860c00 = 0x8000001b, NF+1=28 / NR*OD=3        -> 933.33 MHz (running)
# Corroborated by DT nbclk 0x37a18808 = 933.333 MHz and the arch timer at 933.33/16 =
# 58.33 MHz (dmesg "cp15 timer running at 58.33MHz"). scripts/read-nb-pll.py reads it.
DDR_PLL_HZ = 933_333_333
run_tck = 1_000_000_000 // (DDR_PLL_HZ // 1000)  # S2: tCK_ps = 1e9 / (pll_hz/1000)
run_fe = freq_enum(run_tck)
out("")
out(f"  RUNNING point: NB PLL @0xfd860c00 = {DDR_PLL_HZ / 1e6:.2f} MHz")
out("    (NOT the boot_strap NB_PLL field, which reads 7 = 800 MHz — that is only the")
out(
    "     reset default; the 0x57 preload script reprograms the PLL before the S2 runs)"
)
out(
    f"    tCK = 1e9/(pll/1000) = {run_tck} ps (0x{run_tck:x}) -> enum {run_fe} = AL_DDR_FREQ_{FREQ_E.get(run_fe, '?')}"
)
out(f"    CL  (tmg+0x38, FUN_f2200ed8) = {cl_solve(run_tck)}")
out(f"    CWL (tmg+0x3c, FUN_f2200e74) = {cwl_from_tck(run_tck)}")
out("    timings at the running tCK:")
for nm, ps in (
    ("tRCD", trcd),
    ("tRP", trp),
    ("tRAS", tras),
    ("tRC", trc),
    ("tFAW", tfaw),
    ("tRRD_S", trrds),
    ("tRRD_L", trrdl),
    ("tCCD_L", tccdl),
):
    out(f"      {nm:7} {ps:6d} ps -> {math.ceil(ps / run_tck):3d} clk")
out("")
if run_fe < fe:
    out(
        f"  Running {FREQ_E.get(run_fe, '?')} is BELOW the SPD cap of {FREQ_E.get(fe, '?')},"
    )
    out("  so ddr_freq_change_according_to_spd does NOT downshift the NB PLL.")
else:
    out(
        f"  Running {FREQ_E.get(run_fe, '?')} == the SPD cap {FREQ_E.get(fe, '?')}: the board runs"
    )
    out(
        "  the DIMM at exactly its fastest declared bin. No PLL downshift, no headroom in SPD."
    )

out(f"\nlog -> {LOG}")
