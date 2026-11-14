# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""truesearchd socket server: rix v1 over a unix stream socket (see proto.py)."""

from __future__ import annotations

import itertools
import os
import pwd
import socket
import socketserver
import struct
import time

from . import proto
from .index import Index
from .query import QueryError, to_sql
from .trim import stat_paths_as
from .xdr import XdrError

C = proto.C
ST = proto.ST
OP = proto.OP
CANDIDATE_CAP = (
    20000  # rows pulled per OPEN before trimming; stat of 20k paths is ~0.1 s
)
HANDLE_IDLE_SECONDS = (
    60  # Samba fetches immediately; an abandoned handle must not pin memory
)
FIRST_SCAN_WAIT = (
    5.0  # a Finder query waits this long for a first walk, then gets NO_INDEX
)
HYPER_MASK39 = (1 << 39) - 1


def mountinfo_resolver(path: str = "/proc/self/mountinfo"):
    """dataset name -> mountpoint (zfs mounts), per the mount's source (Samba's own lookup key)."""

    def resolve(dataset: str) -> str | None:
        with open(path) as f:
            for line in f:
                left, _, right = line.partition(" - ")
                fs = right.split()
                if len(fs) >= 2 and fs[0] == "zfs" and fs[1] == dataset:
                    return left.split()[4].encode().decode("unicode_escape")
        return None

    return resolve


def valid_rel(p: str) -> bool:
    if p == "":
        return True
    if p.startswith("/") or p.endswith("/"):
        return False
    return all(c not in ("", ".", "..") for c in p.split("/"))


def peer_identity(sock: socket.socket) -> dict:
    uid_gid = struct.unpack(
        "3i",
        sock.getsockopt(socket.SOL_SOCKET, socket.SO_PEERCRED, struct.calcsize("3i")),
    )
    uid = uid_gid[1]
    try:
        pw = pwd.getpwuid(uid)
        return {
            "uid": uid,
            "gid": pw.pw_gid,
            "gids": os.getgrouplist(pw.pw_name, pw.pw_gid),
        }
    except KeyError:
        return {"uid": uid, "gid": uid_gid[2], "gids": [uid_gid[2]]}


class Handle:
    def __init__(self, hits, truncated):
        self.hits, self.pos, self.truncated = hits, 0, truncated
        self.touched = time.monotonic()


class Daemon:
    def __init__(self, index: Index, resolver=None, max_results=None):
        self.index = index
        self.resolve = resolver or mountinfo_resolver()
        self.max_results = max_results or C["RIX_MAX_RESULTS"]
        self.max_page = C["RIX_MAX_PAGE"]
        self._handles = itertools.count(1)

    # -- one call -> one reply dict --
    def handle(self, call: dict, peer: dict, handles: dict) -> dict:
        xid = call["xid"]
        if call["vers"] != proto.PROTO_VERSION:
            return proto.reply(
                xid, ST["RIX_BAD_VERSION"], detail="unknown protocol version"
            )
        op, val = call["request"]
        now = time.monotonic()
        for h in [
            h for h, v in handles.items() if now - v.touched > HANDLE_IDLE_SECONDS
        ]:
            del handles[h]
        if op == OP["RIX_HELLO"]:
            if val["vers"] != proto.PROTO_VERSION:
                return proto.reply(
                    xid, ST["RIX_BAD_VERSION"], detail="unknown protocol version"
                )
            return proto.reply(
                xid, ST["RIX_OK"],
                (op, {"vers": proto.PROTO_VERSION, "max_results": self.max_results,
                      "max_page": self.max_page, "handle_idle_seconds": HANDLE_IDLE_SECONDS}),
            )  # fmt: skip
        if op == OP["RIX_OPEN"]:
            return self._open(xid, val, peer, handles)
        if op == OP["RIX_FETCH"]:
            h = handles.get(val["handle"])
            if h is None:
                return proto.reply(
                    xid, ST["RIX_BAD_HANDLE"], detail="unknown or expired handle"
                )
            h.touched = now
            limit = min(val["limit"] or self.max_page, self.max_page)
            page = h.hits[h.pos : h.pos + limit]
            h.pos += len(page)
            return proto.reply(
                xid, ST["RIX_OK"], (op, {"hits": page, "more": h.pos < len(h.hits)})
            )
        if op == OP["RIX_CLOSE"]:
            handles.pop(val["handle"], None)
            return proto.reply(xid, ST["RIX_OK"], (op, None))
        return proto.reply(xid, ST["RIX_INTERNAL"], detail="bad op")

    def _open(self, xid, o, peer, handles):
        if not (valid_rel(o["share"]) and valid_rel(o["scope"])):
            return proto.reply(xid, ST["RIX_BAD_SCOPE"], detail="not a relative path")
        try:
            where, params = to_sql(o["query"])
        except QueryError as e:
            return proto.reply(xid, ST["RIX_BAD_QUERY"], detail=str(e))
        mount = self.resolve(o["dataset"])
        if mount is None:
            return proto.reply(xid, ST["RIX_NO_INDEX"], detail="dataset not mounted")
        root = os.path.join(mount, o["share"]) if o["share"] else mount
        if not os.path.isdir(os.path.join(root, o["scope"]) if o["scope"] else root):
            return proto.reply(
                xid, ST["RIX_BAD_SCOPE"], detail="scope is not a directory"
            )
        tid = self.index.register(o["dataset"], o["share"], root)
        if not self.index.ensure(tid, root, FIRST_SCAN_WAIT):
            return proto.reply(
                xid, ST["RIX_NO_INDEX"], detail="first index of this share is running"
            )

        # SO_PEERCRED: only a root peer (smbd) may assert an identity.
        who = o["who"] if peer["uid"] == 0 else peer
        want_max = min(o["max_results"] or self.max_results, self.max_results)
        flags = o["flags"]
        cnids = set(o["cnids"])
        rows = self.index.search(tid, o["scope"], where, params, CANDIDATE_CAP)
        capped = len(rows) >= CANDIDATE_CAP
        if cnids:
            rows = [r for r in rows if r[1] in cnids or (r[1] & HYPER_MASK39) in cnids]
        rows = [r for r in rows if r[0] != ""]
        stats = [None] * len(rows)
        if flags & (
            proto.OPEN_TRIM_BY_IDENTITY
            | proto.OPEN_WANT_STAT
            | proto.OPEN_HIDE_UNREADABLE
        ):
            paths = [r[0] for r in rows]
            if flags & proto.OPEN_TRIM_BY_IDENTITY:
                stats = stat_paths_as(
                    who["uid"], who["gid"], list(who["gids"]), root, paths,
                    bool(flags & proto.OPEN_HIDE_UNREADABLE),
                )  # fmt: skip
            else:
                stats = stat_paths_as(0, 0, [], root, paths, False)
            keep = [(r, s) for r, s in zip(rows, stats) if s is not None]
        else:
            keep = list(zip(rows, stats))
        truncated = capped or len(keep) > want_max
        keep = keep[:want_max]
        volume_id = _volume_id(root)
        hits = []
        for r, s in keep:
            st = (
                _rix_stat(s, volume_id)
                if (s is not None and flags & proto.OPEN_WANT_STAT)
                else None
            )
            hits.append({"path": r[0], "st": st})
        handle = next(self._handles)
        handles[handle] = Handle(hits, truncated)
        return proto.reply(
            xid,
            ST["RIX_OK"],
            (
                OP["RIX_OPEN"],
                {"handle": handle, "ready": len(hits), "truncated": truncated},
            ),
        )


def _volume_id(root: str) -> int:
    """statfs f_fsid, as smbd's QFid volume_id; ZFS keeps it in the pool."""
    try:
        return os.statvfs(root).f_fsid
    except OSError:
        return 0


def _ns(ns: int) -> tuple[int, int]:
    return ns // 10**9, ns % 10**9


def _rix_stat(s: dict, volume_id: int) -> dict:
    # file_id = ino: STATX_GEN is not read, so Samba must run without zfs_core file ids (gen 0 case).
    m, b, a = _ns(s["mtime"]), _ns(s["btime"]), _ns(s["atime"])
    return {
        "qfid": {"file_id": s["ino"], "volume_id": volume_id},
        "unix_meta": {"ino": s["ino"], "gen": 0, "uid": s["uid"], "gid": s["gid"], "mode": s["mode"]},
        "size": s["size"],
        "mtime_sec": m[0], "mtime_nsec": m[1], "btime_sec": b[0], "btime_nsec": b[1],
        "atime_sec": a[0], "atime_nsec": a[1],
    }  # fmt: skip


class _Handler(socketserver.BaseRequestHandler):
    def _read(self, n: int) -> bytes | None:
        buf = b""
        while len(buf) < n:
            chunk = self.request.recv(n - len(buf))
            if not chunk:
                return None
            buf += chunk
        return buf

    def handle(self):
        daemon: Daemon = self.server.daemon
        peer = peer_identity(self.request)
        handles: dict = {}
        while True:
            head = self._read(4)
            if head is None:
                return
            (n,) = struct.unpack(">I", head)
            if n > proto.MAX_FRAME:
                return
            body = self._read(n)
            if body is None:
                return
            try:
                call = proto.decode_call(body)
            except XdrError:
                return  # no xid to answer with
            try:
                reply = daemon.handle(call, peer, handles)
                data = proto.encode_reply(reply)
            except Exception as e:  # noqa: BLE001 - one bad query must not kill the connection
                data = proto.encode_reply(
                    proto.reply(
                        call["xid"], ST["RIX_INTERNAL"], detail=type(e).__name__
                    )
                )
            self.request.sendall(data)


class Server(socketserver.ThreadingMixIn, socketserver.UnixStreamServer):
    daemon_threads = True

    def __init__(self, path: str, daemon: Daemon):
        self.daemon = daemon
        if os.path.exists(path):
            os.unlink(path)
        super().__init__(path, _Handler)
        os.chmod(path, 0o666)  # non-root peers (webshare) get their own identity
