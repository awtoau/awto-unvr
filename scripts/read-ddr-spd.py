#!/usr/bin/env python3
"""Read + decode the DDR4 SPD @ I2C 0x57 on woomera, over the serial console.

Authoritative source for OUR board's DDR params (org/tmg/addrmap) needed to fill
`struct al_ddr_init_cfg` for the U-Boot SPL DDR port (docs/uboot-ddr-port.md §3).

- Enumerates /dev/i2c-N adapters, finds the bus carrying 0x57 (may be behind the
  PCA9546A mux), i2cdump's 512 SPD bytes, and decodes the DDR4 (SPD rev 1.x, JEDEC
  4.1.2.12) key fields: density/width/ranks/banks, speed bin, CL support map,
  tCKmin, tRCD/tRP/tRAS/tRC/tRFC1/tFAW, manufacturer + part number.
- Read-only. Full dump + decode -> tmp/logs/read-ddr-spd.log.

Console contract copied from scripts/i2c-spi-scan.py (Fedora root/login shell over
the tio unix socket).

SPD decode is offline (pure byte math) so it also runs on a saved hexdump:
    read-ddr-spd.py --file <hexdump>   # 512 bytes, i2cdump/hexdump/raw all accepted
"""
from __future__ import annotations
import argparse, re, sys
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import LOGS  # noqa: E402

import _console as con  # noqa: E402

login, sh = con.login, con.sh   # console driver lives in scripts/_console.py
SPD_ADDR = 0x57


def log(m):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    (LOGS / "read-ddr-spd.log").open("a").write(line + "\n")


# ---------- DDR4 SPD decode (JEDEC 4.1.2.12; byte offsets per spec) ----------

def _ftb_mtb(b):
    # byte 17 = timebases: MTB (medium) and FTB (fine). DDR4: MTB=125ps, FTB=1ps.
    return 125, 1  # fixed for DDR4 SPD rev >= 1.0


def decode_ddr4_spd(spd: bytes) -> str:
    if len(spd) < 384:
        return f"SPD too short ({len(spd)} bytes) — need >=384 for DDR4 decode"
    out = []
    dev_type = spd[2]
    dtype = {0x0b: "DDR3", 0x0c: "DDR4", 0x11: "DDR5"}.get(dev_type, f"0x{dev_type:02x}")
    out.append(f"SPD device type (byte2): {dtype}")
    if dev_type != 0x0c:
        out.append("NOT DDR4 — decoder below assumes DDR4; treat with caution")
    mtb, ftb = _ftb_mtb(spd)

    # byte 4: density/banks. bits[3:0]=SDRAM capacity (Mb), bits[5:4]=bank addr,
    # bits[7:6]=bank group.
    cap_code = spd[4] & 0x0f
    sdram_cap_mbit = {0: 256, 1: 512, 2: 1024, 3: 2048, 4: 4096, 5: 8192,
                      6: 16384, 7: 32768, 8: 12288, 9: 24576}.get(cap_code, 0)
    bank_addr_bits = 2 + ((spd[4] >> 4) & 0x3)
    bank_group_bits = (spd[4] >> 6) & 0x3
    out.append(f"SDRAM density/banks (byte4): {sdram_cap_mbit} Mbit/die, "
               f"{bank_addr_bits} bank-addr bits, {bank_group_bits} bank-group bits")

    # byte 5: addressing. bits[2:0]=col-3, bits[5:3]=row-12.
    col_bits = 9 + (spd[5] & 0x7)
    row_bits = 12 + ((spd[5] >> 3) & 0x7)
    out.append(f"Addressing (byte5): {row_bits} row bits, {col_bits} col bits")

    # byte 12: module organization. bits[2:0]=device width (4<<n),
    # bits[5:3]=ranks-1. byte13 bits[2:0]=primary bus width (8<<n).
    dev_width = 4 << (spd[12] & 0x7)
    ranks = 1 + ((spd[12] >> 3) & 0x7)
    bus_width = 8 << (spd[13] & 0x7)
    bus_ext = (spd[13] >> 3) & 0x3  # 0=no ECC, 1=8-bit ECC extension
    out.append(f"Module org (byte12/13): x{dev_width} device, {ranks} rank(s), "
               f"{bus_width}-bit primary bus, ECC-ext={'yes' if bus_ext else 'no'}")

    # total capacity: cap_mbit/8 * bus_width/dev_width * ranks (MB)
    if sdram_cap_mbit and dev_width and bus_width:
        total_mb = (sdram_cap_mbit // 8) * (bus_width // dev_width) * ranks
        out.append(f"Computed module capacity: {total_mb} MiB "
                   f"({total_mb/1024:.2f} GiB) per this SPD")

    # timing: byte18=tCKmin (MTB units), byte19=tCKmax.
    tckmin_ps = spd[18] * mtb + (_s8(spd[125]) * ftb)
    tckmax_ps = spd[19] * mtb + (_s8(spd[124]) * ftb)
    if tckmin_ps:
        mts = round(2_000_000 / tckmin_ps / 100) * 100  # data rate MT/s
        out.append(f"tCKmin (byte18): {tckmin_ps} ps  -> ~DDR4-{mts} "
                   f"(tCKmax {tckmax_ps} ps)")

    # CAS latencies supported: bytes 20-23 bitmap. byte20 bit0 = CL7 (DDR4 base 7).
    cl_map = spd[20] | (spd[21] << 8) | (spd[22] << 16) | (spd[23] << 24)
    cls = [7 + i for i in range(32) if cl_map & (1 << i)]
    out.append(f"CAS latencies supported (byte20-23): {cls}")

    def t(off_mtb, off_ftb=None, name=""):
        v = spd[off_mtb] * mtb
        if off_ftb is not None:
            v += _s8(spd[off_ftb]) * ftb
        return v

    taa = t(24, 123, "tAA")     # CL time
    trcd = t(25, 122)
    trp = t(26, 121)
    # tRAS: 12-bit, byte27 bits[3:0] hi nibble, byte28 lo byte (MTB only)
    tras = (((spd[27] & 0x0f) << 8) | spd[28]) * mtb
    # tRC: byte27 bits[7:4] hi, byte29 lo, +FTB byte120
    trc = ((((spd[27] >> 4) & 0x0f) << 8) | spd[29]) * mtb + _s8(spd[120]) * ftb
    # tRFC1: bytes 30(lo)/31(hi) in MTB
    trfc1 = (spd[30] | (spd[31] << 8)) * mtb
    trfc2 = (spd[32] | (spd[33] << 8)) * mtb
    trfc4 = (spd[34] | (spd[35] << 8)) * mtb
    # tFAW: byte36 bits[3:0] hi, byte37 lo
    tfaw = (((spd[36] & 0x0f) << 8) | spd[37]) * mtb
    # tRRD_S byte38, tRRD_L byte39, tCCD_L byte40 (+FTB 119/118/117)
    trrd_s = t(38, 119)
    trrd_l = t(39, 118)
    tccd_l = t(40, 117)
    for nm, v in [("tAA/CL", taa), ("tRCD", trcd), ("tRP", trp), ("tRAS", tras),
                  ("tRC", trc), ("tRFC1", trfc1), ("tRFC2", trfc2), ("tRFC4", trfc4),
                  ("tFAW", tfaw), ("tRRD_S", trrd_s), ("tRRD_L", trrd_l),
                  ("tCCD_L", tccd_l)]:
        ck = f"  (~{v/tckmin_ps:.1f} nCK)" if tckmin_ps else ""
        out.append(f"{nm}: {v} ps{ck}")

    # manufacturer (bytes 320-321 JEDEC id) + part number (329-348 ASCII)
    part = spd[329:349].decode("ascii", "replace").strip("\x00 ").rstrip()
    out.append(f"Module part number (byte329-348): {part!r}")
    out.append(f"Mfr JEDEC ID (byte320-321): {spd[320]:#04x} {spd[321]:#04x} "
               f"(0x80 0xCE = Samsung)")
    return "\n".join(out)


def _s8(b):  # signed byte for FTB corrections
    return b - 256 if b >= 128 else b


def parse_i2cdump(text: str) -> bytes:
    """Extract byte values from i2cdump -y N 0x57 output (16 bytes/row, hex)."""
    vals = []
    for line in text.splitlines():
        m = re.match(r"^([0-9a-f]{2}):\s+((?:[0-9a-fA-F]{2}\s+){1,16})", line)
        if not m:
            continue
        vals += [int(x, 16) for x in m.group(2).split()]
    return bytes(vals)


def read_eeprom_16bit(s, bus, addr, start, length, chunk=128):
    """Read a 16-bit-addressed EEPROM (AT24C64) via i2ctransfer.
    0x57 is NOT a plain SPD at offset 0: it is a 16-bit-addressed config EEPROM
    (records at base 0x400). A 1-byte i2cdump at offset 0 reads the wrong region.
    Each transfer writes a 2-byte offset then reads up to 255 bytes."""
    data = bytearray()
    off = start
    end = start + length
    while off < end:
        n = min(chunk, end - off)
        hi, lo = (off >> 8) & 0xff, off & 0xff
        _, out = sh(s, f"i2ctransfer -y {bus} w2@0x{addr:02x} 0x{hi:02x} 0x{lo:02x} r{n}", 20)
        vals = re.findall(r"0x([0-9a-fA-F]{2})", out)
        got = bytes(int(v, 16) for v in vals[:n])
        if len(got) != n:
            log(f"WARN: offset 0x{off:04x} wanted {n} got {len(got)} bytes: {out[:120]!r}")
        data += got
        off += n
    return bytes(data)


def find_ddr4_spd(blob: bytes):
    """Locate a DDR4 SPD inside a raw EEPROM dump: SPD byte0=0x23 (size), byte2=0x0C
    (DDR4). Returns (offset, spd_bytes) or (None, b'')."""
    for i in range(0, len(blob) - 3):
        if blob[i + 2] == 0x0C and (blob[i] & 0x0f) in (0x2, 0x3):  # 0x23/0x22 size codes
            return i, blob[i:i + 512]
    # fallback: any byte2==0x0C
    for i in range(0, len(blob) - 3):
        if blob[i + 2] == 0x0C:
            return i, blob[i:i + 512]
    return None, b""


def _hex(b, base=0):
    out = []
    for i in range(0, len(b), 16):
        row = b[i:i + 16]
        hexs = " ".join(f"{x:02x}" for x in row)
        asc = "".join(chr(x) if 32 <= x < 127 else "." for x in row)
        out.append(f"{base + i:04x}: {hexs:<47} {asc}")
    return "\n".join(out)


def find_spd_bus(s):
    """Return the i2c bus number whose i2cdetect shows 0x57, else None."""
    _, out = sh(s, 'for b in /sys/class/i2c-dev/i2c-*; do n=$(basename $b|sed s/i2c-//); '
                   'echo "BUS $n"; i2cdetect -y -r "$n" 2>/dev/null; done', 60)
    log(f"i2cdetect sweep:\n{out}")
    cur = None
    for line in out.splitlines():
        m = re.match(r"BUS (\d+)", line)
        if m: cur = int(m.group(1)); continue
        # SPD row is "50: ... 57 ..." (0x50-0x57 range printed on the 50: row)
        if cur is not None and re.match(r"^50:", line) and re.search(r"\b57\b", line):
            return cur
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--file", help="decode a saved SPD hexdump instead of reading device")
    args = ap.parse_args()

    if args.file:
        raw = Path(args.file).read_bytes()
        spd = parse_i2cdump(raw.decode("latin1")) if b":" in raw else raw
        log(f"decoding {len(spd)} SPD bytes from {args.file}")
        log("\n" + decode_ddr4_spd(spd))
        return 0

    try:
        s = con.connect()
    except FileNotFoundError as e:
        sys.exit(f"{e} — or use --file")
    login(s)

    have = sh(s, "command -v i2cdetect >/dev/null && command -v i2cdump >/dev/null "
                 "&& echo yes || echo no")[1]
    if "yes" not in have:
        log("i2c-tools missing — installing (dnf, up to ~3 min)")
        sh(s, "dnf -y -q install i2c-tools 2>&1 | tail -3", 200)

    bus = find_spd_bus(s)
    if bus is None:
        log(f"0x{SPD_ADDR:02x} not found on any bus — SPD may be behind the PCA9546A "
            "mux and not enumerated as a linux i2c-dev. Inspect the sweep above.")
        s.close(); return 1
    log(f"SPD @ 0x{SPD_ADDR:02x} on i2c bus {bus}")

    # 0x57 is a 16-bit-addressed AT24C64 config EEPROM (records at base 0x400),
    # NOT a plain SPD at offset 0. Dump the full 8 KiB via i2ctransfer.
    log("dumping full 8 KiB via 16-bit i2ctransfer (records live at base 0x400)...")
    blob = read_eeprom_16bit(s, bus, SPD_ADDR, 0x0000, 0x2000)
    raw_path = LOGS / "ddr-eeprom-0x57-full.bin"
    raw_path.write_bytes(blob)
    log(f"saved {len(blob)} bytes -> {raw_path}")
    log("identity region 0x000-0x040:\n" + _hex(blob[0x000:0x040], 0x000))
    log("DDR-records region 0x400-0x480:\n" + _hex(blob[0x400:0x480], 0x400))

    off, spd = find_ddr4_spd(blob)
    if off is None:
        log("no DDR4 SPD (byte2==0x0C) found in the dump — inspect the hexdumps above")
        s.close(); return 1
    log(f"DDR4 SPD found at EEPROM offset 0x{off:04x} ({len(spd)} bytes captured)")
    log("\n===== DDR4 SPD decode =====\n" + decode_ddr4_spd(spd))
    log("\nDONE — full log + raw dump (ddr-eeprom-0x57-full.bin) in tmp/logs/")
    s.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
