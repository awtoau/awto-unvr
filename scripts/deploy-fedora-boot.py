#!/usr/bin/env python3
"""Deploy a fresh Fedora kernel + DTB to woomera's /boot — safely, no reboot.

Stages the new uImage-...-fedora-gz + DTB over HTTP (from selwyn) to /boot on the
SSD root, keeping the current (known-good) kernel+DTB as .bak for U-Boot fallback.
Every file is sha256-verified after transfer. Does NOT reboot — verify first, then
reboot separately (scripts/reboot-and-verify or a manual `reboot`). NAND untouched.

Fallback if the new kernel doesn't boot: at the stock U-Boot prompt,
  ext4load scsi 0:2 0x02000000 /boot/uImage-unvr-ea16-7.1-fedora-gz.bak
  ext4load scsi 0:2 0x04078000 /boot/alpine-v2-ubnt-unvr-ea16-7.1-fedora.dtb.bak
  bootm 0x02000000 - 0x04078000
"""

from __future__ import annotations

import hashlib
import http.server
import socketserver
import sys
import threading
from datetime import datetime, timezone
from functools import partial
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _console as con
from _repo import LOGS

OUT = Path("/mnt/2tb/unvr-port-refs/build-out-71-fedora")
SELWYN_IP = "192.168.25.145"
HTTP_PORT = 8099
# (local source file, /boot destination name)
FILES = [
    ("uImage-unvr-ea16-7.1-fedora-gz", "uImage-unvr-ea16-7.1-fedora-gz"),
    ("alpine-v2-ubnt-unvr-ea16-7.1.dtb", "alpine-v2-ubnt-unvr-ea16-7.1-fedora.dtb"),
]
LOG = LOGS / "deploy-fedora-boot.log"


def log(m):
    line = (
        f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    )
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    LOG.open("a").write(line + "\n")


def sha256(p):
    h = hashlib.sha256()
    h.update(Path(p).read_bytes())
    return h.hexdigest()


def main():
    for src, _ in FILES:
        if not (OUT / src).exists():
            sys.exit(f"missing build artifact: {OUT / src}")
    local_hash = {dst: sha256(OUT / src) for src, dst in FILES}
    for src, dst in FILES:
        log(f"local {src}: {sha256(OUT / src)}  ({(OUT / src).stat().st_size} B)")

    handler = partial(http.server.SimpleHTTPRequestHandler, directory=str(OUT))
    httpd = socketserver.TCPServer(("0.0.0.0", HTTP_PORT), handler)
    threading.Thread(target=httpd.serve_forever, daemon=True).start()
    log(f"serving {OUT} on {SELWYN_IP}:{HTTP_PORT}")

    s = con.connect()
    con.login(s)
    rc, _ = con.sh(s, f"ping -c1 -W2 {SELWYN_IP} >/dev/null 2>&1")
    if rc != 0:
        log("FATAL: woomera cannot reach selwyn — aborting (nothing changed)")
        httpd.shutdown()
        s.close()
        return 1

    ok = True
    for src, dst in FILES:
        b = f"/boot/{dst}"
        # guarded backup: keep the FIRST (known-good) copy, never clobber it
        con.sh(s, f"[ -e {b}.bak ] || cp -a {b} {b}.bak 2>/dev/null; echo bkdone")
        rc, _ = con.sh(s, f"ls {b}.bak >/dev/null 2>&1 && echo have.bak || echo NO.bak")
        url = f"http://{SELWYN_IP}:{HTTP_PORT}/{src}"
        log(f"fetch {src} -> {b}.new")
        rc, out = con.sh(
            s, f"curl -fsS {url} -o {b}.new && echo FETCH_OK || echo FETCH_FAIL", 120
        )
        if "FETCH_OK" not in out:
            log(f"FETCH FAILED for {src}: {out}")
            ok = False
            break
        rc, h = con.sh(s, f"sha256sum {b}.new | cut -d' ' -f1", 30)
        got = h.split()[0] if h.split() else ""
        if got != local_hash[dst]:
            log(
                f"HASH MISMATCH {dst}: box {got} != local {local_hash[dst]} — NOT installing"
            )
            ok = False
            break
        log(f"{dst}: sha256 OK — installing")
        con.sh(s, f"mv {b}.new {b}; sync; echo installed")

    httpd.shutdown()
    if ok:
        _, ls = con.sh(
            s,
            "ls -la --time-style=+%H:%M /boot/uImage-unvr-ea16-7.1-fedora-gz* "
            "/boot/alpine-v2-ubnt-unvr-ea16-7.1-fedora.dtb* 2>/dev/null",
        )
        log("=== /boot now ===\n" + ls)
        log("DEPLOY OK — files staged + verified. NOT rebooted. Verify, then reboot.")
    else:
        con.sh(s, "rm -f /boot/*.new 2>/dev/null; echo cleaned")
        log("DEPLOY ABORTED — .new files removed; current /boot untouched.")
    s.close()
    return 0 if ok else 2


if __name__ == "__main__":
    sys.exit(main())
