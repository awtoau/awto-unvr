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
from pathlib import Path


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
            capture_output=True, text=True, timeout=5,
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
