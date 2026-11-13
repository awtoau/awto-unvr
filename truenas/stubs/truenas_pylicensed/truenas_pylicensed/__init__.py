# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Clean-room stand-in for TrueNAS's closed truenas_pylicensed (#356): always "no licence".

- Written only from how public truenas/middleware release/27.0.0 uses it: docs/truenas-stubs.md.
- verify() -> NO_LICENSE: middleware falls back to the legacy blob, finds none -> Community.
- Licence upload/remove fails in middleware (reload_seq never moves) - intended.
"""

from __future__ import annotations

import dataclasses
import enum
from typing import Any

from .features import FEATURE_TIERS, LicenseFeature, SupportTier

__all__ = [
    "FEATURE_NAME_MAP",
    "FEATURE_TIERS",
    "FeatureEntry",
    "LicenseError",
    "LicenseFeature",
    "LicenseStatus",
    "LicenseType",
    "SupportTier",
    "get_fingerprint",
    "verify",
]


class LicenseType(enum.StrEnum):
    COMMUNITY = "COMMUNITY"
    COMMERCIAL = "COMMERCIAL"
    ENTERPRISE = "ENTERPRISE"


class LicenseError(enum.Enum):
    """Exactly the codes middleware names; test_get_license.py asserts it classifies every member."""

    OK = "OK"
    NO_LICENSE = "NO_LICENSE"
    DAEMON_UNAVAILABLE = "DAEMON_UNAVAILABLE"
    DAEMON_ERROR = "DAEMON_ERROR"
    INTERNAL_ERROR = "INTERNAL_ERROR"
    SCHEMA_ERROR = "SCHEMA_ERROR"
    SIGNATURE_FAILED = "SIGNATURE_FAILED"
    SYSTEM_ID_MISMATCH = "SYSTEM_ID_MISMATCH"


# Wire/legacy feature name -> middleware name (middleware tests: VM -> VMS, jails -> APPS).
FEATURE_NAME_MAP: dict[str, LicenseFeature] = {
    "VM": LicenseFeature.VMS,
    "JAILS": LicenseFeature.APPS,
}


@dataclasses.dataclass(frozen=True)
class FeatureEntry:
    name: str
    start_date: str | None = None
    expires_at: str | None = None
    type: str | None = None


@dataclasses.dataclass(frozen=True)
class LicenseStatus:
    valid: bool
    code: LicenseError
    error: str | None = None
    test: bool = False
    id: str | None = None
    version: int | None = None
    type: LicenseType | None = None
    model: str | None = None
    features: dict[str, FeatureEntry] = dataclasses.field(default_factory=dict)
    system_id: dict[str, Any] | None = None
    enclosures: dict[str, dict[str, int]] = dataclasses.field(default_factory=dict)
    issued_at: str | None = None
    reload_seq: int = 0

    def has_feature(self, name: str) -> bool:
        """Membership by middleware name; features is keyed by wire name (VM, not VMS)."""
        return str(name) in {str(FEATURE_NAME_MAP.get(k, k)) for k in self.features}


def verify() -> LicenseStatus:
    return LicenseStatus(
        valid=False, code=LicenseError.NO_LICENSE, error="No license installed"
    )


def _machine_id() -> str:
    try:
        with open("/etc/machine-id") as f:
            return f.read().strip()
    except OSError:
        return ""


def get_fingerprint() -> dict[str, Any]:
    """Keys daemon.get_fingerprint_b64 reads; smbios components absent -> null in its output."""
    return {"macs": [], "cpu_id": "", "machine_id": _machine_id(), "smbios": {}}
