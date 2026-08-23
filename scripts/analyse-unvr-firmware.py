#!/usr/bin/env python3
"""Parse the UNVR firmware container, extract sections, unpack the kernel.

Format is Ubiquiti's own, published under GPL in mkfwimage (`src/fw.h` in
openwrt/firmware-utils). Verified byte-for-byte against UNVR 5.1.25.

  header  268 B (0x10C): magic[4] "UBNT" + version[256] + crc32 u32 BE @0x104 + pad
  record   56 B (0x38):  magic[4] + name[16] + pad[12] + 6x u32 BE
                         (memaddr, index, baseaddr, entryaddr, data_size, part_size)
           payload data_size B, then 8 B trailer: crc32 u32 BE + pad
           stride = 0x38 + data_size + 8
  END.    12 B / ENDS 264 B (RSA-signed) terminator

Walk by the LENGTH CHAIN, never by scanning for magics: `PART`/`FILE` occur inside
compressed payloads, and the tag differs between firmware revisions (4.1.9 used
FILE for rootfs, 5.1.25 uses PART). The tag is cosmetic; the chain is the structure.

Kernel lives in the section named `kernel`: a legacy U-Boot uImage (64 B header,
magic 0x27051956), gzip-compressed arm64 flat Image, load/entry 0x04080000.

Run: ./scripts/analyse-unvr-firmware.py [--extract]
"""

import argparse
import gzip
import struct
import subprocess
import sys
import zlib
from datetime import datetime, timezone
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DEFAULT_BIN = REPO / "sources" / "UNVR-5.1.25.bin"
LOGS = REPO / "tmp" / "logs"
SECTIONS = REPO / "tmp" / "sections"

HDR_SIZE = 0x10C
REC_SIZE = 0x38
TRAILER = 8
TERMINATORS = {b"END.": 12, b"ENDS": 264}
UIMAGE_MAGIC = 0x27051956
IKCFG_ST = b"IKCFG_ST"


def log(msg):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {msg}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    with (LOGS / "analyse-unvr-firmware.log").open("a") as fh:
        fh.write(line + "\n")


def parse_header(data):
    if data[:4] not in (b"UBNT", b"OPEN", b"GEOS"):
        sys.exit(f"not a Ubiquiti container: magic {data[:4]!r}")
    version = data[4:0x104].split(b"\x00", 1)[0].decode("ascii", "replace")
    stored = struct.unpack_from(">I", data, 0x104)[0]
    calc = zlib.crc32(data[:0x104]) & 0xFFFFFFFF
    log(f"container {data[:4].decode()}  version {version}")
    log(
        f"header crc {stored:#010x} {'OK' if stored == calc else f'MISMATCH (calc {calc:#010x})'}"
    )
    return version


def walk(data):
    """Yield section dicts by following the length chain."""
    off = HDR_SIZE
    while off < len(data):
        magic = data[off : off + 4]
        if magic in TERMINATORS:
            if magic == b"END.":
                stored = struct.unpack_from(">I", data, off + 4)[0]
                calc = zlib.crc32(data[:off]) & 0xFFFFFFFF
                log(
                    f"@{off:#010x} END.  image crc {stored:#010x} "
                    f"{'OK' if stored == calc else f'MISMATCH (calc {calc:#010x})'}"
                )
            else:
                log(f"@{off:#010x} ENDS  256-byte RSA signature, no crc")
            yield {
                "offset": off,
                "magic": magic.decode(),
                "name": "",
                "terminator": True,
            }
            return
        if len(magic) < 4 or not magic.isalnum() and magic not in (b"END.",):
            log(f"@{off:#010x} unexpected magic {magic!r} - stopping")
            return
        name = (
            data[off + 4 : off + 0x14].split(b"\x00", 1)[0].decode("ascii", "replace")
        )
        memaddr, index, baseaddr, entryaddr, dsize, psize = struct.unpack_from(
            ">6I", data, off + 0x20
        )
        end = off + REC_SIZE + dsize
        if end + TRAILER > len(data):
            log(
                f"@{off:#010x} {magic!r} '{name}' data_size {dsize} overruns file - stopping"
            )
            return
        stored = struct.unpack_from(">I", data, end)[0]
        calc = zlib.crc32(data[off:end]) & 0xFFFFFFFF
        ok = stored == calc
        log(
            f"@{off:#010x} {magic.decode():4s} {name:12s} idx={index} "
            f"data={dsize} ({dsize / 1048576:.2f} MB) part={psize} "
            f"mem={memaddr:#010x} base={baseaddr:#010x} entry={entryaddr:#010x} "
            f"crc {'OK' if ok else f'MISMATCH {stored:#010x}/{calc:#010x}'}"
        )
        yield {
            "offset": off,
            "magic": magic.decode(),
            "name": name,
            "index": index,
            "memaddr": memaddr,
            "baseaddr": baseaddr,
            "entryaddr": entryaddr,
            "data_size": dsize,
            "part_size": psize,
            "payload": (off + REC_SIZE, dsize),
            "crc_ok": ok,
            "terminator": False,
        }
        off = end + TRAILER


def unpack_uimage(blob):
    """Strip the 64-byte legacy uImage header, return (info, payload)."""
    magic, _hcrc, tstamp, size, load, ep, _dcrc, _os, arch, _typ, comp = (
        struct.unpack_from(">7I4B", blob, 0)
    )
    if magic != UIMAGE_MAGIC:
        return None, blob
    name = blob[32:64].split(b"\x00", 1)[0].decode("ascii", "replace")
    info = {
        "size": size,
        "load": load,
        "entry": ep,
        "comp": comp,
        "arch": arch,
        "name": name,
        "created": datetime.fromtimestamp(tstamp, timezone.utc).isoformat(),
    }
    log(
        f"uImage '{name}' size={size} load={load:#010x} entry={ep:#010x} "
        f"comp={comp} arch={arch} created={info['created']}"
    )
    return info, blob[64 : 64 + size]


def find_initramfs(image):
    """The kernel is built with CONFIG_INITRAMFS_SOURCE, so a gzip'd cpio is
    embedded in the Image. Try every gzip stream, keep ones that inflate to
    newc-format cpio (magic '070701')."""
    out = []
    pos = 0
    while True:
        i = image.find(b"\x1f\x8b\x08", pos)
        if i == -1:
            break
        pos = i + 1
        try:
            d = zlib.decompressobj(zlib.MAX_WBITS | 16)
            blob = d.decompress(image[i:], 200 << 20)
        except zlib.error:
            continue
        if blob[:6] == b"070701" and len(blob) > 4096:
            log(f"embedded initramfs: gzip @{i} -> {len(blob)} bytes of newc cpio")
            out.append((i, blob))
    return out


def find_ikconfig(image):
    """Extract the embedded .config if CONFIG_IKCONFIG was set."""
    i = image.find(IKCFG_ST)
    if i == -1:
        return None
    start = i + len(IKCFG_ST)
    try:
        return gzip.decompress(image[start:])
    except (OSError, EOFError, zlib.error):
        # gzip stream ends at IKCFG_ED; decompressobj tolerates the trailing bytes
        d = zlib.decompressobj(zlib.MAX_WBITS | 16)
        try:
            return d.decompress(image[start:])
        except zlib.error as exc:
            log(f"IKCFG_ST found at {i} but inflate failed: {exc}")
            return None


if __name__ == "__main__":
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--extract", action="store_true", help="write section payloads to tmp/sections/"
    )
    ap.add_argument(
        "bin",
        nargs="?",
        type=Path,
        default=DEFAULT_BIN,
        help="firmware .bin (default: sources/UNVR-5.1.25.bin)",
    )
    ap.add_argument(
        "--sections-dir",
        type=Path,
        default=SECTIONS,
        help="where --extract writes (default: tmp/sections/)",
    )
    args = ap.parse_args()

    BIN = args.bin
    SECTIONS = args.sections_dir
    if not BIN.exists():
        sys.exit(f"absent: {BIN} - run scripts/fetch-unvr-firmware.py first")
    LOGS.mkdir(parents=True, exist_ok=True)
    data = BIN.read_bytes()
    log(f"--- analyse {BIN.name} ({len(data)} bytes) ---")
    parse_header(data)

    sections = [s for s in walk(data)]
    real = [s for s in sections if not s["terminator"]]
    log(f"{len(real)} section(s), {sum(not s['crc_ok'] for s in real)} crc failure(s)")

    if args.extract:
        SECTIONS.mkdir(parents=True, exist_ok=True)
        for s in real:
            start, size = s["payload"]
            blob = data[start : start + size]
            out = SECTIONS / f"{s['index']:02d}-{s['name']}.bin"
            out.write_bytes(blob)
            log(f"wrote {out} ({len(blob)} bytes)")

            if s["name"] == "kernel":
                info, payload = unpack_uimage(blob)
                if info:
                    (SECTIONS / "kernel-Image.gz").write_bytes(payload)
                    log(f"wrote {SECTIONS / 'kernel-Image.gz'}")
                    try:
                        image = gzip.decompress(payload)
                    except (OSError, EOFError, zlib.error) as exc:
                        log(f"gunzip failed: {exc}")
                        image = None
                    if image:
                        img = SECTIONS / "kernel-Image"
                        img.write_bytes(image)
                        arm64 = image[0x38:0x3C] == b"ARM\x64"
                        log(
                            f"wrote {img} ({len(image)} bytes), arm64 magic {'OK' if arm64 else 'ABSENT'}"
                        )
                        cfg = find_ikconfig(image)
                        if cfg:
                            c = SECTIONS / "kernel.config"
                            c.write_bytes(cfg)
                            log(f"wrote {c} ({len(cfg)} bytes) - embedded IKCONFIG")
                        else:
                            log("no embedded IKCONFIG (CONFIG_IKCONFIG not set)")

                        for n, (at, blob) in enumerate(find_initramfs(image)):
                            cpio = SECTIONS / f"initramfs-{n}.cpio"
                            cpio.write_bytes(blob)
                            log(f"wrote {cpio} ({len(blob)} bytes, gzip @{at})")
                            d = SECTIONS / f"initramfs-{n}"
                            d.mkdir(exist_ok=True)
                            r = subprocess.run(
                                [
                                    "cpio",
                                    "-idmu",
                                    "--no-absolute-filenames",
                                    "-F",
                                    str(cpio),
                                ],
                                cwd=d,
                                capture_output=True,
                                text=True,
                                check=False,
                            )
                            log(
                                f"cpio extract -> {d}: {r.stderr.strip().splitlines()[-1] if r.stderr else 'ok'}"
                            )
    log("done")
