# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Clean-room stand-in for TrueNAS's closed truesearch_client (#356): search service unavailable.

- Written only from public middleware 27 usage: plugins/truesearch.py, etc_files/truesearchd/
  (docs/truenas-stubs.md). middlewared/main.py imports the plugin at start.
- Client() refuses to connect (ConnectionRefusedError): truesearch.query -> "not available" ENOTCONN.
"""

from __future__ import annotations

import dataclasses
import errno
from typing import Any

__all__ = [
    "Busy",
    "CallError",
    "Client",
    "Identity",
    "NoIndex",
    "OpenRequest",
    "Refused",
    "TruesearchError",
]


class TruesearchError(Exception):
    pass


class Busy(TruesearchError):
    pass


class NoIndex(TruesearchError):
    pass


class Refused(TruesearchError):
    detail: str

    def __init__(self, detail: str = ""):
        super().__init__(detail)
        self.detail = detail


class CallError(TruesearchError):
    """A request past a client-side limit; middleware imports it as BoundError."""


@dataclasses.dataclass(frozen=True)
class Identity:
    uid: int
    gid: int
    groups: tuple[int, ...] = ()


@dataclasses.dataclass(frozen=True)
class OpenRequest:
    dataset: str
    share: str
    query: str
    who: Identity
    max_results: int | None = None


@dataclasses.dataclass(frozen=True)
class Hit:
    path: str
    st: Any


@dataclasses.dataclass(frozen=True)
class SearchResult:
    hits: list[Hit] = dataclasses.field(default_factory=list)


class Client:
    """truesearchd client; no daemon is installed, so connecting always fails."""

    def __enter__(self) -> Client:
        raise ConnectionRefusedError(
            errno.ECONNREFUSED, "truesearchd is not installed on this system"
        )

    def __exit__(self, *exc) -> bool:
        return False

    def search(self, request: OpenRequest) -> SearchResult:
        raise NoIndex(request.dataset)
