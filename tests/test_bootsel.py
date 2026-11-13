# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Boot selection (#338): U-Boot env format, defconfig bootcmd, uImage/profile checks.

The bootcmd's behaviour is exercised in U-Boot sandbox by `./dev.py uboot-sandbox`.
"""

from __future__ import annotations

import os
import struct
import sys
import zlib
from pathlib import Path

import pytest

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")

import _bootsel as bs  # noqa: E402
import uboot_env as ue  # noqa: E402


def test_env_roundtrip_sorted_zero_fill():
    env = {"b": "2", "a": "x=y z", "bootcmd": "if a; then b; fi"}
    blob = ue.serialize(env)
    assert len(blob) == ue.ENV_SIZE
    assert blob[4:].startswith(b"a=x=y z\0b=2\0bootcmd=if a; then b; fi\0\0")
    assert set(blob[4 + 40 :]) == {0}
    ok, back = ue.parse(blob)
    assert ok and back == env


def test_env_bad_crc_detected():
    blob = bytearray(ue.serialize({"a": "1"}))
    blob[10] ^= 1
    assert ue.parse(bytes(blob))[0] is False


def test_env_apply_set_delete_and_reject():
    env = {"a": "1", "bootonce": "truenas"}
    assert ue.apply(env, ["bootonce=", "b=2"]) == {"a": "1", "b": "2"}
    for bad in ("noequals", "=v", "a b=1"):
        with pytest.raises(ValueError):
            ue.apply(env, [bad])


def test_env_too_big():
    with pytest.raises(ValueError):
        ue.serialize({"x": "y" * ue.ENV_SIZE})


def test_defconfig_bootcmd_shape():
    c = bs.defconfig_bootcmd()
    # one-shot cleared and saved BEFORE anything is loaded or booted
    assert c.index("setenv bootonce; saveenv") < c.index("scsi scan") < c.index("bootm")
    # never a filesystem write from U-Boot (#347)
    for w in ("ext4write", "fatwrite", "save ", "ext4ls"):
        assert w not in c
    # only bootargs_alt may come in from uEnv.txt
    assert "env import -t 0x0f000000 ${filesize} bootargs_alt &&" in c
    # the unchanged Fedora path is the tail
    assert c.endswith(
        "for d in 0 1 2 3; do if ext4load scsi ${d}:2 0x02000000 /boot/uImage && ext4load scsi ${d}:2 "
        "0x04078000 /boot/unvr.dtb; then echo == booting from scsi ${d}:2 ==; bootm 0x02000000 - "
        "0x04078000; fi; done; echo == no /boot/uImage found on any scsi device ==; fi"
    )


def _uimage(load: int, comp: int, n: int = 4096) -> bytes:
    data = b"\1" * n
    hdr = struct.pack(
        ">IIIIIIIBBBB32s",
        0x27051956,
        0,
        0,
        n,
        load,
        load,
        zlib.crc32(data),
        5,
        22,
        2,
        comp,
        b"t",
    )
    return hdr[:4] + struct.pack(">I", zlib.crc32(hdr)) + hdr[8:] + data


def test_uimage_checks():
    assert bs.uimage_info(_uimage(0x08000000, 0))["load"] == 0x08000000
    assert bs.uimage_info(_uimage(0x08000000, 1))["comp"] == 1
    with pytest.raises(ValueError, match="dtb slot"):
        bs.uimage_info(
            _uimage(0x0D000000, 1)
        )  # gzip out to BOOTM_LEN reaches 0x0f000000
    with pytest.raises(ValueError, match="own source"):
        bs.uimage_info(_uimage(0x02000000, 1))
    bad = bytearray(_uimage(0x08000000, 0))
    bad[100] ^= 1
    with pytest.raises(ValueError, match="CRC"):
        bs.uimage_info(bytes(bad))


@pytest.mark.parametrize("name", ["fedora", "Fedora", "a/b", "", "x" * 40])
def test_profile_names_rejected(name):
    with pytest.raises(ValueError):
        bs.check_profile_name(name)
