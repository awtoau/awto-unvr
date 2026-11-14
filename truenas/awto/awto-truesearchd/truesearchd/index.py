# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""SQLite filename index, one row per file, one target per (dataset, share root).

- Scan = periodic walk (no inotify yet). Stays on the root's st_dev: child datasets are their own target.
- No FTS5: queries are GLOB scans over folded name columns; fine to a few million rows.
- Access control is not in the index; the server re-stats every hit as the asking identity (trim.py).
"""

from __future__ import annotations

import os
import sqlite3
import threading
import time

from .query import fold

SCHEMA = """
CREATE TABLE IF NOT EXISTS targets(
  id TEXT PRIMARY KEY, dataset TEXT, share TEXT, root TEXT,
  last_scan REAL DEFAULT 0, scanning REAL DEFAULT 0);
CREATE TABLE IF NOT EXISTS files(
  target TEXT, path TEXT, name TEXT, name_c TEXT, name_f TEXT, path_c TEXT, path_f TEXT, ext TEXT,
  ino INTEGER, size INTEGER, mtime INTEGER, btime INTEGER, uid INTEGER, gid INTEGER, isdir INTEGER,
  stamp REAL, PRIMARY KEY(target, path)) WITHOUT ROWID;
"""
SKIP_DIRS = {".zfs"}
BATCH = 5000


def target_id(dataset: str, share: str) -> str:
    return f"{dataset}\t{share}"


# serve + index start together on a fresh DB; the WAL switch ignores the busy timeout and
# fails "database is locked" while the other creates the schema (ms). 10 x 0.1 s = 1 s cap.
WAL_TRIES, WAL_WAIT_S = 10, 0.1


def connect(db_path: str) -> sqlite3.Connection:
    con = sqlite3.connect(db_path, timeout=30, check_same_thread=False)
    for i in range(WAL_TRIES):
        try:
            con.execute("PRAGMA journal_mode=WAL")
            break
        except sqlite3.OperationalError as e:
            if "locked" not in str(e) or i == WAL_TRIES - 1:
                raise
            time.sleep(WAL_WAIT_S)
    con.execute("PRAGMA synchronous=NORMAL")
    con.executescript(SCHEMA)
    return con


class Index:
    def __init__(self, db_path: str):
        self.db_path = db_path
        self._local = threading.local()
        self._scan_lock = threading.Lock()
        self._scanning: dict[str, threading.Event] = {}
        connect(db_path).close()

    @property
    def con(self) -> sqlite3.Connection:
        if not hasattr(self._local, "con"):
            self._local.con = connect(self.db_path)
        return self._local.con

    def register(self, dataset: str, share: str, root: str) -> str:
        tid = target_id(dataset, share)
        self.con.execute(
            "INSERT INTO targets(id,dataset,share,root) VALUES(?,?,?,?) "
            "ON CONFLICT(id) DO UPDATE SET root=excluded.root",
            (tid, dataset, share, root),
        )
        self.con.commit()
        return tid

    def targets(self) -> list[tuple]:
        return self.con.execute(
            "SELECT id,dataset,share,root,last_scan FROM targets"
        ).fetchall()

    def last_scan(self, tid: str) -> float:
        row = self.con.execute(
            "SELECT last_scan FROM targets WHERE id=?", (tid,)
        ).fetchone()
        return row[0] if row else 0

    def scan(self, tid: str, root: str) -> int:
        """Walk `root`, replace the target's rows. Returns the number of entries."""
        started = time.time()
        con = self.con
        rows, count = [], 0
        try:
            root_dev = os.stat(root).st_dev
        except OSError:
            return 0
        stack = [""]
        while stack:
            rel = stack.pop()
            try:
                with os.scandir(os.path.join(root, rel) if rel else root) as it:
                    entries = list(it)
            except OSError:
                continue
            for e in entries:
                if rel == "" and e.name in SKIP_DIRS:
                    continue
                try:
                    st = e.stat(follow_symlinks=False)
                except OSError:
                    continue
                isdir = e.is_dir(follow_symlinks=False)
                if isdir and st.st_dev != root_dev:
                    continue
                path = f"{rel}/{e.name}" if rel else e.name
                if isdir:
                    stack.append(path)
                rows.append(_row(tid, path, e.name, isdir, st, started))
                count += 1
                if len(rows) >= BATCH:
                    _flush(con, rows)
                    rows = []
        _flush(con, rows)
        con.execute("DELETE FROM files WHERE target=? AND stamp<?", (tid, started))
        con.execute("UPDATE targets SET last_scan=? WHERE id=?", (started, tid))
        con.commit()
        return count

    def ensure(self, tid: str, root: str, wait: float) -> bool:
        """True when the target has been scanned at least once. Starts a scan thread if not;
        waits at most `wait` s (a Finder query must not hang on a first walk of a big dataset)."""
        if self.last_scan(tid) > 0:
            return True
        with self._scan_lock:
            ev = self._scanning.get(tid)
            if ev is None:
                ev = self._scanning[tid] = threading.Event()

                def run():
                    try:
                        self.scan(tid, root)
                    finally:
                        ev.set()
                        with self._scan_lock:
                            self._scanning.pop(tid, None)

                threading.Thread(target=run, name=f"scan-{tid}", daemon=True).start()
        ev.wait(wait)
        return self.last_scan(tid) > 0

    def search(
        self, tid: str, scope: str, where: str, params: list, limit: int
    ) -> list[tuple]:
        sql = (
            "SELECT path,ino,size,mtime,btime,uid,gid,isdir FROM files WHERE target=? AND "
            + ("path GLOB ? AND " if scope else "")
            + f"({where}) LIMIT ?"
        )
        args = [tid] + ([_scope_glob(scope)] if scope else []) + params + [limit]
        return self.con.execute(sql, args).fetchall()


def _scope_glob(scope: str) -> str:
    esc = "".join(f"[{c}]" if c in "*?[" else c for c in scope)
    return esc + "/*"


def _row(tid, path, name, isdir, st, stamp):
    ext = "" if isdir or "." not in name else name.rsplit(".", 1)[1].lower()
    return (
        tid, path, name, fold(name, "c"), fold(name, "cd"), fold(path, "c"), fold(path, "cd"), ext,
        st.st_ino, st.st_size, int(st.st_mtime), int(min(st.st_mtime, st.st_ctime)),
        st.st_uid, st.st_gid, int(isdir), stamp,
    )  # fmt: skip


def _flush(con, rows):
    if rows:
        con.executemany(
            "INSERT OR REPLACE INTO files VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)", rows
        )
        con.commit()
