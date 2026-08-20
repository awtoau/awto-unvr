#!/usr/bin/env python3
"""Build modules/al_eth natively on woomera itself, against the kernel source
tree already staged at /root/src/linux-v7.1.8 and the repo synced to
/root/awto-unvr (see docs/on-box-build.md for one-time setup).

Default action is BUILD + INSTALL ONLY (make modules_install + depmod) - the
new .ko lands in /lib/modules/<kver>/ but does NOT replace the currently
loaded module. Effective on next `modprobe`/reboot. This avoids the network
outage risk of a live rmmod/insmod cycle (2026-08-20: rmmod leaked a
mdio_bus sysfs kobject that blocked every subsequent reload until a full
reboot - see the filed issue). Pass --reload to additionally do a live
rmmod+insmod, which WILL bounce enp0s1/enp0s2 - only do this deliberately,
never as part of routine iteration.

  ./scripts/build-on-box.py                 # rsync + native build + install
  ./scripts/build-on-box.py --reload        # ...then live rmmod/insmod too
  ./scripts/build-on-box.py --no-sync       # build only, skip the rsync step
"""

from __future__ import annotations

import argparse
import datetime
import subprocess
import sys
from pathlib import Path

DEFAULT_ROOT_PASSWORD = "unvr"  # documented default, see docs/fedora-on-ssd.md
LOG = Path("tmp/logs/build-on-box.log")
KDIR = "/root/src/linux-v7.1.8"
REPO_REMOTE = "/root/awto-unvr"
MODDIR = f"{REPO_REMOTE}/modules/al_eth"


def locate_woomera() -> str:
    out = subprocess.run(
        [sys.executable, "scripts/ssh-woomera.py", "--print"],
        capture_output=True,
        text=True,
        check=True,
    )
    return out.stdout.strip()


def ssh_cmd(host: str, password: str) -> list[str]:
    return [
        "sshpass",
        "-p",
        password,
        "ssh",
        "-o",
        "ConnectTimeout=8",
        "-o",
        "StrictHostKeyChecking=accept-new",
        "-o",
        "PreferredAuthentications=password",
        "-o",
        "PubkeyAuthentication=no",
        f"root@{host}",
    ]


def run_remote(host: str, password: str, remote_script: str, log) -> int:
    result = subprocess.run(
        ssh_cmd(host, password) + [remote_script],
        capture_output=True,
        text=True,
        check=False,
    )
    out = result.stdout + result.stderr
    print(out)
    log.write(out)
    return result.returncode


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--host", help="woomera's address (default: auto-locate by MAC OUI)"
    )
    ap.add_argument("--password", default=DEFAULT_ROOT_PASSWORD)
    ap.add_argument(
        "--no-sync", action="store_true", help="skip rsyncing the repo before building"
    )
    ap.add_argument(
        "--reload",
        action="store_true",
        help="also live rmmod+insmod after the build - BOUNCES enp0s1/enp0s2, only for a "
        "deliberate hardware test, never routine iteration",
    )
    args = ap.parse_args()

    host = args.host or locate_woomera()
    print(f"# woomera at {host}", file=sys.stderr)

    LOG.parent.mkdir(parents=True, exist_ok=True)
    stamp = datetime.datetime.now().astimezone().isoformat(timespec="seconds")
    with LOG.open("a") as log:
        log.write(f"\n=== {stamp} host={host} reload={args.reload} ===\n")

        if not args.no_sync:
            print("# syncing repo -> box", file=sys.stderr)
            files = subprocess.run(
                ["git", "ls-files", "-z"], capture_output=True, check=True
            ).stdout
            ssh_opt = (
                f"sshpass -p {args.password} ssh -o StrictHostKeyChecking=accept-new "
                "-o PreferredAuthentications=password -o PubkeyAuthentication=no"
            )
            rc = subprocess.run(
                [
                    "rsync",
                    "-a",
                    "--files-from=-",
                    "--from0",
                    "-e",
                    ssh_opt,
                    ".",
                    f"root@{host}:{REPO_REMOTE}/",
                ],
                input=files,
                capture_output=True,
                check=False,
            )
            log.write(rc.stdout.decode() + rc.stderr.decode())
            if rc.returncode != 0:
                print("rsync failed, see log", file=sys.stderr)
                return rc.returncode

        # /dev/watchdog is fed by systemd's watchdog handling if enabled elsewhere,
        # but the box has no persistent clock (RTC disabled - #98/rtc wedge;
        # systemd-timesyncd refuses to start - ConditionVirtualization=!container
        # on this Container-Image rootfs, see the filed issue). Every boot starts
        # at epoch, which breaks make's mtime-ordering (spurious "Clock skew
        # detected" warnings) - set it from the build host's own clock first.
        now = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%d %H:%M:%S")
        print(
            "# setting box clock (no persistent RTC/NTP on this box - see filed issue)",
            file=sys.stderr,
        )
        run_remote(host, args.password, f"date -u -s '{now}'", log)

        build_script = (
            f"set -e\n"
            f"cd {KDIR} && make modules_prepare 2>&1 | tail -5\n"
            f"cd {MODDIR}\n"
            f"rm -f *.o *.ko .*.cmd modules.order Module.symvers 2>/dev/null || true\n"
            f"make -C {KDIR} M={MODDIR} modules 2>&1 | tail -30\n"
            f'echo "--- vermagic ---"\n'
            f"modinfo {MODDIR}/al_eth.ko | grep vermagic\n"
            f'RUNNING="$(uname -r)"\n'
            f"BUILT=\"$(modinfo {MODDIR}/al_eth.ko | awk '/vermagic/{{print $2}}')\"\n"
            f'if [ "$RUNNING" != "$BUILT" ]; then\n'
            f'  echo "ABORT: vermagic $BUILT does not match running kernel $RUNNING"\n'
            f"  exit 1\n"
            f"fi\n"
            f"make -C {KDIR} M={MODDIR} modules_install 2>&1 | tail -10\n"
            f'depmod -a "$RUNNING"\n'
            f'echo "installed to /lib/modules/$RUNNING - takes effect on next modprobe/reboot"\n'
        )
        print("# building natively on box", file=sys.stderr)
        rc = run_remote(host, args.password, build_script, log)
        if rc != 0:
            print("build/install failed, see log", file=sys.stderr)
            return rc

        if args.reload:
            print(
                "# --reload: live rmmod/insmod (WILL bounce enp0s1/enp0s2)",
                file=sys.stderr,
            )
            reload_script = "rmmod al_eth; modprobe al_eth; ip -br addr"
            rc = run_remote(host, args.password, reload_script, log)
            if rc != 0:
                print("reload failed, see log - check console", file=sys.stderr)
                return rc

    return 0


if __name__ == "__main__":
    sys.exit(main())
