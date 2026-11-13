# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Clean-room stand-in for TrueNAS's closed truenas_zfstierd_client (#356): tiering unavailable.

- Written only from public middleware 27 usage (plugins/zfs/tier.py, alert/source/zfs_tier.py).
- No jobs exist: enum_jobs() empty, get_last_job KeyError, get_info/get_resolved_failures ValueError
  (the exceptions tier.py treats as "no such job").
- RewriteClient ops raise RewriteClientException(DISABLED) -> middleware CallError.
"""

from __future__ import annotations

from collections.abc import Iterator

from truenas_zfstierd_common import (
    CreateJobResult,
    InfoResult,
    JSONRPCErrorCode,
    RecoverResult,
)

__all__ = [
    "RewriteClient",
    "RewriteClientException",
    "enum_jobs",
    "get_info",
    "get_last_job",
    "get_resolved_failures",
]

_UNAVAILABLE = "ZFS tiering is not available on this system (no truenas_zfstierd)"


class RewriteClientException(Exception):
    code: JSONRPCErrorCode
    message: str

    def __init__(self, code: JSONRPCErrorCode, message: str):
        super().__init__(message)
        self.code = code
        self.message = message


def enum_jobs() -> Iterator[InfoResult]:
    return iter(())


def get_last_job(dataset_name: str) -> InfoResult:
    raise KeyError(dataset_name)


def get_info(dataset_name: str, job_uuid: str) -> InfoResult:
    raise ValueError(f"{dataset_name}@{job_uuid}: no such tier job")


def get_resolved_failures(dataset_name: str, job_uuid: str) -> list:
    raise ValueError(f"{dataset_name}@{job_uuid}: no such tier job")


class RewriteClient:
    """Async context manager; entering succeeds so each op reports DISABLED, not a socket error."""

    async def __aenter__(self) -> RewriteClient:
        return self

    async def __aexit__(self, *exc) -> bool:
        return False

    async def create_job(self, dataset_name: str) -> CreateJobResult:
        raise RewriteClientException(JSONRPCErrorCode.DISABLED, _UNAVAILABLE)

    async def abort_job(self, dataset_name: str, job_uuid: str) -> None:
        raise RewriteClientException(JSONRPCErrorCode.DISABLED, _UNAVAILABLE)

    async def recover_job(self, dataset_name: str, job_uuid: str) -> RecoverResult:
        raise RewriteClientException(JSONRPCErrorCode.DISABLED, _UNAVAILABLE)
