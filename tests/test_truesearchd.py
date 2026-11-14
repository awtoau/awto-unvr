# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""truesearchd (truenas/awto/awto-truesearchd, #357): XDR vs the .x, query, index, server over a real socket."""

from __future__ import annotations

import os
import re
import socket
import struct
import sys
import threading
from pathlib import Path

import pytest

PKG = Path(__file__).resolve().parent.parent / "truenas" / "awto" / "awto-truesearchd"
sys.path.insert(0, str(PKG))

from truesearchd import proto  # noqa: E402
from truesearchd.index import Index  # noqa: E402
from truesearchd.query import QueryError, to_sql  # noqa: E402
from truesearchd.server import Daemon, Server  # noqa: E402
from truesearchd.xdr import Schema, XdrError  # noqa: E402

XFILE = PKG / "truesearchd" / "proto" / "truenas_truesearch.x"
OK = proto.ST["RIX_OK"]
OPN = proto.OP


def test_schema_matches_x_text():
    text = XFILE.read_text()
    consts = dict(re.findall(r"^const (\w+)\s*=\s*(\d+);", text, re.M))
    assert {k: int(v) for k, v in consts.items()} == proto.SCHEMA.consts
    assert proto.SCHEMA.consts["RIX_PROTO_VERSION"] == 1
    for name in (
        "rix_open",
        "rix_stat",
        "rix_hit",
        "rix_welcome",
        "rix_opened",
        "rix_page",
        "rix_call",
        "rix_reply",
    ):
        body = re.search(rf"^struct {name} \{{(.*?)^\}};", text, re.M | re.S).group(1)
        body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
        declared = [m for m in re.findall(r"(\w+)(?:<\w+>)?;", body)]
        assert [f for f, _ in proto.SCHEMA.structs[name]] == declared


def _open(**kw):
    o = dict(dataset="tank/data", share="", scope="", query='* == "a*"cd',
             who={"uid": 1000, "gid": 1000, "gids": [1000, 10]}, flags=7, max_results=0, cnids=[])  # fmt: skip
    o.update(kw)
    return {"vers": 1, "xid": 7, "request": (OPN["RIX_OPEN"], o)}


def test_xdr_call_roundtrip_and_bytes():
    hello = {"vers": 1, "xid": 5, "request": (OPN["RIX_HELLO"], {"vers": 1})}
    wire = proto.encode_call(hello)
    # len=20, vers=1, xid=5, op=0, hello.vers=1
    assert wire == struct.pack(">5I", 16, 1, 5, 0, 1)
    assert proto.decode_call(wire[4:]) == hello
    c = _open(cnids=[1, 2**64 - 1])
    assert proto.decode_call(proto.encode_call(c)[4:]) == c


def test_xdr_reply_roundtrip():
    st = {"qfid": {"file_id": 2**40 + 5, "volume_id": 9},
          "unix_meta": {"ino": 5, "gen": 0, "uid": 1, "gid": 2, "mode": 0o100644},
          "size": 12, "mtime_sec": -5, "mtime_nsec": 1, "btime_sec": 2, "btime_nsec": 3,
          "atime_sec": 4, "atime_nsec": 5}  # fmt: skip
    page = {
        "hits": [{"path": "é/a.txt", "st": st}, {"path": "b", "st": None}],
        "more": True,
    }
    r = proto.reply(3, OK, (OPN["RIX_FETCH"], page))
    assert proto.decode_reply(proto.encode_reply(r)[4:]) == r
    e = proto.reply(3, proto.ST["RIX_BAD_QUERY"], detail="nope")
    assert proto.decode_reply(proto.encode_reply(e)[4:]) == e
    c = proto.reply(4, OK, (OPN["RIX_CLOSE"], None))
    assert proto.decode_reply(proto.encode_reply(c)[4:]) == c


def test_xdr_strict():
    wire = proto.encode_call(_open())[4:]
    with pytest.raises(XdrError):
        proto.decode_call(wire + b"\0\0\0\0")  # trailing
    with pytest.raises(XdrError):
        proto.decode_call(wire[:-4])  # short
    bad = bytearray(
        proto.encode_call(
            {
                "vers": 1,
                "xid": 1,
                "request": (1, _open()["request"][1] | {"dataset": "x"}),
            }
        )[4:]
    )
    # string "x" at fixed offset: vers,xid,op,len -> byte 16 is 'x', 17..19 padding
    bad[17] = 1
    with pytest.raises(XdrError):
        proto.decode_call(bytes(bad))
    with pytest.raises(XdrError):
        proto.SCHEMA.encode("rix_open", {})
    big = _open(query="x" * 5000)
    with pytest.raises(XdrError):
        proto.encode_call(big)


def test_schema_parses_arbitrary_x():
    s = Schema("const N = 2; enum e { A = 0, B = 1 }; union u switch (e t) { case A: unsigned int v; default: void; }; "
               "struct s { u x; string n<N>; bool *o; };")  # fmt: skip
    v = {"x": (0, 7), "n": "ab", "o": True}
    assert s.decode("s", s.encode("s", v)) == v
    with pytest.raises(XdrError):
        s.encode("s", {"x": (0, 7), "n": "abc", "o": None})


# ---- query ----


def test_query_finder_forms():
    sql, p = to_sql('(* == "foo*"cdw) || (kMDItemTextContent == "foo*"cdw)')
    assert "name_f GLOB" in sql and "OR" not in sql.replace(
        "OR name", ""
    )  # content term dropped
    assert p == ["foo*", "*[ ._()-]foo*"]
    sql, p = to_sql('kMDItemFSName == "Re?port*"')
    assert sql == "name GLOB ?" and p == ["Re[?]port*"]
    sql, p = to_sql('kMDItemFSSize > "1000" && kMDItemContentType == "public.folder"')
    assert "size > ?" in sql and "isdir = 1" in sql and p == [1000]
    sql, p = to_sql(
        "kMDItemFSContentChangeDate >= $time.iso(2026-01-01T00:00:00Z)".replace(
            ">=", ">"
        )
    )
    assert p == [1767225600]
    sql, p = to_sql(
        "InRange(kMDItemFSContentChangeDate, $time.iso(2026-01-01T00:00:00Z), $time.iso(2026-01-02T00:00:00Z))"
    )
    assert "BETWEEN" in sql and p == [1767225600, 1767312000]


def test_query_errors():
    for q in (
        "false",
        "",
        'kMDItemTextContent == "x"',
        '(kMDItemFSName == "x"',
        'kMDItemFSName == "x" &&',
        "name",
    ):
        with pytest.raises(QueryError):
            to_sql(q)


# ---- index + server ----


@pytest.fixture
def tree(tmp_path):
    root = tmp_path / "share"
    (root / "docs" / "sub").mkdir(parents=True)
    (root / "docs" / "Alpha Report.txt").write_text("hello")
    (root / "docs" / "sub" / "beta.png").write_bytes(b"x" * 2000)
    (root / "café.md").write_text("c")
    (root / "secret").mkdir()
    (root / "secret" / "alpha-hidden.txt").write_text("s")
    return root


@pytest.fixture
def server(tree, tmp_path):
    idx = Index(str(tmp_path / "i.db"))
    d = Daemon(idx, resolver=lambda ds: str(tree.parent) if ds == "tank/data" else None)
    sock = str(tmp_path / "s.sock")
    srv = Server(sock, d)
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    yield sock
    srv.shutdown()
    srv.server_close()


class Client:
    def __init__(self, path):
        self.s = socket.socket(socket.AF_UNIX)
        self.s.connect(path)
        self.xid = 0

    def call(self, op, val):
        self.xid += 1
        self.s.sendall(
            proto.encode_call({"vers": 1, "xid": self.xid, "request": (OPN[op], val)})
        )
        n = struct.unpack(">I", self._r(4))[0]
        r = proto.decode_reply(self._r(n))
        assert r["xid"] == self.xid
        return r["outcome"]

    def _r(self, n):
        b = b""
        while len(b) < n:
            b += self.s.recv(n - len(b))
        return b

    def search(self, query, **kw):
        o = _open(query=query, share="share", **kw)["request"][1]
        st, ans = self.call("RIX_OPEN", o)
        assert st == OK, ans
        _, opened = ans
        paths = []
        while True:
            _, page = self.call("RIX_FETCH", {"handle": opened["handle"], "limit": 2})[
                1
            ]
            paths += [h["path"] for h in page["hits"]]
            if not page["more"]:
                break
        self.call("RIX_CLOSE", {"handle": opened["handle"]})
        return sorted(paths), opened


def test_server_hello_and_search(server):
    c = Client(server)
    st, (op, w) = c.call("RIX_HELLO", {"vers": 1})
    assert st == OK and w["vers"] == 1 and w["max_page"] == 1000
    paths, opened = c.search('* == "alpha*"cd')
    assert (
        paths == ["docs/Alpha Report.txt", "secret/alpha-hidden.txt"]
        and not opened["truncated"]
    )
    assert c.search('kMDItemFSName == "alpha*"')[0] == [
        "secret/alpha-hidden.txt"
    ]  # case-sensitive without c
    assert c.search('* == "report"cdw')[0] == ["docs/Alpha Report.txt"]  # word prefix
    assert c.search('* == "cafe*"cd')[0] == ["café.md"]  # diacritic-insensitive
    assert c.search('kMDItemContentType == "public.image"')[0] == ["docs/sub/beta.png"]
    assert c.search('kMDItemFSSize > "1000"')[0] == ["docs/sub/beta.png"]
    assert c.search('kMDItemContentType == "public.folder"')[0] == [
        "docs",
        "docs/sub",
        "secret",
    ]


def test_server_scope_stat_cnids_and_errors(server, tree):
    c = Client(server)
    paths, _ = c.search('* == "*"', scope="docs")
    assert paths == ["docs/Alpha Report.txt", "docs/sub", "docs/sub/beta.png"]
    o = _open(query='* == "beta*"cd', share="share", scope="docs")["request"][1]
    _, (op, opened) = c.call("RIX_OPEN", o)
    _, page = c.call("RIX_FETCH", {"handle": opened["handle"], "limit": 0})[1]
    (hit,) = page["hits"]
    assert hit["path"] == "docs/sub/beta.png"
    st = hit["st"]
    assert (
        st["size"] == 2000
        and st["unix_meta"]["ino"]
        == st["qfid"]["file_id"]
        == (tree / "docs/sub/beta.png").stat().st_ino
    )
    o["cnids"] = [st["qfid"]["file_id"] + 1]
    _, (op, opened) = c.call("RIX_OPEN", o)
    assert opened["ready"] == 0
    o["cnids"] = [st["qfid"]["file_id"]]
    assert c.call("RIX_OPEN", o)[1][1]["ready"] == 1
    # errors
    bad = dict(o, query="false", cnids=[])
    assert c.call("RIX_OPEN", bad)[0] == proto.ST["RIX_BAD_QUERY"]
    assert (
        c.call("RIX_OPEN", dict(o, scope="../x", cnids=[]))[0]
        == proto.ST["RIX_BAD_SCOPE"]
    )
    assert (
        c.call("RIX_OPEN", dict(o, scope="nope", cnids=[]))[0]
        == proto.ST["RIX_BAD_SCOPE"]
    )
    assert (
        c.call("RIX_OPEN", dict(o, dataset="x/y", cnids=[]))[0]
        == proto.ST["RIX_NO_INDEX"]
    )
    assert (
        c.call("RIX_FETCH", {"handle": 99999, "limit": 1})[0]
        == proto.ST["RIX_BAD_HANDLE"]
    )
    assert c.call("RIX_HELLO", {"vers": 2})[0] == proto.ST["RIX_BAD_VERSION"]


def test_server_max_results_truncated(server):
    c = Client(server)
    paths, opened = c.search('* == "*"', max_results=2)
    assert len(paths) == 2 and opened["truncated"] and opened["ready"] == 2


@pytest.mark.skipif(os.geteuid() == 0, reason="permission trim needs a non-root runner")
def test_trim_hides_unreadable(server, tree):
    os.chmod(tree / "secret", 0)
    try:
        paths, _ = Client(server).search('* == "alpha*"cd')
        assert paths == ["docs/Alpha Report.txt"]  # hit under 0-mode dir fails lstat
    finally:
        os.chmod(tree / "secret", 0o755)
