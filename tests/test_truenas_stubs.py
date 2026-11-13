# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""truenas/stubs/ closed-lib stand-in semantics (#356), no middleware clone needed.

Coverage against middleware itself: scripts/check-truenas-stubs.py.
"""

from __future__ import annotations

import dataclasses
import sys
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
for _d in (
    "truenas_pylicensed",
    "truenas_pylibsed",
    "truenas_zfstierd",
    "truesearch_client",
    "licenselib",
):
    sys.path.insert(0, str(REPO / "truenas" / "stubs" / _d))

import truenas_pylibsed as sed  # noqa: E402
import truenas_pylicensed as t  # noqa: E402
import truenas_zfstierd_client as tc  # noqa: E402
import truesearch_client as ts  # noqa: E402
from licenselib import license as ll  # noqa: E402
from licenselib.utils import proactive_support_allowed  # noqa: E402
from truenas_pylicensed import features as tf  # noqa: E402
from truenas_zfstierd_common import JSONRPCErrorCode, RewriteJobStatus  # noqa: E402
from truenas_zfstierd_common.configfile import ZFSTierdGlobalConfig, generate_config  # noqa: E402
from truesearch_client import config as tsc  # noqa: E402
from truesearch_client import control as tsctl  # noqa: E402


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


def test_sed_not_capable():
    info = sed.get_device_info(0)
    assert info["locking"] == {"supported": False, "enabled": False, "locked": False}
    for op, args in [
        (sed.unlock, (0, b"pw")),
        (sed.initial_setup, (0, b"pw")),
        (sed.get_locking_info, (0, b"pw")),
        (sed.factory_reset, (0, b"psid")),
        (sed.change_password, (0, b"a", b"b")),
        (sed.is_uninitialized, (0,)),
    ]:
        with pytest.raises(OSError):
            op(*args, device_info=info)


def test_zfstier_no_jobs():
    import asyncio

    assert list(tc.enum_jobs()) == []
    with pytest.raises(KeyError):
        tc.get_last_job("tank/a")
    with pytest.raises(ValueError):
        tc.get_info("tank/a", "u")

    async def create():
        async with tc.RewriteClient() as c:
            await c.create_job("tank/a")

    with pytest.raises(tc.RewriteClientException) as e:
        asyncio.run(create())
    assert e.value.code is JSONRPCErrorCode.DISABLED
    assert all(m.value == m.name for m in RewriteJobStatus)
    assert "enabled = false" in generate_config(ZFSTierdGlobalConfig())


def test_truesearch_unavailable():
    with pytest.raises(OSError):
        with ts.Client():
            pass
    assert tsc.read_config() is None
    with pytest.raises(OSError):
        tsctl.indexer_status(1)
    assert tsctl.wait_released(set()) is True
    assert issubclass(ts.Busy, ts.TruesearchError) and issubclass(
        ts.CallError, ts.TruesearchError
    )


def test_licenselib_decodes_nothing():
    from datetime import date

    with pytest.raises(ValueError):
        ll.License.load("AUgxMAAA")
    lic = ll.License(
        1, "M50", "S1", "", ll.ContractType.legacy, ll.ContractHardware.parts,
        ll.ContractSoftware.none, date(2026, 1, 1), 10, "", "KEY", [], [],
    )  # fmt: skip
    assert lic.contract_end == date(2026, 1, 11)
    assert "KEY" not in repr(lic)
    with pytest.raises(NotImplementedError):
        lic.dump()
    assert proactive_support_allowed("gold") and not proactive_support_allowed(
        "STANDARD"
    )
