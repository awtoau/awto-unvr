# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Clean-room stand-in for TrueNAS's closed truenas_zfstierd_common (#356): tiering unavailable.

- Written only from public middleware 27 usage: plugins/zfs/tier.py, alert/source/zfs_tier.py,
  etc_files/truenas_zfstierd.py (docs/truenas-stubs.md).
"""

from __future__ import annotations

import dataclasses
import enum

__all__ = [
    "CreateJobResult",
    "InfoResult",
    "JSONRPCErrorCode",
    "JobStats",
    "RecoverResult",
    "RewriteJobStatus",
]


class RewriteJobStatus(enum.StrEnum):
    """Value == name: passed straight into the API's TierRewriteJobStatus Literal."""

    COMPLETE = "COMPLETE"
    RUNNING = "RUNNING"
    QUEUED = "QUEUED"
    CANCELLED = "CANCELLED"
    STOPPED = "STOPPED"
    ERROR = "ERROR"


class JSONRPCErrorCode(enum.IntEnum):
    """Only compared by member; values are JSON-RPC server-error range picks of our own."""

    DATASET_NOT_FOUND = -32001
    JOB_NOT_FOUND = -32002
    JOB_ALREADY_EXISTS = -32003
    DISABLED = -32004


@dataclasses.dataclass(frozen=True)
class JobStats:
    start_time: int = 0
    initial_time: int = 0
    update_time: int = 0
    count_items: int = 0
    count_bytes: int = 0
    total_items: int = 0
    total_bytes: int = 0
    failures: int = 0
    success: int = 0
    parent: str | None = None
    name: str | None = None


@dataclasses.dataclass(frozen=True)
class CreateJobResult:
    dataset_name: str
    job_uuid: str
    status: RewriteJobStatus


@dataclasses.dataclass(frozen=True)
class RecoverResult:
    dataset_name: str
    job_uuid: str
    status: RewriteJobStatus


@dataclasses.dataclass(frozen=True)
class InfoResult:
    dataset_name: str
    job_uuid: str
    status: RewriteJobStatus
    stats: JobStats | None = None
    error: str | None = None
