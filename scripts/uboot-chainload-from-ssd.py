#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Chainload a freshly built awto-uboot from the SSD, with NO network (#341).

`./dev.py uboot-test` tftps the binary, which needs a link in U-Boot. Both
ports currently fail there: the 1G never completes autoneg (#341, unchanged by
a 12 s bound) and the 10G hits the TX completion timeout (#253). So tftp cannot
load a fresh binary at all.

The SSD path has no such dependency - awto-uboot's own bootcmd already does
`ext4load scsi ${d}:2`. This scp's the binary to /boot over SSH from Linux,
then from the awto-nas# prompt: ext4load -> cp.b to CONFIG_TEXT_BASE -> go.

Flash is never touched. The running NAND bootloader is unchanged, so a
power-cycle reverts to it - that is the recovery path if the new binary is bad.

Usage:
    ./scripts/uboot-chainload-from-ssd.py              # stage + reboot + chainload
    ./scripts/uboot-chainload-from-ssd.py --no-stage   # binary already on /boot
    ./scripts/uboot-chainload-from-ssd.py --at-prompt  # already at awto-nas#
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
import _box
import _console
from _repo import make_log

log = make_log("uboot-chainload-from-ssd", stamped=False)

# The BUILD output, not tmp/tftp/ - that staging copy is written only by
# ./dev.py uboot-test, so reading it silently chainloads a stale binary. That
# happened: three "identical failure" results were measured against a 90-minute-
# old build before the size mismatch gave it away.
SRC = REPO / "tmp/uboot-build/u-boot.bin"
BIN = REPO / "tmp/tftp/u-boot-chainload.bin"
ON_BOX = "/boot/u-boot-chainload.bin"
# /boot is a DIRECTORY on the rootfs, not its own partition - bootcmd loads
# /boot/uImage from scsi N:2, so the in-partition path keeps the /boot prefix.
BOX_FILE = "/boot/u-boot-chainload.bin"

# 0x02000000 is scratch (where uboot-test tftps to); 0x1100000 is the RUNNING
# bootloader's load/entry AND our CONFIG_TEXT_BASE - must run there, cannot be
# written there, so the two-step load+copy is mandatory (nor-boot-chain.md:56).
LOAD_ADDR = "0x02000000"
TEXT_BASE = "0x1100000"

PROMPT = _console.AWTO_PROMPT
# A full scsi scan is ~10 s (4 bays, two with link timeouts); ext4load of
# 843 KB off the SSD measured 148 ms. 30 s is ~3x that worst case. On expiry:
# the step names itself and we stop rather than `go` an unloaded address.
SCAN_WAIT_S = 30
CMD_WAIT_S = 8
# Our U-Boot reprints its banner within ~1 s of `go`; 15 s is ~15x. On expiry:
# report no banner, meaning the jump died before serial came up.
JUMP_WAIT_S = 15
# Our U-Boot's autoboot countdown is 2 s and is NOT sticky, so the prompt must
# be caught by spamming CR through the reboot rather than waited for.
CATCH_S = 180


def send_expect(s, cmd: str, wait: float, until: str) -> str:
    s.sendall(cmd.encode() + b"\r")
    buf = b""
    end = time.monotonic() + wait
    while time.monotonic() < end:
        try:
            d = s.recv(4096)
        except TimeoutError:
            continue
        if not d:
            break
        buf += d
        if until.encode() in buf:
            break
    return buf.decode("utf-8", "replace")


def step(s, cmd: str, wait: float, until: str, what: str) -> str | None:
    out = send_expect(s, cmd, wait, until)
    if until not in out:
        log(f"STEP-FAILED ({what}): {until!r} not seen in {wait}s after {cmd!r}")
        for ln in out.strip().splitlines()[-6:]:
            log(f"  | {ln[:110]}")
        return None
    return out


def stage() -> bool:
    """Refresh the staging copy from the build output, then scp it onto the
    box's /boot over SSH (needs Linux up)."""
    if not SRC.is_file():
        log(f"FATAL: no binary at {SRC} - run ./dev.py build-uboot")
        return False
    BIN.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(SRC, BIN)
    log(f"staged from {SRC.relative_to(REPO)} ({SRC.stat().st_size} bytes)")
    host = _box.locate()
    if not host:
        log("FATAL: woomera not reachable - boot Linux first, or use --no-stage")
        return False
    log(f"staging {BIN.name} ({BIN.stat().st_size} bytes) -> {host}:{ON_BOX}")
    scp = ["scp", *_box.SSH_OPTS_BATCH, str(BIN), f"root@{host}:{ON_BOX}"]
    if subprocess.run(scp).returncode:
        log("FATAL: scp failed")
        return False
    # Verify, because a chainload of a truncated binary hangs with no message.
    want = subprocess.run(
        ["md5sum", str(BIN)], capture_output=True, text=True
    ).stdout.split()[0]
    got = subprocess.run(
        _box.ssh_argv(host, cmd=[f"sync; md5sum {ON_BOX}"], batch=True),
        capture_output=True,
        text=True,
    ).stdout.split()
    if not got or got[0] != want:
        log(f"FATAL: md5 mismatch - local {want}, box {got[0] if got else 'none'}")
        return False
    log(f"staged and verified (md5 {want})")
    return True


def catch_prompt(s) -> bool:
    """Spam CR until the awto-nas# prompt answers. The countdown is 2 s and
    keeps running, so a slow follow-up lands in Linux instead."""
    log("catching awto-nas# (CR spam)")
    end = time.monotonic() + CATCH_S
    while time.monotonic() < end:
        s.sendall(b"\r")
        buf = b""
        t = time.monotonic() + 1
        while time.monotonic() < t:
            try:
                d = s.recv(4096)
            except TimeoutError:
                continue
            if d:
                buf += d
        if PROMPT.encode() in buf:
            log("at awto-nas#")
            return True
    log(f"FATAL: no awto-nas# within {CATCH_S}s")
    return False


def test_343(s) -> int:
    """Two port activations in ONE U-Boot session (#343).

    Before the fix: the 10G adapter_stop fails (-110; #253 leaves a descriptor
    in flight), leaving the engine non-quiescent, and the NEXT port's
    adapter_init then fails with the RX -110. After: init must succeed.
    """
    log("#343: activation 1 - 10G (its stop is expected to fail, #253)")
    a1 = send_expect(s, "setenv ethact alu_eth_10g", CMD_WAIT_S, PROMPT)
    a1 += send_expect(s, "dhcp", 60, PROMPT)
    log("#343: activation 2 - 1G, SAME session (the actual test)")
    a2 = send_expect(s, "setenv ethact alu_eth_1g", CMD_WAIT_S, PROMPT)
    a2 += send_expect(s, "dhcp", 60, PROMPT)

    keys = (
        "eth rx",
        "eth tx",
        "adapter_init",
        "adapter_stop",
        "TX completion",
        "auto negotiation",
        "phy_startup",
        "DHCP client bound",
    )
    for tag, out in (("A1(10g)", a1), ("A2(1g)", a2)):
        for ln in out.splitlines():
            if any(k in ln for k in keys):
                log(f"  {tag}: {ln.strip()[:100]}")

    # Guard against grading Linux output as a pass: a bash prompt means the
    # countdown won and the commands never reached U-Boot.
    if "command not found" in a1 + a2:
        log("INVALID: commands landed in Linux, not U-Boot - the prompt was lost")
        return 1
    if PROMPT not in a2:
        log("INVALID: no awto-nas# in activation 2 - not at the U-Boot prompt")
        return 1

    rx = "dma state didn't change to Normal" in a2
    init = "al_eth_adapter_init failed" in a2
    log(f"#343 RX-normal failure in activation 2: {'YES' if rx else 'no'}")
    log(f"#343 adapter_init failure in activation 2: {'YES' if init else 'no'}")
    if rx or init:
        log("#343 VERDICT: FAIL - still present")
        return 1
    log("#343 VERDICT: PASS - a failed 10G stop no longer poisons the 1G init")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--no-stage", action="store_true", help="binary already on the box's /boot"
    )
    ap.add_argument(
        "--at-prompt",
        action="store_true",
        help="already at awto-nas#; do not reboot",
    )
    ap.add_argument(
        "--test-343",
        action="store_true",
        help="after the chainload, activate both ports in one session (#343)",
    )
    args = ap.parse_args()

    if not args.no_stage and not stage():
        return 1

    s = _console.connect()
    try:
        if not args.at_prompt:
            # Reboot whenever Linux is up, independent of --no-stage: that flag
            # only says the binary is already there, not that we are at the
            # prompt. Gating the reboot on it left the catch loop spinning for
            # 180 s against a running Linux.
            host = _box.locate()
            if host:
                log("rebooting into U-Boot")
                subprocess.run(
                    _box.ssh_argv(host, cmd=["sync; systemctl reboot"], batch=True),
                    check=False,
                )
            else:
                log("Linux not reachable - assuming the box is already resetting")
            if not catch_prompt(s):
                return 1
        elif not step(s, "", 3, PROMPT, "confirm awto-nas#"):
            return 1

        # Stop the countdown before anything slow, or the next command lands in
        # Linux (the prompt is not sticky).
        if not step(s, "setenv bootdelay -1", CMD_WAIT_S, PROMPT, "stop autoboot"):
            return 1

        if not step(s, "scsi scan", SCAN_WAIT_S, PROMPT, "scsi scan"):
            return 1

        # bootcmd probes 0..3 because /dev/sdX order is not stable across boots;
        # do the same rather than assume an index.
        loaded = False
        for dev in range(4):
            out = step(
                s,
                f"ext4load scsi {dev}:2 {LOAD_ADDR} {BOX_FILE}",
                CMD_WAIT_S,
                PROMPT,
                f"ext4load from scsi {dev}:2",
            )
            if out and "bytes read" in out:
                want = SRC.stat().st_size
                got = None
                for ln in out.splitlines():
                    if "bytes read" in ln:
                        log(f"scsi {dev}:2: {ln.strip()[:90]}")
                        m = re.search(r"(\d+) bytes read", ln)
                        if m:
                            got = int(m.group(1))
                # A stale /boot copy loads cleanly and tests the WRONG binary.
                # Three results were measured that way before a size mismatch
                # gave it away, so this is now fatal rather than cosmetic.
                if got != want:
                    log(
                        f"FATAL: loaded {got} bytes, built {want} - stale binary on /boot"
                    )
                    return 1
                loaded = True
                break
        if not loaded:
            log("FATAL: binary not found on any scsi N:2 - was it staged?")
            return 1

        if not step(
            s,
            f"cp.b {LOAD_ADDR} {TEXT_BASE} $filesize",
            CMD_WAIT_S,
            PROMPT,
            "copy to TEXT_BASE",
        ):
            return 1

        log(f"go {TEXT_BASE}")
        out = send_expect(s, f"go {TEXT_BASE}", JUMP_WAIT_S, "U-Boot 20")
        for ln in out.splitlines():
            if any(k in ln for k in ("U-Boot 20", "Model:", "DRAM:", "awto-")):
                log(f"  | {ln.strip()[:110]}")
        if "U-Boot 20" not in out:
            log(f"FAIL: no banner within {JUMP_WAIT_S}s - the jump died early")
            log("      power-cycle to fall back to the NAND bootloader")
            return 1
        # The fresh binary restarts its own 2 s autoboot countdown, which is
        # NOT sticky and expires during its SATA scan - a plain `setenv
        # bootdelay -1` sent after `go` arrives too late, the box boots Linux,
        # and any follow-up test lands at a bash prompt (observed: "setenv:
        # command not found"). Spam CR to win the countdown first.
        if not catch_prompt(s):
            log("FATAL: lost the new binary's prompt")
            return 1
        if not step(
            s, "setenv bootdelay -1", CMD_WAIT_S, PROMPT, "hold the new prompt"
        ):
            return 1
        log("CHAINLOAD OK - the fresh binary is running; box is held at awto-nas#")
        if args.test_343:
            return test_343(s)
        return 0
    finally:
        s.close()


if __name__ == "__main__":
    sys.exit(main())
