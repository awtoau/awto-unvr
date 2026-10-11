#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Drive al_ssm through dm-crypt (the only path AF_ALG cannot mask, #267).

Checks, on the box, over ssh (no AF_ALG):
- ops landed on the engine: devlink `submitted` delta > 0 (else CE served it);
- data integrity under N parallel writers: pattern written through
  aes-xts-plain64, read back, `cmp`;
- quiesce: `in_flight` and `backlog` return to 0 (#300 / #182 signature),
  `errors` delta 0, `completed` delta == `submitted` delta.
- --unbind-under-load: PCI unbind while writers run (#301). CAN CRASH THE BOX;
  needs --i-accept-a-crash.

    ./scripts/al-ssm-stress.py                       # integrity + quiesce
    ./scripts/al-ssm-stress.py --writers 8 --mib 128
    ./scripts/al-ssm-stress.py --unbind-under-load --i-accept-a-crash

Log: tmp/logs/al-ssm-stress.log. Needs dmsetup, losetup, devlink, dd, cmp on the
box (no cryptsetup). No sleep/timeout: pacing is one ssh round trip per poll.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

import _box

LOG = Path(__file__).resolve().parent.parent / "tmp" / "logs" / "al-ssm-stress.log"
DEFAULT_ROOT_PASSWORD = "unvr"  # docs/fedora-on-ssd.md
MAP = "alssm-stress"
BACKING = "/var/tmp/alssm-stress.img"
PATTERN = "/dev/shm/alssm-stress.pat"

# Bounds. Expected: 64 MiB write+read through the engine at the 235 MiB/s CE
# figure (#182) is ~0.6 s; 8x margin -> 5 s per phase, 120 s for the suite.
SSH_TIMEOUT_S = 120
# Quiesce: all completions land within the 500 us inline window or one 100 us
# worker cycle (jiffies); a whole 64-deep queue drains in < 100 ms. Each poll
# is one ssh round trip (~50 ms); 40 polls ~ 2 s = 20x. On expiry: FAIL.
QUIESCE_POLLS = 40

FIELD = re.compile(r"(\w+):\s+(\S+)")


def log(msg: str) -> None:
    LOG.parent.mkdir(parents=True, exist_ok=True)
    with LOG.open("a") as f:
        f.write(msg + "\n")
    print(msg)


def parse_diag(text: str) -> dict[str, str]:
    """`key: value` pairs from `devlink health diagnose ... reporter ssm`."""
    return dict(FIELD.findall(text))


def find_bdf(show_text: str) -> str | None:
    """BDF of the ssm reporter from `devlink health show` output."""
    m = re.search(r"pci/(\S+):\s+reporter ssm\b", show_text)
    return m.group(1) if m else None


def run(host: str, password: str, script: str) -> tuple[int, str]:
    argv = [*_box.sshpass_argv(password, connect_timeout=8), f"root@{host}", script]
    try:
        r = subprocess.run(
            argv, capture_output=True, text=True, check=False, timeout=SSH_TIMEOUT_S
        )
    except subprocess.TimeoutExpired:
        return 124, (
            f"ssh timed out after {SSH_TIMEOUT_S}s - a writer may be stuck in D "
            "state (#182/#300); only a reboot clears it"
        )
    return r.returncode, r.stdout + r.stderr


def diag(host: str, password: str, bdf: str) -> dict[str, str]:
    rc, out = run(host, password, f"devlink health diagnose pci/{bdf} reporter ssm")
    if rc != 0:
        sys.exit(f"devlink diagnose failed rc={rc}: {out}")
    return parse_diag(out)


def setup_script(mib: int, key_hex: str) -> str:
    sectors = mib * 2048
    return f"""set -e
head -c {mib * 1024 * 1024} /dev/urandom > {PATTERN}
truncate -s {mib}M {BACKING}
LOOP=$(losetup --find --show {BACKING})
echo "loop=$LOOP"
dmsetup create {MAP} --table "0 {sectors} crypt aes-xts-plain64 {key_hex} 0 $LOOP 0"
"""


def writers_script(writers: int, mib: int) -> str:
    blocks = mib * 256  # 4 KiB blocks
    per = blocks // writers
    return f"""set -e
for i in $(seq 0 {writers - 1}); do
  dd if={PATTERN} of=/dev/mapper/{MAP} bs=4k seek=$((i*{per})) skip=$((i*{per})) \\
     count={per} oflag=direct conv=notrunc status=none &
done
wait
dd if=/dev/mapper/{MAP} bs=4k count=$(({per}*{writers})) iflag=direct status=none \\
  | cmp -n $(({per}*{writers}*4096)) - {PATTERN} && echo CMP_OK
"""


def teardown_script() -> str:
    return (
        f"dmsetup remove {MAP} 2>&1; "
        f"for l in $(losetup -j {BACKING} -O NAME -n); do losetup -d $l; done; "
        f"rm -f {BACKING} {PATTERN}"
    )


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--host")
    ap.add_argument("--password", default=DEFAULT_ROOT_PASSWORD)
    ap.add_argument("--writers", type=int, default=4)
    ap.add_argument("--mib", type=int, default=64)
    ap.add_argument("--unbind-under-load", action="store_true")
    ap.add_argument("--i-accept-a-crash", action="store_true")
    a = ap.parse_args()
    if a.unbind_under_load and not a.i_accept_a_crash:
        sys.exit("--unbind-under-load can crash the box: add --i-accept-a-crash")
    if a.mib % a.writers or (a.mib * 256) % a.writers:
        sys.exit("--mib must be a multiple of --writers")

    host = a.host or _box.require()
    pw = a.password
    rc, out = run(host, pw, "devlink health show")
    bdf = find_bdf(out)
    if rc != 0 or not bdf:
        sys.exit(f"no ssm devlink reporter (al_ssm not loaded?): {out}")
    log(f"ssm reporter at {bdf}")

    # random key per run; hex of 64 bytes for aes-xts 512-bit
    key = subprocess.run(
        ["python3", "-I", "-c", "import secrets;print(secrets.token_hex(64))"],
        capture_output=True,
        text=True,
        check=True,
    ).stdout.strip()

    before = diag(host, pw, bdf)
    fails: list[str] = []
    try:
        rc, out = run(host, pw, setup_script(a.mib, key))
        log(out)
        if rc != 0:
            sys.exit("setup failed")
        if a.unbind_under_load:
            log("UNBIND under load (#301)")
            run(
                host,
                pw,
                f"({writers_script(a.writers, a.mib)}) & "
                f"echo {bdf} > /sys/bus/pci/drivers/al_ssm/unbind; wait",
            )
            log("survived; check dmesg for KASAN/DMA-API warnings")
        else:
            rc, out = run(host, pw, writers_script(a.writers, a.mib))
            log(out)
            if "CMP_OK" not in out:
                fails.append("data mismatch or writer failed (cmp)")
            after = diag(host, pw, bdf)
            for _ in range(QUIESCE_POLLS):
                if after.get("in_flight") == "0" and after.get("backlog") == "0":
                    break
                after = diag(host, pw, bdf)
            else:
                fails.append(
                    f"no quiesce: in_flight={after.get('in_flight')} "
                    f"backlog={after.get('backlog')} (#300/#182)"
                )

            def delta(k: str) -> int:
                return int(after[k]) - int(before[k])

            log(
                f"deltas: submitted={delta('submitted')} "
                f"completed={delta('completed')} errors={delta('errors')}"
            )
            if delta("submitted") <= 0:
                fails.append("engine saw no ops: dm-crypt used another driver")
            if delta("completed") != delta("submitted"):
                fails.append("completed != submitted")
            if delta("errors") != 0:
                fails.append("engine reported errors")
    finally:
        log(run(host, pw, teardown_script())[1])

    for f in fails:
        log(f"FAIL: {f}")
    log("PASS" if not fails else "FAILED")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
