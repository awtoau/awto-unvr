# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""scripts/usb-ssd.py: the guard against canned lsblk JSON, script rendering, verify parsing.

The scripts themselves run end to end on a loop device via `./dev.py usb-ssd rehearse`.
"""

from __future__ import annotations

import importlib.util
import os
import sys
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
_spec = importlib.util.spec_from_file_location(
    "usb_ssd", REPO / "scripts" / "usb-ssd.py"
)
us = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(us)

DEV = us.load_devices()


def disk(name, tran, model, serial, size, vendor="ATA", mounts=(), children=()):
    return {
        "name": name, "path": f"/dev/{name}", "tran": tran, "model": model, "vendor": vendor,
        "serial": serial, "size": size, "type": "disk", "mountpoints": list(mounts) or [None],
        "children": [{"name": f"{name}{i}", "type": "part", "mountpoints": [m]} for i, m in enumerate(children, 1)],
    }  # fmt: skip


SATA = [
    disk("sda", "sata", "WDC WD82PURZ-85TEUY0", "VGG8B0LG", 8001563222016),
    disk("sdb", "sata", "WDC WD82PURZ-85TEUY0", "VGG8ASZG", 8001563222016),
    disk("sdc", "sata", "ST6000VX001-2BD186", "WCT3LCS1", 6001175126016),
    disk(
        "sdd",
        "sata",
        "Samsung SSD 850 EVO 1TB",
        "S21BNXAG501763W",
        1000204886016,
        children=["/", None],
    ),
]
MICRON = disk("sde", "usb", "TFDDAK256TDL", "ABC123", 256060514304, vendor="Micron")


def test_known_usb_ssd_allowed():
    assert us.check_target(MICRON, DEV) == []
    tgt, errs = us.pick_target({"blockdevices": [*SATA, MICRON]}, DEV)
    assert errs == [] and tgt["name"] == "sde"


@pytest.mark.parametrize("d", SATA, ids=lambda d: d["name"])
def test_sata_disks_refused(d):
    assert any("not usb" in e for e in us.check_target(d, DEV))
    tgt, errs = us.pick_target({"blockdevices": SATA}, DEV, d["name"])
    assert tgt is None and errs


def test_no_usb_attached():
    tgt, errs = us.pick_target({"blockdevices": SATA}, DEV)
    assert tgt is None and "no USB-transport disk" in errs[0]


def test_sata_with_allowed_model_still_refused():
    d = disk("sdf", "sata", "TFDDAK256TDL", "X", 256060514304)
    assert us.check_target(d, DEV)


def test_unknown_usb_disk_refused():
    d = disk("sde", "usb", "SanDisk Ultra", "Z", 32_000_000_000)
    assert any("allow-list" in e for e in us.check_target(d, DEV))


def test_never_serial_refused_even_if_usb_and_model_match():
    d = {**MICRON, "serial": "S21BNXAG501763W"}
    assert any("never list" in e for e in us.check_target(d, DEV))


def test_oversize_and_mounted_refused():
    assert any(
        "size" in e for e in us.check_target({**MICRON, "size": 8_000_000_000_000}, DEV)
    )
    d = disk(
        "sde",
        "usb",
        "TFDDAK256TDL",
        "ABC",
        256060514304,
        vendor="Micron",
        children=["/mnt/x"],
    )
    assert any("mounted" in e for e in us.check_target(d, DEV))


def test_bridge_only_entry_inert_until_serial_pinned():
    d = disk("sde", "usb", "ASMT1051", "S1", 256060514304, vendor="ASMT")
    assert any("allow-list" in e for e in us.check_target(d, DEV))
    pinned = {"allow": [{**DEV["allow"][1], "serial": "S1"}], "never_serials": []}
    assert us.check_target(d, pinned) == []
    assert us.check_target({**d, "serial": "S2"}, pinned)


def test_two_allowed_disks_need_dev():
    other = {**MICRON, "name": "sdf", "serial": "DEF"}
    tgt, errs = us.pick_target({"blockdevices": [MICRON, other]}, DEV)
    assert tgt is None and "--dev" in errs[0]
    assert (
        us.pick_target({"blockdevices": [MICRON, other]}, DEV, "sdf")[0]["name"]
        == "sdf"
    )


def test_part_naming():
    assert us.part("/dev/sdb", 2) == "/dev/sdb2"
    assert us.part("/dev/loop3", 2) == "/dev/loop3p2"


def test_bootargs_root_replaced_profile_marked():
    cl = "console=ttyS0,115200 root=PARTUUID=old rootfstype=ext4 rw rootwait awto.profile=x"
    ba = us.usb_bootargs(cl, "new-uuid")
    assert (
        "root=PARTUUID=new-uuid" in ba
        and "root=PARTUUID=old" not in ba
        and "awto.profile" not in ba
    )
    assert us.uenv_text(ba).endswith(" awto.profile=usb\n")
    with pytest.raises(ValueError):
        us.usb_bootargs(" ".join(f"a{i}=1" for i in range(70)), "u")


def test_empty_initrd_is_512_newc_trailer():
    b = us.empty_initrd()
    assert len(b) == 512 and b.startswith(b"070701") and b"TRAILER!!!" in b


def test_prepare_script_guards_before_destruction():
    e = us.allow_entry(MICRON, DEV)
    s = us.render_prepare("/dev/sde", e, p2_uuid="u-1", uenv="x\n", initrd=b"i")
    assert (
        s.index("tran") < s.index("wipefs")
        if False
        else s.index("TRAN") < s.index("wipefs")
    )
    assert s.index("never list") < s.index("wipefs") < s.index("sfdisk")
    assert "S21BNXAG501763W" in s and "usb" in s
    assert "uuid=u-1" in s and "PARTUUID=u-1 / ext4" in s and "LABEL=unvr-usb" not in s
    assert "-L unvr-usb" in s  # distinct from the SATA root's label


GOOD = (
    """e2fsck 1.47
/dev/sde2: 11/100 files (0.0% non-contiguous), 20/400 blocks
PARTUUID aaaa-1
SFDISK_BEGIN
/dev/sde1 : start=2048, size=409600, type=C12A7328-F81F-11D2-BA4B-00A0C93EC93B
/dev/sde2 : start=411648, size=9, type=0FC63DAF-8483-4772-8E79-3D69D8477DE4, uuid=AAAA-1
SFDISK_END
UENV bootargs_alt=console=ttyS0 rw root=PARTUUID=aaaa-1 awto.profile=usb
FSTAB_BEGIN
PARTUUID=aaaa-1 / ext4 defaults,noatime 0 1
FSTAB_END
"""
    + "".join(
        f"{'0' * 64}  {f}\n" for f in ("uEnv.txt", "uImage", "unvr.dtb", "initrd")
    )
    + "OSREL Fedora\n"
)


def test_verify_good_and_each_failure():
    assert us.check_verify(us.parse_verify(GOOD)) == []
    for bad, why in (
        (GOOD.replace("root=PARTUUID=aaaa-1", "root=PARTUUID=dcdc"), "uEnv root"),
        (GOOD.replace("PARTUUID=aaaa-1 / ext4", "LABEL=unvr-root / ext4"), "fstab"),
        (GOOD.replace("blocks\n", "\n"), "e2fsck"),
        (GOOD.replace("  initrd\n", "  other\n"), "initrd"),
        (GOOD.replace("OSREL Fedora\n", ""), "os-release"),
    ):
        errs = us.check_verify(us.parse_verify(bad))
        assert errs and any(why in e for e in errs), (why, errs)
