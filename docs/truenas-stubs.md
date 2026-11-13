# truenas_pylicensed stub (#356)

Clean-room stand-in for TrueNAS's closed `truenas_pylicensed`, so middleware 27 imports and runs as an unlicensed Community install.

- Source: `truenas/stubs/truenas_pylicensed/` → Debian binary `python3-truenas-pylicensed` (arch all, pybuild/pyproject, source `truenas-pylicensed`, 3.0 native).
- Derived **only** from public `truenas/middleware` `release/27.0.0`: imports, attribute reads, constructor kwargs, call sites and that repo's unit tests. No closed package, binary or ISO content read (Dan, 2026-11-14).
- Check: `./dev.py check-truenas-stubs` (`scripts/check-truenas-stubs.py`); gate step `pylicensed-stub` (skips without the clone).

## API surface

| Name | Where | Shape |
|---|---|---|
| `LicenseFeature` | `.features`, re-exported | `StrEnum`, value == name: APPS AUTOTUNE CATALOG_ENTERPRISE_TRAIN CONTAINERS DEDUP DIRECTORY_SERVICES_AUTH FIBRECHANNEL HA KMIP MISSION_CRITICAL NETWORK_FEC NFS_SNAPSHOT NVMEOF_SPDK RDMA S3_AUDIT S3_VERSIONING SED SMB_BLOCK_CLONING STIG SUPPORT TNC_SUB TRUESEARCH VMS WEBSHARE ZFSTIER |
| `SupportTier` | `.features`, re-exported | `StrEnum`: BRONZE SILVER SILVERINTERNATIONAL GOLD |
| `FEATURE_TIERS` | `.features`, re-exported | `{SUPPORT: (all SupportTier)}` |
| `LicenseType` | top | `StrEnum`: COMMUNITY COMMERCIAL ENTERPRISE |
| `LicenseError` | top | `Enum`: OK NO_LICENSE DAEMON_UNAVAILABLE DAEMON_ERROR INTERNAL_ERROR SCHEMA_ERROR SIGNATURE_FAILED SYSTEM_ID_MISMATCH |
| `FEATURE_NAME_MAP` | top | `{"VM": VMS, "JAILS": APPS}` |
| `FeatureEntry` | top | frozen dataclass: name, start_date, expires_at, type (ISO date strings / tier str) |
| `LicenseStatus` | top | frozen dataclass: valid, code, error, test, id, version, type, model, features, system_id, enclosures, issued_at, reload_seq; `has_feature(name)` |
| `verify()` | top | `LicenseStatus(valid=False, code=NO_LICENSE, error="No license installed")` |
| `get_fingerprint()` | top | `{"macs": [], "cpu_id": "", "machine_id": <from /etc/machine-id>, "smbios": {}}` |

- Value == name on all three StrEnums: `test_entitlements.py` compares `POLICY` keys to `"HA"`; `legacy.py` does `SupportTier(name.upper())`; `daemon.py` does `str(FEATURE_NAME_MAP.get(name, name))`.
- `LicenseError` members = exactly the codes middleware names; `test_get_license.py::test_every_license_error_code_is_classified` requires that.
- `FEATURE_NAME_MAP`: `VM`→`VMS` from `test_license_utils.py`; `JAILS`→`APPS` from `test_license_legacy_utils.py` ("the jails->APPS bit").

## Semantics per call-site category

- **Licence source** (`utils/license/__init__.py::get_license`, `daemon.py::from_license_status`): `verify()` → `NO_LICENSE`, the only fallback code → legacy blob `/data/license` → absent → `None` → Community.
  - Picked NO_LICENSE over DAEMON_UNAVAILABLE: same end result (None), but it is the truthful code and keeps a legacy blob readable.
- **Entitlement gates** (`utils/entitlements/{matrix,policy,engine}.py`, every plugin calling `truenas.entitlements.check`): the stub supplies only names; middleware's engine decides with `license=None`. CE column grants APPS, CONTAINERS, DEDUP, VMS; everything else denied. No feature is licensed.
- **Upload / remove** (`daemon.py::upload_license`, `remove_license`, `plugins/truenas/license.py`): `reload_seq` never moves → upload raises `CallError` after the 6 x 0.5 s poll and rolls back. Remove with no file returns early. Intended: nothing can install a licence.
- **Fingerprint** (`daemon.py::get_fingerprint_b64`): `cpu_id`/`machine_id` required keys; `smbios` empty → serial fields `null`.
- **Debug bundle** (`plugins/truenas/entitlements.py::debug_info`): reads `code.name, error, test, reload_seq, version, issued_at`.
- **Type checks** (`plugins/system/product.py`, `alert/source/license_status.py`, `plugins/truenas/license.py`): compare `LicenseInfo.type` to `LicenseType.*`; with no licence these branches are skipped.

## Gaps outside this stub

- `utils/license/legacy.py` (imported by `utils/license/__init__.py`, so by middlewared at start) and `plugins/truenas/license.py` import `licenselib` at module level. Not this package; tracked in #356.
- Full plugin imports need the TrueNAS runtime (service framework deps). The check fakes missing host deps and imports every using module; ones that fail on faked deps are logged, not failed. Stub-caused errors fail.

## Check (`scripts/check-truenas-stubs.py`)

- Static: AST over the 27.0.0 tree — imports, `Name.attr` on imported names, `LicenseStatus(...)`/`FeatureEntry(...)` kwargs, attributes read off `verify()` / `LicenseStatus`-annotated values / `.features` entries, subscripted `get_fingerprint()` keys.
- Import: every non-test using module, stub real, missing deps faked (`scripts/_truenas_stubs_harness.py`). `middlewared.utils.license` and `middlewared.utils.entitlements` must import.
- Tests: middleware's own unit tests for license/entitlements run against the stub; exclusions + reasons in the script.
- Doc list below must equal the files that import the package; the check fails otherwise.

## Derived from (middleware `release/27.0.0`)

<!-- derived-from:begin -->
- `src/middlewared/middlewared/alert/source/license_status.py`
- `src/middlewared/middlewared/etc_files/truenas_zfstierd.py`
- `src/middlewared/middlewared/plugins/alert.py`
- `src/middlewared/middlewared/plugins/catalog/config.py`
- `src/middlewared/middlewared/plugins/container/container.py`
- `src/middlewared/middlewared/plugins/disk.py`
- `src/middlewared/middlewared/plugins/docker/update.py`
- `src/middlewared/middlewared/plugins/etc.py`
- `src/middlewared/middlewared/plugins/fc/fc.py`
- `src/middlewared/middlewared/plugins/iscsi_/targets.py`
- `src/middlewared/middlewared/plugins/kmip/update.py`
- `src/middlewared/middlewared/plugins/network.py`
- `src/middlewared/middlewared/plugins/nfs.py`
- `src/middlewared/middlewared/plugins/nvmet/global.py`
- `src/middlewared/middlewared/plugins/pool_/pool.py`
- `src/middlewared/middlewared/plugins/pool_/utils.py`
- `src/middlewared/middlewared/plugins/rdma/rdma.py`
- `src/middlewared/middlewared/plugins/security/info.py`
- `src/middlewared/middlewared/plugins/smb.py`
- `src/middlewared/middlewared/plugins/support.py`
- `src/middlewared/middlewared/plugins/system/product.py`
- `src/middlewared/middlewared/plugins/system_general/update.py`
- `src/middlewared/middlewared/plugins/truenas/entitlement_usage.py`
- `src/middlewared/middlewared/plugins/truenas/entitlements.py`
- `src/middlewared/middlewared/plugins/truenas/license.py`
- `src/middlewared/middlewared/plugins/truenas_s3/bucket_crud.py`
- `src/middlewared/middlewared/plugins/truenas_s3/config.py`
- `src/middlewared/middlewared/plugins/truesearch.py`
- `src/middlewared/middlewared/plugins/update_/profile_.py`
- `src/middlewared/middlewared/plugins/vm/info.py`
- `src/middlewared/middlewared/plugins/webshare/sharing.py`
- `src/middlewared/middlewared/plugins/zfs/resource_create.py`
- `src/middlewared/middlewared/plugins/zfs/tier.py`
- `src/middlewared/middlewared/pytest/unit/entitlements.py`
- `src/middlewared/middlewared/pytest/unit/plugins/pool/test_validate_topology.py`
- `src/middlewared/middlewared/pytest/unit/plugins/test_failover.py`
- `src/middlewared/middlewared/pytest/unit/plugins/test_fc.py`
- `src/middlewared/middlewared/pytest/unit/plugins/test_iscsi_target.py`
- `src/middlewared/middlewared/pytest/unit/plugins/test_kmip_update.py`
- `src/middlewared/middlewared/pytest/unit/plugins/test_support.py`
- `src/middlewared/middlewared/pytest/unit/plugins/test_system_product.py`
- `src/middlewared/middlewared/pytest/unit/plugins/test_truenas_connect.py`
- `src/middlewared/middlewared/pytest/unit/plugins/truenas/test_entitlements_api.py`
- `src/middlewared/middlewared/pytest/unit/plugins/truenas/test_license_remove.py`
- `src/middlewared/middlewared/pytest/unit/plugins/update/test_profile.py`
- `src/middlewared/middlewared/pytest/unit/utils/test_entitlements.py`
- `src/middlewared/middlewared/pytest/unit/utils/test_get_license.py`
- `src/middlewared/middlewared/pytest/unit/utils/test_license_info_wire.py`
- `src/middlewared/middlewared/pytest/unit/utils/test_license_legacy_utils.py`
- `src/middlewared/middlewared/pytest/unit/utils/test_license_utils.py`
- `src/middlewared/middlewared/utils/entitlements/__init__.py`
- `src/middlewared/middlewared/utils/entitlements/engine.py`
- `src/middlewared/middlewared/utils/entitlements/matrix.py`
- `src/middlewared/middlewared/utils/entitlements/policy.py`
- `src/middlewared/middlewared/utils/license/__init__.py`
- `src/middlewared/middlewared/utils/license/daemon.py`
- `src/middlewared/middlewared/utils/license/legacy.py`
- `src/middlewared/middlewared/utils/license/types.py`
- `src/middlewared/middlewared/utils/service/entitlement.py`
<!-- derived-from:end -->
