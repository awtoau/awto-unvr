# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Per-OPEN access trim: stat each candidate hit as the asking identity.

- As the user: lstat of the absolute path needs search (x) on every parent, i.e. "found by browsing".
- hide_unreadable: also os.access(R_OK), as access-based enumeration hides it from a listing.
- Run as `python -m truesearchd.trim` under Popen(user=, group=, extra_groups=): the kernel (and
  ZFS NFSv4 ACLs) decide, the daemon never evaluates a mode itself.
- stdin: JSON {"root","hide_unreadable","paths"}; stdout: JSON list, one entry per path (null = drop).
"""

from __future__ import annotations

import json
import os
import subprocess
import sys


def stat_paths(root: str, paths: list[str], hide_unreadable: bool) -> list[dict | None]:
    out: list[dict | None] = []
    for rel in paths:
        full = os.path.join(root, rel) if rel else root
        try:
            st = os.lstat(full)
            if hide_unreadable and not os.access(full, os.R_OK, follow_symlinks=False):
                out.append(None)
                continue
        except OSError:
            out.append(None)
            continue
        out.append(
            {
                "ino": st.st_ino, "uid": st.st_uid, "gid": st.st_gid, "mode": st.st_mode,
                "size": st.st_size, "dev": st.st_dev,
                "mtime": st.st_mtime_ns, "btime": min(st.st_mtime_ns, st.st_ctime_ns),
                "atime": st.st_atime_ns,
            }
        )  # fmt: skip
    return out


def stat_paths_as(
    uid: int,
    gid: int,
    gids: list[int],
    root: str,
    paths: list[str],
    hide_unreadable: bool,
    run=subprocess.run,
) -> list[dict | None]:
    """Child process dropped to the identity; in-process for root (sees everything anyway)."""
    if uid == 0 or os.geteuid() != 0:
        return stat_paths(root, paths, hide_unreadable)
    req = json.dumps({"root": root, "hide_unreadable": hide_unreadable, "paths": paths})
    # Limit: 20000 paths of stat output is ~3 MB of JSON; the child stats in microseconds each.
    p = run(
        [sys.executable, "-m", "truesearchd.trim"],
        input=req.encode(), capture_output=True, user=uid, group=gid, extra_groups=gids, check=True,
        cwd="/",
    )  # fmt: skip
    return json.loads(p.stdout)


def main() -> int:
    req = json.load(sys.stdin)
    json.dump(stat_paths(req["root"], req["paths"], req["hide_unreadable"]), sys.stdout)
    return 0


if __name__ == "__main__":
    sys.exit(main())
