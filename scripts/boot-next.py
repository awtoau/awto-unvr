#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Pick what awto-uboot boots next, from whichever OS is running (#338, docs/boot-select.md).

  ./dev.py boot-next status                 # bootonce/bootdefault, bootcmd in sync?
  ./dev.py boot-next truenas [--reboot]     # ONE boot of /boot/awto/truenas, then back to default
  ./dev.py boot-next truenas --default      # every boot (until `fedora --default`)
  ./dev.py boot-next usb [--reboot]         # ONE boot from /boot/awto/usb on the USB SSD p2 (#360)
  ./dev.py boot-next fedora [--default]     # clear bootonce (or bootdefault)
  ./dev.py boot-next sync-bootcmd           # saved bootcmd := defconfig CONFIG_BOOTCOMMAND
  ./dev.py boot-next deploy-dtb tmp/dtb/unvr-fedora.dtb   # Fedora /boot/unvr.dtb (.prev kept)
  ./dev.py boot-next stage NAME --uimage F --dtb F --initrd F --bootargs '...' [--base DIR]

- A hung profile: power-cycle (./dev.py power-cycle) -> bootonce already cleared -> default.
- --reboot waits --wait seconds (default 120: Fedora reset -> sshd, worst ~80 s x1.5, #352).
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _bootsel as bs  # noqa: E402

import _box  # noqa: E402
from _repo import TMP, make_log  # noqa: E402

log = make_log("boot-next")


def backup_env(ip: str) -> None:
    """Raw copy of the env partition before every write: tmp/bootsel/env-<stamp>.bin."""
    blob = subprocess.run(
        _box.ssh_argv(ip, cmd=[f"cat {bs.env_call(ip, 'dump')['mtd']}"], batch=True),
        capture_output=True,
        timeout=bs.SSH_TIMEOUT_S,
        check=True,
    ).stdout
    p = TMP / "bootsel" / f"env-{time.strftime('%Y%m%d-%H%M%S')}.bin"
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_bytes(blob)
    log(f"env backup: {p} ({len(blob)} B)")


def env_set(ip: str, *assigns: str) -> dict:
    backup_env(ip)
    r = bs.env_call(ip, "set", *assigns)
    log(
        f"env set {' '.join(a.split('=')[0] + ('=' + a.split('=', 1)[1] if len(a) < 60 else '=<...>') for a in assigns)}"
        f" -> changed={r['changed']} crc_ok={r['crc_ok']}"
    )
    return r


def status(ip: str) -> int:
    d = bs.env_call(ip, "dump")
    env = d["env"]
    sync = env.get("bootcmd") == bs.defconfig_bootcmd()
    log(
        f"{d['mtd']} crc_ok={d['crc_ok']} bootonce={env.get('bootonce', '-')} "
        f"bootdefault={env.get('bootdefault', '-')} bootcmd_in_sync={sync}"
    )
    cmdline = bs.ssh(ip, "cat /proc/cmdline").strip()
    prof = next(
        (w.split("=", 1)[1] for w in cmdline.split() if w.startswith("awto.profile=")),
        "fedora",
    )
    log(f"running: {bs.ssh(ip, 'uname -r').strip()} profile={prof}")
    return 0 if d["crc_ok"] else 1


def select(ip: str, name: str, default: bool) -> None:
    key = "bootdefault" if default else "bootonce"
    if name == "fedora":
        env_set(ip, f"{key}=")
        return
    bs.check_profile_name(name)
    # `usb` lives on the USB SSD's p2, not the SATA SSD this box is running from (#360).
    if name != "usb" and not bs.profile_present(ip, name):
        raise RuntimeError(
            f"{bs.PROFILE_DIR}/{name} incomplete on the box - stage it first"
        )
    if bs.env_call(ip, "dump")["env"].get("bootcmd") != bs.defconfig_bootcmd():
        raise RuntimeError(
            "saved bootcmd != defconfig (no profile support) - run sync-bootcmd"
        )
    env_set(ip, f"{key}={name}")


def deploy_dtb(ip: str, dtb: Path) -> None:
    data = dtb.read_bytes()
    if data[:4] != b"\xd0\x0d\xfe\xed":
        raise ValueError(f"{dtb}: not an FDT")
    h = hashlib.sha256(data).hexdigest()
    subprocess.run(
        ["scp", "-q", *_box.SSH_OPTS_BATCH, str(dtb), f"root@{ip}:/boot/unvr.dtb.new"],
        check=True,
        timeout=bs.SCP_TIMEOUT_S,
    )
    out = bs.ssh(
        ip,
        f"sha256sum /boot/unvr.dtb.new | grep -q '^{h} ' && cp -p /boot/unvr.dtb /boot/unvr.dtb.prev"
        " && mv -f /boot/unvr.dtb.new /boot/unvr.dtb && sync && sha256sum /boot/unvr.dtb",
    )
    if not out.startswith(h):
        raise RuntimeError(f"/boot/unvr.dtb verify failed: {out}")
    log(f"/boot/unvr.dtb = {dtb} ({h[:16]}); previous kept as /boot/unvr.dtb.prev")


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "what", help="status | sync-bootcmd | deploy-dtb | stage | fedora | <profile>"
    )
    ap.add_argument("arg", nargs="?", help="deploy-dtb: dtb path; stage: profile name")
    ap.add_argument(
        "--default", action="store_true", help="set bootdefault instead of bootonce"
    )
    ap.add_argument("--reboot", action="store_true", help="reboot and wait for ssh")
    ap.add_argument(
        "--wait", type=float, default=120.0, help="--reboot: seconds to wait for ssh"
    )
    ap.add_argument("--uimage", type=Path)
    ap.add_argument("--dtb", type=Path)
    ap.add_argument("--initrd", type=Path)
    ap.add_argument("--bootargs")
    ap.add_argument(
        "--base",
        default=bs.PROFILE_DIR,
        help="stage: box dir holding profiles (USB p2 mounted: <mnt>/boot/awto, #360)",
    )
    a = ap.parse_args()
    ip = _box.require()
    try:
        if a.what == "status":
            if not a.reboot:
                return status(ip)
        elif a.what == "sync-bootcmd":
            env_set(ip, f"bootcmd={bs.defconfig_bootcmd()}")
        elif a.what == "deploy-dtb":
            deploy_dtb(ip, Path(a.arg))
        elif a.what == "stage":
            bs.stage_profile(
                ip,
                a.arg,
                uimage=a.uimage,
                dtb=a.dtb,
                initrd=a.initrd,
                bootargs=a.bootargs,
                log=log,
                base=a.base,
            )
        else:
            select(ip, a.what, a.default)
        if a.reboot:
            got = bs.wait_new_boot(bs.reboot(ip, log), a.wait, log)
            if not got:
                return 3
            return status(got[0])
    except (
        RuntimeError,
        ValueError,
        subprocess.SubprocessError,
        json.JSONDecodeError,
    ) as e:
        log(f"FATAL: {e}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
