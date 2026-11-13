# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Feature/tier vocabulary as public middleware 27.0 uses it (#356). Clean-room stand-in."""

from __future__ import annotations

import enum


class LicenseFeature(enum.StrEnum):
    """Value == name: middleware keys dicts by it and compares it to plain "HA" etc."""

    APPS = "APPS"
    AUTOTUNE = "AUTOTUNE"
    CATALOG_ENTERPRISE_TRAIN = "CATALOG_ENTERPRISE_TRAIN"
    CONTAINERS = "CONTAINERS"
    DEDUP = "DEDUP"
    DIRECTORY_SERVICES_AUTH = "DIRECTORY_SERVICES_AUTH"
    FIBRECHANNEL = "FIBRECHANNEL"
    HA = "HA"
    KMIP = "KMIP"
    MISSION_CRITICAL = "MISSION_CRITICAL"
    NETWORK_FEC = "NETWORK_FEC"
    NFS_SNAPSHOT = "NFS_SNAPSHOT"
    NVMEOF_SPDK = "NVMEOF_SPDK"
    RDMA = "RDMA"
    S3_AUDIT = "S3_AUDIT"
    S3_VERSIONING = "S3_VERSIONING"
    SED = "SED"
    SMB_BLOCK_CLONING = "SMB_BLOCK_CLONING"
    STIG = "STIG"
    SUPPORT = "SUPPORT"
    TNC_SUB = "TNC_SUB"
    TRUESEARCH = "TRUESEARCH"
    VMS = "VMS"
    WEBSHARE = "WEBSHARE"
    ZFSTIER = "ZFSTIER"


class SupportTier(enum.StrEnum):
    """Value == name: legacy.py does SupportTier(name.upper()), ValueError -> BRONZE."""

    BRONZE = "BRONZE"
    SILVER = "SILVER"
    SILVERINTERNATIONAL = "SILVERINTERNATIONAL"
    GOLD = "GOLD"


# feature -> tiers it may carry; policy.py's only TierRule is on SUPPORT.
FEATURE_TIERS: dict[LicenseFeature, tuple[SupportTier, ...]] = {
    LicenseFeature.SUPPORT: tuple(SupportTier),
}
