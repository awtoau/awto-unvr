#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""check-truenas-stubs - our truenas/stubs/ stand-ins cover what public middleware 27 uses (#356).

- Sparse-clones truenas/middleware release/27.0.0 (*.py only) to tmp/truenas-middleware-27.
- classify: every top-level import in src/ must be stdlib, local, public, or a stub here (CLOSED).
- static: AST-collect every closed-lib import, attribute, call signature, match pattern and
  stub-class-annotated attribute; plus truenas_pylicensed value flow (verify()/features/fingerprint).
  Asserts docs/truenas-stubs.md lists exactly the using files.
- import: every non-test using module, stubs real, missing host deps faked
  (scripts/_truenas_stubs_harness.py). Fails only on a stub-caused error; CORE_MODULES must import.
- ce: public entitlement engine + stubs on GENERIC hardware: entitled exactly where Vector.ce == 1.
- tests: middleware's own license/entitlement unit tests against the stubs.
- --gate: SKIP (rc 0) when the clone is absent; never clones. Log: tmp/logs/check-truenas-stubs.log
"""

from __future__ import annotations

import argparse
import ast
import dataclasses
import importlib
import inspect
import json
import os
import re
import shutil
import subprocess
import sys
import threading
import time
from collections import defaultdict
from pathlib import Path

from _repo import DOCS, REPO, TMP, make_log

log = make_log("check-truenas-stubs")

URL = "https://github.com/truenas/middleware"
BRANCH = "release/27.0.0"
CLONE = TMP / "truenas-middleware-27"
MW_SRC = CLONE / "src" / "middlewared"
STUBS = REPO / "truenas" / "stubs"
DOC = DOCS / "truenas-stubs.md"
PKG = "truenas_pylicensed"
# Closed top-level module -> our stub dir (truenas/stubs/<dir>).
CLOSED = {
    "truenas_pylicensed": "truenas_pylicensed",
    "truenas_pylibsed": "truenas_pylibsed",
    "truenas_zfstierd_client": "truenas_zfstierd",
    "truenas_zfstierd_common": "truenas_zfstierd",
    "truesearch_client": "truesearch_client",
    "licenselib": "licenselib",
}
STUB_DIRS = sorted({STUBS / d for d in CLOSED.values()})
# Public TrueNAS sources in the scale-build manifest that provide these (checked 2026-11-14, #356).
PUBLIC_TRUENAS = {
    "apps_ci", "apps_schema", "apps_validation", "catalog_reader",  # apps_validation
    "cryptit", "fenced", "ixdiagnose", "ixhardware", "libsg3", "libsgio", "libzfs",
    "scstadmin", "spdk", "zettarepl", "pysss_murmur", "pysss_nss_idmap", "samba", "tdb",
    "wbclient", "truenas_acme_utils", "truenas_api_client", "truenas_api_key",
    "truenas_authenticator", "truenas_connect_utils", "truenas_crypto_utils", "truenas_keyring",
    "truenas_os", "truenas_os_pyutils", "truenas_pam_faillog", "truenas_pam_session",
    "truenas_pylibvirt", "truenas_pylibzfs", "truenas_pymdns", "truenas_pynetif", "truenas_pypam",
    "truenas_pypwenc", "truenas_pyscram", "truenas_pysnmp", "truenas_verify",
}  # fmt: skip
# Debian/PyPI third-party, and middleware-local script/doc siblings imported by bare name.
THIRD_PARTY = {
    "OpenSSL", "acme", "aiohttp", "alembic", "annotated_types", "asyncssh", "aws_requests_auth",
    "boto3", "botocore", "bs4", "certbot_dns_cloudflare", "certbot_dns_digitalocean", "croniter",
    "cryptography", "dateutil", "dns", "docker", "google", "google_auth_httplib2",
    "googleapiclient", "gssapi", "html2text", "httplib2", "importlinter", "influxdb", "ipaclient",
    "ipalib", "ipaplatform", "ipapython", "isodate", "jeepney", "josepy", "json_schema_for_humans",
    "jsonschema", "jwt", "kmip", "ldap", "lexicon", "mako", "netsnmpagent", "packaging",
    "paramiko", "pyVim", "pyVmomi", "pydantic", "pydantic_core", "pyinotify", "pyotp", "pytest",
    "pytz", "pyudev", "remote_pdb", "requests", "rich", "sphinxbootstrap4theme", "sqlalchemy",
    "typing_extensions", "urllib3", "websocket", "yaml",
}  # fmt: skip
LOCAL = {
    "middlewared",
    "auto_config",
    "bases",
    "changelog",
    "config",
    "functions",
    "protocols",
    "third_party",
}

# Host-process guard (awto-dan#53): git prints progress continuously, 60 s silent = hung.
STALL_S = 60
# Backstop: 3x the measured cold clone (#356).
CLONE_LIMIT_S = 20

# Must import under the harness: entitlement vocabulary at module level, legacy licence, disks.
CORE_MODULES = (
    "middlewared.utils.license",
    "middlewared.utils.entitlements",
    "middlewared.utils.disks_.disk_class",
    "middlewared.alert.source.zfs_tier",
    "middlewared.etc_files.truenas_zfstierd",
)

# Middleware unit tests run against the stub, relative to middlewared/pytest/unit.
# Excluded: test_license_legacy_utils (decodes real legacy blobs; our licenselib decodes none),
# test_validate_topology/test_iscsi_target/test_failover (faked service framework deps),
# test_truenas_connect (truenas_connect_utils).
UNIT_TESTS = (
    "utils/test_entitlements.py",
    "utils/test_get_license.py",
    "utils/test_license_utils.py",
    "utils/test_license_info_wire.py",
    "plugins/truenas/test_entitlements_api.py",
    "plugins/truenas/test_license_remove.py",
    "plugins/test_system_product.py",
    "plugins/test_support.py",
    "plugins/test_kmip_update.py",
    "plugins/test_fc.py",
    "plugins/update/test_profile.py",
)
DESELECT = (
    # legacy blob decode (stub decodes none, by design); ejson comes from faked truenas_api_client.
    "utils/test_license_info_wire.py::test_projection_matches_the_recorded_wire[legacy-<lambda>-expected1]",
    "utils/test_license_info_wire.py::test_dates_reach_the_client_as_ejson_dates",
)
# Measured ~4 s for the set above; stall guard is the primary, this the backstop.
TESTS_LIMIT_S = 60


# ---------------------------------------------------------------- clone


def _run_stall(cmd: list[str], limit_s: float, **kw) -> int:
    """Run cmd; kill on STALL_S without output or limit_s overall. Output -> log."""
    log("+ " + " ".join(cmd))
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, **kw)
    last = [time.monotonic(), b""]

    def pump():
        buf = b""
        for chunk in iter(lambda: p.stdout.read1(4096), b""):
            last[0], last[1] = time.monotonic(), chunk
            *lines, buf = (buf + chunk).replace(b"\r", b"\n").split(b"\n")
            for line in lines:
                if line.strip():
                    log("  " + line.decode(errors="replace"))
        if buf.strip():
            log("  " + buf.decode(errors="replace"))

    t = threading.Thread(target=pump, daemon=True)
    t.start()
    start = time.monotonic()
    while p.poll() is None:
        t.join(0.5)
        now = time.monotonic()
        if now - last[0] > STALL_S or now - start > limit_s:
            p.kill()
            why = "stalled" if now - last[0] > STALL_S else "over limit"
            log(
                f"FATAL: {cmd[0]} {why}: {now - start:.0f}s elapsed, limit {limit_s}s, "
                f"stall {STALL_S}s, last output {last[1][-200:]!r}, loadavg {os.getloadavg()}"
            )
            return 124
    t.join(2)
    return p.returncode


def ensure_clone(refresh: bool) -> bool:
    if (MW_SRC / "middlewared").is_dir() and not refresh:
        return True
    if CLONE.exists():
        rc = _run_stall(
            [
                "git",
                "-C",
                str(CLONE),
                "fetch",
                "--depth",
                "1",
                "--progress",
                "origin",
                BRANCH,
            ],
            CLONE_LIMIT_S,
        )
        if rc == 0:
            rc = _run_stall(
                ["git", "-C", str(CLONE), "reset", "--hard", "FETCH_HEAD"],
                CLONE_LIMIT_S,
            )
        return rc == 0
    rc = _run_stall(
        ["git", "clone", "--progress", "--depth", "1", "--filter=blob:none", "--sparse",
         "-b", BRANCH, URL, str(CLONE)],
        CLONE_LIMIT_S,
    )  # fmt: skip
    if rc == 0:
        rc = _run_stall(
            ["git", "-C", str(CLONE), "sparse-checkout", "set", "--no-cone", "*.py"],
            CLONE_LIMIT_S,
        )
    return rc == 0


# ---------------------------------------------------------------- static


@dataclasses.dataclass
class Uses:
    files: set[str] = dataclasses.field(default_factory=set)
    # (module, name) -> sites
    names: dict = dataclasses.field(default_factory=lambda: defaultdict(set))
    # ((module, name|None), attr) -> sites, e.g. (("truenas_pylicensed", "LicenseFeature"), "HA")
    attrs: dict = dataclasses.field(default_factory=lambda: defaultdict(set))
    # (class, kwarg) -> sites, for LicenseStatus(...)/FeatureEntry(...)
    kwargs: dict = dataclasses.field(default_factory=lambda: defaultdict(set))
    # attr -> sites, read off a LicenseStatus / FeatureEntry value
    status_attrs: dict = dataclasses.field(default_factory=lambda: defaultdict(set))
    entry_attrs: dict = dataclasses.field(default_factory=lambda: defaultdict(set))
    # key -> sites: fp["k"] (required) / fp.get("k") (optional)
    fp_keys: dict = dataclasses.field(default_factory=lambda: defaultdict(set))
    fp_opt_keys: dict = dataclasses.field(default_factory=lambda: defaultdict(set))
    # ((module, name), sub-attr|None, n positional, kw names) -> sites
    calls: dict = dataclasses.field(default_factory=lambda: defaultdict(set))
    # ((module, class), attr) -> sites: annotated names and match-class patterns
    inst_attrs: dict = dataclasses.field(default_factory=lambda: defaultdict(set))


def _ann_has(node: ast.AST | None, word: str) -> bool:
    return node is not None and word in ast.unparse(node)


def _strip_or(node: ast.AST) -> ast.AST:
    """`(x.features or {})` -> `x.features`."""
    if isinstance(node, ast.BoolOp) and isinstance(node.op, ast.Or):
        return node.values[0]
    return node


def _walk_scope(scope: ast.AST):
    """ast.walk, not descending into nested functions (each is its own scope)."""
    todo = list(ast.iter_child_nodes(scope))
    while todo:
        n = todo.pop()
        yield n
        if not isinstance(n, (ast.FunctionDef, ast.AsyncFunctionDef, ast.Lambda)):
            todo.extend(ast.iter_child_nodes(n))


def _status_returning_funcs(trees: dict[str, ast.Module]) -> set[str]:
    """Functions anywhere whose return annotation names LicenseStatus (verify, upload_license...)."""
    out = set()
    for tree in trees.values():
        for n in ast.walk(tree):
            if isinstance(n, (ast.FunctionDef, ast.AsyncFunctionDef)) and _ann_has(
                n.returns, "LicenseStatus"
            ):
                out.add(n.name)
    return out


def _scan_scope(scope, rel, local, status_funcs, uses: Uses) -> None:
    """Track LicenseStatus / FeatureEntry / fingerprint values bound in one function or module."""
    status, entry, fp = set(), set(), set()
    verify_names = {k for k, v in local.items() if v == "verify"} | status_funcs
    fp_names = {k for k, v in local.items() if v == "get_fingerprint"}

    def callee(n):
        return (
            n.func.id
            if isinstance(n, ast.Call) and isinstance(n.func, ast.Name)
            else None
        )

    def is_status(n):
        return (callee(n) in verify_names) or (
            isinstance(n, ast.Name) and n.id in status
        )

    def features_of(n):
        n = _strip_or(n)
        return (
            isinstance(n, ast.Attribute) and n.attr == "features" and is_status(n.value)
        )

    def is_entry(v):
        """status.features.get(k) / status.features[k] / either `if ... else None`."""
        if isinstance(v, ast.IfExp):
            v = v.body
        if (
            isinstance(v, ast.Call)
            and isinstance(v.func, ast.Attribute)
            and v.func.attr == "get"
        ):
            return features_of(v.func.value)
        return isinstance(v, ast.Subscript) and features_of(v.value)

    if isinstance(scope, (ast.FunctionDef, ast.AsyncFunctionDef)):
        a = scope.args
        for arg in a.posonlyargs + a.args + a.kwonlyargs:
            if _ann_has(arg.annotation, "LicenseStatus"):
                status.add(arg.arg)
            if _ann_has(arg.annotation, "FeatureEntry"):
                entry.add(arg.arg)

    body_nodes = list(_walk_scope(scope))
    # Two passes: bindings first, so a use above its (loop) rebinding still resolves.
    for _ in range(2):
        for n in body_nodes:
            if (
                isinstance(n, ast.Assign)
                and len(n.targets) == 1
                and isinstance(n.targets[0], ast.Name)
            ):
                tgt, v = n.targets[0].id, n.value
                if is_status(v):
                    status.add(tgt)
                elif callee(v) in fp_names:
                    fp.add(tgt)
                elif is_entry(v):
                    entry.add(tgt)
            elif isinstance(n, ast.AnnAssign) and isinstance(n.target, ast.Name):
                if _ann_has(n.annotation, "LicenseStatus"):
                    status.add(n.target.id)
            elif isinstance(n, ast.withitem) and isinstance(n.optional_vars, ast.Name):
                if is_status(n.context_expr):
                    status.add(n.optional_vars.id)
            elif isinstance(n, (ast.For, ast.comprehension)):
                it = n.iter
                if (
                    isinstance(it, ast.Call)
                    and isinstance(it.func, ast.Attribute)
                    and features_of(it.func.value)
                ):
                    tgt = n.target
                    if (
                        it.func.attr == "items"
                        and isinstance(tgt, ast.Tuple)
                        and len(tgt.elts) == 2
                    ):
                        if isinstance(tgt.elts[1], ast.Name):
                            entry.add(tgt.elts[1].id)
                    elif it.func.attr == "values" and isinstance(tgt, ast.Name):
                        entry.add(tgt.id)

    for n in body_nodes:
        site = f"{rel}:{getattr(n, 'lineno', 0)}"
        if isinstance(n, ast.Attribute):
            if is_status(n.value):
                uses.status_attrs[n.attr].add(site)
            elif isinstance(n.value, ast.Name) and n.value.id in entry:
                uses.entry_attrs[n.attr].add(site)
        elif (
            isinstance(n, ast.Subscript)
            and isinstance(n.value, ast.Name)
            and n.value.id in fp
        ):
            if isinstance(n.slice, ast.Constant):
                uses.fp_keys[n.slice.value].add(site)
        elif (
            isinstance(n, ast.Call)
            and isinstance(n.func, ast.Attribute)
            and n.func.attr == "get"
            and isinstance(n.func.value, ast.Name)
            and n.func.value.id in fp
            and n.args
            and isinstance(n.args[0], ast.Constant)
        ):
            uses.fp_opt_keys[n.args[0].value].add(site)


def _closed(mod: str | None) -> bool:
    return bool(mod) and mod.split(".")[0] in CLOSED


def collect(root: Path) -> Uses:
    uses = Uses()
    trees: dict[str, ast.Module] = {}
    for path in sorted(root.rglob("*.py")):
        text = path.read_text(errors="replace")
        if not any(m in text for m in CLOSED):
            continue
        rel = str(path.relative_to(CLONE))
        trees[rel] = ast.parse(text, filename=rel)
    status_funcs = _status_returning_funcs(trees)

    for rel, tree in trees.items():
        local: dict[
            str, str
        ] = {}  # truenas_pylicensed local name -> stub name (value flow)
        bound: dict[
            str, tuple[str, str | None]
        ] = {}  # local -> (module, name|None for a module)
        for n in ast.walk(tree):
            if isinstance(n, ast.ImportFrom) and not n.level and _closed(n.module):
                uses.files.add(rel)
                for a in n.names:
                    uses.names[(n.module, a.name)].add(f"{rel}:{n.lineno}")
                    bound[a.asname or a.name] = (n.module, a.name)
                    if n.module.split(".")[0] == PKG:
                        local[a.asname or a.name] = a.name
            elif isinstance(n, ast.Import):
                for a in n.names:
                    if _closed(a.name):
                        uses.files.add(rel)
                        uses.names[(a.name, None)].add(f"{rel}:{n.lineno}")
                        bound[a.asname or a.name.split(".")[0]] = (
                            a.asname and a.name or a.name.split(".")[0],
                            None,
                        )
        if not bound:
            continue
        for n in ast.walk(tree):
            site = f"{rel}:{getattr(n, 'lineno', 0)}"
            if (
                isinstance(n, ast.Attribute)
                and isinstance(n.value, ast.Name)
                and n.value.id in bound
            ):
                uses.attrs[(bound[n.value.id], n.attr)].add(site)
            elif isinstance(n, ast.Call):
                ref = None
                if isinstance(n.func, ast.Name) and n.func.id in bound:
                    ref = (bound[n.func.id], None)
                elif (
                    isinstance(n.func, ast.Attribute)
                    and isinstance(n.func.value, ast.Name)
                    and n.func.value.id in bound
                    and bound[n.func.value.id][1] is None
                ):
                    ref = (bound[n.func.value.id], n.func.attr)
                if (
                    ref
                    and not any(isinstance(x, ast.Starred) for x in n.args)
                    and all(k.arg for k in n.keywords)
                ):
                    uses.calls[
                        (ref, len(n.args), tuple(sorted(k.arg for k in n.keywords)))
                    ].add(site)
                    if ref[1] is None and ref[0][1] in (
                        "LicenseStatus",
                        "FeatureEntry",
                    ):
                        for kw in n.keywords:
                            uses.kwargs[(ref[0][1], kw.arg)].add(site)
            elif (
                isinstance(n, ast.MatchClass)
                and isinstance(n.cls, ast.Name)
                and n.cls.id in bound
            ):
                for kw in n.kwd_attrs:
                    uses.inst_attrs[(bound[n.cls.id], kw)].add(site)
        scopes = [tree] + [
            n
            for n in ast.walk(tree)
            if isinstance(n, (ast.FunctionDef, ast.AsyncFunctionDef))
        ]
        for sc in scopes:
            if local:
                _scan_scope(sc, rel, local, status_funcs, uses)
            _scan_annotated(sc, rel, bound, uses)
    return uses


def _scan_annotated(scope, rel, bound, uses: Uses) -> None:
    """Attributes read off names annotated with a from-imported stub class (info: InfoResult)."""
    classes = {
        k: v for k, v in bound.items() if v[1] is not None and v[1][:1].isupper()
    }
    typed: dict[str, list] = {}

    def note(name, ann):
        if ann is None:
            return
        refs = [
            classes[x.id]
            for x in ast.walk(ann)
            if isinstance(x, ast.Name) and x.id in classes
        ]
        # Containers (list[X]) say nothing about the name's own attributes.
        if refs and not isinstance(ann, ast.Subscript):
            typed[name] = refs

    if isinstance(scope, (ast.FunctionDef, ast.AsyncFunctionDef)):
        a = scope.args
        for arg in a.posonlyargs + a.args + a.kwonlyargs:
            note(arg.arg, arg.annotation)
    nodes = list(_walk_scope(scope))
    for n in nodes:
        if isinstance(n, ast.AnnAssign) and isinstance(n.target, ast.Name):
            note(n.target.id, n.annotation)
        # x = y if isinstance(y, Cls) else None
        elif (
            isinstance(n, ast.Assign)
            and len(n.targets) == 1
            and isinstance(n.targets[0], ast.Name)
            and isinstance(n.value, ast.IfExp)
            and isinstance(t := n.value.test, ast.Call)
            and isinstance(t.func, ast.Name)
            and t.func.id == "isinstance"
            and len(t.args) == 2
            and isinstance(t.args[1], ast.Name)
            and t.args[1].id in classes
        ):
            typed[n.targets[0].id] = [classes[t.args[1].id]]
    for n in nodes:
        if (
            isinstance(n, ast.Attribute)
            and isinstance(n.value, ast.Name)
            and n.value.id in typed
        ):
            for ref in typed[n.value.id]:
                uses.inst_attrs[(ref, n.attr)].add(f"{rel}:{n.lineno}")


def _first(sites: set[str]) -> str:
    s = sorted(sites)
    return s[0] + (f" (+{len(s) - 1})" if len(s) > 1 else "")


def _resolve(ref: tuple[str, str | None]):
    mod, name = ref
    m = importlib.import_module(mod)
    return m if name is None else getattr(m, name)


def _has_inst_attr(cls, attr: str) -> bool:
    if hasattr(cls, attr):
        return True
    if dataclasses.is_dataclass(cls):
        return attr in {f.name for f in dataclasses.fields(cls)}
    return attr in getattr(cls, "__annotations__", {})


def classify_imports() -> list[str]:
    """Every top-level import under src/ must have a known provider; a new closed lib fails here."""
    names: dict[str, str] = {}
    for p in (CLONE / "src").rglob("*.py"):
        try:
            tree = ast.parse(p.read_text(errors="replace"))
        except SyntaxError:
            continue
        for n in ast.walk(tree):
            mods = []
            if isinstance(n, ast.Import):
                mods = [a.name for a in n.names]
            elif isinstance(n, ast.ImportFrom) and n.module and not n.level:
                mods = [n.module]
            for m in mods:
                names.setdefault(m.split(".")[0], f"{p.relative_to(CLONE)}:{n.lineno}")
    known = (
        set(sys.stdlib_module_names)
        | LOCAL
        | PUBLIC_TRUENAS
        | THIRD_PARTY
        | set(CLOSED)
    )
    unknown = {k: v for k, v in names.items() if k not in known}
    return [
        f"unclassified import {k!r} ({v}): public source, third-party, or a new closed lib?"
        for k, v in sorted(unknown.items())
    ]


def check_static(uses: Uses) -> list[str]:
    for d in STUB_DIRS:
        sys.path.insert(0, str(d))
    stub = importlib.import_module(PKG)
    errs = []
    for (mod, name), sites in sorted(uses.names.items(), key=str):
        try:
            m = importlib.import_module(mod)
        except ImportError:
            errs.append(f"missing module {mod} ({_first(sites)})")
            continue
        if name is not None and not hasattr(m, name):
            errs.append(f"missing {mod}.{name} ({_first(sites)})")
    for (ref, attr), sites in sorted(uses.attrs.items(), key=str):
        try:
            if not hasattr(_resolve(ref), attr):
                errs.append(f"missing {ref[0]}.{ref[1] or ''}.{attr} ({_first(sites)})")
        except (ImportError, AttributeError):
            pass  # reported above
    for (ref, attr), sites in sorted(uses.inst_attrs.items(), key=str):
        try:
            if not _has_inst_attr(_resolve(ref), attr):
                errs.append(
                    f"{ref[0]}.{ref[1]} instances lack .{attr} ({_first(sites)})"
                )
        except (ImportError, AttributeError):
            pass
    for ((ref, sub), nargs, kws), sites in sorted(uses.calls.items(), key=str):
        try:
            fn = _resolve(ref)
            fn = getattr(fn, sub) if sub else fn
            sig = inspect.signature(fn)
        except (ImportError, AttributeError, TypeError, ValueError):
            continue
        try:
            sig.bind(*range(nargs), **dict.fromkeys(kws))
        except TypeError as e:
            label = f"{ref[0]}.{ref[1] or ''}{'.' + sub if sub else ''}"
            errs.append(
                f"call {label}({nargs} pos, kw {list(kws)}): {e} ({_first(sites)})"
            )
    for (cls, kw), sites in sorted(uses.kwargs.items()):
        if kw not in {f.name for f in dataclasses.fields(getattr(stub, cls))}:
            errs.append(f"{cls}() has no field {kw!r} ({_first(sites)})")
    st = stub.verify()
    for attr, sites in sorted(uses.status_attrs.items()):
        if not hasattr(st, attr):
            errs.append(f"LicenseStatus has no .{attr} ({_first(sites)})")
    fe = stub.FeatureEntry(name="X")
    for attr, sites in sorted(uses.entry_attrs.items()):
        if not hasattr(fe, attr):
            errs.append(f"FeatureEntry has no .{attr} ({_first(sites)})")
    fp = stub.get_fingerprint()
    for key, sites in sorted(uses.fp_keys.items()):
        if key not in fp:
            errs.append(
                f"get_fingerprint() lacks required key {key!r} ({_first(sites)})"
            )
    # Semantics: no licence, the code middleware falls back to legacy on.
    if st.valid or st.code is not stub.LicenseError.NO_LICENSE:
        errs.append(
            f"verify() must be valid=False code=NO_LICENSE, got {st.valid} {st.code}"
        )
    for enum_cls in (stub.LicenseFeature, stub.SupportTier, stub.LicenseType):
        if bad := [m.name for m in enum_cls if m.value != m.name]:
            errs.append(f"{enum_cls.__name__} value != name: {bad}")
    return errs


def check_doc(uses: Uses) -> list[str]:
    text = DOC.read_text()
    try:
        block = text.split("<!-- derived-from:begin -->")[1].split(
            "<!-- derived-from:end -->"
        )[0]
    except IndexError:
        return [f"{DOC.name}: no derived-from markers"]
    listed = {
        ln.strip().strip("-").strip().split("`")[1]
        for ln in block.splitlines()
        if ln.strip().startswith("- `")
    }
    errs = []
    if missing := sorted(uses.files - listed):
        errs.append(f"{DOC.name} does not list: {missing}")
    if extra := sorted(listed - uses.files):
        errs.append(f"{DOC.name} lists files that no longer use a closed lib: {extra}")
    return errs


# ---------------------------------------------------------------- runtime

_IMPORT_PROBE = r"""
import faulthandler, importlib, json, sys, traceback
faulthandler.dump_traceback_later(50, exit=True)
import _truenas_stubs_harness as h
stub_dirs = json.loads(sys.argv[1])
closed = tuple(json.loads(sys.argv[2]))
res = {}
for mod in sys.argv[3:]:
    try:
        importlib.import_module(mod)
        res[mod] = ["ok", ""]
    except BaseException as e:
        tb = traceback.extract_tb(e.__traceback__)
        obj = getattr(e, "obj", None)
        objmod = str(getattr(obj, "__module__", None) or getattr(obj, "__name__", None) or "")
        stub = (
            any(f.filename.startswith(tuple(stub_dirs)) for f in tb)
            or (isinstance(e, ImportError) and any(c in str(e) for c in closed))
            or (isinstance(e, AttributeError) and objmod.startswith(closed))
        )
        res[mod] = ["stub" if stub else "runtime", f"{type(e).__name__}: {e}"[:300]]
res["__faked__"] = sorted(h.FINDER.faked)
print("JSON" + json.dumps(res))
"""


HARNESS_DIR = TMP / "truenas-stubs-harness"
_IMPORT_RE = re.compile(r"^\s*(?:from|import)\s+([A-Za-z_]\w*)", re.M)


def _mw_import_names() -> set[str]:
    """Top-level names middleware imports: the only ones the harness may fake."""
    names = set()
    for p in (MW_SRC / "middlewared").rglob("*.py"):
        names.update(_IMPORT_RE.findall(p.read_text(errors="replace")))
    return names - {"middlewared"} - set(CLOSED)


def _env() -> dict:
    """Harness copied to its own dir: scripts/ on the path would shadow names it should fake."""
    HARNESS_DIR.mkdir(parents=True, exist_ok=True)
    shutil.copy2(REPO / "scripts" / "_truenas_stubs_harness.py", HARNESS_DIR)
    env = dict(os.environ)
    env["TRUENAS_STUBS_FAKE"] = ",".join(sorted(_mw_import_names()))
    env["PYTHONPATH"] = os.pathsep.join(
        [*map(str, STUB_DIRS), str(MW_SRC), str(HARNESS_DIR)]
    )
    env["PYTHONDONTWRITEBYTECODE"] = "1"
    return env


def check_imports(uses: Uses) -> list[str]:
    mods = []
    for rel in sorted(uses.files):
        p = Path(rel)
        if "pytest" in p.parts or p.parts[:2] != ("src", "middlewared"):
            continue
        dotted = ".".join(p.with_suffix("").parts[2:])
        mods.append(dotted.removesuffix(".__init__"))
    # Measured ~3 s for all ~42 modules; the backstop is generous because it is one process.
    out = subprocess.run(
        [
            sys.executable,
            "-c",
            _IMPORT_PROBE,
            json.dumps([str(d) for d in STUB_DIRS]),
            json.dumps(sorted(CLOSED)),
            *mods,
        ],  # fmt: skip
        capture_output=True,
        text=True,
        env=_env(),
        cwd=MW_SRC,
        timeout=TESTS_LIMIT_S,
    )
    line = next((ln for ln in out.stdout.splitlines() if ln.startswith("JSON")), None)
    if line is None:
        return [f"import probe crashed rc={out.returncode}: {out.stderr[-800:]}"]
    res = json.loads(line[4:])
    faked = res.pop("__faked__")
    errs = []
    counts = defaultdict(int)
    for mod, (kind, msg) in sorted(res.items()):
        counts[kind] += 1
        if kind == "stub":
            errs.append(f"import {mod}: stub-caused {msg}")
        elif kind == "runtime":
            log(f"  import {mod}: not testable on host ({msg[:120]})")
    for core in CORE_MODULES:
        if res.get(core, ["missing"])[0] != "ok":
            errs.append(f"core module {core} must import: {res.get(core)}")
    log(f"import: {dict(counts)} of {len(res)}; faked deps: {', '.join(faked)}")
    return errs


_CE_PROBE = r"""
import json
import _truenas_stubs_harness
from middlewared.utils.license import get_license
from middlewared.utils.entitlements import (
    DERIVED_VECTORS, POLICY, TARGET_VECTORS, EntitlementFacts, HardwareClass, check_entitlement,
)
lic = get_license()
facts = EntitlementFacts(hardware_class=HardwareClass.GENERIC, license=lic)
out = {"license": repr(lic), "rows": {}}
for key in POLICY:
    vec = TARGET_VECTORS.get(key) or DERIVED_VECTORS[key]
    e = check_entitlement(key, facts)
    out["rows"][str(key)] = [bool(vec.ce), e.entitled, e.column, str(e.reason)]
print("JSON" + json.dumps(out))
"""
# The ce column of utils/entitlements/matrix.py as of 27.0.0; the probe also derives it.
CE_ENTITLED = {"APPS", "CONTAINERS", "DEDUP", "VMS"}


def check_ce_column() -> list[str]:
    """No licence on generic hardware must land every feature in the ce column (Dan, #356)."""
    out = subprocess.run(
        [sys.executable, "-c", _CE_PROBE],
        capture_output=True,
        text=True,
        env=_env(),
        cwd=MW_SRC,
        timeout=TESTS_LIMIT_S,
    )
    line = next((ln for ln in out.stdout.splitlines() if ln.startswith("JSON")), None)
    if line is None:
        return [f"ce probe crashed rc={out.returncode}: {out.stderr[-800:]}"]
    res = json.loads(line[4:])
    errs = []
    if res["license"] != "None":
        errs.append(f"get_license() with stubs must be None, got {res['license']}")
    entitled = set()
    for key, (ce, got, column, reason) in sorted(res["rows"].items()):
        if got:
            entitled.add(key)
        if got != ce:
            errs.append(
                f"ce column: {key} entitled={got} but Vector.ce={int(ce)} ({column}, {reason})"
            )
    log(f"ce: entitled {sorted(entitled)} of {len(res['rows'])} policy keys")
    if entitled != CE_ENTITLED:
        errs.append(
            f"ce: entitled {sorted(entitled)} != expected {sorted(CE_ENTITLED)}"
        )
    return errs


def run_unit_tests() -> list[str]:
    unit = MW_SRC / "middlewared" / "pytest" / "unit"
    cmd = [
        sys.executable, "-m", "pytest", "-q", "-p", "no:cacheprovider", "-p", "_truenas_stubs_harness",
        "-W", "ignore", "--tb=short", "--rootdir", str(unit), "-c", os.devnull, *UNIT_TESTS,
    ]  # fmt: skip
    for d in DESELECT:
        cmd += ["--deselect", d]
    rc = _run_stall(cmd, TESTS_LIMIT_S, env=_env(), cwd=unit)
    return [] if rc == 0 else [f"middleware unit tests failed rc={rc} (see log)"]


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument(
        "--refresh", action="store_true", help="re-fetch the middleware branch"
    )
    ap.add_argument(
        "--gate", action="store_true", help="SKIP when no clone; never clone"
    )
    ap.add_argument("--static-only", action="store_true")
    a = ap.parse_args()

    if a.gate and not (MW_SRC / "middlewared").is_dir():
        log(
            f"SKIP: no middleware clone at {CLONE.relative_to(REPO)} (run ./dev.py check-truenas-stubs)"
        )
        return 0
    if not a.gate and not ensure_clone(a.refresh):
        return 1
    head = subprocess.run(
        ["git", "-C", str(CLONE), "rev-parse", "--short", "HEAD"],
        capture_output=True,
        text=True,
    )
    log(f"middleware {BRANCH} @ {head.stdout.strip()}")

    uses = collect(CLONE / "src")
    log(
        f"static: {len(uses.files)} files, {len(uses.names)} imports, {len(uses.attrs)} attrs, "
        f"{len(uses.calls)} call shapes, {len(uses.inst_attrs)} instance attrs, status attrs {sorted(uses.status_attrs)}, "
        f"entry attrs {sorted(uses.entry_attrs)}, fingerprint keys {sorted(uses.fp_keys)} "
        f"optional {sorted(uses.fp_opt_keys)}"
    )
    errs = classify_imports() + check_static(uses) + check_doc(uses)
    if not a.static_only:
        errs += check_imports(uses)
        errs += check_ce_column()
        errs += run_unit_tests()
    for e in errs:
        log(f"FAIL: {e}")
    log("PASS" if not errs else f"{len(errs)} failure(s)")
    return 1 if errs else 0


if __name__ == "__main__":
    sys.exit(main())
