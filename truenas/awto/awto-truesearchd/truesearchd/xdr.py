# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Minimal XDR (RFC 4506) codec driven by an rpcgen-style .x file.

- Parses the subset the truesearch .x uses: const, enum, struct, union switch, string<N>,
  [unsigned] int/hyper, bool, T<N>, T *p, void.
- Values: struct -> dict, enum -> int, union -> (tag, value), optional -> None|value, array -> list.
- Strict like the Rust side: non-zero padding, trailing bytes, over-limit lengths all raise XdrError.
"""

from __future__ import annotations

import re
import struct


class XdrError(ValueError):
    pass


_TOKEN = re.compile(r"\s*(?:(/\*.*?\*/)|([A-Za-z_]\w*|-?\d+|[{}()<>;:=,*]))", re.S)


def _tokens(text: str) -> list[str]:
    out, pos = [], 0
    text = text.rstrip()
    while pos < len(text):
        m = _TOKEN.match(text, pos)
        if not m:
            raise XdrError(f"cannot tokenize at {text[pos : pos + 20]!r}")
        pos = m.end()
        if m.group(2):
            out.append(m.group(2))
    return out


class Schema:
    def __init__(self, text: str):
        self.consts: dict[str, int] = {}
        self.enums: dict[str, dict[str, int]] = {}
        self.structs: dict[str, list[tuple[str, tuple]]] = {}
        self.unions: dict[str, tuple[str, str, dict[int | None, tuple | None]]] = {}
        self._t = _tokens(text)
        self._i = 0
        while self._i < len(self._t):
            self._definition()

    def _next(self) -> str:
        t = self._t[self._i]
        self._i += 1
        return t

    def _peek(self) -> str:
        return self._t[self._i]

    def _expect(self, tok: str) -> None:
        got = self._next()
        if got != tok:
            raise XdrError(f"expected {tok!r}, got {got!r}")

    def _num(self, tok: str) -> int:
        return self.consts[tok] if tok in self.consts else int(tok)

    def _definition(self) -> None:
        kw = self._next()
        if kw == "const":
            name = self._next()
            self._expect("=")
            self.consts[name] = int(self._next())
            self._expect(";")
        elif kw == "enum":
            name = self._next()
            self._expect("{")
            members: dict[str, int] = {}
            while True:
                m = self._next()
                self._expect("=")
                members[m] = int(self._next())
                if self._peek() == ",":
                    self._next()
                    continue
                break
            self._expect("}")
            self._expect(";")
            self.enums[name] = members
        elif kw == "struct":
            name = self._next()
            self._expect("{")
            fields = []
            while self._peek() != "}":
                fields.append(self._declaration())
            self._expect("}")
            self._expect(";")
            self.structs[name] = fields
        elif kw == "union":
            name = self._next()
            self._expect("switch")
            self._expect("(")
            dtype = self._next()
            self._next()  # discriminant name
            self._expect(")")
            self._expect("{")
            arms: dict[int | None, tuple | None] = {}
            while self._peek() != "}":
                if self._peek() == "default":
                    self._next()
                    key = None
                else:
                    self._expect("case")
                    case = self._next()
                    key = self.enums[dtype][case]
                self._expect(":")
                if self._peek() == "void":
                    self._next()
                    arms[key] = None
                else:
                    arms[key] = self._declaration(want_semicolon=False)[1]
                self._expect(";")
            self._expect("}")
            self._expect(";")
            self.unions[name] = (dtype, name, arms)
        else:
            raise XdrError(f"unexpected {kw!r}")

    def _declaration(self, want_semicolon: bool = True) -> tuple[str, tuple]:
        """-> (field name, type); type is ('uint'|'int'|'uhyper'|'hyper'|'bool'|'string'|
        'enum'|'struct'|'union', name, limit) / ('array', elem, limit) / ('opt', elem)."""
        words = [self._next()]
        if words[0] == "unsigned" and self._peek() in ("int", "hyper"):
            words.append(self._next())
        base = " ".join(words)
        if base.startswith("unsigned "):
            typ: tuple = ("u" + base.split()[1],)
        elif base in ("int", "hyper", "bool", "string"):
            typ = (base,)
        elif base in self.enums:
            typ = ("enum", base)
        elif base in self.structs:
            typ = ("struct", base)
        elif base in self.unions:
            typ = ("union", base)
        else:
            raise XdrError(f"unknown type {base!r}")
        optional = False
        if self._peek() == "*":
            self._next()
            optional = True
        name = self._next()
        limit = None
        if self._peek() == "<":
            self._next()
            limit = self._num(self._next())
            self._expect(">")
        if want_semicolon:
            self._expect(";")
        if optional:
            return name, ("opt", typ)
        if typ == ("string",):
            return name, ("string", limit)
        if limit is not None:
            return name, ("array", typ, limit)
        return name, typ

    # ---- encode ----

    def encode(self, typ: str, value) -> bytes:
        out = bytearray()
        self._enc(("struct", typ), value, out)
        return bytes(out)

    def _enc(self, t: tuple, v, out: bytearray) -> None:
        k = t[0]
        if k == "uint":
            out += struct.pack(">I", _rng(v, 0, 2**32 - 1))
        elif k == "int":
            out += struct.pack(">i", _rng(v, -(2**31), 2**31 - 1))
        elif k == "uhyper":
            out += struct.pack(">Q", _rng(v, 0, 2**64 - 1))
        elif k == "hyper":
            out += struct.pack(">q", _rng(v, -(2**63), 2**63 - 1))
        elif k == "bool":
            out += struct.pack(">I", 1 if v else 0)
        elif k == "enum":
            if v not in self.enums[t[1]].values():
                raise XdrError(f"bad {t[1]} value {v}")
            out += struct.pack(">I", v)
        elif k == "string":
            b = v.encode("utf-8")
            if t[1] is not None and len(b) > t[1]:
                raise XdrError("string over limit")
            out += struct.pack(">I", len(b)) + b + b"\0" * (-len(b) % 4)
        elif k == "opt":
            if v is None:
                out += struct.pack(">I", 0)
            else:
                out += struct.pack(">I", 1)
                self._enc(t[1], v, out)
        elif k == "array":
            if len(v) > t[2]:
                raise XdrError("array over limit")
            out += struct.pack(">I", len(v))
            for e in v:
                self._enc(t[1], e, out)
        elif k == "struct":
            for fname, ft in self.structs[t[1]]:
                if fname not in v:
                    raise XdrError(f"{t[1]}.{fname} missing")
                self._enc(ft, v[fname], out)
        elif k == "union":
            dtype, _, arms = self.unions[t[1]]
            tag, val = v
            self._enc(("enum", dtype), tag, out)
            arm = arms[tag] if tag in arms else arms.get(None, "nodefault")
            if arm == "nodefault":
                raise XdrError(f"{t[1]}: no arm for {tag}")
            if arm is not None:
                self._enc(arm, val, out)
        else:
            raise XdrError(f"bad type {t}")

    # ---- decode ----

    def decode(self, typ: str, data: bytes):
        v, pos = self._dec(("struct", typ), memoryview(data), 0)
        if pos != len(data):
            raise XdrError(f"{len(data) - pos} trailing bytes")
        return v

    def _take(self, buf, pos: int, n: int) -> int:
        if pos + n > len(buf):
            raise XdrError("short message")
        return pos + n

    def _dec(self, t: tuple, buf, pos: int):
        k = t[0]
        if k in ("uint", "int", "enum", "bool"):
            end = self._take(buf, pos, 4)
            raw = bytes(buf[pos:end])
            if k == "int":
                return struct.unpack(">i", raw)[0], end
            n = struct.unpack(">I", raw)[0]
            if k == "bool":
                if n > 1:
                    raise XdrError("bad bool")
                return bool(n), end
            if k == "enum" and n not in self.enums[t[1]].values():
                raise XdrError(f"bad {t[1]} value {n}")
            return n, end
        if k in ("uhyper", "hyper"):
            end = self._take(buf, pos, 8)
            return struct.unpack(">Q" if k == "uhyper" else ">q", bytes(buf[pos:end]))[
                0
            ], end
        if k == "string":
            n, pos = self._dec(("uint",), buf, pos)
            if t[1] is not None and n > t[1]:
                raise XdrError("string over limit")
            end = self._take(buf, pos, n + (-n % 4))
            if any(bytes(buf[pos + n : end])):
                raise XdrError("non-zero padding")
            try:
                return bytes(buf[pos : pos + n]).decode("utf-8"), end
            except UnicodeDecodeError as e:
                raise XdrError("invalid UTF-8") from e
        if k == "opt":
            flag, pos = self._dec(("bool",), buf, pos)
            return self._dec(t[1], buf, pos) if flag else (None, pos)
        if k == "array":
            n, pos = self._dec(("uint",), buf, pos)
            if n > t[2]:
                raise XdrError("array over limit")
            out = []
            for _ in range(n):
                e, pos = self._dec(t[1], buf, pos)
                out.append(e)
            return out, pos
        if k == "struct":
            d = {}
            for fname, ft in self.structs[t[1]]:
                d[fname], pos = self._dec(ft, buf, pos)
            return d, pos
        if k == "union":
            dtype, _, arms = self.unions[t[1]]
            tag, pos = self._dec(("enum", dtype), buf, pos)
            arm = arms[tag] if tag in arms else arms.get(None, "nodefault")
            if arm == "nodefault":
                raise XdrError(f"{t[1]}: no arm for {tag}")
            if arm is None:
                return (tag, None), pos
            val, pos = self._dec(arm, buf, pos)
            return (tag, val), pos
        raise XdrError(f"bad type {t}")


def _rng(v, lo: int, hi: int) -> int:
    if not isinstance(v, int) or not lo <= v <= hi:
        raise XdrError(f"integer out of range: {v!r}")
    return v
