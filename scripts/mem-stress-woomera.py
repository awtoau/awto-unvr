#!/usr/bin/env python3
"""Userspace memory + stress testing on woomera (no reboot, live kernel).

memtest86 is bare-metal (boots its own image) and canNOT run from userspace; the
userspace analogs, run here under the running kernel:
  - memtester : memtest86-style pattern test over an mmap'd RAM region
  - stress-ng : CPU + VM + cache stressors — stability / thermal / overclock val

Installs them via dnf if absent. Read-only w.r.t. persistent state. Later, run at
an overclocked DDR freq to confirm stability under load. Output ->
tmp/logs/mem-stress-woomera.log.

Durations/sizes (all have a reason — this is timing/throughput measurement):
  --memtester-mb 256 : region big enough to exercise the controller, small enough
                       not to OOM the box or evict Protect's working set.
  --stress-secs  60  : a 60 s smoke. Real stability/overclock validation wants
                       many minutes to hours — raise it then.
"""
from __future__ import annotations
import argparse, sys
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _console as con  # noqa: E402
from _repo import LOGS  # noqa: E402

LOG = LOGS / "mem-stress-woomera.log"


def log(m):
    line = f"{datetime.now(timezone.utc).astimezone().isoformat(timespec='seconds')}  {m}"
    print(line, flush=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    LOG.open("a").write(line + "\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--memtester-mb", type=int, default=256)
    ap.add_argument("--stress-secs", type=int, default=60)
    ap.add_argument("--skip-install", action="store_true")
    a = ap.parse_args()

    s = con.connect()
    con.login(s)
    log("shell ready")

    if not a.skip_install:
        rc, have = con.sh(s, "command -v memtester stress-ng >/dev/null && echo yes || echo no")
        if "yes" not in have:
            log("installing memtester + stress-ng (dnf, up to ~4 min)")
            con.sh(s, "dnf -y -q install memtester stress-ng 2>&1 | tail -3", 300)

    # nproc + free for context
    _, ctx = con.sh(s, "nproc; free -m | awk '/Mem:/{print \"MemFree(MB): \"$4\"  MemAvail: \"$7}'")
    log("context:\n" + ctx)

    # memtester: 1 pass over the region. Timeout scaled to size (~a few s/MB worst case).
    to = max(120, a.memtester_mb * 2)
    log(f"memtester {a.memtester_mb}M x1 (timeout {to}s)...")
    rc, out = con.sh(s, f"memtester {a.memtester_mb}M 1 2>&1 | tail -40", to)
    ok = "FAILURE" not in out and ("ok" in out.lower() or "Done" in out or rc == 0)
    log(f"memtester rc={rc}  verdict={'PASS' if ok else 'CHECK/FAIL'}\n{out}")

    # stress-ng: cpu + vm + cache for the smoke window, with metrics.
    # --cpu 0 = one CPU stressor per online core.
    log(f"stress-ng cpu+vm+cache for {a.stress_secs}s...")
    cmd = (f"stress-ng --cpu 0 --vm 2 --vm-bytes 256M --cache 2 "
           f"--timeout {a.stress_secs}s --metrics-brief 2>&1 | tail -30")
    rc, out = con.sh(s, cmd, a.stress_secs + 120)
    sok = "successful run completed" in out.lower() or rc == 0
    log(f"stress-ng rc={rc}  verdict={'PASS' if sok else 'CHECK/FAIL'}\n{out}")

    # thermal after load (SoC die + adt7475), to watch for throttling headroom
    _, temp = con.sh(s, "for h in /sys/class/hwmon/hwmon*; do n=$(cat $h/name); "
                        "t=$(cat $h/temp1_input 2>/dev/null); echo \"$n temp1=$t\"; done")
    log("post-load temps:\n" + temp)

    s.close()
    log("DONE — full log in tmp/logs/mem-stress-woomera.log")
    return 0 if (ok and sok) else 2


if __name__ == "__main__":
    sys.exit(main())
