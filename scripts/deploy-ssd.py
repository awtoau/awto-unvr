#!/usr/bin/env python3
"""Deploy the built kernel + DTB + modules onto woomera's SSD over the 1G port.

This is the deploy path for the current boot chain (#216): stock U-Boot
chainloads awto-uboot from NAND, and awto-uboot boots /boot/uImage from the
SSD. Nothing goes through NAND or TFTP - the kernel is just a file on the
running box's root filesystem, so a deploy is an scp and a reboot.

`./dev.py flash` (NAND) is the OLD path and is now only for recovery.

Run: ./dev.py deploy-ssd            # copy kernel+DTB+modules, leave box up
     ./dev.py deploy-ssd --reboot   # ... and reboot into it
"""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
from _fedora_deploy import KVER, TFTP_DIMG, TFTP_KIMG  # noqa: E402

from _repo import LOGS  # noqa: E402

# The gzipped uImage + DTB that publish-fedora produces - the same artifacts
# awto-uboot loads. Modules are synced by publish-fedora itself.
KERNEL = TFTP_KIMG
DTB = TFTP_DIMG

SSH = REPO / "scripts/ssh-woomera.py"


def log(m: str) -> None:
    print(m, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    (LOGS / "deploy-ssd.log").open("a").write(m + "\n")


def box_ip() -> str:
    """Resolve the box by its 1G MAC - the lease moves, the MAC does not."""
    r = subprocess.run(
        [sys.executable, str(SSH), "--print"],
        capture_output=True,
        text=True,
        check=False,
        env={**os.environ, "AWTO_ALLOW_DIRECT_SCRIPT": "1"},
    )
    ip = r.stdout.strip().splitlines()[-1] if r.stdout.strip() else ""
    if not ip:
        sys.exit("woomera not found on the LAN - is it up?")
    return ip


def run(cmd: list[str], label: str) -> None:
    log(f"  {label}")
    r = subprocess.run(cmd, check=False)
    if r.returncode:
        sys.exit(f"FAILED: {label} (rc={r.returncode})")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--reboot", action="store_true", help="reboot into the new kernel")
    a = ap.parse_args()

    for p in (KERNEL, DTB):
        if not p.exists():
            sys.exit(f"missing artifact: {p} (run ./dev.py publish-fedora)")

    ip = box_ip()
    log(f"deploy-ssd -> {ip}, kernel {KVER}")
    host = f"root@{ip}"
    opts = [
        "-o",
        "StrictHostKeyChecking=no",
        "-o",
        "UserKnownHostsFile=/dev/null",
        "-o",
        "LogLevel=ERROR",
    ]

    run(["scp", *opts, str(KERNEL), f"{host}:/boot/uImage"], "kernel -> /boot/uImage")
    run(["scp", *opts, str(DTB), f"{host}:/boot/unvr.dtb"], "dtb -> /boot/unvr.dtb")
    log("modules: already synced by publish-fedora")
    log("DONE - kernel+DTB on the SSD.")

    if a.reboot:
        # The box drops the connection as it goes down; that is success here,
        # so the reboot's own exit status is not meaningful.
        log("rebooting...")
        subprocess.run(["ssh", *opts, host, "systemctl reboot"], check=False)
        log("reboot issued - watch the serial console.")
    else:
        log("not rebooting (pass --reboot). The box is still on the OLD kernel.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
