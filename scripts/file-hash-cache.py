#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Lazy SHA-256 cache in user.* xattrs on woomera's data filesystems (#378).

  ./dev.py file-hash install|uninstall   # copy to the box, systemd timers per filesystem
  ./dev.py file-hash status              # heartbeat + backlog per filesystem, flags a stalled pass
  ./dev.py file-hash get PATH            # hash, age and staleness of one file (path on the box)
  python3 file-hash-cache.py scan ROOT --name NAME [--budget-s N]   # on the box

- xattrs: user.sha256, user.sha256.mtime_ns, user.sha256.size (what was hashed), user.sha256.at (epoch s).
- Stale = mtime or size differ from what was hashed. Fresh files cost one stat per pass.
- Heartbeat /var/lib/awto-filehash/NAME.json: pass start/end, counts, backlog, oldest-unhashed age.
- status flags a heartbeat older than STALL_FACTOR x the pass interval (daemon stopped or starved).
- Files modified < SETTLE_S ago are skipped (still being written).
- Passes stop at the time budget and resume next run; first pass over 8 TB takes days.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
try:
    from _repo import make_log

    log = make_log("file-hash-cache")
except ImportError:  # on the box: no repo helpers

    def log(msg: str) -> None:
        print(time.strftime("%Y-%m-%dT%H:%M:%S%z"), msg, file=sys.stderr, flush=True)


A_HASH = "user.sha256"
A_MTIME = "user.sha256.mtime_ns"
A_SIZE = "user.sha256.size"
A_AT = "user.sha256.at"
SKIP_DIRS = {"#recycle", ".tools", ".snapshots"}
# Writers close files within seconds; 60 s covers a slow SMB/NFS client flush.
SETTLE_S = 60
CHUNK = 1 << 20
STATE_DIR = Path("/var/lib/awto-filehash")
FILESYSTEMS = {"mirror": "/srv/mirror", "single": "/srv/single"}
# Timer fires 5 min after a pass ends; pass budget 10 min -> a healthy cycle is <= 15 min.
BUDGET_S = 600
TIMEOUT_S = 720  # 1.2 x budget: the last hashed file may overrun the budget
INTERVAL_S = BUDGET_S + 300
STALL_FACTOR = 2  # heartbeat older than 2 cycles = not running
BOX_COPY = "/usr/local/bin/file-hash-cache.py"
UNIT = "awto-filehash@"


def read_attrs(path: str) -> dict | None:
    try:
        return {
            "sha256": os.getxattr(path, A_HASH, follow_symlinks=False).decode(),
            "mtime_ns": int(os.getxattr(path, A_MTIME, follow_symlinks=False)),
            "size": int(os.getxattr(path, A_SIZE, follow_symlinks=False)),
            "at": int(os.getxattr(path, A_AT, follow_symlinks=False)),
        }
    except (OSError, ValueError):
        return None


def is_stale(st: os.stat_result, attrs: dict | None) -> bool:
    return (
        attrs is None
        or attrs["mtime_ns"] != st.st_mtime_ns
        or attrs["size"] != st.st_size
    )


def hash_file(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while chunk := f.read(CHUNK):
            h.update(chunk)
    return h.hexdigest()


def write_attrs(path: str, digest: str, st: os.stat_result, now: float) -> None:
    for k, v in (
        (A_HASH, digest),
        (A_MTIME, str(st.st_mtime_ns)),
        (A_SIZE, str(st.st_size)),
        (A_AT, str(int(now))),
    ):
        os.setxattr(path, k, v.encode(), follow_symlinks=False)


def walk(root: str):
    for dirpath, dirs, files in os.walk(root):
        dirs[:] = sorted(d for d in dirs if d not in SKIP_DIRS)
        for name in sorted(files):
            yield os.path.join(dirpath, name)


def scan(root: str, name: str, budget_s: float, state_dir: Path = STATE_DIR) -> dict:
    """One resumable pass. Returns the heartbeat dict (also written to state_dir/NAME.json)."""
    started = time.time()
    hb = {
        "name": name,
        "root": root,
        "started": int(started),
        "finished": None,
        "complete": False,
        "hashed": 0,
        "fresh": 0,
        "settling": 0,
        "errors": 0,
        "backlog": 0,
        "oldest_backlog_age_s": 0,
        "interval_s": INTERVAL_S,
    }
    out_of_time = False
    for path in walk(root):
        try:
            st = os.lstat(path)
            if not os.path.isfile(path) or os.path.islink(path):
                continue
            attrs = read_attrs(path)
            if not is_stale(st, attrs):
                hb["fresh"] += 1
                continue
            age = started - st.st_mtime
            if age < SETTLE_S:
                hb["settling"] += 1
                continue
            if out_of_time or time.time() - started > budget_s:
                out_of_time = True
                hb["backlog"] += 1
                hb["oldest_backlog_age_s"] = max(hb["oldest_backlog_age_s"], int(age))
                continue
            digest = hash_file(path)
            st2 = os.lstat(path)
            if st2.st_mtime_ns != st.st_mtime_ns or st2.st_size != st.st_size:
                hb["settling"] += 1  # changed while hashing: next pass
                continue
            write_attrs(path, digest, st2, time.time())
            hb["hashed"] += 1
        except OSError as e:
            hb["errors"] += 1
            log(f"{path}: {e}")
    hb["finished"] = int(time.time())
    hb["complete"] = not out_of_time
    state_dir.mkdir(parents=True, exist_ok=True)
    tmp = state_dir / f"{name}.json.tmp"
    tmp.write_text(json.dumps(hb))
    tmp.replace(state_dir / f"{name}.json")
    return hb


def describe_heartbeat(hb: dict | None, now: float) -> str:
    if hb is None:
        return "no heartbeat: never ran"
    age = now - (hb["finished"] or hb["started"])
    stalled = age > STALL_FACTOR * hb["interval_s"]
    return (
        f"{'STALLED ' if stalled else ''}last pass ended {int(age)}s ago "
        f"({'complete' if hb['complete'] else 'budget hit'}): hashed {hb['hashed']}, "
        f"fresh {hb['fresh']}, settling {hb['settling']}, errors {hb['errors']}, "
        f"backlog {hb['backlog']} (oldest {hb['oldest_backlog_age_s']}s)"
    )


def cmd_status_here() -> int:
    for name in FILESYSTEMS:
        f = STATE_DIR / f"{name}.json"
        hb = json.loads(f.read_text()) if f.exists() else None
        print(f"{name}: {describe_heartbeat(hb, time.time())}")
    return 0


def cmd_get_here(path: str) -> int:
    st = os.lstat(path)
    attrs = read_attrs(path)
    now = time.time()
    if attrs is None:
        print(json.dumps({"path": path, "sha256": None, "state": "unhashed"}))
        return 1
    stale = is_stale(st, attrs)
    print(
        json.dumps(
            {
                "path": path,
                "sha256": attrs["sha256"],
                "state": "stale" if stale else "fresh",
                "hash_age_s": int(now - attrs["at"]),
                "mtime_ns": st.st_mtime_ns,
            }
        )
    )
    return 2 if stale else 0


def service_text() -> str:
    return f"""# managed by scripts/file-hash-cache.py (#378)
[Unit]
Description=lazy SHA-256 xattr cache of /srv/%i
ConditionPathIsMountPoint=/srv/%i

[Service]
Type=oneshot
Nice=19
IOSchedulingClass=idle
TimeoutStartSec={TIMEOUT_S}
StateDirectory=awto-filehash
ExecStart=/usr/bin/python3 {BOX_COPY} scan /srv/%i --name %i --budget-s {BUDGET_S}
"""


def timer_text() -> str:
    return """# managed by scripts/file-hash-cache.py (#378)
[Unit]
Description=lazy SHA-256 xattr cache of /srv/%i

[Timer]
OnBootSec=5min
OnUnitInactiveSec=5min

[Install]
WantedBy=timers.target
"""


def ssh(cmd: str, check: bool = True) -> str:
    log(f"box$ {cmd[:160]}")
    r = subprocess.run(
        [sys.executable, str(HERE / "ssh-woomera.py"), "--", cmd],
        capture_output=True,
        text=True,
        timeout=60,  # one systemctl/cp over LAN ssh, expected <3 s
    )
    out = (r.stdout + r.stderr).strip()
    if out:
        print(out)
    if check and r.returncode:
        raise SystemExit(f"remote command failed rc={r.returncode}")
    return out


def put(path: str, text: str) -> None:
    import base64

    ssh(f"echo {base64.b64encode(text.encode()).decode()} | base64 -d > {path}")


def cmd_install() -> int:
    put(BOX_COPY, Path(__file__).read_text())
    ssh(f"chmod 0755 {BOX_COPY}")
    put(f"/etc/systemd/system/{UNIT}.service", service_text())
    put(f"/etc/systemd/system/{UNIT}.timer", timer_text())
    ssh(
        f"systemd-analyze verify /etc/systemd/system/{UNIT}.service /etc/systemd/system/{UNIT}.timer"
    )
    ssh("systemctl daemon-reload")
    for n in FILESYSTEMS:
        ssh(f"systemctl enable --now {UNIT}{n}.timer")
    return 0


def cmd_uninstall() -> int:
    for n in FILESYSTEMS:
        ssh(f"systemctl disable --now {UNIT}{n}.timer", check=False)
    ssh(
        f"rm -f /etc/systemd/system/{UNIT}.service /etc/systemd/system/{UNIT}.timer {BOX_COPY}; systemctl daemon-reload"
    )
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("install")
    sub.add_parser("uninstall")
    sub.add_parser("status")
    g = sub.add_parser("get")
    g.add_argument("path")
    s = sub.add_parser("scan")
    s.add_argument("root")
    s.add_argument("--name", required=True)
    s.add_argument("--budget-s", type=float, default=BUDGET_S)
    s.add_argument("--state-dir", default=str(STATE_DIR))
    ap.add_argument("--here", action="store_true", help="run status/get on this host")
    a = ap.parse_args()
    if a.cmd == "install":
        return cmd_install()
    if a.cmd == "uninstall":
        return cmd_uninstall()
    if a.cmd == "scan":
        hb = scan(a.root, a.name, a.budget_s, Path(a.state_dir))
        log(describe_heartbeat(hb, time.time()))
        return 0
    if a.cmd == "status":
        if a.here:
            return cmd_status_here()
        ssh(f"python3 {BOX_COPY} --here status")
        return 0
    if a.here:
        return cmd_get_here(a.path)
    ssh(f"python3 {BOX_COPY} --here get {a.path!r}", check=False)
    return 0


if __name__ == "__main__":
    sys.exit(main())
