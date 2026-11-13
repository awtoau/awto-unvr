# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Clean-room stand-in for TrueNAS's closed truenas_pylibsed (#356): no drive is SED-capable.

- Written only from public middleware 27 usage: utils/disks_/disk_class.py (docs/truenas-stubs.md).
- get_device_info() reports locking unsupported -> is_sed False, sed_status NO_SED, setup NO_SED.
- Every authenticated op raises OSError(EOPNOTSUPP); callers catch Exception, re-raise only TimeoutError.
"""

from __future__ import annotations

import errno
import os
from typing import Any

__all__ = [
    "change_password",
    "factory_reset",
    "get_device_info",
    "get_locking_info",
    "initial_setup",
    "is_uninitialized",
    "unlock",
]


def _serial(fd: int) -> str:
    """Kernel's serial for the opened disk, or ""; only feeds sedutil-style hashing in middleware."""
    try:
        dev = os.path.basename(os.readlink(f"/proc/self/fd/{fd}"))
        with open(f"/sys/class/block/{dev}/device/serial") as f:
            return f.read().strip()
    except OSError:
        return ""


def get_device_info(fd: int) -> dict[str, Any]:
    """Level 0 Discovery shape middleware reads: locking{supported,enabled,locked}, ssc_type, serials."""
    serial = _serial(fd)
    return {
        "device_type": None,
        "ssc_type": None,
        "tper": {"supported": False},
        "locking": {"supported": False, "enabled": False, "locked": False},
        "geometry": None,
        "lnx_serial": serial,
        "bsd_serial": serial,
    }


def _unsupported(op: str):
    raise OSError(
        errno.EOPNOTSUPP, f"SED {op}: device does not support TCG Opal/Enterprise"
    )


def is_uninitialized(fd: int, *, device_info: dict | None = None) -> bool:
    _unsupported("discovery")


def unlock(fd: int, password: bytes, *, device_info: dict | None = None) -> None:
    _unsupported("unlock")


def initial_setup(fd: int, password: bytes, *, device_info: dict | None = None) -> None:
    _unsupported("initial setup")


def get_locking_info(
    fd: int, password: bytes, *, device_info: dict | None = None
) -> dict[str, Any]:
    _unsupported("locking info")


def factory_reset(fd: int, psid: bytes, *, device_info: dict | None = None) -> None:
    _unsupported("factory reset")


def change_password(
    fd: int,
    old_password: bytes,
    new_password: bytes,
    *,
    device_info: dict | None = None,
) -> None:
    _unsupported("change password")
