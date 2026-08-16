#!/usr/bin/env python3
"""Bring up the 8 TB spinning drives, SMART-check them, spin down the idle ones.

Runs ON the device (woomera / Fedora), not the host. The bays auto-power at boot
(DTB gpio-hog on PCA9575 @0x21 pwren lines 0-3), so the drives are already up;
this checks health then parks anything not in use to save power/heat/wear.

Default: check + spin down idle drives. See --help for individual actions.

  hdd-manage.py            # SMART report, then spin down idle WD drives
  hdd-manage.py --check    # SMART report only, touch nothing
  hdd-manage.py --selftest # kick a SMART short self-test on each (spins them up)
  hdd-manage.py --keep-up  # check only, do not spin anything down

"In use" = a partition mounted, or the disk is a member of an active md array.
Such drives are never spun down.

Bay POWER-OFF (cutting the PCA9575 pwren line) is NOT done here: the DTB hogs
those lines output-high, so they are not user-controllable at runtime. Spin-down
(hdparm -Y standby) is the available power saving. To cut bay power at runtime,
drop the gpio-hog from the DTB and drive the line via gpiod - tracked separately.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

SPIN_MODEL_HINT = "WD8"          # WD82PURZ etc.; spinning 8 TB WD Purple
SIZE_MIN_BYTES = 6_000_000_000_000   # >6 TB => the 8 TB platters, not the SSD/USB


def sh(*cmd: str) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, capture_output=True, text=True)


def read(path: str) -> str:
    try:
        return Path(path).read_text().strip()
    except OSError:
        return ""


def spinning_disks() -> list[str]:
    """Return /dev/sdX for rotational disks of platter size (excludes SSD, USB)."""
    out = []
    for blk in sorted(Path("/sys/block").glob("sd*")):
        name = blk.name
        if read(f"/sys/block/{name}/queue/rotational") != "1":
            continue
        # size is in 512-byte sectors
        sectors = read(f"/sys/block/{name}/size")
        if not sectors or int(sectors) * 512 < SIZE_MIN_BYTES:
            continue
        out.append(f"/dev/{name}")
    return out


def mounted_or_md(dev: str) -> str | None:
    """Return a reason string if dev (or a partition of it) is in use, else None."""
    base = Path(dev).name
    # any partition mounted?
    fm = sh("findmnt", "-rno", "SOURCE")
    for src in fm.stdout.split():
        if Path(src).name.startswith(base):
            return f"mounted ({src})"
    # member of an active md array?
    mdstat = read("/proc/mdstat")
    if base in mdstat:
        return "md array member"
    return None


def smart_report(dev: str) -> dict:
    """smartctl health + key attributes as a dict (best-effort)."""
    r = sh("smartctl", "-H", "-A", "-j", dev)
    try:
        j = json.loads(r.stdout)
    except (json.JSONDecodeError, ValueError):
        return {"device": dev, "error": "smartctl parse failed", "raw": r.stdout[:200]}
    passed = j.get("smart_status", {}).get("passed")
    attrs = {a["name"]: a["raw"]["value"]
             for a in j.get("ata_smart_attributes", {}).get("table", [])}
    temp = j.get("temperature", {}).get("current")
    return {
        "device": dev,
        "model": j.get("model_name", "?"),
        "serial": j.get("serial_number", "?"),
        "health": "PASSED" if passed else ("FAILED" if passed is False else "?"),
        "temp_c": temp,
        "power_on_hours": j.get("power_on_time", {}).get("hours"),
        "reallocated": attrs.get("Reallocated_Sector_Ct"),
        "pending": attrs.get("Current_Pending_Sector"),
        "uncorrectable": attrs.get("Offline_Uncorrectable"),
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    g = ap.add_mutually_exclusive_group()
    g.add_argument("--check", action="store_true", help="SMART report only")
    g.add_argument("--keep-up", action="store_true", help="check, never spin down")
    g.add_argument("--selftest", action="store_true", help="SMART short self-test each")
    a = ap.parse_args()

    disks = spinning_disks()
    if not disks:
        print("no platter-class spinning disks found", file=sys.stderr)
        return 1
    print(f"spinning disks: {', '.join(disks)}")

    fail = False
    for dev in disks:
        rep = smart_report(dev)
        flags = []
        for k in ("reallocated", "pending", "uncorrectable"):
            v = rep.get(k)
            if v not in (None, "0", 0):
                flags.append(f"{k}={v}")
        if rep.get("health") == "FAILED":
            fail = True
        badge = "  <-- ATTENTION" if (rep.get("health") == "FAILED" or flags) else ""
        print(f"  {dev}  {rep.get('model','?')}  health={rep.get('health','?')}  "
              f"temp={rep.get('temp_c','?')}C  poh={rep.get('power_on_hours','?')}"
              f"{('  ' + ' '.join(flags)) if flags else ''}{badge}")
        if rep.get("error"):
            print(f"      {rep['error']}")

    if a.selftest:
        for dev in disks:
            r = sh("smartctl", "-t", "short", dev)
            print(f"  self-test started on {dev}: {'ok' if r.returncode == 0 else r.stderr.strip()}")
        print("  (results in ~2 min: smartctl -l selftest <dev>)")
        return 1 if fail else 0

    if a.check or a.keep_up:
        return 1 if fail else 0

    # default: spin down idle drives
    for dev in disks:
        reason = mounted_or_md(dev)
        if reason:
            print(f"  {dev}: in use ({reason}) - left spinning")
            continue
        r = sh("hdparm", "-Y", dev)   # -Y = immediate standby (spin down)
        print(f"  {dev}: spun down" if r.returncode == 0
              else f"  {dev}: spindown failed: {r.stderr.strip()}")
    return 1 if fail else 0


if __name__ == "__main__":
    sys.exit(main())
