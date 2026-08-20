#!/usr/bin/env python3
"""Rescue the UNVR boot USB (Phison "UF3 8GB") to a disk image.

- Device has CONFIRMED medium errors (unrecovered read error @ LBA 229376).
  Plain dd is unusable: no retry, not resumable. ddrescue + mapfile is.
- Identity-checked by USB serial before any read; /dev/sdX is not stable.
- Two passes: fast skip-on-error, then scrape/retry. Both resume from mapfile.

Run: sudo ./scripts/rescue-unvr-usb.py [rescue|status|manifest]
"""

import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
IMAGES = REPO / "images"
LOGS = REPO / "tmp" / "logs"

# Identity of the one disk this script may ever read. Guards against /dev/sdX drift.
BY_ID = Path("/dev/disk/by-id/usb-_UF3_8GB_07190176402AEE98-0:0")
EXPECT_SECTORS = 15482880  # 7.93 GB / 7.38 GiB, from sd 9:0:0:0 [sdd] attach
EXPECT_BYTES = EXPECT_SECTORS * 512

STEM = "unvr-usb-07190176402AEE98"
IMG = IMAGES / f"{STEM}.img"
MAP = IMAGES / f"{STEM}.map"

# ddrescue --timeout: abort if NO sector reads successfully for this long.
# Measured good-region rate 770 kB/s (2026-08-15), i.e. a 64 KiB cluster lands
# in ~85 ms. Nothing at all for 120 s (pass 1) / 300 s (scrape) means the
# controller has wedged, not that it is merely slow.
# On expiry: ddrescue exits non-zero, mapfile is intact, rerunning resumes.
TIMEOUT_SKIP = 120
TIMEOUT_SCRAPE = 300


def log(msg):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {msg}"
    print(line, flush=True)
    with LOGFILE.open("a") as fh:
        fh.write(line + "\n")


def run(cmd, check=True):
    """Run a command, teeing combined output to console and log."""
    log(f"$ {' '.join(str(c) for c in cmd)}")
    with LOGFILE.open("a") as fh:
        proc = subprocess.run(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
        )
        if proc.stdout:
            sys.stdout.write(proc.stdout)
            sys.stdout.flush()
            fh.write(proc.stdout)
    if check and proc.returncode != 0:
        log(f"!! exit {proc.returncode}")
    return proc


def resolve_device():
    """Return the real device path, or exit if identity does not match."""
    if not BY_ID.exists():
        sys.exit(
            f"FATAL: {BY_ID} absent - UNVR USB not plugged in (or enumerated elsewhere)"
        )
    dev = BY_ID.resolve()
    size = int((Path("/sys/class/block") / dev.name / "size").read_text().strip())
    if size != EXPECT_SECTORS:
        sys.exit(
            f"FATAL: {dev} is {size} sectors, expected {EXPECT_SECTORS}. Refusing."
        )
    mounts = Path("/proc/mounts").read_text()
    if str(dev) in mounts:
        sys.exit(f"FATAL: {dev} is mounted. Unmount before imaging.")
    log(f"device {dev} verified: {size} sectors ({EXPECT_BYTES} bytes), not mounted")
    return dev


def give_back(path):
    """chown outputs to the invoking user - the script runs under sudo."""
    uid = int(os.environ.get("SUDO_UID", os.getuid()))
    gid = int(os.environ.get("SUDO_GID", os.getgid()))
    if path.exists():
        os.chown(path, uid, gid)


def rescue(dev):
    common = ["ddrescue", "--idirect", "-b", "512", "-v"]

    # Pass 1: read everything easy, skip error zones fast. Gets the bulk of the
    # data off a dying device before further stress kills more of it.
    log("=== pass 1: fast skip-on-error (-n) ===")
    t0 = time.time()
    run(
        common + ["-n", f"--timeout={TIMEOUT_SKIP}", str(dev), str(IMG), str(MAP)],
        check=False,
    )
    log(f"pass 1 done in {time.time() - t0:.0f}s")

    # Pass 2: scrape + retry the areas pass 1 skipped.
    log("=== pass 2: scrape and retry (-r3) ===")
    t0 = time.time()
    run(
        common + ["-r3", f"--timeout={TIMEOUT_SCRAPE}", str(dev), str(IMG), str(MAP)],
        check=False,
    )
    log(f"pass 2 done in {time.time() - t0:.0f}s")

    give_back(IMG)
    give_back(MAP)


def status():
    if not MAP.exists():
        sys.exit(f"no mapfile at {MAP} - nothing rescued yet")
    run(["ddrescuelog", "-t", str(MAP)], check=False)


def sha256(path):
    h = hashlib.sha256()
    with path.open("rb") as fh:
        for block in iter(lambda: fh.read(4 << 20), b""):
            h.update(block)
    return h.hexdigest()


def manifest():
    if not IMG.exists():
        sys.exit(f"no image at {IMG}")
    log("hashing image (this reads 7.4 GiB from local NVMe, not the USB)")
    digest = sha256(IMG)
    stat = IMG.stat()
    rescued = subprocess.run(
        ["ddrescuelog", "-t", str(MAP)], capture_output=True, text=True
    ).stdout

    doc = f"""# UNVR boot USB image

Source device (FAILING - see below):

| | |
|---|---|
| Model | Phison `UF3 8GB` (USB `13fe:5500`) |
| Serial | `07190176402AEE98` |
| by-id | `{BY_ID}` |
| Size | {EXPECT_BYTES} bytes ({EXPECT_SECTORS} x 512-byte sectors) |
| Layout | no partition table; whole-disk ext4, UUID `ff18e0d4-05d8-47a0-a6b0-59b3b4d51c18` |
| Contents | UniFi OS rootfs, hostname `UNVR` |

## Device is failing

`Medium Error / Unrecovered read error`, first hit at LBA 229376 (112 MiB).
Sequential read ran at 770 kB/s - the controller is retrying internally across
a wide area. Imaged with ddrescue, not dd. Do not run further full-device reads
against the original; work from the image.

## Files

- `{IMG.name}` - the image (sparse; gitignored, too big to track)
- `{MAP.name}` - ddrescue mapfile: which sectors are good/bad. TRACKED - it is
  the record of what was and was not recoverable, and makes the rescue resumable.

## Image

| | |
|---|---|
| Captured | {datetime.now(timezone.utc).astimezone().isoformat(timespec="seconds")} |
| Apparent size | {stat.st_size} bytes |
| On-disk (sparse) | {stat.st_blocks * 512} bytes |
| sha256 | `{digest}` |

Regenerate this file with `sudo ./scripts/rescue-unvr-usb.py manifest`.

## ddrescuelog

```
{rescued.strip()}
```

## Mounting the image

```
sudo losetup --find --show --read-only images/{IMG.name}
sudo mount -o ro,noload /dev/loopN /mnt/point
```

`noload` matters: the ext4 journal carries `needs_recovery`, and a plain mount
would replay it and mutate the image.
"""
    out = IMAGES / "README.md"
    out.write_text(doc)
    give_back(out)
    log(f"wrote {out}")
    log(f"sha256 {digest}")


if __name__ == "__main__":
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "action", nargs="?", default="rescue", choices=["rescue", "status", "manifest"]
    )
    args = ap.parse_args()

    IMAGES.mkdir(parents=True, exist_ok=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    LOGFILE = LOGS / "rescue-unvr-usb.log"
    give_back(IMAGES)
    give_back(REPO / "tmp")
    give_back(LOGS)

    if args.action in ("rescue",) and os.geteuid() != 0:
        sys.exit("FATAL: needs root to read the raw device. Run under sudo.")
    if not shutil.which("ddrescue"):
        sys.exit("FATAL: ddrescue not installed (dnf install ddrescue)")

    log(f"--- {args.action} ---")
    if args.action == "rescue":
        rescue(resolve_device())
        give_back(LOGFILE)
    elif args.action == "status":
        status()
    else:
        manifest()
        give_back(LOGFILE)
