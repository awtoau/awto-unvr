#!/usr/bin/env python3
"""Enumerate every RPS pin-table entry in rpsd's .data, recovering the pin-id enum.

Each pin descriptor is 0x40 bytes of 8 LE64 words:
  +0x00 pin id (the RPS_PIN_* enum value)
  +0x08 -> name string in .rodata ("RPS_PIN_...")
  +0x10 -> backend "compatible" string ("gpiolib-sysfs", "gpio-pca953x", ...)
  +0x18 location: lo32 = offset within the chip, hi32 = gpio base / absolute gpio
  +0x20 flags
  +0x28 backend-private (i2c bus/addr for expander backends)
  +0x30 bomrev_min (u32) | bomrev_max (u32) in the high half
  +0x38 reserved

Layout confirmed against rps_drv_pin_init (Ghidra @0x10d070): stride 0x40, the
bomrev gate reads *(u32*)(entry+0x30) / *(u32*)(entry+0x34), the backend name is
entry+0x10 and the location word entry+0x18.

Scans .data for any 0x40-aligned word pair where +0x08 points at a "RPS_PIN_"
string and +0x10 points at a known backend name, so it finds every board's table
without needing each board descriptor's address. Output -> tmp/logs/rps-pin-table.log
"""

from __future__ import annotations

import logging
import struct
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
BIN = REPO / "tmp" / "rps-spec" / "root" / "sbin" / "rpsd"
LOG = REPO / "tmp" / "logs" / "rps-pin-table.log"

# readelf -S: .rodata addr==off 0x1d728 len 0x50be ; .data addr 0x383e8 off 0x283e8
ROD_V, ROD_L = 0x1D728, 0x50BE
DAT_V, DAT_F, DAT_L = 0x383E8, 0x283E8, 0x45A8

BACKENDS = {
    "gpiolib-sysfs",
    "gpio-pca953x",
    "gpio-custompath",
    "libubnt-pindrv",
    "hwmon-adt7475",
    "hwmon-ina230",
    "hwmon-ina237",
    "hwmon-isl28022",
    "uart-common",
}

log = logging.getLogger(__name__)


def main() -> int:
    LOG.parent.mkdir(parents=True, exist_ok=True)
    logging.basicConfig(
        level=logging.INFO,
        format="%(message)s",
        handlers=[logging.FileHandler(LOG), logging.StreamHandler(sys.stdout)],
    )
    d = BIN.read_bytes()

    def cstr(v: int) -> str | None:
        if not (ROD_V <= v < ROD_V + ROD_L):
            return None
        e = d.find(b"\x00", v)
        try:
            return d[v:e].decode("ascii")
        except UnicodeDecodeError:
            return None

    rows = []
    for off in range(DAT_F, DAT_F + DAT_L - 0x40, 8):
        w = struct.unpack_from("<8Q", d, off)
        name = cstr(w[1])
        backend = cstr(w[2])
        if not name or not name.startswith("RPS_PIN_"):
            continue
        if backend not in BACKENDS:
            continue
        rows.append(
            dict(
                off=off,
                vaddr=off + (DAT_V - DAT_F),
                pin_id=w[0],
                name=name,
                backend=backend,
                chip_off=w[3] & 0xFFFFFFFF,
                gpio=w[3] >> 32,
                flags=w[4],
                priv=w[5],
                bom_min=w[6] & 0xFFFFFFFF,
                bom_max=w[6] >> 32,
            )
        )

    log.info("%d pin descriptors found in .data", len(rows))
    log.info(
        "%-8s %-8s %-3s %-26s %-16s %-6s %-8s %-6s %s",
        "file",
        "vaddr",
        "id",
        "name",
        "backend",
        "gpio",
        "flags",
        "chip",
        "bomrev range",
    )
    for r in rows:
        log.info(
            "%08x %08x %-3d %-26s %-16s %-6d 0x%-6x %-6d [0x%08x..0x%08x]",
            r["off"],
            r["vaddr"],
            r["pin_id"],
            r["name"],
            r["backend"],
            r["gpio"],
            r["flags"],
            r["chip_off"],
            r["bom_min"],
            r["bom_max"],
        )

    # collapse to the id -> name enum
    enum: dict[int, set[str]] = {}
    for r in rows:
        enum.setdefault(r["pin_id"], set()).add(r["name"])
    log.info("\nRecovered RPS_PIN_* enum (id -> name):")
    for k in sorted(enum):
        log.info("  %2d  %s", k, ", ".join(sorted(enum[k])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
