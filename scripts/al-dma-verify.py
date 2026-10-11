#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""dmatest-driven hardware checks for al_dma: #289, #288, #287, #290, #169.

Runs on the box over ssh. dmatest is never unloaded (a dmatest thread outlives
`rmmod dmatest` and wedges the channel until reboot); `echo 0 > run` is how a
run is stopped, and that releases the channel, i.e. exercises free_chan_resources.

  ./scripts/al-dma-verify.py                  # runs A, B on dma0chan0
  ./scripts/al-dma-verify.py --reload --i-accept-a-reboot
  ./scripts/al-dma-verify.py --reload --pq --i-accept-a-reboot

Phases (a failed phase stops the rest; the log says which):
  A  copy+xor, N iterations, test_buf_size 16384: 0 failures, no
     'Failed to init DMA queue', devlink in_flight back to 0.
  B  the same again on the same channel without a reload: A's `echo 0 > run`
     released the channel, so B only passes if free/alloc work (#289).
  C  test_buf_size 131072 (#290): lengths over the engine's limits (memcpy
     64 KiB, xor 16 KiB) are refused; the only failures allowed are
     'prep error', never a data error or a timeout.
  D  --reload: rmmod al_dma; modprobe al_dma [enable_pq=1]; run as A. This is
     the #23 'UDMA state survives reload' claim; if the reload wedges the
     engine only a reboot clears it, hence --i-accept-a-reboot.
  E  --pq (implies enable_pq=1 on the reload): PQ at test_buf_size 8192 must
     give 0 failures (#169: Q/PQ over 8 KiB are rejected, not mis-computed);
     at 16384 the only failures allowed are 'prep error' (len > 8192 refused).

Log: tmp/logs/al-dma-verify.log. No sleep and no `timeout` command: every wait
is one blocking ssh whose subprocess timeout is derived in run_limit_s().
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

import _box

LOG = Path(__file__).resolve().parent.parent / "tmp" / "logs" / "al-dma-verify.log"
DEFAULT_ROOT_PASSWORD = "unvr"  # docs/fedora-on-ssd.md
CHAN = "dma0chan0"
SYSFS = "/sys/module/dmatest/parameters"

# dmatest per-op completion timeout (ms). Waits for: one op's completion
# callback. Expected ~1 ms/op (#23); the driver aborts a stalled op at
# AL_DMA_STALL_MS = 50 (#288), so the abort lands before this does: 100 ms =
# 2x that. On expiry dmatest counts 'test timed out' and the check fails.
DMATEST_OP_TIMEOUT_MS = 100
# Slowest rate any thread has measured on this engine: pq 1998 iops, xor 2130
# iops before the #286 fix (#23 comment 2026-08-29); 12.7k xor after.
MIN_IOPS = 2000
# dmatest sleeps 100 ms after every 'prep error' (drivers/dma/dmatest.c, the
# `if (!tx)` branch), so a run that expects refusals is paced by them.
PREP_ERROR_SLEEP_S = 0.1
# Iterations for runs where most ops are refused on purpose: 300 x ~0.9 x
# 0.1 s ~= 27 s per thread, versus ~30 min at the default 20000.
REFUSAL_ITERATIONS = 300
SSH_CONNECT_S = 8  # _box.SSH_OPTS_PASSWORD ConnectTimeout in sshpass_argv
SLACK_S = 5  # dmatest thread start + dmesg grep over ssh

SUMMARY = re.compile(
    r"dmatest: (?P<thr>\S+): summary (?P<tests>\d+) tests, (?P<fail>\d+) failures "
    r"(?P<iops>[\d.]+) iops (?P<kbs>\d+) KB/s"
)
RESULT = re.compile(r"dmatest: (?P<thr>\S+): result #\d+: '(?P<what>[^']+)'")
BAD_KERNEL = re.compile(
    r"Failed to init DMA queue|BUG:|Oops|WARNING:|never completed|queue reset failed"
)


def log(msg: str) -> None:
    LOG.parent.mkdir(parents=True, exist_ok=True)
    with LOG.open("a") as f:
        f.write(msg + "\n")
    print(msg)


def run_limit_s(iterations: int, refused_fraction: float = 0.0) -> int:
    """Subprocess limit for one blocking dmatest run: the slowest expected
    thread (MIN_IOPS) plus the dmatest sleep per refused op, x 1.25, plus ssh
    connect and slack. Expiry: the engine is presumed wedged; the phase FAILs
    and the script stops."""
    work = iterations / MIN_IOPS + iterations * refused_fraction * PREP_ERROR_SLEEP_S
    return int(work * 1.25) + SSH_CONNECT_S + SLACK_S


def parse_dmesg(text: str) -> dict:
    """Summaries and failure kinds per dmatest thread, plus kernel trouble."""
    summaries = {m["thr"]: m for m in SUMMARY.finditer(text)}
    kinds: dict[str, dict[str, int]] = {}
    for m in RESULT.finditer(text):
        k = kinds.setdefault(m["thr"], {})
        k[m["what"]] = k.get(m["what"], 0) + 1
    return {
        "summaries": {
            t: {
                "tests": int(m["tests"]),
                "failures": int(m["fail"]),
                "iops": float(m["iops"]),
            }
            for t, m in summaries.items()
        },
        "failure_kinds": kinds,
        "kernel_trouble": BAD_KERNEL.findall(text),
    }


def judge(parsed: dict, iterations: int, allow_kinds: set[str] = frozenset()):
    """Failure strings for one run; empty list = pass. `allow_kinds`: dmatest
    result strings that may account for failures (e.g. 'prep error')."""
    fails: list[str] = []
    if not parsed["summaries"]:
        fails.append("no dmatest summary lines (did the run start?)")
    for thr, s in parsed["summaries"].items():
        kinds = parsed["failure_kinds"].get(thr, {})
        bad = {k: n for k, n in kinds.items() if k not in allow_kinds}
        if s["failures"] and (bad or not kinds):
            fails.append(f"{thr}: {s['failures']} failures {bad or '(kinds unknown)'}")
        elif s["failures"] != sum(kinds.values()) and kinds:
            fails.append(f"{thr}: {s['failures']} failures but {kinds}")
        if s["tests"] != iterations:
            fails.append(f"{thr}: ran {s['tests']} of {iterations} tests")
    if parsed["kernel_trouble"]:
        fails.append(f"kernel log: {sorted(set(parsed['kernel_trouble']))}")
    return fails


def ssh(host: str, password: str, script: str, limit_s: int) -> tuple[int, str]:
    argv = [
        *_box.sshpass_argv(password, connect_timeout=SSH_CONNECT_S),
        f"root@{host}",
        script,
    ]
    try:
        r = subprocess.run(
            argv, capture_output=True, text=True, check=False, timeout=limit_s
        )
    except subprocess.TimeoutExpired:
        return 124, f"ssh exceeded {limit_s}s: engine presumed wedged; reboot"
    return r.returncode, r.stdout + r.stderr


def dmatest_script(marker: str, iterations: int, buf: int) -> str:
    """Stop any previous run (releases the channel), configure, run, block on
    `wait`, print the dmesg since the marker. dmatest starts one thread per
    capability the channel advertises (copy0, xor0, and pq0 with enable_pq)."""
    return f"""set -e
modprobe dmatest 2>/dev/null || true
cd {SYSFS}
echo 0 > run
echo '{marker}' > /dev/kmsg
echo {iterations} > iterations
echo {buf} > test_buf_size
echo {DMATEST_OP_TIMEOUT_MS} > timeout
echo 1 > threads_per_chan
echo {CHAN} > channel
echo 1 > run
cat wait > /dev/null
dmesg | sed -n '/{marker}/,$p'
"""


def find_bdf(show_text: str) -> str | None:
    """BDF of the dma reporter from `devlink health show` output."""
    m = re.search(r"pci/(\S+):\s+reporter dma\b", show_text)
    return m.group(1) if m else None


def one_run(
    host, pw, name, iterations, buf, allow_kinds=frozenset(), refused_fraction=0.0
) -> bool:
    marker = f"al-dma-verify {name} buf={buf}"
    log(f"== {name}: {iterations} iterations, test_buf_size {buf}, {CHAN}")
    rc, out = ssh(
        host,
        pw,
        dmatest_script(marker, iterations, buf),
        run_limit_s(iterations, refused_fraction),
    )
    log(out)
    if rc != 0:
        log(f"FAIL {name}: ssh rc={rc}")
        return False
    fails = judge(parse_dmesg(out), iterations, allow_kinds)
    for f in fails:
        log(f"FAIL {name}: {f}")
    if not fails:
        log(f"PASS {name}")
    return not fails


def reload_module(host, pw, enable_pq: bool) -> bool:
    opt = "enable_pq=1" if enable_pq else ""
    # Waits for: rmmod/modprobe, each ~ms. Limit = the connect + slack.
    rc, out = ssh(
        host,
        pw,
        f"echo 0 > {SYSFS}/run; rmmod al_dma && modprobe al_dma {opt}; "
        f"cat /sys/module/al_dma/parameters/enable_pq; dmesg | tail -n 8",
        SSH_CONNECT_S + SLACK_S,
    )
    log(out)
    return rc == 0


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument("--host")
    ap.add_argument("--password", default=DEFAULT_ROOT_PASSWORD)
    ap.add_argument("--iterations", type=int, default=20000)
    ap.add_argument("--reload", action="store_true")
    ap.add_argument("--pq", action="store_true")
    ap.add_argument("--i-accept-a-reboot", action="store_true")
    a = ap.parse_args()
    if (a.reload or a.pq) and not a.i_accept_a_reboot:
        sys.exit("--reload/--pq can wedge the engine (#23): add --i-accept-a-reboot")
    if a.pq and not a.reload:
        sys.exit("--pq needs --reload (enable_pq is a module parameter)")

    host = a.host or _box.require()
    n = a.iterations
    ok = (
        one_run(host, a.password, "A", n, 16384)
        and one_run(host, a.password, "B-second-run-same-channel", n, 16384)
        and one_run(
            host,
            a.password,
            "C-oversize",
            min(n, REFUSAL_ITERATIONS),
            131072,
            allow_kinds={"prep error"},
            refused_fraction=0.9,  # xor: 87.5% of lengths 1..131072 exceed 16 KiB
        )
    )
    if ok and a.reload:
        ok = reload_module(host, a.password, a.pq) and one_run(
            host, a.password, "D-after-reload", n, 16384
        )
    if ok and a.pq:
        ok = one_run(host, a.password, "E-pq-8k", n, 8192) and one_run(
            host,
            a.password,
            "E-pq-16k",
            min(n, REFUSAL_ITERATIONS),
            16384,
            allow_kinds={"prep error"},
            refused_fraction=0.5,  # pq: lengths over 8 KiB are half of 1..16384
        )
    # Waits for: two devlink queries, each ~ms. Limit = connect + slack.
    rc, out = ssh(host, a.password, "devlink health show", SSH_CONNECT_S + SLACK_S)
    bdf = find_bdf(out)
    if bdf:
        rc, out = ssh(
            host,
            a.password,
            f"devlink health diagnose pci/{bdf} reporter dma",
            SSH_CONNECT_S + SLACK_S,
        )
        log(f"devlink dma reporter: {out}")
    else:
        log("no dma devlink reporter found")
    log("PASS" if ok else "FAILED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
