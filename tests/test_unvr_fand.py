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


def _hwmon(root, n, name, temp, usb=False):
    d = root / f"hwmon{n}"
    d.mkdir()
    (d / "name").write_text(name + "\n")
    (d / "temp1_input").write_text(f"{temp}\n")
    dev = root / f"dev{n}" / ("usb1" if usb else "ata1")
    dev.mkdir(parents=True)
    (d / "device").symlink_to(dev)


def test_drivetemp_reads_internal_drives_only(tmp_path):
    _hwmon(tmp_path, 0, "adt7475", 1000)
    _hwmon(tmp_path, 1, "drivetemp", 58000)
    _hwmon(tmp_path, 2, "drivetemp", 59500)
    _hwmon(tmp_path, 3, "drivetemp", 30000, usb=True)
    assert fand.drivetemp_temps(str(tmp_path)) == [58.0, 59.5]


def test_drivetemp_absent_returns_empty(tmp_path):
    _hwmon(tmp_path, 0, "adt7475", 1000)
    assert fand.drivetemp_temps(str(tmp_path)) == []
