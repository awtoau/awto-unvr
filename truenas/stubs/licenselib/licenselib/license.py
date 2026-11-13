# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Legacy licence record (#356). load() always fails, dump() refuses: no licence exists or is written.

- legacy.py: a failed load -> get_legacy_license_info() None, describe -> MALFORMED.
- plugins/truenas/license.py: _is_legacy_license() False, so a v2 PEM upload is not misread.
- truenas-hw-license.py: dump() raises -> its record_error path; no hardware-only record.
"""

from __future__ import annotations

import enum
from datetime import date, timedelta


class Features(enum.Enum):
    """Legacy bitmask features; middleware maps jails -> APPS, vm -> VMS via FEATURE_NAME_MAP."""

    dedup = 1
    jails = 2
    fibrechannel = 4
    vm = 8


class ContractType(enum.Enum):
    """Middleware reads .name.upper(); only `legacy` is named in code."""

    legacy = 0
    standard = 1
    bronze = 2
    silver = 3
    silverinternational = 4
    gold = 5
    freenascertified = 6


class ContractHardware(enum.Enum):
    none = 0
    parts = 1


class ContractSoftware(enum.Enum):
    none = 0


class License:
    def __init__(
        self,
        version: int,
        model: str,
        system_serial: str,
        system_serial_ha: str,
        contract_type: ContractType,
        contract_hardware: ContractHardware,
        contract_software: ContractSoftware,
        contract_start: date,
        contract_duration: int,
        customer_name: str,
        customer_key: str,
        features: list[Features],
        addhw: list[tuple[int, int]],
    ):
        self.version = version
        self.model = model
        self.system_serial = system_serial
        self.system_serial_ha = system_serial_ha
        self.contract_type = contract_type
        self.contract_hardware = contract_hardware
        self.contract_software = contract_software
        self.contract_start = contract_start
        self.contract_duration = contract_duration
        self.contract_end = contract_start + timedelta(days=contract_duration)
        self.customer_name = customer_name
        self.customer_key = customer_key
        self.features = list(features)
        self.addhw = list(addhw)

    def __repr__(self) -> str:
        # legacy.py: a repr must not carry customer_key.
        return f"License(model={self.model!r}, system_serial={self.system_serial!r})"

    @classmethod
    def load(cls, text: str | bytes) -> License:
        raise ValueError("legacy licence decoding is not supported by this build")

    def dump(self) -> bytes:
        raise NotImplementedError(
            "legacy licence encoding is not supported by this build"
        )
