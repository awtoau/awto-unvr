#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""check-pylicensed-stub - our truenas_pylicensed stub covers what public middleware 27 uses (#356).

- Sparse-clones truenas/middleware release/27.0.0 (*.py only) to tmp/truenas-middleware-27.
- static: AST-collect every truenas_pylicensed import, attribute and constructor kwarg, plus
  attributes read off verify()/LicenseStatus/FeatureEntry/get_fingerprint() values; assert the
  stub has each. Also asserts docs/truenas-pylicensed-stub.md lists exactly the using files.
- import: import every non-test using module with the stub real and missing host deps faked
  (scripts/_pylicensed_harness.py). Fails only on a stub-caused error; core modules must import.
- tests: run middleware's own license/entitlement unit tests against the stub.
- --gate: SKIP (rc 0) when the clone is absent; never clones. Log: tmp/logs/check-pylicensed-stub.log
"""

from __future__ import annotations

import argparse
import ast
import dataclasses
import importlib
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

log = make_log("check-pylicensed-stub")

URL = "https://github.com/truenas/middleware"
BRANCH = "release/27.0.0"
CLONE = TMP / "truenas-middleware-27"
MW_SRC = CLONE / "src" / "middlewared"
STUB_ROOT = REPO / "truenas" / "stubs" / "truenas_pylicensed"
DOC = DOCS / "truenas-pylicensed-stub.md"
PKG = "truenas_pylicensed"

# Host-process guard (awto-dan#53): git prints progress continuously, 60 s silent = hung.
STALL_S = 60
# Backstop: 3x the measured cold clone (#356).
CLONE_LIMIT_S = 20

# Must import under the harness: they hold every enum member at module level (matrix/policy/engine).
CORE_MODULES = ("middlewared.utils.license", "middlewared.utils.entitlements")

# Middleware unit tests run against the stub, relative to middlewared/pytest/unit.
# Excluded, all for faked deps not the stub: test_license_legacy_utils (licenselib),
# test_validate_topology/test_iscsi_target/test_failover (service framework),
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
    # legacy blob decode needs real licenselib; ejson comes from truenas_api_client.
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
    # (stub name, attr) -> sites, e.g. ("LicenseFeature", "HA")
    attrs: dict = dataclasses.field(default_factory=lambda: defaultdict(set))
    # (class, kwarg) -> sites, for LicenseStatus(...)/FeatureEntry(...)
    kwargs: dict = dataclasses.field(default_factory=lambda: defaultdict(set))
    # attr -> sites, read off a LicenseStatus / FeatureEntry value
    status_attrs: dict = dataclasses.field(default_factory=lambda: defaultdict(set))
    entry_attrs: dict = dataclasses.field(default_factory=lambda: defaultdict(set))
    # key -> sites: fp["k"] (required) / fp.get("k") (optional)
    fp_keys: dict = dataclasses.field(default_factory=lambda: defaultdict(set))
    fp_opt_keys: dict = dataclasses.field(default_factory=lambda: defaultdict(set))
    bare_imports: set[str] = dataclasses.field(default_factory=set)


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


def collect(root: Path) -> Uses:
    uses = Uses()
    trees: dict[str, ast.Module] = {}
    for path in sorted(root.rglob("*.py")):
        text = path.read_text(errors="replace")
        if PKG not in text:
            continue
        rel = str(path.relative_to(CLONE))
        trees[rel] = ast.parse(text, filename=rel)
    status_funcs = _status_returning_funcs(trees)

    for rel, tree in trees.items():
        local: dict[str, str] = {}  # local name -> stub name
        for n in ast.walk(tree):
            if (
                isinstance(n, ast.ImportFrom)
                and n.module
                and n.module.split(".")[0] == PKG
            ):
                uses.files.add(rel)
                for a in n.names:
                    uses.names[(n.module, a.name)].add(f"{rel}:{n.lineno}")
                    local[a.asname or a.name] = a.name
            elif isinstance(n, ast.Import):
                for a in n.names:
                    if a.name.split(".")[0] == PKG:
                        uses.files.add(rel)
                        uses.bare_imports.add(f"{rel}:{n.lineno}")
        if not local:
            continue
        for n in ast.walk(tree):
            if (
                isinstance(n, ast.Attribute)
                and isinstance(n.value, ast.Name)
                and n.value.id in local
            ):
                uses.attrs[(local[n.value.id], n.attr)].add(f"{rel}:{n.lineno}")
            elif (
                isinstance(n, ast.Call)
                and isinstance(n.func, ast.Name)
                and local.get(n.func.id)
                in (
                    "LicenseStatus",
                    "FeatureEntry",
                )
            ):
                for kw in n.keywords:
                    if kw.arg:
                        uses.kwargs[(local[n.func.id], kw.arg)].add(f"{rel}:{n.lineno}")
        scopes = [tree] + [
            n
            for n in ast.walk(tree)
            if isinstance(n, (ast.FunctionDef, ast.AsyncFunctionDef))
        ]
        for s in scopes:
            _scan_scope(s, rel, local, status_funcs, uses)
    return uses


def _first(sites: set[str]) -> str:
    s = sorted(sites)
    return s[0] + (f" (+{len(s) - 1})" if len(s) > 1 else "")


def check_static(uses: Uses) -> list[str]:
    sys.path.insert(0, str(STUB_ROOT))
    stub = importlib.import_module(PKG)
    errs = []
    if uses.bare_imports:
        errs.append(f"`import {PKG}` form not modelled: {sorted(uses.bare_imports)}")
    for (mod, name), sites in sorted(uses.names.items()):
        try:
            m = importlib.import_module(mod)
        except ImportError:
            errs.append(f"missing module {mod} ({_first(sites)})")
            continue
        if not hasattr(m, name):
            errs.append(f"missing {mod}.{name} ({_first(sites)})")
    for (name, attr), sites in sorted(uses.attrs.items()):
        if not hasattr(getattr(stub, name, None), attr):
            errs.append(f"missing {name}.{attr} ({_first(sites)})")
    for (cls, kw), sites in sorted(uses.kwargs.items()):
        fields = {f.name for f in dataclasses.fields(getattr(stub, cls))}
        if kw not in fields:
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
        bad = [m.name for m in enum_cls if m.value != m.name]
        if bad:
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
        ln.strip().strip("-").strip().strip("`")
        for ln in block.splitlines()
        if ln.strip().startswith("-")
    }
    errs = []
    if missing := sorted(uses.files - listed):
        errs.append(f"{DOC.name} does not list: {missing}")
    if extra := sorted(listed - uses.files):
        errs.append(f"{DOC.name} lists files that no longer use {PKG}: {extra}")
    return errs


# ---------------------------------------------------------------- runtime

_IMPORT_PROBE = r"""
import faulthandler, importlib, json, sys, traceback
faulthandler.dump_traceback_later(50, exit=True)
import _pylicensed_harness as h
stub_dir = sys.argv[1]
res = {}
for mod in sys.argv[2:]:
    try:
        importlib.import_module(mod)
        res[mod] = ["ok", ""]
    except BaseException as e:
        tb = traceback.extract_tb(e.__traceback__)
        obj = getattr(e, "obj", None)
        objmod = str(getattr(obj, "__module__", None) or getattr(obj, "__name__", None) or "")
        stub = (
            any(f.filename.startswith(stub_dir) for f in tb)
            or (isinstance(e, ImportError) and "truenas_pylicensed" in str(e))
            or (isinstance(e, AttributeError) and objmod.startswith("truenas_pylicensed"))
        )
        res[mod] = ["stub" if stub else "runtime", f"{type(e).__name__}: {e}"[:300]]
res["__faked__"] = sorted(h.FINDER.faked)
print("JSON" + json.dumps(res))
"""


HARNESS_DIR = TMP / "pylicensed-harness"
_IMPORT_RE = re.compile(r"^\s*(?:from|import)\s+([A-Za-z_]\w*)", re.M)


def _mw_import_names() -> set[str]:
    """Top-level names middleware imports: the only ones the harness may fake."""
    names = set()
    for p in (MW_SRC / "middlewared").rglob("*.py"):
        names.update(_IMPORT_RE.findall(p.read_text(errors="replace")))
    return names - {"middlewared", PKG}


def _env() -> dict:
    """Harness copied to its own dir: scripts/ on the path would shadow names it should fake."""
    HARNESS_DIR.mkdir(parents=True, exist_ok=True)
    shutil.copy2(REPO / "scripts" / "_pylicensed_harness.py", HARNESS_DIR)
    env = dict(os.environ)
    env["PYLICENSED_FAKE"] = ",".join(sorted(_mw_import_names()))
    env["PYTHONPATH"] = os.pathsep.join([str(STUB_ROOT), str(MW_SRC), str(HARNESS_DIR)])
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
        [sys.executable, "-c", _IMPORT_PROBE, str(STUB_ROOT), *mods],
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


def run_unit_tests() -> list[str]:
    unit = MW_SRC / "middlewared" / "pytest" / "unit"
    cmd = [
        sys.executable, "-m", "pytest", "-q", "-p", "no:cacheprovider", "-p", "_pylicensed_harness",
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
            f"SKIP: no middleware clone at {CLONE.relative_to(REPO)} (run ./dev.py check-pylicensed-stub)"
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

    uses = collect(MW_SRC)
    log(
        f"static: {len(uses.files)} files, {len(uses.names)} imports, {len(uses.attrs)} attrs, "
        f"{len(uses.kwargs)} kwargs, status attrs {sorted(uses.status_attrs)}, "
        f"entry attrs {sorted(uses.entry_attrs)}, fingerprint keys {sorted(uses.fp_keys)} "
        f"optional {sorted(uses.fp_opt_keys)}"
    )
    errs = check_static(uses) + check_doc(uses)
    if not a.static_only:
        errs += check_imports(uses)
        errs += run_unit_tests()
    for e in errs:
        log(f"FAIL: {e}")
    log("PASS" if not errs else f"{len(errs)} failure(s)")
    return 1 if errs else 0


if __name__ == "__main__":
    sys.exit(main())
