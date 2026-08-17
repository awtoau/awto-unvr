#!/usr/bin/env python3
"""Userspace proper-driver read of the 0x57 config EEPROM via at24 (no reboot).

The current DTB has no eeprom@57 node, so at24 isn't bound. We instantiate it at
runtime (i2c new_device) on the parent PLD bus, read the whole 8 KiB through the
nvmem sysfs (/sys/bus/i2c/devices/0-0057/eeprom), and byte-compare against the
raw i2ctransfer dump we already trust (docs/nor-reference/ddr-config-eeprom-0x57-8k.bin).
Match => the at24/nvmem path our new DTB adds will read the exact same bytes.
Read-only + cleans up (delete_device). Output -> tmp/logs/at24-verify-woomera.log.
"""
from __future__ import annotations
import sys, hashlib
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _console as con  # noqa: E402
from _repo import LOGS  # noqa: E402

REF = Path(__file__).resolve().parent.parent / "docs/nor-reference/ddr-config-eeprom-0x57-8k.bin"
BUS = 0          # 0x57 sits on the parent PLD bus (i2c-0), confirmed by i2cdetect
ADDR = 0x57
LOG = LOGS / "at24-verify-woomera.log"


def log(m):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    LOG.open("a").write(line + "\n")


def main():
    ref = REF.read_bytes() if REF.exists() else b""
    s = con.connect()
    con.login(s)
    dev = f"/sys/bus/i2c/devices/{BUS}-00{ADDR:02x}"
    sysfs_new = f"/sys/bus/i2c/devices/i2c-{BUS}/new_device"
    sysfs_del = f"/sys/bus/i2c/devices/i2c-{BUS}/delete_device"

    con.sh(s, "modprobe at24")
    # instantiate at24 as a 24c64 (8 KiB, 16-bit addressed) at 0x57
    rc, out = con.sh(s, f"echo 24c64 0x{ADDR:02x} > {sysfs_new} 2>&1; echo done", 15)
    log(f"new_device rc={rc}: {out}")
    rc, eepath = con.sh(s, f"ls {dev}/eeprom 2>/dev/null || echo MISSING")
    log(f"eeprom sysfs: {eepath}")
    if "MISSING" in eepath:
        log("at24 did not bind — 0x57 may be claimed elsewhere or wrong bus. Inspect dmesg.")
        con.sh(s, f"dmesg | tail -5")
        s.close(); return 1

    # sha256 ON THE BOX = one line, immune to the console's async journald noise
    # (transferring 8 KiB of od over serial gets shredded). Proves byte-identity.
    rc, h = con.sh(s, f"sha256sum {dev}/eeprom", 30)
    dev_hash = h.split()[0] if h.split() else ""
    log(f"at24 nvmem sha256: {dev_hash}")
    rc, sample = con.sh(s, f"dd if={dev}/eeprom bs=1 skip=1024 count=16 2>/dev/null | od -An -tx1")
    log(f"at24 nvmem 0x400 sample: {sample.strip()}")

    # cleanup: remove the runtime device (leave the box as we found it)
    con.sh(s, f"echo 0x{ADDR:02x} > {sysfs_del} 2>&1; echo done", 10)
    log("delete_device done (box restored)")
    s.close()

    ref_hash = hashlib.sha256(ref).hexdigest() if ref else ""
    if ref_hash and dev_hash:
        same = ref_hash == dev_hash
        log("COMPARE sha256 (at24 nvmem vs raw i2ctransfer dump): "
            + ("MATCH — proper-driver read is byte-identical" if same else "DIFFER"))
        log(f"  ref  {ref_hash}")
        return 0 if same else 2
    log("no reference hash or no device hash to compare")
    return 1


if __name__ == "__main__":
    sys.exit(main())
