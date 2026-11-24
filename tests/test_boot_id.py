"""boot-id: reboot vs hang vs same boot (#243)."""

import importlib.util
from pathlib import Path

_spec = importlib.util.spec_from_file_location(
    "bid", Path(__file__).resolve().parent.parent / "scripts" / "boot-id.py"
)
bid = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(bid)

OLD = {"boot_id": "aaa", "uptime_s": 1000.0}


def test_same_boot():
    assert bid.compare(OLD, {"boot_id": "aaa", "uptime_s": 1500.0})[0] == 0


def test_new_boot_id_is_a_reboot():
    assert bid.compare(OLD, {"boot_id": "bbb", "uptime_s": 20.0})[0] == 1


def test_uptime_going_backwards_is_a_reboot():
    assert bid.compare(OLD, {"boot_id": "aaa", "uptime_s": 10.0})[0] == 1


def test_unreachable_is_inconclusive():
    code, msg = bid.compare(OLD, None)
    assert code == 2 and "console" in msg
