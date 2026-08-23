#!/usr/bin/env python3
"""Self-contained: fetch+verify+unpack ALL Ubiquiti GPL archives ON woomera.

Runs on the device (Fedora), downloading straight to the 7.3 TB unvr-work drive
(ample space -> unpack everything). No repo dependencies. Verifies each tarball's
sha256 against archive.org's .sha256 sidecar (md5 fallback for the two without a
sidecar). curl is the device doing its own download over its own internet route.

  # on woomera, with the work drive mounted at /mnt/work:
  python3 fetch-gpl-woomera.py
"""

import hashlib
import subprocess
import tarfile
from datetime import datetime, timezone
from pathlib import Path

BASE = "https://archive.org/download/unifi-udr-gpl-archives"
DEST = Path("/mnt/work/gpl")
TARS = DEST / "tarballs"

# (dir, file, md5-fallback-if-no-sha256-sidecar)
FILES = [
    ("UDMP", "UDMPRO-2.4.27-GPL.tar.gz", "13c768fd2db9137f2dccd22c17ee6794"),
    ("UDMP", "linux-udmp-2.4.27.tar.gz", "9ebc1851b77f9cf1108b0c77fc90e5ba"),
    ("UDMSE", "UDMSE-3.0.13-GPL.tar.gz", None),
    ("UDMSE", "linux-udmse-3.0.13.tar.gz", None),
    ("UDM", "UDM-2.4.27-GPL.tar.gz", None),
    ("UDM", "linux-udm-2.4.27.tar.gz", None),
    ("UDR", "UDR-3.0.13-GPL.tar.gz", None),
    ("UDR", "UDR-3.0.17-GPL.tar.gz", "5caa17bc4ef3b277a072154b481aa430"),
    ("UDR", "linux-udr-2.2.12.tar.gz", None),
    ("UDR", "linux-udr-3.0.13.tar.gz", None),
    ("UDR", "linux-udr-3.0.17.tar.gz", None),
]


def log(m):
    line = (
        f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    )
    print(line, flush=True)
    DEST.mkdir(parents=True, exist_ok=True)
    with (DEST / "fetch.log").open("a") as fh:
        fh.write(line + "\n")


def curl(url, dest):
    return (
        subprocess.run(
            ["curl", "-fL", "-C", "-", "-o", str(dest), url], check=False
        ).returncode
        == 0
    )


def digest(p, algo):
    h = hashlib.new(algo)
    with p.open("rb") as f:
        for c in iter(lambda: f.read(1 << 20), b""):
            h.update(c)
    return h.hexdigest()


def main():
    TARS.mkdir(parents=True, exist_ok=True)
    log(f"start; dest={DEST}")
    ok, bad = [], []
    for d, fn, md5 in FILES:
        tar = TARS / fn
        log(f"=== {d}/{fn} ===")
        if not curl(f"{BASE}/{d}/{fn}", tar):
            log("  DOWNLOAD FAILED")
            bad.append(fn)
            continue
        # verify: prefer sha256 sidecar, else md5 fallback
        side = tar.with_suffix(tar.suffix + ".sha256")
        verified = False
        if (
            curl(f"{BASE}/{d}/{fn}.sha256", side)
            and side.exists()
            and side.stat().st_size
        ):
            want = side.read_text().split()[0].strip().lower()
            got = digest(tar, "sha256")
            verified = got == want
            log(f"  sha256 {'OK' if verified else 'MISMATCH'}")
        elif md5:
            verified = digest(tar, "md5") == md5.lower()
            log(f"  md5 {'OK' if verified else 'MISMATCH'} (no sha256 sidecar)")
        else:
            log("  NO HASH available - cannot verify")
        if not verified:
            bad.append(fn)
            continue
        ok.append(fn)
        dest = DEST / d
        dest.mkdir(parents=True, exist_ok=True)
        log(f"  unpacking -> {dest}")
        try:
            with tarfile.open(tar) as t:
                t.extractall(dest, filter="data")
            log("  unpacked ok")
        except (tarfile.TarError, OSError) as e:
            log(f"  UNPACK FAILED: {e}")
            bad.append(fn + "(unpack)")
    log(
        f"DONE. verified={len(ok)} failed={len(bad)} {('BAD:' + ','.join(bad)) if bad else ''}"
    )


if __name__ == "__main__":
    main()
