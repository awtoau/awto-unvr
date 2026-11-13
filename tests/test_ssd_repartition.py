# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""scripts/ssd-repartition.py: geometry, conf, sfdisk parsing, cpio, uImage checks.

The /init itself is rehearsed end to end by `./dev.py ssd-repartition rehearse`.
Run: pytest tests/test_ssd_repartition.py -q
"""

from __future__ import annotations

import gzip
import importlib.util
import os
import shutil
import stat
import struct
import subprocess
import sys
import zlib
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

_spec = importlib.util.spec_from_file_location(
    "ssd_repartition", REPO / "scripts" / "ssd-repartition.py"
)
sr = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(sr)

# sfdisk -d /dev/sdb on woomera, 2026-11-13.
REAL_DUMP = """label: gpt
label-id: 9901C3BC-211F-42D9-9E94-D94DB2DDB8DB
device: /dev/sdb
unit: sectors
first-lba: 34
last-lba: 1953525134
sector-size: 512

/dev/sdb1 : start=          40, size=      409600, type=C12A7328-F81F-11D2-BA4B-00A0C93EC93B, uuid=79CD524F-DEEA-43FE-BA09-1C0DCCF82A1F, name="EFI System Partition"
/dev/sdb2 : start=      409640, size=  1953115488, type=7C3457EF-0000-11AA-AA11-00306543ECAC, uuid=DCDC291E-9956-48CD-9D7C-48219877881A
"""


@pytest.mark.parametrize("g", [sr.REAL, sr.REHEARSE])
def test_geometry(g):
    new = g.p2_new_sectors
    assert (g.p2_start + new) % sr.ALIGN == 0
    assert new * sr.SECTOR >= g.fs_target_bytes + sr.MARGIN_SECTORS * sr.SECTOR
    assert new * sr.SECTOR < g.fs_target_bytes + sr.MARGIN_SECTORS * sr.SECTOR + (
        1 << 20
    )
    assert new < g.p2_sectors


def test_real_numbers():
    assert sr.REAL.fs_target_bytes == 800 * (1 << 30)
    assert sr.REAL.p2_new_sectors == 1678264320 - 409640


def test_dump_before_and_after():
    d = sr.parse_dump(REAL_DUMP)
    assert d["label_id"] == "9901C3BC-211F-42D9-9E94-D94DB2DDB8DB"
    assert sr.check_table(d, sr.REAL, after=False) == []
    assert sr.check_table(d, sr.REAL, after=True)  # no p3, old size
    new = sr.REAL.p2_new_sectors
    p3s = sr.REAL.p2_start + new
    after = REAL_DUMP.replace("size=  1953115488", f"size={new}") + (
        f"/dev/sdb3 : start={p3s}, size={1953525134 - p3s + 1 - 2047}, "
        f'type={sr.P3_TYPE}, uuid=11111111-2222-3333-4444-555555555555, name="truenas-boot"\n'
    )
    assert sr.check_table(sr.parse_dump(after), sr.REAL, after=True) == []
    moved = after.replace("start=      409640", "start=      411648")
    assert any(
        "p2 start" in e
        for e in sr.check_table(sr.parse_dump(moved), sr.REAL, after=True)
    )


def test_conf_is_bash():
    conf = sr.render_conf(sr.REAL, "real", P3_NAME="truenas-boot")
    out = subprocess.run(
        ["bash", "-c", '. /dev/stdin; echo "$MODE $P2_NEW_SECTORS $SSD_MODEL"'],
        input=conf,
        capture_output=True,
        text=True,
        check=True,
    ).stdout.split(maxsplit=2)
    assert out == ["real", str(sr.REAL.p2_new_sectors), "Samsung SSD 850\n"]


def test_cpio_roundtrip():
    files = {
        "dev": (stat.S_IFDIR | 0o755, b""),
        "dev/console": (stat.S_IFCHR | 0o600, (5, 1)),
        "bin": (stat.S_IFLNK | 0o777, "usr/bin"),
        "init": (stat.S_IFREG | 0o755, b"#!/usr/bin/bash\n"),
    }
    blob = sr.build_cpio(files)
    back = sr.read_cpio(blob)
    assert back["init"] == (stat.S_IFREG | 0o755, b"#!/usr/bin/bash\n")
    assert back["bin"][1] == b"usr/bin"
    if shutil.which("cpio"):
        names = subprocess.run(
            ["cpio", "-it", "--quiet"], input=blob, capture_output=True, check=True
        ).stdout.split()
        assert sorted(names) == [b"bin", b"dev", b"dev/console", b"init"]


def _uimage(raw: bytes, name: bytes = b"unvr-ea16-7.4-maint") -> bytes:
    data = gzip.compress(raw)

    def hdr(hc):
        return struct.pack(
            ">IIIIIIIBBBB32s",
            0x27051956,
            hc,
            0,
            len(data),
            sr.UIMAGE_LOAD,
            sr.UIMAGE_LOAD,
            zlib.crc32(data),
            5,
            22,
            2,
            1,
            name.ljust(32, b"\0"),
        )

    return hdr(zlib.crc32(hdr(0))) + data


def test_uimage_check():
    assert sr.uimage_check(_uimage(b"x" * 1000)) == ("unvr-ea16-7.4-maint", 1000)
    with pytest.raises(ValueError, match="BOOTM_LEN"):
        sr.uimage_check(_uimage(b"\0" * (sr.IMAGE_MAX + 1)))
    bad = bytearray(_uimage(b"x" * 1000))
    bad[-1] ^= 1
    with pytest.raises(ValueError, match="CRC"):
        sr.uimage_check(bytes(bad))


def test_init_syntax():
    subprocess.run(["bash", "-n", str(sr.INIT_SRC)], check=True)


def _fake_ssh(monkeypatch, sums: dict[str, str], calls: list[str]):
    def ssh(_ip, cmd, **_kw):
        calls.append(cmd)
        if cmd.startswith("sha256sum"):
            return "".join(f"{h}  {p}\n" for p, h in sums.items())
        if cmd.startswith("cp -p /boot/uImage.normal /boot/uImage"):
            sums["/boot/uImage"] = sums["/boot/uImage.normal"]
        return ""

    monkeypatch.setattr(sr, "ssh", ssh)


@pytest.mark.parametrize(
    ("uimage", "normal", "copies"),
    [
        ("a" * 64, None, False),  # install failed early: untouched
        ("b" * 64, "a" * 64, True),  # maint installed, verified copy exists
        ("b" * 64, "c" * 64, False),  # copy is not the original: never use it
    ],
)
def test_rollback_only_from_verified_copy(monkeypatch, uimage, normal, copies):
    sums = {"/boot/uImage": uimage}
    if normal:
        sums["/boot/uImage.normal"] = normal
    calls: list[str] = []
    _fake_ssh(monkeypatch, sums, calls)
    sr.rollback("192.0.2.1", "a" * 64)
    assert any(c.startswith("cp -p") for c in calls) == copies
