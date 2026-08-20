#!/usr/bin/env python3
"""Fetch + verify + unpack Ubiquiti GPL source archives (archive.org).

The UDM-Pro (UDMP) is Alpine V2 / AL-324 - the same SoC as our UNVR - so its
GPL dump carries the stock U-Boot source, Alpine bootloader/DDR-init, AL-324
BSP, kernel, and the real al_* drivers + DTS. UDMSE is (probably) the same SoC,
newer firmware. UDM/UDR are different SoCs (kept as tarballs, not unpacked).

curl is used deliberately here (playwrong has no binary-download tool) WITH
mandatory sha256/md5 verification against archive.org's published hashes, per
the sources/ convention. Owner-approved.

Tarballs -> sources/gpl/ (gitignored). Unpacked -> /mnt/2tb/unvr-port-refs/gpl/.
Space-aware: /mnt/2tb is tight, so only `unpack=True` items are extracted, and
their tarball is removed after a successful unpack to reclaim space.
"""

from __future__ import annotations

import hashlib
import os
import subprocess
import sys
import tarfile
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import LOGS, REPO

BASE = "https://archive.org/download/unifi-udr-gpl-archives"
TARBALLS = REPO / "sources" / "gpl"
UNPACK_ROOT = Path("/mnt/2tb/unvr-port-refs/gpl")

# (dir, file, sha256-or-None, md5-fallback-or-None, unpack?)
# sha256 comes from the adjacent .sha256 sidecar (fetched at run time) when the
# hash below is None; md5 fallbacks are from the archive.org metadata API.
FILES = [
    # UDM-Pro = Alpine V2 / AL-324 = our SoC -> unpack
    (
        "UDMP",
        "UDMPRO-2.4.27-GPL.tar.gz",
        None,
        "13c768fd2db9137f2dccd22c17ee6794",
        True,
    ),
    (
        "UDMP",
        "linux-udmp-2.4.27.tar.gz",
        None,
        "9ebc1851b77f9cf1108b0c77fc90e5ba",
        True,
    ),
    # UDM-SE = (probably) same AL-324, newer fw. Unpack the kernel (DTS/drivers
    # cross-check); keep the big full-GPL as a tarball to save space.
    ("UDMSE", "UDMSE-3.0.13-GPL.tar.gz", None, None, False),
    ("UDMSE", "linux-udmse-3.0.13.tar.gz", None, None, True),
    # different / unconfirmed SoCs -> download + verify only, do NOT unpack (space)
    ("UDM", "UDM-2.4.27-GPL.tar.gz", None, None, False),
    ("UDM", "linux-udm-2.4.27.tar.gz", None, None, False),
    ("UDR", "UDR-3.0.13-GPL.tar.gz", None, None, False),
    ("UDR", "UDR-3.0.17-GPL.tar.gz", None, "5caa17bc4ef3b277a072154b481aa430", False),
    ("UDR", "linux-udr-2.2.12.tar.gz", None, None, False),
    ("UDR", "linux-udr-3.0.13.tar.gz", None, None, False),
    ("UDR", "linux-udr-3.0.17.tar.gz", None, None, False),
]


def log(m: str) -> None:
    line = (
        f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    )
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    with (LOGS / "fetch-gpl-sources.log").open("a") as fh:
        fh.write(line + "\n")


def free_gb(path="/mnt/2tb") -> float:
    st = os.statvfs(path)
    return st.f_bavail * st.f_frsize / 1e9


def curl(url: str, dest: Path) -> bool:
    # -C - resumes a partial file; -f fails on HTTP error; -L follows redirects.
    r = subprocess.run(["curl", "-fL", "-C", "-", "-o", str(dest), url])
    return r.returncode == 0


def sha256_of(p: Path) -> str:
    h = hashlib.sha256()
    with p.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def md5_of(p: Path) -> str:
    h = hashlib.md5()
    with p.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def verify(tar: Path, d: str, fn: str, sha: str | None, md5: str | None) -> bool:
    # Prefer the .sha256 sidecar; fall back to the known md5.
    side = tar.with_suffix(tar.suffix + ".sha256")
    if sha is None and curl(f"{BASE}/{d}/{fn}.sha256", side) and side.exists():
        want = side.read_text().split()[0].strip()
        if want:
            sha = want
    if sha:
        got = sha256_of(tar)
        ok = got.lower() == sha.lower()
        log(f"  sha256 {'OK' if ok else 'MISMATCH'} {fn}")
        return ok
    if md5:
        got = md5_of(tar)
        ok = got.lower() == md5.lower()
        log(f"  md5 {'OK' if ok else 'MISMATCH'} {fn} (no sha256 available)")
        return ok
    log(f"  NO HASH available for {fn} - cannot verify")
    return False


def main() -> int:
    TARBALLS.mkdir(parents=True, exist_ok=True)
    UNPACK_ROOT.mkdir(parents=True, exist_ok=True)
    log(f"start; free on /mnt/2tb: {free_gb():.1f} GB")
    ok, bad = [], []
    for d, fn, sha, md5, unpack in FILES:
        tar = TARBALLS / fn
        log(f"=== {d}/{fn} (unpack={unpack}) ===")
        if not curl(f"{BASE}/{d}/{fn}", tar):
            log(f"  DOWNLOAD FAILED {fn}")
            bad.append(fn)
            continue
        if not verify(tar, d, fn, sha, md5):
            bad.append(fn)
            continue
        ok.append(fn)
        if unpack:
            dest = UNPACK_ROOT / d
            dest.mkdir(parents=True, exist_ok=True)
            if free_gb() < 8:
                log(f"  SKIP unpack {fn}: <8 GB free")
                continue
            log(f"  unpacking -> {dest}")
            with tarfile.open(tar) as t:
                t.extractall(dest, filter="data")
            tar.unlink()  # reclaim space; re-downloadable + verified
            log(f"  unpacked, tarball removed. free: {free_gb():.1f} GB")
    log(
        f"DONE. verified={len(ok)} failed={len(bad)} {('BAD:' + ','.join(bad)) if bad else ''}"
    )
    log(f"tarballs: {TARBALLS} ; unpacked: {UNPACK_ROOT}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
