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
