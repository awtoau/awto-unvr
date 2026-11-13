# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""truesearch indexer control socket (#356): no indexer runs, every request fails with OSError."""

from __future__ import annotations

import dataclasses
import errno
import os
from collections.abc import Iterable

INDEXER_CONTROL = "/run/truesearch/indexer.sock"
# Seconds middleware waits for released datasets; nothing holds one here, so never waited on.
RELEASE_TIMEOUT = 30


@dataclasses.dataclass(frozen=True)
class IndexerTarget:
    activity: str
    live: int = 0
    watching: int = 0
    last_full: int = 0
    last_dirs: int = 0
    size: int = 0


@dataclasses.dataclass(frozen=True)
class UnindexedTarget:
    why: str


@dataclasses.dataclass(frozen=True)
class IndexerStatus:
    index_bytes: int = 0
    free_bytes: int = 0
    space_low: bool = False
    targets: dict[str, IndexerTarget | UnindexedTarget] = dataclasses.field(
        default_factory=dict
    )


def _refused():
    raise ConnectionRefusedError(
        errno.ECONNREFUSED, "truesearch indexer is not installed on this system"
    )


def request(path: str, command: str) -> str:
    _refused()


def indexer_status(timeout: float) -> IndexerStatus:
    _refused()


def served_ids(timeout: float) -> set[str]:
    _refused()


def devices_of(paths: Iterable[str]) -> set[int]:
    out = set()
    for p in paths:
        try:
            out.add(os.stat(p).st_dev)
        except OSError:
            pass
    return out


def wait_released(devices: set[int]) -> bool:
    """No daemon holds a device, so they are released already."""
    return True
