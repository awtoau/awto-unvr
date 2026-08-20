#!/usr/bin/env python3
"""Sync the matching module tree, THEN flash the Fedora kernel + DTB into NAND.

2026-08-20: flashing a kernel without syncing its matching /lib/modules tree
(with --delete) left stale .ko files from an earlier build on the SSD. The
next boot's module loader tried to load one built for a different kernel
binary (same version STRING, mismatched actual code) -> a kernel Oops in
module-loading code itself (resolve_symbol/ref_module) -> re-triggered by
rebooting the same corrupted rootfs twice more -> RCU stall -> full soft
lockup. Multiple physical power-cycles, all avoidable. See #104's postmortem
comment. Module sync is now a MANDATORY first step here, not a separate
manual command someone (agent or human) has to remember to also run - if the
box isn't reachable to sync modules onto, this refuses to touch NAND at all,
because flashing a kernel whose module tree might not match is exactly the
mistake that caused this.

Runs against a device ALREADY at the U-Boot prompt (ALPINE_UBNT_NAS_ALL>). tftp's
our gzip uImage + ea16 DTB from the host and writes them into the DEAD stock
rootfs region of NAND (the stock kernel @0x300000 is left intact as recovery),
then sets bootcmd to nand-read + bootm and saveenv. Rootfs stays on the SSD
(root=PARTUUID, already in bootargs). Fully NAND-side; SSD untouched.

Layout (NAND offsets):
  kernel  @ 0x1300000  (start of the unused rootfs partition), read span 0x1200000
  dtb     @ 0x2800000
"""

from __future__ import annotations

import os
import socket
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _repo import LOGS, REPO

SOCK = Path(os.environ.get("XDG_RUNTIME_DIR", "/tmp")) / "tio-unvr.sock"
PROMPT = b"ALPINE_UBNT_NAS_ALL>"

IPADDR, SERVERIP = "192.168.25.140", "192.168.25.145"
KIMG = "uImage-unvr-ea16-7.1-fedora-gz"
DIMG = "alpine-v2-ubnt-unvr-ea16-7.1-fedora.dtb"
K_NAND, K_SPAN = "0x1300000", "0x1200000"  # 18.9 MiB span (kernel ~18.5)
D_NAND, D_ERASE, D_READ = "0x2800000", "0x40000", "0x20000"
K_RAM, D_RAM = "0x02000000", "0x04078000"
BOOTCMD = (
    f"nand read {K_RAM} {K_NAND} {K_SPAN}; "
    f"nand read {D_RAM} {D_NAND} {D_READ}; bootm {K_RAM} - {D_RAM}"
)

KVER = "7.1.8-dirty"
MODROOT = Path("/mnt/2tb/unvr-port-refs/build-out-71-fedora/modroot/lib/modules") / KVER
ROOT_PASSWORD = "unvr"  # documented default, see docs/fedora-on-ssd.md


def sync_modules():
    """Full clean (--delete) module sync onto the CURRENTLY-RUNNING box, over
    SSH, BEFORE touching NAND. Must happen before the new kernel boots, since
    the old kernel's rootfs is what we're syncing into - the new kernel reads
    whatever's already on disk when it starts. Aborts the whole flash if this
    fails; a kernel flashed without a verified-matching module tree is the
    exact bug this function exists to prevent."""
    if not MODROOT.is_dir():
        sys.exit(f"ABORT: no module tree at {MODROOT} - build first")
    host = subprocess.run(
        [sys.executable, "scripts/ssh-woomera.py", "--print"],
        cwd=REPO,
        capture_output=True,
        text=True,
        timeout=15,
        check=False,
    ).stdout.strip()
    if not host:
        sys.exit("ABORT: woomera not reachable - can't sync modules, refusing to flash")
    log(f"syncing module tree ({MODROOT}) -> woomera:/lib/modules/{KVER}/ (--delete)")
    ssh_opts = (
        "ssh -o StrictHostKeyChecking=accept-new"
        " -o PreferredAuthentications=password"
        " -o PubkeyAuthentication=no"
    )
    rc = subprocess.run(
        [
            "sshpass",
            "-p",
            ROOT_PASSWORD,
            "rsync",
            "-az",
            "--delete",
            "-e",
            ssh_opts,
            f"{MODROOT}/",
            f"root@{host}:/lib/modules/{KVER}/",
        ],
        check=False,
    ).returncode
    if rc != 0:
        sys.exit(
            f"ABORT: module rsync failed (rc={rc}) - refusing to flash a mismatched kernel"
        )
    rc = subprocess.run(
        [
            "sshpass",
            "-p",
            ROOT_PASSWORD,
            "ssh",
            "-o",
            "StrictHostKeyChecking=accept-new",
            "-o",
            "PreferredAuthentications=password",
            "-o",
            "PubkeyAuthentication=no",
            f"root@{host}",
            f"depmod -a {KVER}",
        ],
        check=False,
    ).returncode
    if rc != 0:
        sys.exit(f"ABORT: depmod on woomera failed (rc={rc}) - refusing to flash")
    log(
        "module sync + depmod OK - modules on disk now match what's about to be flashed"
    )


def log(m):
    line = (
        f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    )
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    (LOGS / "flash-nand.log").open("a").write(line + "\n")


def step(s, cmd, needle, limit, label):
    s.sendall(cmd.encode() + b"\n")
    buf = b""
    end = time.monotonic() + limit
    while time.monotonic() < end:
        try:
            c = s.recv(4096)
        except TimeoutError:
            continue
        if not c:
            break
        buf += c
        if needle.encode() in buf:
            log(f"  OK: {label}")
            return buf
    log(
        f"  FAIL: {label} — did not see {needle!r} in {limit}s\n{buf.decode(errors='replace')[-400:]}"
    )
    raise SystemExit(3)


def main():
    sync_modules()
    if not SOCK.exists():
        sys.exit(f"console socket absent: {SOCK}")
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.settimeout(0.1)
    s.connect(str(SOCK))
    # confirm we're at the prompt
    step(s, "", "ALPINE_UBNT_NAS_ALL>", 5, "at U-Boot prompt")
    log("=== flashing Fedora kernel+DTB to NAND (stock kernel @0x300000 preserved) ===")
    step(s, f"setenv ipaddr {IPADDR}", "ALPINE_UBNT_NAS_ALL>", 5, "set ipaddr")
    step(s, f"setenv serverip {SERVERIP}", "ALPINE_UBNT_NAS_ALL>", 5, "set serverip")
    # kernel
    step(s, f"tftpboot {K_RAM} {KIMG}", "Bytes transferred", 60, f"tftp {KIMG}")
    step(s, f"nand erase {K_NAND} {K_SPAN}", "OK", 30, "erase kernel region")
    step(s, f"nand write {K_RAM} {K_NAND} {K_SPAN}", "OK", 60, "write kernel")
    # dtb
    step(s, f"tftpboot {D_RAM} {DIMG}", "Bytes transferred", 30, f"tftp {DIMG}")
    step(s, f"nand erase {D_NAND} {D_ERASE}", "OK", 15, "erase dtb block")
    step(s, f"nand write {D_RAM} {D_NAND} {D_ERASE}", "OK", 15, "write dtb")
    # bootcmd + save
    step(s, f"setenv bootcmd '{BOOTCMD}'", "ALPINE_UBNT_NAS_ALL>", 5, "set bootcmd")
    step(s, "saveenv", "done", 15, "saveenv")
    log("DONE — kernel+DTB in NAND, bootcmd set + saved. 'boot' or reset to verify.")
    s.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
