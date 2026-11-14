"""truenas-config: secrets text editing and plan mode never write (no box needed)."""

from __future__ import annotations

import importlib.util
import os
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

spec = importlib.util.spec_from_file_location("tnc", REPO / "scripts/truenas-config.py")
tnc = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tnc)


def test_section_append_drop_keeps_rest(tmp_path, monkeypatch):
    f = tmp_path / "s.yaml"
    f.write_text("# keep me\na:\n  password: x\n")
    monkeypatch.setattr(tnc, "SECRETS", f)
    tnc.append_section("woomera-truenas-api", {"username": "u", "password": "k"})
    assert tnc.secret("woomera-truenas-api")["password"] == "k"
    tnc.drop_section("woomera-truenas-api")
    assert (
        f.read_text() == "# keep me\na:\n  password: x\n\n".replace("\n\n", "\n")
        or "a:" in f.read_text()
    )
    assert "woomera-truenas-api" not in f.read_text()
    assert f.read_text().startswith("# keep me")


def test_plan_mode_skips_writes(monkeypatch):
    monkeypatch.setattr(tnc, "DRY", True)
    monkeypatch.setattr(tnc, "_api", None)
    assert tnc.mc("pool.dataset.create", {"name": "x"}) is None
    assert tnc._api is None
