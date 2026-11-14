# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Spotlight query text -> SQL over the files table (grammar: Samba mdssvc es_lexer.l / es_parser.y).

- Subset served: name, path, size, owner/group ids, dates, folder/extension content types.
- Unknown attribute or type: that term is dropped (Samba's behaviour), so Finder's
  `(* == "x"cdw) || (kMDItemTextContent == "x"cdw)` still searches names.
- Whole query dropped, a bare true/false, or a syntax error: QueryError -> RIX_BAD_QUERY.
- Modifiers on a string value: c case-insensitive, d diacritic-insensitive, w word prefix.
"""

from __future__ import annotations

import re
import unicodedata
from datetime import datetime, timezone

SP_RAW_TIME_OFFSET = 978307200  # 2001-01-01T00:00:00Z in unix seconds


class QueryError(ValueError):
    pass


_ASC = r"[A-Za-z0-9_*:\-.]|[\u0080-\U0010ffff]"
_TOK = re.compile(
    r"""\s*(?:
    (?P<iso>\$time\.iso)
  | (?P<phrase>"(?:[^"\\]|\\.)*")
  | (?P<op>&&|\|\||==|!=|=|<|>|,|\(|\))
  | (?P<word>(?:%s)+)
  )"""
    % _ASC,
    re.X,
)


def lex(text: str) -> list[tuple[str, str]]:
    out, pos = [], 0
    text = text.rstrip()
    while pos < len(text):
        m = _TOK.match(text, pos)
        if not m:
            raise QueryError(f"cannot tokenize at {text[pos : pos + 16]!r}")
        pos = m.end()
        out.append((m.lastgroup, m.group(m.lastgroup)))
    return out


class _Parser:
    def __init__(self, toks):
        self.t, self.i = toks, 0

    def peek(self):
        return self.t[self.i] if self.i < len(self.t) else (None, None)

    def take(self):
        tok = self.peek()
        self.i += 1
        return tok

    def expect(self, val):
        k, v = self.take()
        if v != val or k not in ("op", "word"):
            raise QueryError(f"expected {val!r}")

    def parse(self):
        node = self.expr_or()
        if self.i != len(self.t):
            raise QueryError("trailing tokens")
        return node

    def expr_or(self):
        node = self.expr_and()
        while self.peek() == ("op", "||"):
            self.take()
            node = _join("or", node, self.expr_and())
        return node

    def expr_and(self):
        node = self.primary()
        while self.peek() == ("op", "&&"):
            self.take()
            node = _join("and", node, self.primary())
        return node

    def primary(self):
        k, v = self.peek()
        if (k, v) == ("op", "("):
            self.take()
            node = self.expr_or()
            self.expect(")")
            return node
        if k == "word" and v in ("true", "false"):
            raise QueryError("boolean query")
        if k == "word" and v == "InRange":
            self.take()
            self.expect("(")
            attr = self.word()
            self.expect(",")
            lo = self.value()
            self.expect(",")
            hi = self.value()
            self.expect(")")
            return ("range", attr, lo, hi)
        attr = self.word()
        k, op = self.take()
        if k != "op" or op not in ("==", "=", "!=", "<", ">"):
            raise QueryError("expected comparison operator")
        val = self.value()
        mods = ""
        while self.peek()[0] == "word":  # `match WORD`: modifiers such as cdw
            mods += self.take()[1]
        return ("cmp", attr, "==" if op == "=" else op, val, mods)

    def word(self):
        k, v = self.take()
        if k != "word":
            raise QueryError("expected word")
        return v

    def value(self):
        k, v = self.take()
        if k == "phrase":
            return (
                "str",
                re.sub(
                    r"\\(.)",
                    lambda m: "\x01" if m.group(1) == "*" else m.group(1),
                    v[1:-1],
                ),
            )
        if k == "iso":
            self.expect("(")
            w = self.word()
            self.expect(")")
            try:
                dt = datetime.strptime(w, "%Y-%m-%dT%H:%M:%SZ").replace(
                    tzinfo=timezone.utc
                )
            except ValueError as e:
                raise QueryError("bad $time.iso") from e
            return ("date", int(dt.timestamp()))
        if k == "word":
            return ("str", v)
        raise QueryError("expected value")


def _join(op, a, b):
    if a is None:
        return b
    if b is None:
        return a
    return (op, a, b)


def parse(text: str):
    return _Parser(lex(text)).parse()


# ---- SQL ----

NAME_ATTRS = {"*", "kMDItemFSName", "kMDItemDisplayName"}
DATE_COLS = {
    "kMDItemFSContentChangeDate": "mtime",
    "kMDItemContentModificationDate": "mtime",
    "kMDItemAttributeChangeDate": "mtime",
    "kMDItemFSCreationDate": "btime",
    "kMDItemContentCreationDate": "btime",
}
NUM_COLS = {
    "kMDItemFSSize": "size",
    "kMDItemFSOwnerUserID": "uid",
    "kMDItemFSOwnerGroupID": "gid",
}
TYPE_ATTRS = {"kMDItemContentType", "kMDItemContentTypeTree", "_kMDItemGroupId"}
_EXT = {
    "image": "jpg jpeg png gif tif tiff bmp heic webp",
    "movie": "mov mp4 m4v avi mkv webm mpg mpeg",
    "audio": "mp3 m4a aac wav flac ogg aiff",
    "pdf": "pdf",
    "text": "txt md rtf html xml json csv",
}
_TYPE_MAP = {
    "public.folder": "dir",
    "public.directory": "dir",
    "9": "dir",
    "public.image": "image",
    "13": "image",
    "public.jpeg": "jpg jpeg",
    "public.png": "png",
    "public.tiff": "tif tiff",
    "com.compuserve.gif": "gif",
    "public.movie": "movie",
    "7": "movie",
    "public.audio": "audio",
    "10": "audio",
    "public.mp3": "mp3",
    "com.adobe.pdf": "pdf",
    "11": "pdf",
    "public.text": "text",
    "public.plain-text": "txt",
    "public.html": "html",
    "public.xml": "xml",
}


def fold(s: str, mods: str = "cd") -> str:
    s = unicodedata.normalize("NFC", s)
    if "c" in mods:
        s = s.casefold()
    if "d" in mods:
        s = "".join(
            c for c in unicodedata.normalize("NFD", s) if not unicodedata.combining(c)
        )
        s = unicodedata.normalize("NFC", s)
    return s


def _glob(value: str) -> str:
    out = []
    for ch in value:
        if ch == "*":
            out.append("*")
        elif ch == "\x01":
            out.append("[*]")
        elif ch in "?[":
            out.append(f"[{ch}]")
        else:
            out.append(ch)
    return "".join(out)


def _str_cond(
    col_base: str, op: str, value: str, mods: str, params: list
) -> str | None:
    if op not in ("==", "!="):
        return None
    c, d, w = "c" in mods, "d" in mods, "w" in mods
    col = {
        (True, True): f"{col_base}_f",
        (False, True): f"{col_base}_f",
        (True, False): f"{col_base}_c",
    }.get((c, d), col_base)
    pat = _glob(fold(value, ("c" if c else "") + ("d" if d else "")))
    if w:
        pat = pat if pat.endswith("*") else pat + "*"
        params += [pat, "*[ ._()-]" + pat]
        cond = f"({col} GLOB ? OR {col} GLOB ?)"
    else:
        params.append(pat)
        cond = f"{col} GLOB ?"
    return f"NOT {cond}" if op == "!=" else cond


def _num(v) -> int | None:
    kind, val = v
    if kind == "date":
        return val
    try:
        return int(float(val))
    except ValueError:
        return None


def _node_sql(node, params: list) -> str | None:
    kind = node[0]
    if kind in ("and", "or"):
        a, b = _node_sql(node[1], params), _node_sql(node[2], params)
        if a is None:
            return b
        if b is None:
            return a
        return f"({a} {kind.upper()} {b})"
    if kind == "range":
        _, attr, lo, hi = node
        col = DATE_COLS.get(attr) or NUM_COLS.get(attr)
        lo_n, hi_n = _num(lo), _num(hi)
        if col is None or lo_n is None or hi_n is None:
            return None
        if attr in DATE_COLS and lo[0] != "date":
            lo_n, hi_n = lo_n + SP_RAW_TIME_OFFSET, hi_n + SP_RAW_TIME_OFFSET
        params += [lo_n, hi_n]
        return f"({col} BETWEEN ? AND ?)"
    _, attr, op, val, mods = node
    if attr in NAME_ATTRS:
        return _str_cond("name", op, val[1], mods, params)
    if attr == "kMDItemPath":
        return _str_cond("path", op, val[1], mods, params)
    if attr in TYPE_ATTRS:
        return _type_cond(op, val[1], params)
    col = DATE_COLS.get(attr) or NUM_COLS.get(attr)
    if col is None:
        return None
    n = _num(val)
    if n is None:
        return None
    if attr in DATE_COLS and val[0] != "date":
        n += SP_RAW_TIME_OFFSET
    sql_op = {"==": "=", "!=": "!=", "<": "<", ">": ">"}[op]
    params.append(n)
    return f"{col} {sql_op} ?"


def _type_cond(op: str, value: str, params: list) -> str | None:
    t = _TYPE_MAP.get(value)
    if t is None or op not in ("==", "!="):
        return None
    if t == "dir":
        cond = "isdir = 1"
    else:
        exts = _EXT.get(t, t).split()
        cond = "(isdir = 0 AND (" + " OR ".join(["ext = ?"] * len(exts)) + "))"
        params += exts
    return f"NOT {cond}" if op == "!=" else cond


def to_sql(text: str) -> tuple[str, list]:
    """-> (WHERE fragment, params). Raises QueryError."""
    node = parse(text)
    if node is None:
        raise QueryError("empty query")
    params: list = []
    sql = _node_sql(node, params)
    if sql is None:
        raise QueryError("no supported attribute in query")
    return sql, params
