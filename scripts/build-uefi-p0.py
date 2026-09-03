#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""Build the UNVR P0 EDK2 firmware volume (docs/uefi.md).

Clones/updates tianocore/edk2 at a pinned stable tag (not tip - an earlier
attempt against tip found ArmLib and other library classes mid-refactor,
not what the doc's own checklist and the imbushuo/ccr2004-uefi reference
assumed), builds BaseTools, then builds Platform/Ubiquiti/UNVR/Unvr.dsc.

Out: EDK2_OUT/Build/UNVR/DEBUG_GCC/FV/UNVR.fd - loaded by U-Boot via
`go`/`bootm`, never flashed (docs/uefi.md §1). This script does NOT wrap
it as a uImage or touch the hardware - see docs/uefi.md §5's dry
chainload probe for that, deliberately marked owner-run only.

Usage:
    ./dev.py build-uefi-p0
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
os.environ.setdefault("AWTO_ALLOW_DIRECT_SCRIPT", "1")
from _repo import REPO, log_path  # noqa: E402

LOG = log_path("build-uefi-p0")
EDK2_OUT = Path(os.environ.get("AWTO_EDK2_SRC", "/mnt/2tb/unvr-port-refs/edk2"))
EDK2_TAG = "edk2-stable202608"
# Small (<25MB each) submodules actually needed by the DEC files our P0
# component list pulls in - not the large ones (openssl/mbedtls/libspdm's
# own huge deps/TPM) since we have no crypto/TLS/network/TPM components.
SUBMODULES = [
    "BaseTools/Source/C/BrotliCompress/brotli",
    "MdePkg/Library/MipiSysTLib/mipisyst",
    "MdeModulePkg/Library/BrotliCustomDecompressLib/brotli",
    "SecurityPkg/DeviceSecurity/SpdmLib/libspdm",
]


def log(msg: str) -> None:
    print(msg, flush=True)
    LOG.parent.mkdir(parents=True, exist_ok=True)
    with LOG.open("a") as f:
        f.write(msg + "\n")


def run(
    cmd: list[str],
    cwd: Path | None = None,
    env: dict | None = None,
    timeout: float = 300,
) -> None:
    log("+ " + " ".join(str(c) for c in cmd))
    p = subprocess.run(cmd, cwd=cwd, env=env, timeout=timeout)
    if p.returncode != 0:
        raise SystemExit(f"FATAL: {cmd[0]} exited {p.returncode}")


def ensure_edk2() -> None:
    if not (EDK2_OUT / ".git").exists():
        log(f"cloning edk2 @ {EDK2_TAG} into {EDK2_OUT}")
        run(
            [
                "git",
                "clone",
                "--depth",
                "1",
                "--branch",
                EDK2_TAG,
                "https://github.com/tianocore/edk2.git",
                str(EDK2_OUT),
            ],
            timeout=180,
        )
    for sm in SUBMODULES:
        if not any((EDK2_OUT / sm).iterdir()) if (EDK2_OUT / sm).is_dir() else True:
            run(
                ["git", "submodule", "update", "--init", "--depth", "1", sm],
                cwd=EDK2_OUT,
                timeout=120,
            )


def main() -> int:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    ap.add_argument(
        "--target",
        choices=["DEBUG", "RELEASE", "NOOPT"],
        default="DEBUG",
        help="EDK2 build target - RELEASE compiles out ASSERT() "
        "(docs/uefi.md P0 status: useful for checking whether "
        "an ASSERT-only failure is masking an otherwise-working "
        "boot path)",
    )
    args = ap.parse_args()

    ensure_edk2()

    env = dict(os.environ)
    env["WORKSPACE"] = str(EDK2_OUT)
    env["EDK_TOOLS_PATH"] = str(EDK2_OUT / "BaseTools")
    env["PACKAGES_PATH"] = f"{EDK2_OUT}:{REPO}"
    env["GCC_AARCH64_PREFIX"] = "aarch64-linux-gnu-"
    env["PYTHON_COMMAND"] = "python3"

    log("building BaseTools")
    run(["make", "-C", "BaseTools"], cwd=EDK2_OUT, env=env, timeout=300)

    target_txt = EDK2_OUT / "Conf/target.txt"
    if not target_txt.exists():
        run(
            ["bash", "-c", f"cd {EDK2_OUT} && source ./edksetup.sh BaseTools"],
            env=env,
            timeout=60,
        )
    text = target_txt.read_text()
    for old, new in (
        (
            r"ACTIVE_PLATFORM.*",
            "ACTIVE_PLATFORM       = Platform/Ubiquiti/UNVR/Unvr.dsc",
        ),
        (r"TARGET_ARCH.*", "TARGET_ARCH           = AARCH64"),
        (r"TOOL_CHAIN_TAG.*", "TOOL_CHAIN_TAG        = GCC"),
    ):
        text = re.sub(r"^" + old, new, text, flags=re.MULTILINE)
    target_txt.write_text(text)

    log(f"building Platform/Ubiquiti/UNVR/Unvr.dsc (P0, target={args.target})")
    build_cmd = (
        f"cd {EDK2_OUT} && source ./edksetup.sh BaseTools >/dev/null && "
        f"build -b {args.target}"
    )
    run(["bash", "-c", build_cmd], env=env, timeout=300)

    fd = EDK2_OUT / f"Build/UNVR/{args.target}_GCC/FV/UNVR.fd"
    if not fd.exists():
        raise SystemExit(f"FATAL: build reported success but {fd} is missing")
    log(f"DONE: {fd} ({fd.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
