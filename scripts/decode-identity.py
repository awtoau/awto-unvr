#!/usr/bin/env python3
"""Decode UNVR (Alpine V2, sysid 0xea16) SPI-NOR identity partitions.

Partitions decoded (5.1.25 generation dump):
  mtd03 Factory  - blank check
  mtd04 EEPROM   - header, UBNT redundant block, UBNT-TLV block, RSA priv key
  mtd06 config   - ext4 superblock (parse only, no mount)
  mtd07 cksum    - blank/zero check

Secrets policy: the RSA key at 0xe004 is a PRIVATE key. This script reports
ONLY offset/length/format/math properties. It NEVER prints or carves key bytes.

Logs to tmp/logs/decode-identity.log. Field map (no secrets) -> docs/nor-reference/.
"""

import collections
import itertools
import json
import logging
import math
import os
import struct
import sys
import uuid
import zlib

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DUMP = os.path.join(
    REPO, "images/mtd/UNVR-74acb941a811-sysidea16-20260815-212945-post-5.1.25-final"
)
LOGDIR = os.path.join(REPO, "tmp/logs")
os.makedirs(LOGDIR, exist_ok=True)
logging.basicConfig(
    level=logging.INFO,
    format="%(message)s",
    handlers=[
        logging.FileHandler(os.path.join(LOGDIR, "decode-identity.log")),
        logging.StreamHandler(sys.stdout),
    ],
)
log = logging.getLogger().info


def part(name):
    for f in os.listdir(DUMP):
        if name in f and f.endswith(".img"):
            return open(os.path.join(DUMP, f), "rb").read()
    raise FileNotFoundError(name)


def entropy(b):
    if not b:
        return 0.0
    c = collections.Counter(b)
    n = len(b)
    return -sum((v / n) * math.log2(v / n) for v in c.values())


def blank_verdict(name):
    d = part(name)
    c = collections.Counter(d)
    ff = c.get(0xFF, 0)
    z = c.get(0, 0)
    if ff == len(d):
        v = "100% 0xFF (erased/blank)"
    elif z == len(d):
        v = "100% 0x00 (actively zeroed)"
    else:
        v = f"WRITTEN ff={ff * 100 / len(d):.1f}% zero={z * 100 / len(d):.1f}% distinct={len(c)}"
    log(f"[{name}] len={len(d)} -> {v}")
    return d


def crc_sweep(data, stored):
    """Try zlib CRC-32 (reflected, poly 0x04C11DB7) over data; return match str."""
    got = zlib.crc32(data) & 0xFFFFFFFF
    return got, (got == stored)


def decode_eeprom():
    d = part("mtd04-EEPROM")
    fm = {"partition": "mtd04 EEPROM", "size": len(d), "fields": []}
    log("\n=== EEPROM (mtd04) ===")
    mac = ":".join(f"{x:02x}" for x in d[0:6])
    mac2 = ":".join(f"{x:02x}" for x in d[6:12])
    boardid = d[0x0C:0x0E].hex()
    hwrev = d[0x0E:0x10].hex()
    devid = d[0x10:0x14].hex()
    log(f"0x0000 base MAC       {mac}")
    log(f"0x0006 MAC+2          {mac2}")
    log(f"0x000C board id       0x{boardid}")
    log(f"0x000E hw rev         0x{hwrev}")
    log(f"0x0010 device id      0x{devid}")
    fm["fields"] += [
        {"off": "0x0000", "len": 6, "name": "base MAC", "value": mac},
        {
            "off": "0x0006",
            "len": 6,
            "name": "MAC+2 (base MAC, first octet +2)",
            "value": mac2,
        },
        {"off": "0x000C", "len": 2, "name": "board id", "value": "0x" + boardid},
        {"off": "0x000E", "len": 2, "name": "hw rev", "value": "0x" + hwrev},
        {"off": "0x0010", "len": 4, "name": "device id", "value": "0x" + devid},
        {"off": "0x0014", "len": 0x7FEC, "name": "padding 0x00", "value": "zeroed"},
    ]
    # UBNT redundant block @0x8000
    log("\n-- UBNT redundant block @0x8000 --")
    assert d[0x8000:0x8004] == b"UBNT"
    crc_stored = int.from_bytes(d[0x8004:0x8008], "big")
    blen = int.from_bytes(d[0x8008:0x800C], "big")
    r_ver = d[0x800C:0x800E].hex()
    r_ver2 = d[0x800E:0x8010].hex()
    r_hwrev = d[0x8010:0x8012].hex()
    r_boardid = d[0x8012:0x8014].hex()
    r_devid = d[0x8014:0x8018].hex()
    r_mac = ":".join(f"{x:02x}" for x in d[0x8018:0x801E])
    r_maccnt = int.from_bytes(d[0x801E:0x8020], "little")  # 02 00 = 2
    log("0x8000 magic          'UBNT'")
    log(f"0x8004 crc32          0x{crc_stored:08x}")
    log(f"0x8008 length         0x{blen:x} ({blen})")
    log(f"0x800C ver?           0x{r_ver} 0x{r_ver2}")
    log(f"0x8010 hw rev         0x{r_hwrev}")
    log(f"0x8012 board id       0x{r_boardid}")
    log(f"0x8014 device id      0x{r_devid}")
    log(f"0x8018 base MAC       {r_mac}")
    log(f"0x801E MAC count      {r_maccnt}")
    # CRC over payload after crc field
    for lo, hi, desc in [
        (0x8008, 0x8008 + blen, "0x8008..+len"),
        (0x800C, 0x800C + blen, "0x800c..+len"),
        (0x8008, 0x8020, "0x8008..0x8020"),
    ]:
        got, ok = crc_sweep(d[lo:hi], crc_stored)
        log(f"   crc32({desc}) = 0x{got:08x} match={ok}")
    fm["fields"] += [
        {"off": "0x8000", "len": 4, "name": "UBNT magic", "value": "UBNT"},
        {"off": "0x8004", "len": 4, "name": "crc32", "value": f"0x{crc_stored:08x}"},
        {"off": "0x8008", "len": 4, "name": "length", "value": f"0x{blen:x}"},
        {
            "off": "0x800C",
            "len": 4,
            "name": "version words",
            "value": f"0x{r_ver} 0x{r_ver2}",
        },
        {"off": "0x8010", "len": 2, "name": "hw rev", "value": "0x" + r_hwrev},
        {"off": "0x8012", "len": 2, "name": "board id", "value": "0x" + r_boardid},
        {"off": "0x8014", "len": 4, "name": "device id", "value": "0x" + r_devid},
        {"off": "0x8018", "len": 6, "name": "base MAC", "value": r_mac},
        {"off": "0x801E", "len": 2, "name": "MAC count", "value": r_maccnt},
    ]
    # UBNT-TLV block @0xd000
    log("\n-- UBNT-TLV block @0xd000 (ONIE 'TlvInfo' magic) --")
    assert d[0xD000:0xD007] == b"TlvInfo"
    tver = d[0xD008]
    tlen = int.from_bytes(d[0xD009:0xD00B], "big")
    log(f"0xd000 magic 'TlvInfo\\0' ver={tver} total_len=0x{tlen:x}")
    o = 0xD00B
    endt = 0xD00B + tlen
    tlvs = []
    while o < endt:
        t = d[o]
        l = d[o + 1]
        v = d[o + 2 : o + 2 + l]
        printable = all(32 <= b < 127 for b in v)
        disp = v.decode() if printable else v.hex()
        log(f"   @0x{o:05x} type=0x{t:02x} len={l} val={disp}")
        tlvs.append({"off": f"0x{o:05x}", "type": f"0x{t:02x}", "len": l, "val": disp})
        o += 2 + l
    crc_stored_t = int.from_bytes(d[endt - 4 : endt], "big")
    for lo, hi, desc in [
        (0xD000, endt - 4, "0xd000..crc"),
        (0xD000, endt, "0xd000..end incl crc"),
    ]:
        got, ok = crc_sweep(d[lo:hi], crc_stored_t)
        log(f"   crc32({desc}) = 0x{got:08x} stored=0x{crc_stored_t:08x} match={ok}")
    fm["tlvinfo"] = {
        "off": "0xd000",
        "version": tver,
        "total_len": tlen,
        "crc": f"0x{crc_stored_t:08x}",
        "tlvs": tlvs,
    }
    # high-entropy blob @0x9000
    blob = d[0x9000:0xC000]
    log(
        f"\n-- blob @0x9000..0xc000 len={len(blob)} entropy={entropy(blob):.3f} b/byte "
        f"(unidentified, near-random) --"
    )
    fm["fields"].append(
        {
            "off": "0x9000",
            "len": len(blob),
            "name": "unidentified high-entropy blob",
            "value": f"entropy {entropy(blob):.2f} b/byte",
        }
    )
    # RSA private key @0xe004 - report ONLY structure, NEVER bytes
    log("\n-- RSA private key @0xe004 (SSH-wire format) [NO KEY MATERIAL] --")
    hdr = d[0xE004:0xE00B]
    log(
        f"0xe004 7-byte prefix   {hdr.hex()} ('{''.join(chr(b) if 32 <= b < 127 else '.' for b in hdr)}')"
    )
    o = 0xE00B
    end = 0xE330
    while d[end - 1] == 0xFF:
        end -= 1
    fields = []
    while o < end and o + 4 <= end:
        ln = int.from_bytes(d[o : o + 4], "big")
        if ln == 0 or ln > 4096 or o + 4 + ln > end:
            break
        fields.append((o, ln, d[o + 4 : o + 4 + ln]))
        o += 4 + ln
    nums = []
    for off, ln, v in fields:
        if all(32 <= b < 127 for b in v) and ln < 32:
            log(f"   @0x{off:05x} len={ln} tag='{v.decode()}'")
        else:
            iv = int.from_bytes(v, "big")
            log(f"   @0x{off:05x} len={ln} mpint bits={iv.bit_length()}")
            nums.append(iv)
    N = next((x for x in nums if x.bit_length() > 2000), None)
    primes = [x for x in nums if 900 < x.bit_length() < 1100]
    verified = (
        any(a * b == N for a, b in itertools.combinations(primes, 2)) if N else False
    )
    log(f"   modulus bits={N.bit_length() if N else '?'}  p*q==n VERIFIED={verified}")
    log(f"   total key region 0xe004..0x{end:05x} = {end - 0xE004} bytes")
    fm["rsa_key"] = {
        "off": "0xe004",
        "end": f"0x{end:05x}",
        "total_len": end - 0xE004,
        "format": "SSH-wire (length-prefixed big-endian mpints), NOT DER PKCS#1",
        "layout": "7B prefix, 'ssh-rsa' tag, e, n(2048b), d, p(1024b), q(1024b)",
        "modulus_bits": N.bit_length() if N else None,
        "pq_eq_n": verified,
        "NOTE": "PRIVATE key - no bytes stored in this map",
    }
    fm["content_end"] = 0xE330
    return fm


def decode_config():
    d = part("mtd06-config")
    log("\n=== config (mtd06) ext4 (superblock parse, NOT mounted) ===")
    sb = d[0x400:0x800]
    magic = struct.unpack_from("<H", sb, 0x38)[0]
    blkcnt = struct.unpack_from("<I", sb, 4)[0]
    logbs = struct.unpack_from("<I", sb, 0x18)[0]
    bs = 1024 << logbs
    u = uuid.UUID(bytes=sb[0x68:0x78])
    label = sb[0x78:0x88].split(b"\x00")[0].decode("latin1")
    lastmnt = sb[0x88:0xC8].split(b"\x00")[0].decode("latin1")
    log(f"magic=0x{magic:04x} ext4={'OK' if magic == 0xEF53 else 'NO'}")
    log(f"blocks={blkcnt} block_size={bs} fs_bytes={blkcnt * bs}")
    log(f"UUID={u} label='{label}' last_mounted='{lastmnt}'")
    return {
        "partition": "mtd06 config",
        "fs": "ext4",
        "uuid": str(u),
        "label": label,
        "last_mounted": lastmnt,
        "block_size": bs,
        "blocks": blkcnt,
    }


def main():
    log("UNVR identity decode (5.1.25 generation)")
    blank_verdict("mtd03-Factory")
    blank_verdict("mtd07-cksum")
    fm_eeprom = decode_eeprom()
    fm_config = decode_config()
    # carve field map (NO secrets) to docs/nor-reference/
    out = os.path.join(REPO, "docs/nor-reference/eeprom-fieldmap.json")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    with open(out, "w") as f:
        json.dump({"eeprom": fm_eeprom, "config": fm_config}, f, indent=2)
    log(f"\nfield map -> {out}")


if __name__ == "__main__":
    main()
