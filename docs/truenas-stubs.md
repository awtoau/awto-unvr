# TrueNAS closed-library stand-ins (#356)

Clean-room stand-ins for the closed Python libraries public middleware 27 imports, so middlewared loads and runs as an unlicensed Community install with no SED, tiering or search.

- Derived **only** from public `truenas/middleware` `release/27.0.0`: imports, attribute reads, call shapes, match patterns, annotations and that repo's unit tests. No closed package, binary or ISO content read (Dan, 2026-11-14).
- All arch all, pybuild/pyproject, Debian trixie, 3.0 native, version 27.0.0. Binary names = middleware's `src/middlewared/debian/control` Depends.
- Check: `./dev.py check-truenas-stubs` (`scripts/check-truenas-stubs.py`); gate step `truenas-stubs` (skips without the clone).

| Closed module(s) | Stub dir (`truenas/stubs/`) | Debian source → binary | Semantics |
|---|---|---|---|
| `truenas_pylicensed` | `truenas_pylicensed/` | `truenas-pylicensed` → `python3-truenas-pylicensed` | no licence → Community |
| `truenas_pylibsed` | `truenas_pylibsed/` | `truenas-pylibsed` → `python3-truenas-pylibsed` | no drive is SED-capable |
| `truenas_zfstierd_client`, `truenas_zfstierd_common` | `truenas_zfstierd/` | `truenas-zfstierd` → `python3-truenas-zfstierd` | tiering unavailable |
| `truesearch_client` | `truesearch_client/` | `truenas-truesearch` → `truenas-truesearch` | search service unavailable |
| `licenselib` | `licenselib/` | `licenselib` → `python3-licenselib` | no legacy licence decodes |

## Closed dependencies of middleware 27

- Python, imported: exactly the six modules above. `classify_imports()` fails on any top-level import under `src/` that is not stdlib, middleware-local, a public TrueNAS source, known third-party, or stubbed here.
- `licenselib` exists as public `github.com/truenas/licenselib`; not used (Dan's rule covers it), stubbed clean-room instead.
- Non-Python closed packages, Depends of `truenas` (`debian/debian/control`): `truenas-file-manager`, `truenas-licensed` (daemon behind `truenas_pylicensed`), `libtruenas-licensed-dev`, `truenas-s3` (`truenas_s3` systemd unit, `plugins/service_/services/s3.py`). Not stubbed; their services fail to start.
- Daemons the stubs stand in front of, absent: `truenas_zfstierd` unit (`services/truenas_zfstierd.py`), `truesearchd` + `truesearch-indexer` units (`services/truesearch.py`).

## truenas_pylicensed

### API surface

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

### Semantics per call-site category

- **Licence source** (`utils/license/__init__.py::get_license`, `daemon.py::from_license_status`): `verify()` → `NO_LICENSE`, the only fallback code → legacy blob `/data/license` → absent → `None` → Community.
  - Picked NO_LICENSE over DAEMON_UNAVAILABLE: same end result (None), but it is the truthful code and keeps a legacy blob readable.
- **Entitlement gates** (`utils/entitlements/{matrix,policy,engine}.py`, every plugin calling `truenas.entitlements.check`): the stub supplies only names; middleware's engine decides with `license=None`. CE column grants APPS, CONTAINERS, DEDUP, VMS; everything else denied. No feature is licensed.
- **Upload / remove** (`daemon.py::upload_license`, `remove_license`, `plugins/truenas/license.py`): `reload_seq` never moves → upload raises `CallError` after the 6 x 0.5 s poll and rolls back. Remove with no file returns early. Intended: nothing can install a licence.
- **Fingerprint** (`daemon.py::get_fingerprint_b64`): `cpu_id`/`machine_id` required keys; `smbios` empty → serial fields `null`.
- **Debug bundle** (`plugins/truenas/entitlements.py::debug_info`): reads `code.name, error, test, reload_seq, version, issued_at`.
- **Type checks** (`plugins/system/product.py`, `alert/source/license_status.py`, `plugins/truenas/license.py`): compare `LicenseInfo.type` to `LicenseType.*`; with no licence these branches are skipped.

- **CE column**: the check runs the public engine (`utils/entitlements/{matrix,engine,policy}.py`) with the stubs on `HardwareClass.GENERIC`: `get_license()` is `None`, and every `POLICY` key is entitled exactly where `Vector.ce == 1` — APPS, CONTAINERS, DEDUP, VMS. Everything else denied.

## truenas_pylibsed (`utils/disks_/disk_class.py`, core: middlewared disk enumeration)

- `get_device_info(fd)` → `locking{supported, enabled, locked}` all False, `ssc_type None`, `lnx_serial`/`bsd_serial` from sysfs.
  - `is_sed` False, `sed_status` `NO_SED`, `sed_initial_setup` `NO_SED`, `sed_unlock` `(True, None)` (not locked).
- `is_uninitialized`, `unlock`, `initial_setup`, `get_locking_info`, `factory_reset`, `change_password` (all `fd, ..., *, device_info=`): raise `OSError(EOPNOTSUPP)`. Callers catch `Exception`; `TimeoutError` is the only re-raise, never raised here.

## truenas_zfstierd_client / _common (`plugins/zfs/tier.py`, `alert/source/zfs_tier.py`, `etc_files/truenas_zfstierd.py`)

- `RewriteJobStatus` StrEnum COMPLETE RUNNING QUEUED CANCELLED STOPPED ERROR (= API `TierRewriteJobStatus` Literal). `JSONRPCErrorCode` IntEnum DATASET_NOT_FOUND JOB_NOT_FOUND JOB_ALREADY_EXISTS DISABLED (values ours, only compared by member).
- `InfoResult`/`CreateJobResult`/`RecoverResult`/`JobStats` dataclasses with the fields `tier.py` reads.
- No jobs: `enum_jobs()` empty; `get_last_job` → `KeyError`; `get_info`, `get_resolved_failures` → `ValueError` (tier.py's "no such job" paths).
- `RewriteClient` enters; `create_job`/`abort_job`/`recover_job` raise `RewriteClientException(DISABLED, msg)` → middleware `CallError`.
- `configfile.ZFSTierdGlobalConfig` + `generate_config()` → `key = value` text (format ours; no daemon reads it). Rendered `enabled` stays False: ZFSTIER is not entitled in the ce column.

## truesearch_client (`plugins/truesearch.py` — imported by `middlewared/main.py` at start; `etc_files/truesearchd/`)

- Exceptions: `TruesearchError` base; `Busy`, `NoIndex`, `Refused(.detail)`, `CallError` (imported as `BoundError`) subclass it.
- `Client()` `__enter__` raises `ConnectionRefusedError` → `truesearch.query` "TrueSearch is not available" ENOTCONN.
- `config`: `TARGETS_DIR` never a mountpoint → etc render `FileShouldNotExist`; `read_config()` → `None` (no file); `TargetRow` frozen dataclass (id, dataset, root, exclude, enabled) for `replace()`; `target_id`, `render_config` ours.
- `control`: `indexer_status`, `served_ids`, `request` raise `ConnectionRefusedError` (callers catch `OSError`) → status `running=False`; `devices_of` real `st_dev` set; `wait_released` True; `IndexerStatus`/`IndexerTarget`/`UnindexedTarget` dataclasses for the `match` in `_dataset_status`.

## licenselib (`utils/license/legacy.py`, `plugins/truenas/license.py`, `src/freenas/usr/local/bin/truenas-hw-license.py`)

- `license.License.load()` always raises `ValueError` → `get_legacy_license_info()` None, `describe_legacy_license` MALFORMED, `_is_legacy_license` False.
- `License(...)` constructs (13 positional, order from `truenas-hw-license.py`); `dump()` raises → that script's `record_error` path, no hardware-only record. `repr` omits `customer_key`.
- `Features` dedup jails fibrechannel vm (names from public tests/comments); `ContractType` lowercase members, `legacy` the only one named in code; `ContractHardware.parts`, `ContractSoftware.none`.
- `utils.proactive_support_allowed(name)`: GOLD/SILVER/SILVERINTERNATIONAL, the tiers `policy.py` grants proactive support.

## Gaps

- Full plugin imports need the TrueNAS runtime (service framework deps). The check fakes missing host deps and imports every using module; ones that fail on faked deps are logged, not failed. Stub-caused errors fail.
- Static attribute coverage of returned objects is by annotation / `isinstance` narrowing / match patterns; `get_resolved_failures` items and `Client.search` results are unchecked (stubs never return any).

## Check (`scripts/check-truenas-stubs.py`)

- Classify: every top-level import under `src/` has a known provider.
- Static: AST over the 27.0.0 tree — imports, `Name.attr` on imported names, call shapes bound against stub signatures, match-class kwargs, attrs on stub-class-annotated names, truenas_pylicensed value flow (`verify()` / `.features` / `get_fingerprint()`).
- Import: every non-test using module, stubs real, missing deps faked (`scripts/_truenas_stubs_harness.py`). `CORE_MODULES` must import.
- CE: public entitlement engine with the stubs; entitled set == ce column == {APPS, CONTAINERS, DEDUP, VMS}.
- Tests: middleware's own license/entitlement unit tests against the stubs; exclusions + reasons in the script.
- Doc list below must equal the files that import a closed lib; the check fails otherwise.

## Derived from (middleware `release/27.0.0`)

<!-- derived-from:begin -->
- `src/freenas/usr/local/bin/truenas-hw-license.py` (licenselib)
- `src/middlewared/middlewared/alert/source/license_status.py` 
- `src/middlewared/middlewared/alert/source/zfs_tier.py` (truenas_zfstierd_client, truenas_zfstierd_common)
- `src/middlewared/middlewared/etc_files/truenas_zfstierd.py` (truenas_pylicensed, truenas_zfstierd_common)
- `src/middlewared/middlewared/etc_files/truesearchd/truesearch.conf.py` (truesearch_client)
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
- `src/middlewared/middlewared/plugins/truenas/license.py` (licenselib, truenas_pylicensed)
- `src/middlewared/middlewared/plugins/truenas_s3/bucket_crud.py` 
- `src/middlewared/middlewared/plugins/truenas_s3/config.py` 
- `src/middlewared/middlewared/plugins/truesearch.py` (truenas_pylicensed, truesearch_client)
- `src/middlewared/middlewared/plugins/update_/profile_.py` 
- `src/middlewared/middlewared/plugins/vm/info.py` 
- `src/middlewared/middlewared/plugins/webshare/sharing.py` 
- `src/middlewared/middlewared/plugins/zfs/resource_create.py` 
- `src/middlewared/middlewared/plugins/zfs/tier.py` (truenas_pylicensed, truenas_zfstierd_client, truenas_zfstierd_common)
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
- `src/middlewared/middlewared/pytest/unit/utils/test_license_legacy_utils.py` (licenselib, truenas_pylicensed)
- `src/middlewared/middlewared/pytest/unit/utils/test_license_utils.py` 
- `src/middlewared/middlewared/utils/disks_/disk_class.py` (truenas_pylibsed)
- `src/middlewared/middlewared/utils/entitlements/__init__.py` 
- `src/middlewared/middlewared/utils/entitlements/engine.py` 
- `src/middlewared/middlewared/utils/entitlements/matrix.py` 
- `src/middlewared/middlewared/utils/entitlements/policy.py` 
- `src/middlewared/middlewared/utils/license/__init__.py` 
- `src/middlewared/middlewared/utils/license/daemon.py` 
- `src/middlewared/middlewared/utils/license/legacy.py` (licenselib, truenas_pylicensed)
- `src/middlewared/middlewared/utils/license/types.py` 
- `src/middlewared/middlewared/utils/service/entitlement.py` 
<!-- derived-from:end -->
