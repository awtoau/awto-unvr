#!/usr/bin/env python3
"""Deploy the built kernel + DTB + modules onto woomera's SSD over the 1G port.

This is the deploy path for the current boot chain (#216): stock U-Boot
chainloads awto-uboot from NAND, and awto-uboot boots /boot/uImage from the
SSD. Nothing goes through NAND or TFTP - the kernel is just a file on the
running box's root filesystem, so a deploy is an scp and a reboot.

`./dev.py flash` (NAND) is the OLD path and is now only for recovery.

`--reboot` waits for the box to come back and then verifies the kernel the
box actually booted carries this tree's revision (#258/#165) - a stale
deploy used to be invisible until it produced a confusing failure (#105,
#131, #161). Non-zero exit if it does not. `--no-verify` opts out.

Run: ./dev.py deploy-ssd            # copy kernel+DTB+modules, leave box up
     ./dev.py deploy-ssd --reboot   # ... reboot into it, then verify
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

import _box  # noqa: E402
from _repo import LOGS  # noqa: E402

# The gzipped uImage + DTB that publish-fedora produces - the same artifacts
# awto-uboot loads. Modules are synced by publish-fedora itself.
KERNEL = TFTP_KIMG
DTB = TFTP_DIMG


def log(m: str) -> None:
    print(m, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    (LOGS / "deploy-ssd.log").open("a").write(m + "\n")


def run(cmd: list[str], label: str) -> None:
    log(f"  {label}")
    r = subprocess.run(cmd, check=False)
    if r.returncode:
        sys.exit(f"FAILED: {label} (rc={r.returncode})")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--reboot", action="store_true", help="reboot into the new kernel")
    ap.add_argument(
        "--no-verify",
        action="store_true",
        help="with --reboot, skip the post-boot version check (#258)",
    )
    a = ap.parse_args()

    for p in (KERNEL, DTB):
        if not p.exists():
            sys.exit(f"missing artifact: {p} (run ./dev.py publish-fedora)")

    ip = _box.require()
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
        log("reboot issued.")
        if a.no_verify:
            log("--no-verify: not checking what actually came back up.")
            return 0
        return wait_and_verify()

    log("not rebooting (pass --reboot). The box is still on the OLD kernel.")
    return 0


def wait_and_verify() -> int:
    """#258/#165: after a --reboot, check what actually came back up rather
    than assuming the deploy took.

    Both steps have their own timeouts and progress output, so nothing here
    blocks blindly: wait-for-boot.py polls the console log every 2 s to a
    300 s cap (its own docstring derives that from a ~200 s observed worst
    case), and verify-versions.py only reads a file."""
    log("waiting for the box to come back (wait-for-boot.py)...")
    boot = subprocess.run(
        [sys.executable, str(REPO / "scripts/wait-for-boot.py")],
        check=False,
        env={**os.environ, "AWTO_ALLOW_DIRECT_SCRIPT": "1"},
    )
    if boot.returncode == 3:
        log("FAILED: box did not reach a login prompt - versions not checked")
        return 1
    if boot.returncode == 1:
        log("box booted but logged trouble lines - checking versions anyway")

    # --require kernel: this deploy just wrote /boot/uImage, so the kernel
    # banner MUST be in the log we were watching. Absent means the box did
    # not boot the thing we deployed, which is exactly the silent failure
    # this exists to catch. U-Boot/UEFI are not required - a deploy-ssd does
    # not touch either.
    log("verifying the running build against the working tree...")
    return subprocess.run(
        [
            sys.executable,
            str(REPO / "scripts/verify-versions.py"),
            "--require",
            "kernel",
        ],
        check=False,
        env={**os.environ, "AWTO_ALLOW_DIRECT_SCRIPT": "1"},
    ).returncode


if __name__ == "__main__":
    raise SystemExit(main())
