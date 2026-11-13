#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""awto-uboot's saved env (NOR mtd "Factory") from Linux - runs ON the box, stdlib only.

- Replaces fw_setenv: works on Fedora and TrueNAS alike (u-boot-tools absent there), and
  refuses to write over an env whose CRC is bad (a bad CRC = U-Boot uses compiled defaults).
- Format (CONFIG_ENV_SIZE 0x10000, not redundant): le32 CRC32 of the rest, then
  sorted `k=v\\0` entries, `\\0` terminator, zero fill - what U-Boot's saveenv writes.
- Write = MEMERASE whole partition, write, read back, byte-compare.
- Host side pipes it: `ssh box python3 - <args> < scripts/uboot_env.py` (scripts/_bootsel.py).

  python3 uboot_env.py dump                  # JSON {mtd, crc_ok, env}
  python3 uboot_env.py set k=v [k= ...]      # k= deletes k
"""

from __future__ import annotations

import fcntl
import json
import os
import struct
import sys
import zlib

LABEL = "Factory"
ENV_SIZE = 0x10000
MEMERASE = 0x40084D02  # _IOW('M', 2, struct erase_info_user {u32 start, length})


def parse(blob: bytes) -> tuple[bool, dict[str, str]]:
    (crc,) = struct.unpack("<I", blob[:4])
    data = blob[4:]
    ok = zlib.crc32(data) & 0xFFFFFFFF == crc
    env: dict[str, str] = {}
    for item in data.split(b"\0"):
        if not item:
            break
        k, _, v = item.decode("utf-8", "replace").partition("=")
        env[k] = v
    return ok, env


def serialize(env: dict[str, str], size: int = ENV_SIZE) -> bytes:
    body = b"".join(f"{k}={env[k]}".encode() + b"\0" for k in sorted(env)) + b"\0"
    if len(body) > size - 4:
        raise ValueError(f"env {len(body)} B > {size - 4} B")
    data = body.ljust(size - 4, b"\0")
    return struct.pack("<I", zlib.crc32(data) & 0xFFFFFFFF) + data


def apply(env: dict[str, str], assigns: list[str]) -> dict[str, str]:
    out = dict(env)
    for a in assigns:
        k, eq, v = a.partition("=")
        if not eq or not k or any(c in k for c in "=\0 "):
            raise ValueError(f"bad assignment {a!r} (want k=v, or k= to delete)")
        if v:
            out[k] = v
        else:
            out.pop(k, None)
    return out


def find_mtd() -> str:
    with open("/proc/mtd") as f:
        for line in f.read().splitlines()[1:]:
            dev, size, _erase, name = line.split(None, 3)
            if name.strip('"') == LABEL:
                if int(size, 16) != ENV_SIZE:
                    raise RuntimeError(f"{LABEL} is {size}, want {ENV_SIZE:#x}")
                return "/dev/" + dev.rstrip(":")
    raise RuntimeError(f"no mtd labelled {LABEL!r} (spi-nor driver not loaded?)")


def read(dev: str) -> bytes:
    with open(dev, "rb") as f:
        blob = f.read(ENV_SIZE)
    if len(blob) != ENV_SIZE:
        raise RuntimeError(f"short read {len(blob)} from {dev}")
    return blob


def write(dev: str, blob: bytes) -> None:
    fd = os.open(dev, os.O_RDWR | os.O_SYNC)
    try:
        fcntl.ioctl(fd, MEMERASE, struct.pack("II", 0, ENV_SIZE))
        if os.pwrite(fd, blob, 0) != ENV_SIZE:
            raise RuntimeError("short write")
    finally:
        os.close(fd)
    if read(dev) != blob:
        raise RuntimeError(f"read-back of {dev} differs from what was written")


def main(argv: list[str]) -> int:
    dev = find_mtd()
    ok, env = parse(read(dev))
    if not argv or argv[0] == "dump":
        print(json.dumps({"mtd": dev, "crc_ok": ok, "env": env}))
        return 0
    if argv[0] != "set":
        print(f"usage: {sys.argv[0]} dump | set k=v [k= ...]", file=sys.stderr)
        return 2
    if not ok:
        print(
            f"refusing: {dev} CRC bad (U-Boot is on compiled defaults)", file=sys.stderr
        )
        return 1
    new = apply(env, argv[1:])
    if new != env:
        write(dev, serialize(new))
    ok2, env2 = parse(read(dev))
    print(json.dumps({"mtd": dev, "crc_ok": ok2, "env": env2, "changed": new != env}))
    return 0 if ok2 and env2 == new else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
