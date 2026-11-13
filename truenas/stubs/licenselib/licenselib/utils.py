# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""licenselib helpers middleware imports (#356)."""

from __future__ import annotations

# Same tier set middleware's own policy.py grants proactive support to.
_PROACTIVE = frozenset({"GOLD", "SILVER", "SILVERINTERNATIONAL"})


def proactive_support_allowed(contract_type_name: str) -> bool:
    return contract_type_name.upper() in _PROACTIVE
