"""unvr-fand policy (pure function): fans off unless SoC >= 80 C, off again at <= 50 C."""

import importlib.util
from pathlib import Path

_spec = importlib.util.spec_from_file_location(
    "unvr_fand", Path(__file__).resolve().parent.parent / "scripts" / "unvr-fand.py"
)
fand = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(fand)


def test_off_when_cool():
    assert fand.target_pwm(40, False) == (0, False)
    assert fand.target_pwm(79, False) == (0, False)


def test_full_at_threshold():
    assert fand.target_pwm(80, False) == (255, True)
    assert fand.target_pwm(95, False) == (255, True)


def test_hysteresis_holds_until_50():
    assert fand.target_pwm(70, True) == (255, True)
    assert fand.target_pwm(51, True) == (255, True)
    assert fand.target_pwm(50, True) == (0, False)


def test_unreadable_soc_fails_safe():
    assert fand.target_pwm(None, False) == (255, True)
