#!/usr/bin/env python3
"""Fetch the UNVR firmware .bin from Ubiquiti and verify it.

- playwrong has no binary-download tool (only `pdf` writes files, PDF-only), so
  this uses curl by explicit approval. Ubiquiti publishes an sha256, so the
  silent-wrong-content failure the no-curl rule guards against is detectable.
- Verification is mandatory: size AND sha256 must match the fw-update.ui.com API
  values, or the file is deleted rather than left looking valid.

Run: ./scripts/fetch-unvr-firmware.py [fetch|verify]
"""

import argparse
import hashlib
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
SOURCES = REPO / "sources"
LOGS = REPO / "tmp" / "logs"
LOGFILE = LOGS / "fetch-unvr-firmware.log"

# From https://fw-update.ui.com/api/firmware-latest?filter=eq~~platform~~unvr
# queried 2026-08-15. Re-query to pick up a newer release.
URL = "https://fw-download.ubnt.com/data/unifi-nvr/44dc-UNVR-5.1.25-4d75bc90-b895-4d8f-baff-382f24fd36cf.bin"
VERSION = "v5.1.25+84c48e7"
UBNT_VERSION = "UNVR4.al324.v5.1.25.84c48e7.260710.1602"
EXPECT_SIZE = 786253430
EXPECT_SHA256 = "74f2833356e832bd97f59bb3686eaedaf704335631b5daabedfce4d30bb222fc"
EXPECT_MD5 = "3f661a852b6fb70946e2a94712df6923"

DEST = SOURCES / "UNVR-5.1.25.bin"

# curl stall detection: abort if throughput stays under 10 kB/s for 30 s.
# A working link delivers MB/s; 30 s below 10 kB/s is a dead transfer, not a
# slow one. Preferred over a fixed deadline, which would punish a slow link.
# --max-time is the backstop: 3600 s tolerates down to ~220 kB/s for 750 MiB.
# On expiry curl exits non-zero, the partial file is deleted, nothing is trusted.
SPEED_FLOOR = 10240
SPEED_WINDOW = 30
MAX_TIME = 3600


def log(msg):
    # Not _repo.make_log: this fetch script runs standalone (no dev.py
    # command) and must not import _repo's direct-invocation guard.
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {msg}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    with LOGFILE.open("a") as fh:
        fh.write(line + "\n")


def digest(path, algo):
    h = hashlib.new(algo)
    with path.open("rb") as fh:
        for block in iter(lambda: fh.read(4 << 20), b""):
            h.update(block)
    return h.hexdigest()


def verify(strict=True):
    if not DEST.exists():
        sys.exit(f"absent: {DEST}")
    size = DEST.stat().st_size
    log(
        f"size {size} (expect {EXPECT_SIZE}) {'OK' if size == EXPECT_SIZE else 'MISMATCH'}"
    )
    sha = digest(DEST, "sha256")
    md5 = digest(DEST, "md5")
    log(f"sha256 {sha}")
    log(f"md5    {md5}")
    ok = size == EXPECT_SIZE and sha == EXPECT_SHA256 and md5 == EXPECT_MD5
    log("VERIFIED" if ok else "VERIFICATION FAILED")
    if not ok and strict:
        DEST.unlink()
        sys.exit(
            "deleted the bad copy - a file that fails verification is worse than none"
        )
    return ok


def manifest():
    doc = f"""# sources

Downloaded artifacts. Binaries are gitignored; this manifest is the tracked part.

## UNVR-5.1.25.bin

| | |
|---|---|
| URL | <{URL}> |
| Version | `{VERSION}` |
| ubnt_version | `{UBNT_VERSION}` |
| Released | 2026-07-27 |
| Retrieved | {datetime.now(timezone.utc).astimezone().isoformat(timespec="seconds")} |
| Size | {EXPECT_SIZE} bytes |
| sha256 | `{EXPECT_SHA256}` |
| md5 | `{EXPECT_MD5}` |

Discovered via `https://fw-update.ui.com/api/firmware-latest?filter=eq~~platform~~unvr`
(platform `UNVR`, product `unifi-nvr`).

### Telling a good copy from a bad one

- Exactly {EXPECT_SIZE} bytes. Any other size is wrong.
- sha256 must match above. Verify: `./scripts/fetch-unvr-firmware.py verify`
- Not an HTML login/error page: `file` reports data, not HTML, and the first
  bytes are not `<!DOCTYPE`.

Fetched with curl by explicit approval - playwrong has no binary-download tool.
The stock checksum is what makes that safe; do not skip verification.
"""
    (SOURCES / "README.md").write_text(doc)
    log(f"wrote {SOURCES / 'README.md'}")


def fetch():
    SOURCES.mkdir(parents=True, exist_ok=True)
    if DEST.exists() and DEST.stat().st_size == EXPECT_SIZE:
        log(
            f"{DEST.name} already present at expected size - verifying instead of refetching"
        )
        verify()
        manifest()
        return
    log(f"GET {URL}")
    cmd = [
        "curl",
        "-L",
        "--fail",
        "--progress-bar",
        "--speed-limit",
        str(SPEED_FLOOR),
        "--speed-time",
        str(SPEED_WINDOW),
        "--max-time",
        str(MAX_TIME),
        "-o",
        str(DEST),
        URL,
    ]
    proc = subprocess.run(cmd, check=False)
    if proc.returncode != 0:
        log(f"curl exit {proc.returncode}")
        if DEST.exists():
            DEST.unlink()
        sys.exit("download failed - partial file removed")
    log(f"downloaded {DEST.stat().st_size} bytes")
    verify()
    manifest()


if __name__ == "__main__":
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("action", nargs="?", default="fetch", choices=["fetch", "verify"])
    a = ap.parse_args()
    log(f"--- {a.action} ---")
    fetch() if a.action == "fetch" else verify(strict=False)
