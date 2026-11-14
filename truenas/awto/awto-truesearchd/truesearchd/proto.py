# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Wire protocol "rix" v1: the schema is the .x that Samba's mdssvc truesearch backend is built from.

- proto/truenas_truesearch.x is a verbatim copy of source3/rpc_server/mdssvc/truenas_truesearch.x
  (public Samba fork, release/27.0.0). Record = 4-byte BE length (excl. itself) + XDR.
"""

from __future__ import annotations

import struct
from importlib import resources

from .xdr import Schema, XdrError

SCHEMA = Schema(
    resources.files("truesearchd")
    .joinpath("proto/truenas_truesearch.x")
    .read_text("utf-8")
)
C = SCHEMA.consts
OP = SCHEMA.enums["rix_op"]
ST = SCHEMA.enums["rix_status"]
PROTO_VERSION = C["RIX_PROTO_VERSION"]
MAX_FRAME = C["RIX_MAX_FRAME"]
OPEN_TRIM_BY_IDENTITY = (
    C["RIX_OPEN_TRIM_BY_IDENTITY"] if "RIX_OPEN_TRIM_BY_IDENTITY" in C else 1
)
OPEN_WANT_STAT = 2
OPEN_HIDE_UNREADABLE = 4


def decode_call(body: bytes) -> dict:
    return SCHEMA.decode("rix_call", body)


def encode_call(call: dict) -> bytes:
    return frame(SCHEMA.encode("rix_call", call))


def encode_reply(reply: dict) -> bytes:
    return frame(SCHEMA.encode("rix_reply", reply))


def decode_reply(body: bytes) -> dict:
    return SCHEMA.decode("rix_reply", body)


def frame(body: bytes) -> bytes:
    if len(body) > MAX_FRAME:
        raise XdrError("record over RIX_MAX_FRAME")
    return struct.pack(">I", len(body)) + body


def reply(xid: int, status: int, answer=None, detail: str = "") -> dict:
    """status RIX_OK -> answer is (op, value); otherwise detail string."""
    if status == ST["RIX_OK"]:
        outcome = (status, answer)
    else:
        outcome = (status, detail[: C["RIX_MAX_DETAIL"]])
    return {"vers": PROTO_VERSION, "xid": xid, "outcome": outcome}
