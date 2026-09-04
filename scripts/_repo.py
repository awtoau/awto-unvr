"""Shared repo-root resolution for every script here.

No absolute path is ever hardcoded. Resolution order, first hit wins:

  1. $UNVR_REPO            - explicit override, for CI or an odd checkout
  2. `git rev-parse --show-toplevel` - authoritative, works from any subdir
  3. this file's parent's parent      - works with no git and no env

VSCode is deliberately NOT used as a source: it exports VSCODE_CWD, which is the
directory VSCode was launched from (e.g. $HOME), not the workspace root, and it
sets no workspace-root variable in an integrated terminal. Keying off it would
resolve to the wrong directory.

Usage - scripts/ is on sys.path automatically when a script in it is executed:

    from _repo import REPO, TMP, LOGS, log_path
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path


def _enforce_via_devpy() -> None:
    """Every script here imports this module, so this is the one choke
    point that can require './dev.py <cmd>' instead of a direct
    'python3 scripts/foo.py' call - the thing agents were told repeatedly
    this session to stop doing and kept doing anyway (stale-build/crash
    incidents, 2026-08-24). dev.py sets AWTO_VIA_DEVPY on every child it
    spawns (_run_script); a real interactive human debugging session can
    set AWTO_ALLOW_DIRECT_SCRIPT=1 to bypass deliberately."""
    if os.environ.get("AWTO_VIA_DEVPY") or os.environ.get("AWTO_ALLOW_DIRECT_SCRIPT"):
        return
    prog = Path(sys.argv[0]).name if sys.argv else "this script"
    sys.exit(
        f"{prog}: direct script invocation is disabled - use './dev.py <command>' "
        f"instead (see './dev.py' with no args for the list). If you're a human "
        f"deliberately debugging this script interactively, set "
        f"AWTO_ALLOW_DIRECT_SCRIPT=1 to bypass."
    )


_enforce_via_devpy()


def _resolve() -> Path:
    env = os.environ.get("UNVR_REPO")
    if env:
        p = Path(env).expanduser().resolve()
        if p.is_dir():
            return p
    here = Path(__file__).resolve().parent
    try:
        out = subprocess.run(
            ["git", "-C", str(here), "rev-parse", "--show-toplevel"],
            capture_output=True,
            text=True,
            timeout=5,
            check=False,
        )
        if out.returncode == 0 and out.stdout.strip():
            return Path(out.stdout.strip()).resolve()
    except (OSError, subprocess.SubprocessError):
        pass
    return here.parent


REPO = _resolve()
TMP = REPO / "tmp"
LOGS = TMP / "logs"
SOURCES = REPO / "sources"
IMAGES = REPO / "images"
DOCS = REPO / "docs"
# The ONE tftp scratch root every script must use (#119: images/tftp and
# tmp/tftp used to be two independently-hardcoded paths; a tftpd left
# running against the stale one silently served a 4-day-old U-Boot build).
TFTP_ROOT = TMP / "tftp"


def log_path(name: str) -> Path:
    """tmp/logs/<name>.log, directory created."""
    LOGS.mkdir(parents=True, exist_ok=True)
    return LOGS / f"{name}.log"


def rel(p: Path | str) -> str:
    """Path relative to the repo root, for logs and docs that must not leak
    an absolute path."""
    try:
        return str(Path(p).resolve().relative_to(REPO))
    except ValueError:
        return str(p)


# Build parallelism for every `make -j` in this repo (#146). 28 of this host's
# 32 cores; the 4 spare keep the box interactive during a kernel build. Env
# override AWTO_NPROC for a different host.
NPROC = max(1, min(os.cpu_count() or 4, int(os.environ.get("AWTO_NPROC") or 28)))


def kernel_build_out(kasan: bool | None = None) -> Path:
    """The single source of truth for where a Fedora kernel build's output
    lives - build-linux-fedora.py (the producer) and _fedora_deploy.py (the
    consumer) used to each hardcode their own copy of this default, and one
    got updated without the other during a rename, silently deploying a
    3-day-stale build. Both now call this instead of hardcoding it again.

    AWTO_KERNEL_OUT always wins if set (matches build-linux-fedora.py's own
    override precedent). `kasan=None` reads AWTO_KASAN_BUILD from the
    environment; pass True/False to force a choice regardless of env."""
    if kasan is None:
        kasan = bool(os.environ.get("AWTO_KASAN_BUILD"))
    default = (
        "/mnt/2tb/unvr-port-refs/build-out-fedora-kasan"
        if kasan
        else "/mnt/2tb/unvr-port-refs/build-out-fedora"
    )
    return Path(os.environ.get("AWTO_KERNEL_OUT", default))


def ea16_build_out() -> Path:
    """Same single-source-of-truth reasoning as kernel_build_out(), for
    build-linux-ea16.py's own output dir instead of the fedora one."""
    return Path(
        os.environ.get("AWTO_KERNEL_OUT", "/mnt/2tb/unvr-port-refs/build-out-ea16")
    )


def kernel_src() -> Path:
    """AWTO_KERNEL_SRC as build-linux-fedora.py resolves it - the kernel
    tree this project tracks forward against torvalds/linux mainline
    (docs/build.md). Single source of truth so kernel_build_ver() below
    reads the same tree the actual build ran against."""
    return Path(
        os.environ.get("AWTO_KERNEL_SRC", "/mnt/2tb/unvr-port-refs/linux-v7.3-fresh")
    )


# Source-level patch markers: file -> (string that must be present, patch,
# issue). A Kconfig symbol cannot stand in for these - the i2c fix is a
# behaviour change inside a function, with no CONFIG of its own, so an
# unapplied patch would otherwise build clean and wedge the bus on the box.
KERNEL_PATCH_MARKERS = {
    "drivers/i2c/busses/i2c-designware-common.c": (
        '"snps,no-enable-abort"',
        "patches/i2c-designware-no-enable-abort.patch",
        "#86",
    ),
}


def check_kernel_patches(log_fn=None) -> None:
    """FATAL if the kernel tree about to be built is missing a source-level
    patch from patches/. Mirrors the CONFIG_AHCI_ALPINE=y check (#92) for
    the changes that have no config symbol to check."""
    src = kernel_src()
    for relpath, (marker, patch, issue) in KERNEL_PATCH_MARKERS.items():
        path = src / relpath
        if not path.exists() or marker not in path.read_text():
            msg = (
                f"FATAL: {relpath} in {src} does not contain {marker} - "
                f"apply {patch} in AWTO_KERNEL_SRC ({issue})"
            )
            if log_fn:
                log_fn(msg)
            else:
                print(msg, file=sys.stderr)
            sys.exit(1)


def kernel_build_ver() -> str:
    """Short 'VERSION.PATCHLEVEL' label (e.g. "7.3") baked into every
    output filename (uImage/dtb/tftp names) and the uImage's own Image
    Name field - the deploy step must read the exact same value or look
    for files that don't exist. Single source of truth for the same
    reason as kernel_build_out().

    Derived from kernel_src()'s own Makefile, not hardcoded - this
    project deliberately tracks mainline forward (docs/build.md), so a
    fixed default goes stale the moment the tree moves. Confirmed live
    2026-09-03: a stale "7.2" default sat unnoticed for days after
    AWTO_KERNEL_SRC's default had already moved to a 7.3 tree, so every
    build's own filenames/Image Name lied about actually being 7.3."""
    if "AWTO_KERNEL_VER" in os.environ:
        return os.environ["AWTO_KERNEL_VER"]
    makefile = kernel_src() / "Makefile"
    version = patchlevel = None
    for line in makefile.read_text().splitlines()[:10]:
        if line.startswith("VERSION ="):
            version = line.split("=", 1)[1].strip()
        elif line.startswith("PATCHLEVEL ="):
            patchlevel = line.split("=", 1)[1].strip()
        if version and patchlevel:
            break
    if not (version and patchlevel):
        raise RuntimeError(
            f"could not parse VERSION/PATCHLEVEL from {makefile} - "
            "is AWTO_KERNEL_SRC a real kernel tree?"
        )
    return f"{version}.{patchlevel}"
