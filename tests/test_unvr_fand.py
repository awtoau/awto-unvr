"""unvr-fand curve / ramp logic (pure functions)."""

import importlib.util
from pathlib import Path

_p = Path(__file__).resolve().parent.parent / "scripts" / "unvr-fand.py"
_s = importlib.util.spec_from_file_location("unvr_fand", _p)
fand = importlib.util.module_from_spec(_s)
_s.loader.exec_module(fand)


def test_never_below_floor():
    assert fand.curve("drive", -20) == fand.FLOOR_PWM
    assert fand.target_pwm({"drive": [0], "soc": [20], "board": [20]}) == fand.FLOOR_PWM


def test_full_at_and_above_top():
    assert fand.curve("drive", 55) == 255
    assert fand.curve("drive", 66) == 255
    assert fand.curve("soc", 85) == 255


def test_ramp_monotonic():
    v = [fand.curve("drive", t) for t in range(30, 70)]
    assert v == sorted(v)
    assert 90 < fand.curve("drive", 47) < 255


def test_hottest_source_wins():
    r = {"drive": [41, 58], "soc": [50], "board": [45]}
    assert fand.target_pwm(r) == 255


def test_unreadable_fails_safe():
    assert fand.target_pwm({"drive": [None], "soc": [50], "board": [40]}) == 255
    assert fand.target_pwm({"drive": [30], "soc": [None], "board": [40]}) == 255


def test_up_immediate_down_slow_with_deadband():
    assert fand.next_pwm(100, 200) == 200
    assert fand.next_pwm(200, 197) == 200  # inside deadband: hold
    assert fand.next_pwm(200, 100) == 200 - fand.DOWN_STEP
    assert fand.next_pwm(None, 90) == 90
