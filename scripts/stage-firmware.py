#!/usr/bin/env python3
"""Stage every ladder firmware onto the UNVR's USERDEV stick.

Downloads each release to sources/ (sha256-verified against the Ubiquiti API),
then TFTPs it to ${MNT_RWFS}/firmware/ on the device so upgrades can be applied
with no network.

Why ${MNT_RWFS}/firmware and not ${MNT_RWFS}/upgrade:
  - `upgrade/fw-image.bin` is the drop-in trigger. The initramfs flashes ANY file
    at that exact path on the next boot, so a staging area must not be it.
  - `data/` is wrong too: overlay_cleanup wipes it on every upgrade boot, and
    purify_userdev_data deletes a whitelist of paths inside it.
  - A directory beside `data/` is touched by neither (verified in the 5.1.25
    initramfs, scripts/ui-boot-proto).

Applying one afterwards is then:
    cp /mnt/.rwfs/firmware/UNVR-<ver>.bin /mnt/.rwfs/upgrade/fw-image.bin && reboot
"""

import argparse
import hashlib
import re
import socket
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import REPO, TFTP_ROOT

LOGS = REPO / "tmp" / "logs"
LOG = LOGS / "stage-firmware.log"
SOURCES = REPO / "sources"
SOCK = Path("/run/user/1000/tio-unvr.sock")
TFTP_PORT = 69

API = (
    "https://fw-update.ubnt.com/api/firmware"
    "?filter=eq~~platform~~UNVR&filter=eq~~channel~~release"
    "&sort=created&limit=200"
)

LADDER = ["1.4.9", "2.3.14", "3.1.16", "4.1.22", "5.1.25"]

STAGE_DIR = "/mnt/.rwfs/firmware"

# Push limit. Observed ~4 MB/s over the device's 100 Mbps link; the largest image
# is 786 MB, so ~200 s. 900 s is 4.5x that - generous enough for a slow link,
# short enough that a stalled transfer is reported rather than hung on.
PUSH_LIMIT = 900.0

_SEQ = [0]


def log(msg, level="INFO"):
    line = f"{time.strftime('%Y-%m-%dT%H:%M:%S%z')}  {level:5s} {msg}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    with LOG.open("a", encoding="utf-8") as fh:
        fh.write(line + "\n")


def console(cmd: str, wait: float = 5.0) -> str:
    """Run one command on the device, delimited by a unique sentinel."""
    if not SOCK.exists():
        sys.exit(f"console socket absent: {SOCK} - start ./dev.py console")
    _SEQ[0] += 1
    tag = f"__SF{_SEQ[0]}__"
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(1.0)
    s.connect(str(SOCK))
    s.sendall(f"{cmd}; echo {tag}$?".encode() + b"\r")
    got, end = "", time.monotonic() + wait
    try:
        while time.monotonic() < end:
            try:
                chunk = s.recv(65536)
            except TimeoutError:
                continue
            if not chunk:
                break
            got += chunk.decode("utf-8", "replace")
            if re.search(rf"{tag}(\d+)", got):
                break
    finally:
        s.close()
    return got


def sha256(p: Path) -> str:
    h = hashlib.sha256()
    with p.open("rb") as fh:
        for blk in iter(lambda: fh.read(1 << 20), b""):
            h.update(blk)
    return h.hexdigest()


def api_rows() -> list[dict]:
    import json
    import urllib.request

    with urllib.request.urlopen(API, timeout=60) as r:
        return json.load(r)["_embedded"]["firmware"]


def pick(rows: list[dict], version: str) -> dict:
    for r in rows:
        if r["tags"].get("ubnt_version", "").split(".v")[-1].startswith(version + "."):
            return r
    for r in rows:
        if f".v{version}." in r["tags"].get("ubnt_version", ""):
            return r
    sys.exit(f"no API row for {version}")


def fetch(row: dict, ver: str) -> Path:
    dest = SOURCES / f"UNVR-{ver}.bin"
    want = row["sha256_checksum"]
    if dest.exists() and sha256(dest) == want:
        log(f"  {ver}: already have sources/{dest.name}, sha256 OK")
        return dest
    url = row["_links"]["data"]["href"]
    log(f"  {ver}: GET {url}")
    SOURCES.mkdir(parents=True, exist_ok=True)
    subprocess.run(["curl", "-fL", "--progress-bar", "-o", str(dest), url], check=True)
    got = sha256(dest)
    if got != want:
        dest.unlink(missing_ok=True)
        sys.exit(f"{ver}: sha256 mismatch, deleted (want {want}, got {got})")
    log(f"  {ver}: {dest.stat().st_size} B, sha256 verified")
    return dest


def host_ip() -> tuple[str, str]:
    out = console("ip -4 addr show", wait=5.0)
    cands = [
        ip
        for ip in re.findall(r"inet (\d+\.\d+\.\d+\.\d+)/", out)
        if not ip.startswith(("127.", "169.254."))
    ]
    if not cands:
        sys.exit(f"no routable IPv4 on the device:\n{out}")
    dev = cands[0]
    p = subprocess.run(
        ["ip", "-o", "route", "get", dev], capture_output=True, text=True, check=False
    )
    m = re.search(r"\bsrc (\d+\.\d+\.\d+\.\d+)", p.stdout)
    if not m:
        sys.exit(f"no route to {dev}")
    return m.group(1), dev


def _tftp_port_bound() -> bool:
    r = subprocess.run(
        ["ss", "-lun", f"sport = :{TFTP_PORT}"],
        capture_output=True,
        text=True,
        check=False,
    )
    return "UNCONN" in r.stdout


def ensure_tftpd():
    if _tftp_port_bound():
        # #119: never silently reuse - a process bound here from an earlier,
        # possibly-differently-rooted invocation would serve stale content.
        log("  killing stale process on tftp port (#119 - never reuse)")
        subprocess.run(
            ["sudo", "fuser", "-k", f"{TFTP_PORT}/udp"],
            capture_output=True,
            check=False,
        )
        for _ in range(20):
            if not _tftp_port_bound():
                break
            time.sleep(0.05)
    log("  starting tftpd")
    TFTP_ROOT.mkdir(parents=True, exist_ok=True)
    subprocess.Popen(
        [
            sys.executable,
            str(REPO / "scripts" / "tftpd.py"),
            "--root",
            str(TFTP_ROOT.relative_to(REPO)),
            "--port",
            str(TFTP_PORT),
        ],
        cwd=REPO,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        start_new_session=True,
    )
    for _ in range(40):
        if _tftp_port_bound():
            return
        time.sleep(0.05)
    sys.exit("tftpd did not bind")


def push(src: Path, ver: str, host: str) -> bool:
    name = f"UNVR-{ver}.bin"
    link = TFTP_ROOT / name
    if not link.exists():
        link.hardlink_to(src) if hasattr(link, "hardlink_to") else None
    want_md5 = hashlib.md5(src.read_bytes()).hexdigest()

    out = console(f"md5sum {STAGE_DIR}/{name} 2>/dev/null", wait=120.0)
    if want_md5 in out:
        log(f"  {ver}: already on device, md5 matches - skipping")
        return True

    log(f"  {ver}: pushing {src.stat().st_size} B (limit {PUSH_LIMIT:.0f}s)")
    console(f"mkdir -p {STAGE_DIR}", wait=15.0)
    console(
        f"cd {STAGE_DIR} && tftp -g -r {name} -l {STAGE_DIR}/{name} {host}",
        wait=PUSH_LIMIT,
    )
    out = console(f"md5sum {STAGE_DIR}/{name}", wait=180.0)
    if want_md5 not in out:
        log(f"  {ver}: MD5 MISMATCH after push", "ERROR")
        return False
    log(f"  {ver}: staged, md5 {want_md5} verified on device")
    return True


if __name__ == "__main__":
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--download-only",
        action="store_true",
        help="fetch to sources/ but do not touch the device",
    )
    ap.add_argument("--only", help="comma list of versions, default the whole ladder")
    a = ap.parse_args()

    plan = a.only.split(",") if a.only else LADDER
    log(f"staging plan: {plan}")

    rows = api_rows()
    local: dict[str, Path] = {}
    for ver in plan:
        local[ver] = fetch(pick(rows, ver), ver)

    total = sum(p.stat().st_size for p in local.values())
    log(f"downloaded {len(local)} images, {total / 2**30:.2f} GiB total")

    if a.download_only:
        log("--download-only: done")
        sys.exit(0)

    ensure_tftpd()
    host, dev = host_ip()
    log(f"device {dev}, host {host}")

    free = console("df -h /mnt/.rwfs | tail -1", wait=15.0)
    log(f"  userdev: {' '.join(free.split()[-6:-1])}")

    ok = True
    for ver, p in local.items():
        if not push(p, ver, host):
            ok = False
    log(f"staged into {STAGE_DIR} on the USERDEV")
    log(
        f"apply one with: cp {STAGE_DIR}/UNVR-<ver>.bin /mnt/.rwfs/upgrade/fw-image.bin; reboot"
    )
    sys.exit(0 if ok else 1)
