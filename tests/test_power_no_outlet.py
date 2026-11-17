"""woomera has no smart outlet: power helpers must refuse, never report a false success (#339)."""

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "scripts"))
import _power  # noqa: E402


def test_cycle_refuses():
    with pytest.raises(_power.NoOutletError):
        _power.power_cycle_verified(log=lambda *_: None)


def test_set_refuses():
    with pytest.raises(_power.NoOutletError):
        _power.power_set_verified(True, log=lambda *_: None)
