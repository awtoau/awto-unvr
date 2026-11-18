"""file-hash-cache: staleness, lazy scan, settle window, heartbeat (#378)."""

import hashlib
import importlib.util
import json
import os
import time
from pathlib import Path

import pytest

_spec = importlib.util.spec_from_file_location(
    "fhc", Path(__file__).resolve().parent.parent / "scripts" / "file-hash-cache.py"
)
fhc = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(fhc)


def _xattr_ok(d: Path) -> bool:
    p = d / ".probe"
    p.write_text("x")
    try:
        os.setxattr(p, "user.t", b"1")
        return True
    except OSError:
        return False
    finally:
        p.unlink()


@pytest.fixture
def tree(tmp_path):
    if not _xattr_ok(tmp_path):
        pytest.skip("filesystem has no user xattrs")
    root = tmp_path / "data"
    root.mkdir()
    return root


def _old(path: Path, age_s: int = 3600) -> None:
    t = time.time() - age_s
    os.utime(path, (t, t))


def test_scan_hashes_then_skips_fresh(tree, tmp_path):
    f = tree / "a.flac"
    f.write_bytes(b"hello")
    _old(f)
    st = tmp_path / "state"
    hb = fhc.scan(str(tree), "t", 60, st)
    assert hb["hashed"] == 1 and hb["complete"]
    assert fhc.read_attrs(str(f))["sha256"] == hashlib.sha256(b"hello").hexdigest()
    hb = fhc.scan(str(tree), "t", 60, st)
    assert hb["hashed"] == 0 and hb["fresh"] == 1


def test_changed_file_is_rehashed(tree, tmp_path):
    f = tree / "a"
    f.write_bytes(b"one")
    _old(f)
    fhc.scan(str(tree), "t", 60, tmp_path / "s")
    f.write_bytes(b"three")
    _old(f, 1800)
    hb = fhc.scan(str(tree), "t", 60, tmp_path / "s")
    assert hb["hashed"] == 1
    assert fhc.read_attrs(str(f))["sha256"] == hashlib.sha256(b"three").hexdigest()


def test_recent_file_is_settling_not_hashed(tree, tmp_path):
    f = tree / "new"
    f.write_bytes(b"still writing")
    hb = fhc.scan(str(tree), "t", 60, tmp_path / "s")
    assert hb["settling"] == 1 and hb["hashed"] == 0
    assert fhc.read_attrs(str(f)) is None


def test_budget_leaves_backlog_and_resumes(tree, tmp_path):
    for i in range(3):
        f = tree / f"f{i}"
        f.write_bytes(b"x" * (i + 1))
        _old(f, 7200)
    hb = fhc.scan(str(tree), "t", -1, tmp_path / "s")  # budget already spent
    assert hb["hashed"] == 0 and hb["backlog"] == 3 and not hb["complete"]
    assert hb["oldest_backlog_age_s"] >= 7000
    hb = fhc.scan(str(tree), "t", 60, tmp_path / "s")
    assert hb["hashed"] == 3 and hb["backlog"] == 0 and hb["complete"]


def test_skip_dirs_and_heartbeat_file(tree, tmp_path):
    (tree / "#recycle").mkdir()
    r = tree / "#recycle" / "x"
    r.write_bytes(b"x")
    _old(r)
    fhc.scan(str(tree), "t", 60, tmp_path / "s")
    assert fhc.read_attrs(str(r)) is None
    assert json.loads((tmp_path / "s" / "t.json").read_text())["name"] == "t"


def test_heartbeat_stall_detection():
    now = 1_000_000.0
    hb = {
        "finished": now - 100,
        "started": now - 110,
        "interval_s": 900,
        "complete": True,
        "hashed": 0,
        "fresh": 0,
        "settling": 0,
        "errors": 0,
        "backlog": 0,
        "oldest_backlog_age_s": 0,
    }
    assert "STALLED" not in fhc.describe_heartbeat(hb, now)
    hb["finished"] = now - 1801
    assert fhc.describe_heartbeat(hb, now).startswith("STALLED")
    assert "never ran" in fhc.describe_heartbeat(None, now)
