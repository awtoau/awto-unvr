#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Run the defconfig CONFIG_BOOTCOMMAND in U-Boot sandbox before it goes into the box's env.

- No serial console on woomera: a bootcmd that fails to parse parks the box at awto-nas#
  with no way back. This runs the EXACT string (old hush, same commands) on the host.
- Scenarios: profile via bootonce / bootdefault, fedora, missing profile, canary, no disks.
  `scsi` -> `host` (a GPT disk image with ext4 p2) for the ones that need files; the
  exact string runs unmodified in the no-disk scenarios.
- Sandbox build: tmp/uboot-sandbox (UBOOT_TREE, sandbox_defconfig + 1 GiB RAM, env nowhere).

  ./dev.py uboot-sandbox
"""

from __future__ import annotations

import os
import struct
import subprocess
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _bootsel as bs  # noqa: E402

from _repo import TMP, make_log  # noqa: E402

log = make_log("bootcmd-sandbox")
TREE = Path(os.environ.get("UBOOT_TREE", "/mnt/2tb/unvr-port-refs/u-boot-v2026.07"))
BUILD = TMP / "uboot-sandbox"
WORK = TMP / "bootcmd-sandbox"
UBOOT = BUILD / "u-boot"
# Sandbox build from scratch: ~1 min at -j24 here. Stall-free make; 600 s backstop.
BUILD_TIMEOUT_S = 600
# One sandbox run: < 1 s. 30 s backstop; expiry -> TimeoutExpired, scenario fails.
RUN_TIMEOUT_S = 30
BOOTARGS = "console=ttyS0,115200 root=PARTUUID=feed rw"
ALT_ARGS = "console=ttyS0,115200 root=ZFS=boot-pool/ROOT/x ro"


def build() -> None:
    if UBOOT.exists():
        return
    cfg = BUILD / ".config"
    mk = ["make", f"O={BUILD}", "NO_SDL=1"]
    subprocess.run(
        [*mk, "sandbox_defconfig"], cwd=TREE, check=True, timeout=BUILD_TIMEOUT_S
    )
    subprocess.run(
        [
            "./scripts/config",
            "--file",
            str(cfg),
            "--set-val",
            "SANDBOX_RAM_SIZE_MB",
            "1024",
        ]
        + [
            x
            for s in (
                "TOOLS_LIBCRYPTO",
                "FIT_SIGNATURE",
                "FIT_CIPHER",
                "ENV_IS_IN_FAT",
                "ENV_IS_IN_EXT4",
                "ENV_IS_IN_SPI_FLASH",
            )
            for x in ("--disable", s)
        ],
        cwd=TREE,
        check=True,
        timeout=BUILD_TIMEOUT_S,
    )
    subprocess.run([*mk, "olddefconfig"], cwd=TREE, check=True, timeout=BUILD_TIMEOUT_S)
    # `make u-boot` still tries host tools that need ENGINE_* (absent from this openssl);
    # the u-boot binary itself links. Judge by the binary, not make's rc.
    subprocess.run(
        [*mk, "-j24", "u-boot"],
        cwd=TREE,
        check=False,
        timeout=BUILD_TIMEOUT_S,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    if not UBOOT.exists():
        raise RuntimeError(f"sandbox build produced no {UBOOT}")


def fake_uimage() -> bytes:
    """Valid legacy header (arm64 Linux kernel): sandbox bootm refuses the arch = a bootm failure."""
    data = b"\0" * 4096
    hdr = struct.pack(
        ">IIIIIIIBBBB32s",
        0x27051956,
        0,
        0,
        len(data),
        0x08000000,
        0x08000000,
        zlib.crc32(data),
        5,
        22,
        2,
        0,
        b"fake",
    )
    hcrc = zlib.crc32(hdr)
    return hdr[:4] + struct.pack(">I", hcrc) + hdr[8:] + data


def disk() -> Path:
    root = WORK / "p2"
    files = {
        "boot/uImage": fake_uimage(),
        "boot/unvr.dtb": b"\xd0\x0d\xfe\xed" + b"\0" * 60,
        "boot/awto/test/uImage": fake_uimage(),
        "boot/awto/test/unvr.dtb": b"\xd0\x0d\xfe\xed" + b"\0" * 60,
        "boot/awto/test/initrd": b"070701" + b"\0" * 1024,
        "boot/awto/test/uEnv.txt": f"bootargs_alt={ALT_ARGS}\nbootcmd=echo HIJACK\n".encode(),
    }
    for rel, data in files.items():
        p = root / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(data)
    img, p2 = WORK / "disk.img", WORK / "p2.img"
    for f in (img, p2):
        f.unlink(missing_ok=True)
    mib = 1 << 20
    with open(img, "wb") as f:
        f.truncate(64 * mib)
    subprocess.run(
        ["sgdisk", "-n1:2048:+8M", "-t1:EF00", "-n2:0:0", "-t2:8300", str(img)],
        check=True,
        capture_output=True,
        timeout=RUN_TIMEOUT_S,
    )
    out = subprocess.run(
        ["sgdisk", "-i2", str(img)],
        check=True,
        capture_output=True,
        text=True,
        timeout=RUN_TIMEOUT_S,
    ).stdout
    first = int(out.split("First sector: ")[1].split()[0])
    last = int(out.split("Last sector: ")[1].split()[0])
    with open(p2, "wb") as f:
        f.truncate((last - first + 1) * 512)
    subprocess.run(
        ["mkfs.ext4", "-q", "-F", "-d", str(root), str(p2)],
        check=True,
        capture_output=True,
        timeout=RUN_TIMEOUT_S,
    )
    with open(img, "r+b") as f, open(p2, "rb") as s:
        f.seek(first * 512)
        f.write(s.read())
    return img


def run(
    name: str,
    bootcmd: str,
    env: dict[str, str],
    pre: str,
    want: list[str],
    unwanted: list[str],
    after: dict[str, str | None],
) -> bool:
    envf = WORK / f"{name}.env"
    envf.write_text(
        "".join(
            f"{k}={v}\n"
            for k, v in {"bootcmd": bootcmd, "bootargs": BOOTARGS, **env}.items()
        )
    )
    probe = "echo PROBES; " + "; ".join(f"env print {k}" for k in after)
    cmd = (
        f"host load hostfs - 0x1000000 {envf}; env import -t 0x1000000 ${{filesize}}; "
        f"env delete bootonce bootdefault; env import -t 0x1000000 ${{filesize}}; {pre}"
        f"run bootcmd; {probe}"
    )
    r = subprocess.run(
        [str(UBOOT), "-c", cmd],
        capture_output=True,
        text=True,
        timeout=RUN_TIMEOUT_S,
        check=False,
    )
    out = r.stdout + r.stderr
    (WORK / f"{name}.out").write_text(out)
    # Sandbox has no env store -> no saveenv command; the box has both. Seen = it was reached.
    out = out.replace("Unknown command 'saveenv'", "SAVEENV-REACHED")
    errs = [f"missing {w!r}" for w in want if w not in out]
    errs += [f"unexpected {u!r}" for u in unwanted if u in out]
    probes = out.split("PROBES\n", 1)[-1]
    for k, v in after.items():
        # env print truncates at CONFIG_SYS_PBSIZE (1044): compare a long value by prefix
        line = f'## Error: "{k}" not defined\n' if v is None else f"{k}={v}"[:1000]
        line += "" if v is None or len(line) >= 1000 else "\n"
        if line not in probes:
            errs.append(f"{k} != {v!r}")
    log(
        f"{'PASS' if not errs else 'FAIL'} {name}"
        + ("" if not errs else f": {'; '.join(errs)} (see {WORK / (name + '.out')})")
    )
    return not errs


def main() -> int:
    WORK.mkdir(parents=True, exist_ok=True)
    try:
        build()
        img = disk()
    except (RuntimeError, subprocess.SubprocessError) as e:
        log(f"FATAL: {e}")
        return 1
    exact = bs.defconfig_bootcmd()
    # Legacy-image bootm segfaults in sandbox (bootm.c image_get_kernel() skips map_sysmem):
    # probe the args + bootargs bootm would get instead. A real bootm failure also falls through.
    hosted = (
        exact.replace("scsi scan", f"host bind 0 {img}")
        .replace("scsi ${d}", "host ${d}")
        .replace("bootm ", "echo BOOTM bootargs=${bootargs} args ")
    )
    if (
        exact.count("scsi ${d}") != 8
        or exact.count("scsi scan") != 1
        or "scsi ${d}" in hosted
    ):
        log(
            "FATAL: scsi -> host substitution did not hit the expected 6 loads + 2 echoes + scan"
        )
        return 1
    fed = "== booting from host 0:2 =="
    bootm_fed = f"BOOTM bootargs={BOOTARGS} args 0x02000000 - 0x04078000"
    bootm_alt = f"BOOTM bootargs={ALT_ARGS} args 0x02000000 0x20000000:406 0x0f000000"
    cases = [
        (
            "exact-nodisk-bootonce",
            exact,
            {"bootonce": "test"},
            "",
            [
                "== boot profile test ==",
                "SAVEENV-REACHED",
                "test did not boot - falling back",
                "no /boot/uImage found",
            ],
            ["CANARY", "Unknown command", "syntax error"],
            {"bootonce": None, "bootargs": BOOTARGS},
        ),
        (
            "exact-nodisk-plain",
            exact,
            {},
            "",
            ["no /boot/uImage found"],
            ["boot profile", "Unknown command", "syntax error"],
            {"bootargs": BOOTARGS},
        ),
        (
            "canary",
            exact,
            {"bootonce": "test"},
            "mw.l 0x10000000 0xb0075709; ",
            ["CANARY set"],
            ["boot profile", "scsi"],
            {"bootonce": "test"},
        ),
        (
            "bootonce-test",
            hosted,
            {"bootonce": "test"},
            "",
            [
                "== boot profile test ==",
                "SAVEENV-REACHED",
                "== booting test from host 0:2 ==",
                bootm_alt,
                "test did not boot",
                fed,
                bootm_fed,
            ],
            ["HIJACK", "Unknown command", "syntax error"],
            {
                "bootonce": None,
                "bootargs": BOOTARGS,
                "bootargs_alt": ALT_ARGS,
                "bootcmd": hosted,
            },
        ),
        (
            "bootdefault-test",
            hosted,
            {"bootdefault": "test"},
            "",
            ["== booting test from host 0:2 ==", bootm_alt, fed, bootm_fed],
            ["HIJACK", "SAVEENV-REACHED"],
            {"bootdefault": "test", "bootargs": BOOTARGS},
        ),
        (
            "bootonce-beats-default",
            hosted,
            {"bootdefault": "test", "bootonce": "fedora"},
            "",
            [fed, bootm_fed, "SAVEENV-REACHED"],
            ["boot profile"],
            {"bootonce": None, "bootdefault": "test", "bootargs": BOOTARGS},
        ),
        (
            "bootonce-missing",
            hosted,
            {"bootonce": "nosuch"},
            "",
            ["== boot profile nosuch ==", "nosuch did not boot", fed, bootm_fed],
            ["booting nosuch", "BOOTM bootargs=console=ttyS0,115200 root=ZFS"],
            {"bootonce": None, "bootargs": BOOTARGS},
        ),
        (
            "no-selection",
            hosted,
            {},
            "",
            [fed, bootm_fed],
            ["boot profile", "SAVEENV-REACHED"],
            {"bootargs": BOOTARGS},
        ),
    ]
    ok = all([run(*c) for c in cases])
    log(
        f"{'ALL PASS' if ok else 'FAILURES'}: {len(cases)} scenarios, bootcmd {len(exact)} chars"
    )
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
