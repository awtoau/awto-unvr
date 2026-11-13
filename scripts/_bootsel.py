# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Boot-profile selection on woomera (#338): host side of awto-uboot's bootonce/bootdefault.

- Env access = scripts/uboot_env.py piped to the box's python3 (Fedora or TrueNAS).
- Profiles live on p2 at /boot/awto/<name>/{uEnv.txt,uImage,unvr.dtb,initrd}; U-Boot only reads them.
- Load map (CONFIG_BOOTCOMMAND): uImage @0x02000000, uEnv.txt + dtb @0x0f000000, initrd @0x20000000.
"""

from __future__ import annotations

import hashlib
import json
import re
import shlex
import struct
import subprocess
import time
import zlib
from pathlib import Path

import _box
from _repo import REPO

ENV_HELPER = REPO / "scripts" / "uboot_env.py"
DEFCONFIG = REPO / "uboot-port" / "configs" / "alpine_v2_unvr_defconfig"
PROFILE_DIR = "/boot/awto"
PROFILE_RE = re.compile(r"^[a-z0-9][a-z0-9-]{0,31}$")

UIMAGE_LOAD, ALT_LOAD, INITRD_LOAD = 0x02000000, 0x0F000000, 0x20000000
UIMAGE_MAX = ALT_LOAD - UIMAGE_LOAD  # loaded uImage must end below the dtb/uEnv slot
INITRD_MAX = 0x10000000  # 256 MiB: far above any initramfs here, far below bank-0 top
BOOTM_LEN = 0x4000000  # CONFIG_SYS_BOOTM_LEN: max decompressed kernel
DTB_MAX = 0x100000
UENV_MAX = 0x1000

# One ssh exec: ~1 s on the LAN (ssd-repartition.py). Env write = 64 KiB NOR erase +
# program + read-back, < 2 s. 30 s backstop; expiry -> TimeoutExpired up to the CLI.
SSH_TIMEOUT_S = 30
# scp of a <= 256 MiB profile file over 1G: ~3 s at line rate. 60 s backstop.
SCP_TIMEOUT_S = 60


def ssh(
    ip: str, cmd: str, *, stdin: bytes | None = None, timeout: float = SSH_TIMEOUT_S
) -> str:
    r = subprocess.run(
        _box.ssh_argv(ip, cmd=[cmd], batch=True),
        input=stdin,
        capture_output=True,
        timeout=timeout,
        check=False,
    )
    if r.returncode:
        raise RuntimeError(
            f"ssh rc={r.returncode}: {cmd}\n{r.stderr.decode(errors='replace').strip()}"
        )
    return r.stdout.decode(errors="replace")


def env_call(ip: str, *args: str) -> dict:
    out = ssh(
        ip,
        "python3 - " + " ".join(shlex.quote(a) for a in args),
        stdin=ENV_HELPER.read_bytes(),
    )
    return json.loads(out.strip().splitlines()[-1])


def defconfig_bootcmd() -> str:
    m = re.search(r'^CONFIG_BOOTCOMMAND="(.*)"$', DEFCONFIG.read_text(), re.M)
    if not m or "\\" in m.group(1) or '"' in m.group(1):
        raise RuntimeError(f"CONFIG_BOOTCOMMAND missing or has escapes in {DEFCONFIG}")
    return m.group(1)


def check_profile_name(name: str) -> None:
    if not PROFILE_RE.match(name) or name == "fedora":
        raise ValueError(f"bad profile name {name!r}")


def uimage_info(blob: bytes) -> dict:
    """Header + CRC check of a legacy uImage; refuses what would overlap the dtb slot."""
    hdr = blob[:64]
    magic, hcrc, _t, size, load, ep, dcrc = struct.unpack(">IIIIIII", hdr[:28])
    if magic != 0x27051956:
        raise ValueError("bad uImage magic")
    if zlib.crc32(hdr[:4] + b"\0" * 4 + hdr[8:]) & 0xFFFFFFFF != hcrc:
        raise ValueError("bad uImage header CRC")
    if len(blob) - 64 != size or zlib.crc32(blob[64:]) & 0xFFFFFFFF != dcrc:
        raise ValueError("bad uImage data size/CRC")
    if len(blob) > UIMAGE_MAX:
        raise ValueError(
            f"uImage {len(blob)} B > {UIMAGE_MAX} B (overlaps the dtb slot)"
        )
    # bootm writes [load, load + out) before it relocates the dtb: must stay below ALT_LOAD.
    # Uncompressed = memmove (overlap with its source is fine); gzip must not overlap it.
    out = size if hdr[31] == 0 else BOOTM_LEN
    if load + out > ALT_LOAD:
        raise ValueError(
            f"uImage load {load:#x} + {out:#x} reaches the dtb slot {ALT_LOAD:#x}"
        )
    if hdr[31] != 0 and load < UIMAGE_LOAD + len(blob) and load + out > UIMAGE_LOAD:
        raise ValueError(f"compressed uImage load {load:#x} overlaps its own source")
    return {
        "name": hdr[32:].rstrip(b"\0").decode(),
        "load": load,
        "ep": ep,
        "comp": hdr[31],
        "size": size,
    }


def sha256(b: bytes) -> str:
    return hashlib.sha256(b).hexdigest()


def stage_profile(
    ip: str, name: str, *, uimage: Path, dtb: Path, initrd: Path, bootargs: str, log
) -> None:
    """Copy one profile into /boot/awto/<name>/ via a .new dir; sha256-verified; then swap."""
    check_profile_name(name)
    files = {
        "uImage": uimage.read_bytes(),
        "unvr.dtb": dtb.read_bytes(),
        "initrd": initrd.read_bytes(),
    }
    if "\n" in bootargs or not bootargs.strip():
        raise ValueError("bootargs must be one non-empty line")
    # `setenv bootargs ${bootargs_alt}` splits on spaces: CONFIG_SYS_MAXARGS 64 incl. setenv + name.
    if len(bootargs.split()) + 1 > 62:
        raise ValueError("bootargs: too many words for one setenv (SYS_MAXARGS 64)")
    files["uEnv.txt"] = f"bootargs_alt={bootargs} awto.profile={name}\n".encode()
    info = uimage_info(files["uImage"])
    for f, lim in (
        ("unvr.dtb", DTB_MAX),
        ("initrd", INITRD_MAX),
        ("uEnv.txt", UENV_MAX),
    ):
        if len(files[f]) > lim:
            raise ValueError(f"{f} {len(files[f])} B > {lim} B")
    if files["unvr.dtb"][:4] != b"\xd0\x0d\xfe\xed":
        raise ValueError("dtb: bad FDT magic")
    log(
        f"stage {name}: uImage '{info['name']}' {len(files['uImage'])} B, initrd {len(files['initrd'])} B"
    )
    d, new = f"{PROFILE_DIR}/{name}", f"{PROFILE_DIR}/{name}.new"
    ssh(ip, f"rm -rf {new} && mkdir -p {new}")
    for f, data in files.items():
        tmp = REPO / "tmp" / "bootsel" / f"{name}-{f}"
        tmp.parent.mkdir(parents=True, exist_ok=True)
        tmp.write_bytes(data)
        subprocess.run(
            ["scp", "-q", *_box.SSH_OPTS_BATCH, str(tmp), f"root@{ip}:{new}/{f}"],
            check=True,
            timeout=SCP_TIMEOUT_S,
        )
    out = ssh(ip, f"cd {new} && sync && sha256sum " + " ".join(files))
    got = {p: h for h, p in re.findall(r"^([0-9a-f]{64})  (\S+)$", out, re.M)}
    want = {f: sha256(b) for f, b in files.items()}
    if got != want:
        raise RuntimeError(f"profile {name}: sha256 mismatch after copy\n{out}")
    ssh(
        ip,
        f"rm -rf {d}.old; if [ -d {d} ]; then mv {d} {d}.old; fi; mv {new} {d} && rm -rf {d}.old && sync",
    )
    log(
        f"stage {name}: {d} verified ({', '.join(f'{f} {h[:12]}' for f, h in want.items())})"
    )


def profile_present(ip: str, name: str) -> bool:
    d = f"{PROFILE_DIR}/{name}"
    return (
        ssh(
            ip,
            f"for f in uEnv.txt uImage unvr.dtb initrd; do [ -s {d}/$f ] || echo missing $f; done",
        )
        == ""
    )


def wait_new_boot(
    old_boot_id: str, limit_s: float, log, poll_s: float = 3.0
) -> tuple[str, str] | None:
    """Poll until ssh answers with a different boot_id; (ip, uname -r) or None after limit_s."""
    t0, last = time.monotonic(), ""
    while (el := time.monotonic() - t0) < limit_s:
        ip = _box.locate()
        if ip:
            try:
                k, bid = _box.boot_state(ip)
                if bid != old_boot_id:
                    log(f"back after {el:.0f}s: {k} at {ip}")
                    return ip, k
            except RuntimeError as e:
                last = str(e)
        time.sleep(poll_s)
    log(f"TIMEOUT: no new boot within {limit_s:.0f}s; last ssh error: {last}")
    return None


def reboot(ip: str, log) -> str:
    """systemctl reboot; returns the pre-reboot boot_id."""
    _, bid = _box.boot_state(ip)
    try:
        # The session drops as the box goes down: rc is meaningless. 30 s backstop.
        subprocess.run(
            _box.ssh_argv(ip, cmd=["systemctl reboot"], batch=True),
            capture_output=True,
            timeout=30,
            check=False,
        )
    except subprocess.TimeoutExpired:
        log("systemctl reboot: ssh did not return in 30 s (going down?) - polling")
    log("reboot issued")
    return bid
