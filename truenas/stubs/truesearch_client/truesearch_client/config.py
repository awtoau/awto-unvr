# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 Awto / Daniel Tyrrell
"""truesearchd target file model (#356). No file is ever written or read here."""

from __future__ import annotations

import dataclasses

# Never a mountpoint here, so etc_files/truesearchd renders nothing (FileShouldNotExist).
TARGETS_DIR = "/var/lib/truesearch/targets"


@dataclasses.dataclass(frozen=True)
class TargetRow:
    id: str
    dataset: str
    root: str
    exclude: tuple[str, ...] = ()
    enabled: bool = True


def target_id(dataset_name: str, guid: object) -> str:
    return f"{dataset_name.split('/')[0]}-{guid}"


def read_config() -> list[TargetRow] | None:
    """None = no configuration file; middleware treats that as no targets."""
    return None


def render_config(rows: list[TargetRow]) -> str:
    out = ["# truesearch stand-in (awto-unvr #356): no search daemon is installed"]
    out += [
        f"{r.id}\t{r.dataset}\t{r.root}\t{int(r.enabled)}\t{','.join(r.exclude)}"
        for r in rows
    ]
    return "\n".join(out) + "\n"
