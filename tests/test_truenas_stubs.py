# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""truenas_pylicensed stub semantics (#356), no middleware clone needed.

Coverage against middleware itself: scripts/check-truenas-stubs.py.
"""

from __future__ import annotations

import dataclasses
import sys
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "truenas" / "stubs" / "truenas_pylicensed"))

import truenas_pylicensed as t  # noqa: E402
from truenas_pylicensed import features as tf  # noqa: E402


def test_verify_is_no_license():
    st = t.verify()
    assert st.valid is False
    assert st.code is t.LicenseError.NO_LICENSE
    assert st.features == {} and st.reload_seq == 0
    assert not st.has_feature(t.LicenseFeature.HA)


@pytest.mark.parametrize("cls", [t.LicenseFeature, t.SupportTier, t.LicenseType])
def test_strenum_value_is_name(cls):
    assert all(m.value == m.name and m == m.name for m in cls)


def test_features_module_reexported():
    assert t.LicenseFeature is tf.LicenseFeature
    assert t.SupportTier is tf.SupportTier
    assert t.FEATURE_TIERS is tf.FEATURE_TIERS


def test_support_tiers_cover_proactive_policy():
    # policy.py's TierRule allows GOLD/SILVER/SILVERINTERNATIONAL on SUPPORT.
    allowed = {
        t.SupportTier.GOLD,
        t.SupportTier.SILVER,
        t.SupportTier.SILVERINTERNATIONAL,
    }
    assert allowed <= set(t.FEATURE_TIERS[t.LicenseFeature.SUPPORT])


def test_feature_name_map_renames():
    assert str(t.FEATURE_NAME_MAP.get("VM", "VM")) == "VMS"
    assert str(t.FEATURE_NAME_MAP.get("DEDUP", "DEDUP")) == "DEDUP"


def test_has_feature_maps_wire_names():
    st = t.LicenseStatus(
        valid=True, code=t.LicenseError.OK, features={"VM": t.FeatureEntry(name="VM")}
    )
    assert st.has_feature(t.LicenseFeature.VMS)


def test_status_frozen():
    with pytest.raises(dataclasses.FrozenInstanceError):
        t.verify().valid = True


def test_fingerprint_shape():
    fp = t.get_fingerprint()
    assert {"macs", "cpu_id", "machine_id", "smbios"} <= set(fp)
    assert isinstance(fp["smbios"], dict)
