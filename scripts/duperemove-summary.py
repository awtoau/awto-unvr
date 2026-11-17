#!/usr/bin/env python3
# Copyright (c) Awto / Daniel Tyrrell. SPDX-License-Identifier: GPL-2.0-or-later
"""Estimate dedupe savings from a duperemove report log (last run in the log).

Sums length*(copies-1) over every "Showing N identical extents of length X" line.
Lengths are rounded by duperemove -h, so the total is approximate.
"""

import re
import sys
from pathlib import Path

LOG = Path(sys.argv[1] if len(sys.argv) > 1 else "tmp/logs/duperemove-mirror.log")
UNIT = {"B": 1, "KB": 1 << 10, "MB": 1 << 20, "GB": 1 << 30}
text = LOG.read_text(errors="replace")
text = text.rsplit("\n=== ", 1)[-1]
pat = re.compile(r"Showing (\d+) identical extents of length ([\d.]+)(B|KB|MB|GB)")
groups = saved = 0
for n, size, unit in pat.findall(text):
    groups += 1
    saved += (int(n) - 1) * float(size) * UNIT[unit]
print(f"groups={groups} approx_saved={saved / (1 << 30):.2f} GiB")
