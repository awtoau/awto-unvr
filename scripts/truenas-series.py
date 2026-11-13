#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""truenas-series - our board support onto TrueNAS's 6.18 kernel as a generated series (#338).

- check (default, the gate step): apply to a scratch index of the pin. No checkout.
  Skips with rc 0 when the Linux store is absent (CI host without it).
- apply: reset the build tree (tmp/refs/truenas-unvr) to the pin and `git am -3` the series.
- Registry of 6.18-only deltas: patches/truenas-6.18/series.json (scripts/_truenas.py).
- Log: tmp/logs/truenas-series.log
"""

from __future__ import annotations

import argparse
import sys

import _truenas

from _repo import LINUX_STORE, kernel_src, make_log

log = make_log("truenas-series")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("action", nargs="?", default="check", choices=["check", "apply"])
    a = ap.parse_args()
    if not (LINUX_STORE / "MAINTAINERS").exists() or not kernel_src().exists():
        log(f"SKIP: no Linux store at {LINUX_STORE} or no {kernel_src()} (#350)")
        return 0
    try:
        if a.action == "check":
            _truenas.check(log)
        else:
            _truenas.apply(log)
    except _truenas.SeriesError as e:
        log(f"FATAL: {e}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
